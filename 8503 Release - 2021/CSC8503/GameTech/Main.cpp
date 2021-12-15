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
	pauseReminder -= dt;
	if (pauseReminder < 0) {
		pauseReminder += 1.0f;
	}
	if (Window::GetKeyboard()-> KeyDown(KeyboardKeys::P)) {
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
	TutorialGame* game = nullptr;
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
	TutorialGame* game = nullptr;
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

}

protected:
	float pauseReminder = 1;
	TutorialGame* game = nullptr;
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
__________
AI Section
__________
*/

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
	
	//TestPushdownAutomata(w);
	srand(time(0));
	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);
	
	TutorialGame* g = new TutorialGame();
	w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	
	
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			w->SetWindowPosition(0, 0);
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));
		
		g->UpdateGame(dt);		
	}
	Window::DestroyGameWindow();
}