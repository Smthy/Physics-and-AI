#include <iostream>
#include <utility>

#include "TutorialGame.h"
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


using namespace NCL;
using namespace CSC8503;


TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);
	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png"); //checkerboard.png 
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();

	TestPathfinding();
	DisplayPathfinding();
	TestStateMachine();
	
}

TutorialGame::~TutorialGame()	{
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

void TutorialGame::UpdateGame(float dt) {
	
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

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

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
	

	if (testStateObject) {
		testStateObject->Update(dt);
	}


	/*if (InitLevel2)
	{
		return;
	}
	else {
		DisplayPathfinding();
	}*/
	

	//std::cout << world->GetMainCamera()->GetPosition() << std::endl;
}

void TutorialGame::MovingWall(float dt) {
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

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject	= nullptr;
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

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
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
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
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

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

//void PushdownAutomata() {
//	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM3)){
//		
//	}
//}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	
	
	
	//InitMainMenu();
	//InitLevel1();
	InitLevel2();

	//InitMixedGridWorld(5, 5, 3.5f, 3.5f);
	//InitGameExamples();
	//testStateObject = AddStateObjectToWorld(Vector3(0, 20, 0));
	//BridgeConstraintTest();

		

	//AddFloorToWorld(Vector3(0, -2, 0));
	//DisplayPathfinding();
	
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(3, 0.1, 3);
	
	float invCubeMass = 5; //how heavy the middle pieces are
	int numLinks = 8;
	float maxDistance = 2; // constraint distance
	float cubeDistance = 8; // distance between links
	
	Vector3 startPos = Vector3(-50, 100, 0);
	
	GameObject * start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, Vector4(1, 0, 0, 1), "bridge", 0);
	GameObject * end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, Vector4(1, 1, 1, 1), "bridge", 0);
	GameObject * previous = start;
	
	for (int i = 0; i < numLinks; ++i) {
		GameObject * block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, Vector4(1, 1, 1, 1), "bridge", invCubeMass);
		PositionConstraint * constraint = new PositionConstraint(previous,
		block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	
	PositionConstraint * constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/

GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize	= Vector3(500, 0.01, 500);
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	
	

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();


	world->AddGameObject(floor);

	floor->GetTransform().SetScale(floorSize * 2).SetPosition(position);

	return floor;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 color, string name, float inverseMass) {
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

	cube->SetName(name);
	cube->GetRenderObject()->SetColour(color);

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddAABBCubeToWorld(const Vector3 & position, Vector3 dimensions, Vector4 color, string name, float inverseMass) {
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
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, string name, float inverseMass) {
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
	


	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}



void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, "Not Needed", 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims,  Vector4(1, 1, 1, 1), "cube",10.0f);
			}
			else {
				AddSphereToWorld(position, sphereRadius, "hello");
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, Vector4(1, 1, 1, 1), "cube" ,1.0f);
		}
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

void TutorialGame::InitMainMenu() {	
	mainMenuActive = true;	
}

void TutorialGame::InitLevel1() {
	mainMenuActive = false;
	//Section_1
	AddCubeToWorld(Vector3(0, -15, 0), Vector3(3, 0.1, 3), Debug::ORANGE, "Cube_01" ,0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -5.0f));
	AddCubeToWorld(Vector3(10, -20, 0), Vector3(5, 0.1, 3),  Debug::ORANGE, "Cube_02", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(20, -30, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(26, -25, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(32, -20, 0), Vector3(3, 8, 3),  Debug::DARKGREEN, "BoostPadV", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(38, -15, 0), Vector3(3, 8, 3), Debug::DARKGREEN, "BoostPadV", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(44, -10, 0), Vector3(3, 8, 3),  Debug::DARKGREEN, "BoostPadV", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -25.0f));
	AddCubeToWorld(Vector3(57, -3, 0), Vector3(11, 0.1, 3), Debug::ORANGE, "Cube_03", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	
	//section_2
	AddCubeToWorld(Vector3(68, -2, 0), Vector3(0.1, 2, 3), Debug::DARKPURPLE, "PullWallX", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f)); 
	AddCubeToWorld(Vector3(65, -2, -3), Vector3(3, 2, 0.1), Debug::DARKGREEN, "BoostPadHZ", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));

	AddCubeToWorld(Vector3(68, -3.25, 17), Vector3(0.1, 1.5, 14), Debug::ORANGE, "Wall_01", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(62, -3.25, 17), Vector3(0.1, 1.5, 14), Debug::ORANGE, "Wall_02", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));

	AddCubeToWorld(Vector3(65, -3.5, 5), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_04", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 15.0f));
	AddCubeToWorld(Vector3(65, -3.9, 8), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_05", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 15.0f));
	AddCubeToWorld(Vector3(65, -4.2, 11), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_06", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 15.0f));
	AddCubeToWorld(Vector3(65, -4.4, 14), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_07", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(65, -4.4, 17), Vector3(3, 0.1, 1.5),  Debug::ORANGE, "Cube_08", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 0.0f));
	AddCubeToWorld(Vector3(65, -4.2, 20), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_09", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, -15.0f));
	AddCubeToWorld(Vector3(65, -3.9, 23), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_10", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, -15.0f));
	AddCubeToWorld(Vector3(65, -3.3, 26), Vector3(3, 0.1, 1.5), Debug::ORANGE, "Cube_11", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, -5.0f));
	//section_3
	AddCubeToWorld(Vector3(65, -3.5, 45), Vector3(3, 0.1, 5), Debug::ORANGE, "Cube_12", 0);
	AddCubeToWorld(Vector3(65, 5, 50), Vector3(3, 10.5, 0.1), Debug::DARKPURPLE, "PullWallZ", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 0.0f));
	AddCubeToWorld(Vector3(68, -2.5, 47), Vector3(0.1, 2, 3), Debug::DARKGREEN, "BoostPadHX", 0)->GetTransform().SetOrientation(Quaternion(1, 0, 0, 0.0f));
	AddCubeToWorld(Vector3(57, -3.5, 47), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_13", 0);

	movingWall = AddCubeToWorld(Vector3(51, -5.5, 47), Vector3(1, 2, 3), Debug::CYAN, "BouncyWall", 0);
	movingWall->GetPhysicsObject()->SetElasticity(100.0f);

	AddCubeToWorld(Vector3(45, -3.5, 47), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_14", 0);
	AddCubeToWorld(Vector3(35.5, -5.415, 47), Vector3(5, 0.1, 3), Debug::ORANGE, "Cube_15", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, 5.0f));	
	AddCubeToWorld(Vector3(24, -4, 47), Vector3(0.1, 3, 3), Debug::ORANGE, "Cube_16", 0)->GetTransform().SetOrientation(Quaternion(0, 0, 1, -7.5f));
	AddCubeToWorld(Vector3(31, -10.25, 47), Vector3(0.1, 3, 3), Debug::DARKGREEN, "BoostPadHX", 0);
	AddCubeToWorld(Vector3(11, -13.25, 47), Vector3(20, 0.1, 3), Debug::ORANGE, "Cube_17", 0);	
	AddCubeToWorld(Vector3(14, -11.25, 50), Vector3(17, 2, 0.1), Debug::DARKBLUE, "Wall_03", 0);
	AddCubeToWorld(Vector3(14, -13, 70), Vector3(17, 2, 0.1), Debug::DARKBLUE, "Wall_05", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -5.0f));
	AddCubeToWorld(Vector3(-3, -11.25, 60), Vector3(0.1, 2, 10), Debug::DARKBLUE, "Wall_07", 0);
	AddCubeToWorld(Vector3(11, -11.25, 44), Vector3(20, 2, 0.1), Debug::DARKRED, "Wall_04", 0);
	AddCubeToWorld(Vector3(11, -13, 76), Vector3(20, 2, 0.1), Debug::DARKRED, "Wall_06", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -5.0f));
	AddCubeToWorld(Vector3(-9, -11.25, 60), Vector3(0.1, 2, 16), Debug::DARKRED, "Wall_08", 0);
	AddCubeToWorld(Vector3(-6, -12, 45), Vector3(0.1, 3, 3), Debug::CYAN, "Bank_01", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -45.0f));
	AddCubeToWorld(Vector3(-7, -12, 74), Vector3(0.1, 3, 3), Debug::CYAN, "Bank_02", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0,1,0), 45.0f));
	AddCubeToWorld(Vector3(-6, -13.25, 61), Vector3(3, 0.1, 15), Debug::ORANGE, "Cube_18", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -2.0f));
	AddCubeToWorld(Vector3(11, -15, 73), Vector3(20, 0.1, 3), Debug::ORANGE, "Cube_19", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -5.0f));

	AddCubeToWorld(Vector3(69, -30, 73), Vector3(40, 0.1, 20), Debug::ORANGE, "Cube_20", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -17.5f));
	AddCubeToWorld(Vector3(69, -28, 53), Vector3(40, 3, 0.1), Debug::DARKRED, "Cube_20", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -17.5f));
	AddCubeToWorld(Vector3(69, -28, 93), Vector3(40, 3, 0.1), Debug::DARKRED, "Cube_20", 0)->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -17.5f));
	AddSphereToWorld(Vector3(38, -20, 73), 2.5, "Sphere_01", 0);	

	windMill = AddCubeToWorld(Vector3(72, -30, 64), Vector3(0.1, 2, 7.5), Debug::DARKRED, "WindMill", 0);
	windMill->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 17.5f));
	windMill_2 = AddCubeToWorld(Vector3(64, -27.5, 80), Vector3(0.1, 2, 7.5), Debug::DARKRED, "WindMill_2", 0);
	windMill_2->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 17.5f));

	AddSphereToWorld(Vector3(100, -40, 89), 1.5, "Sphere_02", 0);
	AddSphereToWorld(Vector3(100, -40, 66), 1.5, "Sphere_03", 0);


	AddCubeToWorld(Vector3(110, -50, 88), Vector3(5, 0.1, 15), Debug::CYAN, "Cube_21", 0);// Win Condition
	AddCubeToWorld(Vector3(110, -50, 58), Vector3(5, 0.1, 15), Debug::BLACK, "Cube_22", 0);//Restart Condition

	   	  
	//ball = AddSphereToWorld(Vector3(57, 25, 47), 1.0f, "Ball", 1.0f);
	//ball->SetColor(Debug::DARKRED);
	 
	ball = AddSphereToWorld(Vector3(0, 25, 0), 1.0f, "Ball", 1.0f);  //------------------- Starting Position of the ball
	
	AddFloorToWorld(Vector3(0, -100, 0));
}

void TutorialGame::MazeLoader(const string& file) {
	
}

void TutorialGame::InitLevel2() {
	mainMenuActive = false;
	vector<Vector3> testNodes;
	ball = AddSphereToWorld(Vector3(180, 0, 180), 1.0f, "Ball", 1.0f);
	bAI = AddSphereToWorld(Vector3(20, 0, 20), 1.0f, "BTai", 1.0f);

	NavigationGrid grid("TestGrid1.txt");
	NavigationPath outPath;

	Vector3 startPos = ball->GetTransform().GetPosition();
	Vector3 endPos = bAI->GetTransform().GetPosition();
	bool found = grid.FindPath(startPos, endPos, outPath);

	std::cout << grid.FindPath(startPos, endPos, outPath) << std::endl;
		
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}

	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Debug::DARKGREEN);
	}
	
	//testStateObject = AddStateObjectToWorld(Vector3(0, 20, 0));
	AddFloorToWorld(Vector3(0, 0, 0));
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
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
GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

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

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
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

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	
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
bool TutorialGame::SelectObject() {	

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
				lockedObject	= nullptr;			
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

	else if(selectionObject){
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

void TutorialGame::MoveSelectedObject() {	
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
			if (closestCollision.node == selectionObject){
				
				targetPos = selectionObject->GetTransform().GetPosition();
				currentBallPos = ball->GetTransform().GetPosition();
				distance = targetPos - currentBallPos;
				
				if (distance.Length() < 15) {					
					if (selectionObject->GetName() == "BoostPadV") {										
						ball->GetPhysicsObject()->AddForce(Vector3(10, 25, 0) * forceMagnitude);
										
					}
					if (selectionObject->GetName() == "BoostPadHZ") {
						ball->GetPhysicsObject()->AddForce(Vector3(0, 0, -25) * forceMagnitude);
					
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

// ----- AI -----
vector<Vector3> testNodes;
void TutorialGame::TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void
		{
			std::cout << "I'm in state!\n";
			data++;
		}
	);

	State* B = new State([&](float dt)->void
		{
			std::cout << "I'm in state!\n";
			data--;
		}
	);

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool
		{
			return data > 10;
		}
	);

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool
		{
			return data < 0;
		}
	);

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}
}

void TutorialGame::TestPathfinding() {
	NavigationGrid grid("TestGrid1.txt");
	NavigationPath outPath;

	Vector3 startPos(80, 0, 10);
	Vector3 endPos(80, 0, 80);

	bool found = grid.FindPath(startPos, endPos, outPath);
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}


void TutorialGame::DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Debug::CYAN);
	}
}
void TutorialGame::TestBehaviourTree() {
	float behaviourTimer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("Find Key", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for Key!\n";
			behaviourTimer = rand() % 100;
			state = Ongoing;
		}
		else if (state == Ongoing) {
			behaviourTimer -= dt;
			if (behaviourTimer <= 0.0f) {
				std::cout << "Found a key\n";
				return Success;
			}
		}
		return state;
		}
	);

	BehaviourAction* goToRoom = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Going into the loot room! \n";
			state = Ongoing;
		}
		else if (state == Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << "Reached Room! \n";
				return Success;
			}
		}
		return state;
		}
	);

	BehaviourAction* openDoor = new BehaviourAction("Open Door", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Opening Door!\n";
			return Success;
		}
		return state;
		}
	);

	BehaviourAction* lookForTreasure = new BehaviourAction("Look for Treasure", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for Treasure!\n";
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some treasure!\n";
				return Success;
			}
			std::cout << "No treasure in here...\n";
			return Failure;
		}
		return state;
		}
	);

	BehaviourAction* lookForItems = new BehaviourAction("Look for Items", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for Items!\n";
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some items!\n";
				return Success;
			}
			std::cout << "No items in here...\n";
			return Failure;
		}
		return state;
		}
	);

	BehaviourSequence* sequence =
		new BehaviourSequence("Room Sequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection =
		new BehaviourSelector("Loot Selection");
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence =
		new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; ++i)
	{
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		std::cout << "We're going on an adventure!\n";
		while (state == Ongoing) {
			state = rootSequence->Execute(1.0f);
		}
		if (state == Success) {
			std::cout << "What a successful adventure!\n";
		}
		else if (state == Failure) {
			std::cout << "What a waste of time!\n";
		}
	}
	std::cout << "All Done!\n";
}