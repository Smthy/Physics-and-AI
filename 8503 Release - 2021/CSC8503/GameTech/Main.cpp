#include "../../Common/Window.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourSelector.h"

#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"

#include "TutorialGame.h"
#include "Level1.h"
#include "Level2.h"
using namespace NCL;
using namespace CSC8503;

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
	
	if (Window::GetKeyboard()-> KeyDown(KeyboardKeys::ESCAPE)) {
		
		return PushdownResult::Pop;
	}							
	return PushdownResult::NoChange;
};

void OnAwake() override {
	std::cout << "Entering 1" << std::endl;
	Load();
}
void Load(){
	Window* g = Window::GetWindow();
	srand(time(0));
	g->ShowOSPointer(false);
	g->LockMouseToWindow(true);
	Level1* l1 = new Level1();
	g->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!

	
	while (g->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = g->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			g->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			g->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			g->SetWindowPosition(0, 0);
		}

		g->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));
		
		l1->UpdateGame(dt);		
	}
	PushdownResult::Pop;
}
protected:
	float pauseReminder = 1;
	TutorialGame* game = nullptr;
};

class GameScreen2 : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {

			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
};

void OnAwake() override {
	std::cout << "Entering 2" << std::endl;
	load();
}
void load() {
	Window* g = Window::GetWindow();
	srand(time(0));
	g->ShowOSPointer(false);
	g->LockMouseToWindow(true);
	Level2* l2 = new Level2();
	g->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!


	while (g->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = g->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			g->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			g->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			g->SetWindowPosition(0, 0);
		}

		g->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		l2->UpdateGame(dt);
	}
	PushdownResult::Pop;
}
protected:
	float pauseReminder = 1;
	TutorialGame* game = nullptr;
};

class IntroScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
	Debug::FlushRenderables(dt);
	renderer->Render();

	renderer->DrawString("Physics and AI", Vector2(20, 30), Debug::DARKRED, 50.0f);
	renderer->DrawString("Physics Level - 1", Vector2(20, 50), Debug::DARKGREEN, 35.0f);
	renderer->DrawString("AI Level - 2", Vector2(20, 60), Debug::DARKGREEN, 35.0f);
	renderer->DrawString("Exit Game - Escape", Vector2(20, 70), Debug::DARKBLUE , 35.0f);

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
				
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::SPACE)) {
		*newState = new GameScreen();
		return PushdownResult::Push;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
		return PushdownResult::Pop;
	}
				
	return PushdownResult::NoChange;
};

void OnAwake() override {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	Debug::SetRenderer(renderer);
}

protected:
	float pauseReminder = 1;
	TutorialGame* game = nullptr;
	GameWorld* world;
	GameTechRenderer* renderer;
};


void TestPushdownAutomata(Window* w) {
	PushdownMachine machine(new IntroScreen());
	while (w->UpdateWindow()) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if(!machine.Update(dt)) {
			return;
		}
	}

}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);	
	if (!w->HasInitialised()) {
		return -1;
	}	
	srand(time(0));
	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);
	
	TestPushdownAutomata(w);
}