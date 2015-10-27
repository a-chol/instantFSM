/**

Game states logic using instantFSM

*/

#include "instantFSM.h"

#include <string>
#include <chrono>
#include <thread>

/**
  The following few classes implement the behaviour of
  corresponding scenes in the workflow of a game.
  There use is based on the regular call of the update(float)
  method that lets the class display the content of its scene.
*/

struct SplashScreen{
  void update(float dt);
};

struct Menu{
  void update(float dt);
};

struct Loader{
  void update(float dt);
};

struct Game{
  void update(float dt);
};

struct Pause{
  void update(float dt);
};

/**
  The Application class aggregates the different scenes, 
  handles user input and regular issu of the "update" event
  to the state machine.
  In order to be properly displayed, each scene needs its update()
  method to be regularly called when the scene is active.
  The dispatch of this call is implemented by the state machine
  by means of a transition in each state that react to the "update" event
  and calls the proper scene update method.
*/
struct Application{
  
  Application();
  
  void start();
  void quit();
  void newgame();
  void loadgame(const std::string& save);
  
  ifsm::StateMachine gameStateLogic;
  bool loop;
  SplashScreen splashscreen;
  Menu menu;
  Loader loader;
  Game game;
  Pause pauseScreen;
  float deltaTime;
  std::string selectedSave;
};

using namespace ifsm;
Application::Application()
: gameStateLogic(
    State("splashscreen", initialTag,
      Transition(OnEvent("splashscreentimer_done"), Target("menu")),
      Transition(OnEvent("update"), Action([=](){
        splashscreen.update(deltaTime);
      }))
    ),
    State("menu",
      Transition(
        OnEvent("quit"), 
        Action([=](){this->quit();})
      ),
      Transition(
        OnEvent("newgame"), 
        Action([=](){this->newgame(); }),
        Target("loading")
      ),
      Transition(
        OnEvent("loadgame"), 
        Action([=](){this->loadgame(selectedSave); }),
        Target("loading")
      ),
      Transition(OnEvent("update"), Action([=](){
        menu.update(deltaTime);
      }))
    ),
    State("loading",
      Transition(OnEvent("update"), Action([=](){
        loader.update(deltaTime);
      })),
      Transition(
        OnEvent("game_loaded"),
        Target("ingame")
      )
    ),
    State("ingame",
      Transition(OnEvent("update"), Action([=](){
        game.update(deltaTime);
      })),
      Transition(OnEvent("pause"), Target("paused"))
    ),
    State("paused",
      Transition(OnEvent("update"), Action([=](){
        pauseScreen.update(deltaTime);
      })),
      Transition(OnEvent("unpause"), Target("ingame")),
      Transition(
        OnEvent("quit"), 
        Action([=](){this->quit();})
      )
    )
  ) {
  
}

void Application::start(){

  gameStateLogic.enter();
  
  loop = true;
  std::thread updateThread([=](){
    std::chrono::steady_clock::time_point before = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now;
    while (loop){
      now = std::chrono::steady_clock::now();
      deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count()/1000.f;
      before = now;
      gameStateLogic.pushEvent("update");
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
  
  
  while (loop) {
    std::string command;
    std::getline(std::cin, command);
    gameStateLogic.pushEvent(command);
  }

  gameStateLogic.leave();
  
  updateThread.join();
}

void Application::quit(){
  loop = false;
}

void Application::newgame(){

}

void Application::loadgame(const std::string& save){

}

void SplashScreen::update(float dt){
  std::cout << "SplashScreen::update -> command : splashscreentimer_done" << std::endl;
}

void Menu::update(float dt){
  std::cout << "Menu::update -> commands : newgame, loadgame, quit" << std::endl;
}

void Loader::update(float dt){
  std::cout << "Loader::update -> command : game_loaded" << std::endl;
}

void Game::update(float dt){
  std::cout << "Game::update -> command : pause" << std::endl;
}

void Pause::update(float dt){
  std::cout << "Pause::update -> commands : unpause, quit" << std::endl;
}

int main(){

  std::cout<<
  "usage : input the each state's commands in stdin to trigger transitions"
  <<std::endl;
  Application app;
  app.start();

  return 0;
}