#pragma once
#include <fstream>
#include "GameTechRenderer.h"
#include "../CSC8503Common/StateGameObject.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"

namespace NCL {
	namespace CSC8503 {
		class Level1 {
		public:
			Level1();
			~Level1();
			virtual void UpdateGame(float dt);

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			void InitGameExamples();
			void InitSphereOnly();
			void InitMainMenu();
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void BridgeConstraintTest();
			void InitLevel1(); //Marble Run
			void InitLevel2(); //Maze
			void AIMovement(float dt);

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			void MovingWall(float dt);
			void WindMillSpin(float dt);

			void TestStateMachine();
			void AIMovementStateMachine();
			//void TestPathfinding();
			void DisplayPathfinding();
			void DrawPath();
			void TestBehaviourTree();

			void MazeLoader(const std::string& filename);

			GameObject* AddFloorToWorld(const Vector3& position, string name);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, string name, float elstacity,float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 Color, string name, float elstacity,float inverseMass = 10.0f);
			GameObject* AddAABBCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 Color, string name, float elstacity,float inverseMass = 10.0f);

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);
			StateGameObject* AddStateObjectToWorld(const Vector3& position);

			GameTechRenderer* renderer;
			PhysicsSystem* physics;
			GameWorld* world;
			GameObject* ball;
			GameObject* bAI;
			GameObject* movingWall;
			GameObject* windMill;
			GameObject* windMill_2;
			StateGameObject* testStateObject;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;


			OGLMesh* capsuleMesh = nullptr;
			OGLMesh* cubeMesh = nullptr;
			OGLMesh* sphereMesh = nullptr;
			OGLTexture* basicTex = nullptr;
			OGLShader* basicShader = nullptr;

			//Coursework Meshes
			OGLMesh* charMeshA = nullptr;
			OGLMesh* charMeshB = nullptr;
			OGLMesh* enemyMesh = nullptr;
			OGLMesh* bonusMesh = nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject = nullptr;
			Vector3 lockedOffset = Vector3(0, 14, 20);

			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			int pass = 0;
			Vector3 pos[2] = { Vector3(51, -0.5, 47), Vector3(51, -6.5, 47) };
			float wallSpeed = 1.0f;
			float speed = 0.5f;

			bool mainMenuActive = false;
			bool level2Loaded = false;

			vector<Vector3> testNodes;


			int bAIPos = 0;
			Vector3 tarPos;
			Vector3 distance;
			int choice = 0;

			int playerFall = 3;


		};

	}
}

