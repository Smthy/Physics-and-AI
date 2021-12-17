#include <iostream>
#include <utility>
#include "Level1.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourSelector.h"
#include "../../Common/Assets.h"
#include <chrono>  

using namespace NCL;
using namespace CSC8503;


Level1::Level1() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	forceMagnitude = 10.0f;
	useGravity = false;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);
	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void Level1::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh", &cubeMesh);
	loadFunc("sphere.msh", &sphereMesh);
	loadFunc("Male1.msh", &charMeshA);
	loadFunc("courier.msh", &charMeshB);
	loadFunc("security.msh", &enemyMesh);
	loadFunc("coin.msh", &bonusMesh);
	loadFunc("capsule.msh", &capsuleMesh);

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("newTex.png"); //checkerboard.png 
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

Level1::~Level1() {
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void Level1::UpdateGame(float dt) {

	//TestBehaviourTree();

	if (!mainMenuActive) {
		if (useGravity) {
			Debug::Print("(G)ravity on", Vector2(5, 95), Debug::WHITE);
		}
		else {
			Debug::Print("(G)ravity off", Vector2(5, 95), Debug::WHITE);
		}
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));
		renderer->DrawString("Click Force " + std::to_string(forceMagnitude), Vector2(10, 20));

	}
	else if (mainMenuActive) {
		renderer->DrawString("Physics and AI", Vector2(25, 30), Debug::MAGENTA, 50.0f);
		renderer->DrawString("Physcis Level 1", Vector2(10, 45), Debug::DARKGREEN, 35.0f);
		renderer->DrawString("AI Level 2", Vector2(10, 55), Debug::DARKGREEN, 35.0f);
		renderer->DrawString("Exit", Vector2(10, 65), Debug::DARKRED, 35.0f);
	}

	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();

	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);				
	}

	MovingWall(dt);

	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();

	if (windMill != NULL && windMill_2 != NULL) {
		windMill->GetPhysicsObject()->SetAngularVelocity(Vector3(0, 5, 0));
		windMill_2->GetPhysicsObject()->SetAngularVelocity(Vector3(0, -5, 0));
	}
	//std::cout << world->GetMainCamera()->GetPosition() << std::endl;


	if (ball->GetTransform().GetPosition().y < -90) {
		playerFall--;
		if (playerFall == 0) {
			std::cout << "PLAYER FELL --- GAME OVER --- PLAYER FELL" << std::endl;
			dt = 0;
		}
		else {
			ball->GetTransform().SetPosition(Vector3(0, 25, 0));
			ball->GetPhysicsObject()->SetAngularVelocity(Vector3(0, 0, 0));
			ball->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
		}		
	}

	
}

void Level1::MovingWall(float dt) {
	if (movingWall == NULL) {
		return;
	}
	Vector3 currentPos = movingWall->GetTransform().GetPosition();
	Vector3 distance;
	Vector3 target = pos[pass];

	distance = target - currentPos;

	if (distance.Length() < 1) {
		pass++;
		target = pos[pass];

		if (pass >= 2) {
			pass = 0;
		}
	}

	Vector3 dir = distance.Normalised();
	movingWall->GetTransform().SetPosition(currentPos + (dir * wallSpeed * dt));
}

void Level1::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void Level1::LockedObjectMovement() {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
	}
}

void Level1::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void Level1::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void Level1::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	InitLevel1();
	BridgeConstraintTest();
}

void Level1::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(1, 0.1, 3);
	float invCubeMass = 1; //how heavy the middle pieces are
	int numLinks = 3;
	float maxDistance = 4; // constraint distance
	float cubeDistance = 2; // distance between links
	
	Vector3 startPos = Vector3(5, -20, 0);
	GameObject * start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, Debug::RED, "cubeStart", 0.825f, 0);
	GameObject * end = AddCubeToWorld(startPos + Vector3((numLinks + 2)  * cubeDistance, 0, 0), cubeSize, Debug::RED, "cubeEnd", 0.825f, 0);
	end->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 25.0f));
	GameObject * previous = start;
	for (int i = 0; i < numLinks; ++i) {
		GameObject * block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, Debug::WHITE, "cubeMid", 0.825f, invCubeMass);
		PositionConstraint * constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint * constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/

GameObject* Level1::AddFloorToWorld(const Vector3& position, string name) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(500, 0.01, 500);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);



	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();


	world->AddGameObject(floor);

	floor->GetTransform().SetScale(floorSize * 2).SetPosition(position);

	return floor;
}

GameObject* Level1::AddCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 color, string name, float elstacity,float inverseMass) {
	GameObject* cube = new GameObject();

	OBBVolume* volume = new OBBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);


	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetPhysicsObject()->SetElasticity(elstacity);

	cube->SetName(name);
	cube->GetRenderObject()->SetColour(color);

	world->AddGameObject(cube);

	return cube;
}

GameObject* Level1::AddAABBCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 color, string name, float elstacity,float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);


	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetPhysicsObject()->SetElasticity(elstacity);

	cube->SetName(name);
	cube->GetRenderObject()->SetColour(color);

	world->AddGameObject(cube);

	return cube;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* Level1::AddSphereToWorld(const Vector3& position, float radius, string name, float elstacity,float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->SetName(name);
	sphere->GetPhysicsObject()->InitSphereInertia();
	sphere->GetPhysicsObject()->SetElasticity(elstacity);



	world->AddGameObject(sphere);

	return sphere;
}

GameObject* Level1::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius * 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}
void Level1::InitLevel1() {
	mainMenuActive = false;
	//Section_1
	AddCubeToWorld(Vector3(0, -15, 0), Vector3(3, 0.1, 3), Debug::ORANGE, "Cube_01", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -5.0f));
	//AddCubeToWorld(Vector3(10, -20, 0), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_02", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(20, -30, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(26, -25, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(32, -20, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(38, -15, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(44, -10, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(57, -3, 0), Vector3(11, 0.1, 3), Debug::ORANGE, "Cube_03", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));

	//section_2
	AddCubeToWorld(Vector3(68, -2, 0), Vector3(0.1, 2, 3), Debug::DARKPURPLE, "PullWallX", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(65, -2, -3), Vector3(3, 2, 0.1), Debug::DARKGREEN, "BoostPadHZ", 0.825f, 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));

	AddCubeToWorld(Vector3(68, -3.25, 17), Vector3(0.1, 1.5, 14), Debug::ORANGE, "Wall_01", 0.825f,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(62, -3.25, 17), Vector3(0.1, 1.5, 14), Debug::ORANGE, "Wall_02", 0.825f,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));

	AddCubeToWorld(Vector3(65, -3.5, 5), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_04", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 15.0f));
	AddCubeToWorld(Vector3(65, -3.9, 8), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_05", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 15.0f));
	AddCubeToWorld(Vector3(65, -4.2, 11), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_06", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 15.0f));
	AddCubeToWorld(Vector3(65, -4.4, 14), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_07", 0.825f,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(65, -4.4, 17), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_08", 0.825f,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(65, -4.2, 20), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_09", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, -15.0f));
	AddCubeToWorld(Vector3(65, -3.9, 23), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_10", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, -15.0f));
	AddCubeToWorld(Vector3(65, -3.3, 26), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_11", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, -5.0f));
	//section_3
	AddCubeToWorld(Vector3(65, -3.5, 45), Vector3(3, 0.1, 5), Debug::ORANGE, "Cube_12", 0.825f,0);
	AddCubeToWorld(Vector3(65, 5, 50), Vector3(3, 10.5, 0.1), Debug::DARKPURPLE, "PullWallZ", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 0.0f));
	AddCubeToWorld(Vector3(68, -2.5, 47), Vector3(0.1, 2, 3), Debug::DARKGREEN, "BoostPadHX", 0.825f,0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 0.0f));
	AddCubeToWorld(Vector3(57, -3.5, 47), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_13", 0.825f, 0);

	movingWall = AddCubeToWorld(Vector3(51, -5.5, 47), Vector3(1, 2, 3), Debug::CYAN, "BouncyWall", 5.0f,0);

	AddCubeToWorld(Vector3(45, -3.5, 47), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_14", 0.825f,0);
	AddCubeToWorld(Vector3(35.5, -5.415, 47), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_15", 0.825f,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 5.0f));
	AddCubeToWorld(Vector3(24, -4, 47), Vector3(0.1, 3, 3), Debug::ORANGE, "Cube_16", 0.825f,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -7.5f));
	AddCubeToWorld(Vector3(31, -10.25, 47), Vector3(0.1, 3, 3), Debug::DARKGREEN, "BoostPadHX", 0.825f,0);
	AddCubeToWorld(Vector3(11, -13.25, 47), Vector3(20, 0.1, 3), Debug::ORANGE, "Cube_17", 0.825f,0);
	AddCubeToWorld(Vector3(14, -11.25, 50), Vector3(17, 2, 0.1), Debug::DARKBLUE, "Wall_03", 0.825f, 0);
	AddCubeToWorld(Vector3(14, -13, 70), Vector3(17, 2, 0.1), Debug::DARKBLUE, "Wall_05", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -5.0f));
	AddCubeToWorld(Vector3(-3, -11.25, 60), Vector3(0.1, 2, 10), Debug::DARKBLUE, "Wall_07", 0.825f, 0);
	AddCubeToWorld(Vector3(11, -11.25, 44), Vector3(20, 2, 0.1), Debug::DARKRED, "Wall_04", 0.825f, 0);
	AddCubeToWorld(Vector3(11, -13, 76), Vector3(20, 2, 0.1), Debug::DARKRED, "Wall_06", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -5.0f));
	AddCubeToWorld(Vector3(-9, -11.25, 60), Vector3(0.1, 2, 16), Debug::DARKRED, "Wall_08", 0.825f, 0);
	AddCubeToWorld(Vector3(-6, -12, 45), Vector3(0.1, 3, 3), Debug::CYAN, "Bank_01", 0.825f,0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -45.0f));
	AddCubeToWorld(Vector3(-7, -12, 74), Vector3(0.1, 3, 3), Debug::CYAN, "Bank_02", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 45.0f));
	AddCubeToWorld(Vector3(-6, -13.25, 61), Vector3(3, 0.1, 15), Debug::ORANGE, "Cube_18", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -2.0f));
	AddCubeToWorld(Vector3(11, -15, 73), Vector3(20, 0.1, 3), Debug::ORANGE, "Cube_19", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -5.0f));

	AddCubeToWorld(Vector3(69, -30, 73), Vector3(40, 0.1, 20), Debug::ORANGE, "Cube_20", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -17.5f));
	AddCubeToWorld(Vector3(69, -28, 53), Vector3(40, 3, 0.1), Debug::DARKRED, "Cube_20", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -17.5f));
	AddCubeToWorld(Vector3(69, -28, 93), Vector3(40, 3, 0.1), Debug::DARKRED, "Cube_20", 0.825f, 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -17.5f));
	AddSphereToWorld(Vector3(38, -20, 73), 2.5, "Sphere_01", 1.2f,0);

	windMill = AddCubeToWorld(Vector3(72, -30, 64), Vector3(0.1, 2, 7.5), Debug::DARKRED, "WindMill", 1.2f, 0);
	windMill->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 17.5f));
	windMill_2 = AddCubeToWorld(Vector3(64, -27.5, 80), Vector3(0.1, 2, 7.5), Debug::DARKRED, "WindMill_2", 1.2f, 0);
	windMill_2->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 17.5f));

	AddSphereToWorld(Vector3(100, -40, 89), 1.5, "Sphere_02", 1.0f, 0);
	AddSphereToWorld(Vector3(100, -40, 66), 1.5, "Sphere_03", 1.0f, 0);


	AddCubeToWorld(Vector3(110, -40, 88), Vector3(5, 0.1, 25), Debug::CYAN, "Cube_21", 0.0f, 0);// Win Condition


	//ball = AddSphereToWorld(Vector3(57, 25, 47), 1.0f, "Ball", 1.0f);
	//ball->SetColor(Debug::DARKRED);

	ball = AddSphereToWorld(Vector3(0, 25, 0), 1.0f, "Ball", 0.825f ,1.0f);  //------------------- Starting Position of the ball

	AddFloorToWorld(Vector3(0, -100, 0), "floor");
	level2Loaded = false;
}

GameObject* Level1::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);
	//lockedObject = character;
	return character;
}
GameObject* Level1::AddEnemyToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* Level1::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* Level1::AddStateObjectToWorld(const Vector3& position) {

	StateGameObject* coin = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	coin->SetBoundingVolume((CollisionVolume*)volume);
	coin->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), bonusMesh, nullptr, basicShader));
	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume()));

	coin->GetPhysicsObject()->SetInverseMass(1.0f);
	coin->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(coin);

	return coin;
}
/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool Level1::SelectObject() {

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {


		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {

			if (selectionObject) {	//set colour to deselected;
				Vector4 newColor = selectionObject->GetRenderObject()->GetColour();
				selectionObject->GetRenderObject()->SetColour(newColor);

				selectionObject = nullptr;
				lockedObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				//Debug::DrawLine(selectionObject->GetTransform().GetPosition() + world->GetMainCamera()->GetPosition(), selectionObject->GetTransform().GetPosition(), Debug::YELLOW, 100.0f);
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));

				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		if (!mainMenuActive) {
			renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
		}
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if (selectionObject) {
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void Level1::MoveSelectedObject() {
	Vector3 distance;
	Vector3 targetPos;
	Vector3 currentBallPos;

	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 5.0f;
	if (!selectionObject) {
		return;
	}

	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;

		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {

				targetPos = selectionObject->GetTransform().GetPosition();
				currentBallPos = ball->GetTransform().GetPosition();
				distance = targetPos - currentBallPos;

				if (distance.Length() < 15) {
					if (selectionObject->GetName() == "BoostPadV") {
						ball->GetPhysicsObject()->AddForce(Vector3(10, 25, 0) * forceMagnitude);

					}
					if (selectionObject->GetName() == "BoostPadHZ") {
						ball->GetPhysicsObject()->AddForce(Vector3(0, 0, 25) * forceMagnitude);

					}
					if (selectionObject->GetName() == "BoostPadHX") {
						ball->GetPhysicsObject()->AddForce(Vector3(-25, 0, 0) * forceMagnitude);
					}

					if (selectionObject->GetName() == "PullWallX") {
						ball->GetPhysicsObject()->AddForce(Vector3(-10, 0, 0) * -50.0f);
					}

					if (selectionObject->GetName() == "PullWallZ") {
						ball->GetPhysicsObject()->AddForce(Vector3(0, 0, -10) * -50.0f);
					}
				}
			}
		}
	}
}