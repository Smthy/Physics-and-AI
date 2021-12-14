#pragma once
#include <fstream>
#include "GameTechRenderer.h"
#include "../CSC8503Common/StateGameObject.h"
#include "../CSC8503Common/PhysicsSystem.h"

#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"

namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

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
			void AIMovement();
	
			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			void MovingWall(float dt);
			void WindMillSpin(float dt);

			void TestStateMachine();
			void TestPathfinding();
			void DisplayPathfinding();
			void TestBehaviourTree();

			void MazeLoader(const string& file);

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, string name,float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 Color, string name,float inverseMass = 10.0f);
			GameObject* AddAABBCubeToWorld(const Vector3& position, Vector3 dimensions, Vector4 Color, string name, float inverseMass = 10.0f);

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);
			StateGameObject* AddStateObjectToWorld(const Vector3& position);			

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;
			GameObject*			ball;
			GameObject*			bAI;
			GameObject*			movingWall;
			GameObject*			windMill;
			GameObject*			windMill_2;
			StateGameObject* testStateObject;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;
			

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;
			
			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);

			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}		

			int pass = 0;
			Vector3 pos[2] = { Vector3(51, -0.5, 47), Vector3(51, -6.5, 47) };
			float wallSpeed = 1.0f;	

			bool mainMenuActive = false;
		};

		class PauseScreen : public PushdownState {
			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override {
				std::cout << "Press U to unpause game!\n";
			}
		};

		class GameScreen : public PushdownState {
			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				pauseReminder -= dt;
				if (pauseReminder < 0) {
					
					pauseReminder += 1.0f;
				}
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
					*newState = new PauseScreen();
					return PushdownResult::Push;
				}
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::F1)) {
					std::cout << "Returning to main menu!\n";
					return PushdownResult::Pop;
				}
				if (rand() % 7 == 0) {
					coinsMined++;
				}
				return PushdownResult::NoChange;
			};

			void OnAwake() override {
				std::cout << "Preparing to mine coins!\n";
			}
		protected:
			int coinsMined = 0;
			float pauseReminder = 1;
		};

		class GameScreen1 : public PushdownState {
			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				pauseReminder -= dt;
				if (pauseReminder < 0) {
					
					//game;

					pauseReminder += 1.0f;
				}
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
					*newState = new PauseScreen();
					return PushdownResult::Push;
				}							
				return PushdownResult::NoChange;
			};

			void OnAwake() override {
				std::cout << "Entering 1" << std::endl;
			}
		protected:
			float pauseReminder = 1;
			TutorialGame* game;
		};

		class GameScreen2 : public PushdownState {
			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				pauseReminder -= dt;
				if (pauseReminder < 0) {	
					std::cout << "Level 2" << std::endl;
					pauseReminder += 1.0f;
				}
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
					*newState = new PauseScreen();
					return PushdownResult::Push;
				}
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
					*newState = new GameScreen();
					return PushdownResult::Push;
				}				
				return PushdownResult::NoChange;
			};

			void OnAwake() override {
				std::cout << "Entering 2" << std::endl;
			}
		protected:
			float pauseReminder = 1;
			TutorialGame* game;
		};

		class IntroScreen : public PushdownState {
			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				Debug::FlushRenderables(dt);

				Debug::Print("Physics and AI", Vector2(30, 30), Debug::DARKPURPLE);
				Debug::Print("Physics Level - Click 1", Vector2(20, 30), Debug::DARKPURPLE);
				Debug::Print("AI Level - Click 2", Vector2(25, 30), Debug::DARKPURPLE);
				Debug::Print("QUIT!", Vector2(40, 30), Debug::DARKPURPLE);

				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1)) {
					*newState = new GameScreen1();
					return PushdownResult::Push;
				}

				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM2)) {
					*newState = new GameScreen2();
					return PushdownResult::Push;
				}

				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM3)) {
					*newState = new IntroScreen();
					return PushdownResult::Push;
				}
				/*
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::SPACE)) {
					*newState = new GameScreen();
					return PushdownResult::Push;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
					return PushdownResult::Pop;
				}
				*/
				return PushdownResult::NoChange;
			};

			void OnAwake() override {

			}
		};
		
		/*void PushdownAutomata() {
			PushdownMachine machine(new IntroScreen());			
		}*/
		
		
	}
}

