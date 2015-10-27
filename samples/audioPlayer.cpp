
/**

music player logic using instantFSM

*/

#include "instantFSM.h"

#include <string>
#include <algorithm>

/**
  PlayerInterface displays buttons as ASCII art in stdout.
  It does not need worry about which button can be shown in any given state,
  only about displaying what is asked for : play or pause, stop shown or hidden.
  We use a switch flag to chose between play and pause but we could have used an activation
  flag like for the stop button all the same. The idea of not displaying both play and pause buttons
  at the same time would be handled only in the playerLogic state machine.
*/
struct PlayerInterface{

  PlayerInterface();
  
  void print();
  
  void showPlay();
  void showPause();
  void enableStop(bool enable);
  
  private:
  bool mShowPlay;//if true, show play, else show pause
  bool mEnableStop;
  
  std::string mPlayGraph;
  std::string mPauseGraph;
  std::string mStopGraph;
  std::string mEmptyGraph;

  static const unsigned int GraphWidth = 22;
  static const unsigned int GraphHeight = 15;
};


int main(int argc, char** argv){
  
  PlayerInterface gui;
  
  bool loop = true;
  
  /**
    Here is the interesting part : 
    The logic of the player is defined as 3 states : stopped, playing and idle.
    - in stopped, the play button is shown instead of pause, the stop button is hidden
    - in playing, the pause button is shown instead of play and the stop button is shown
    - in pause, the play button is shown as well as the stop button
    Transitions between states are handled using events. If a stop event is issued even though 
    we already are in the stop state, nothing happens and we don't even need to think about it.
    
    Display characteristics of the PlayerInterface are set when entering states using OnEntry functions.
    This is the best way to ensure that whatever which transition has been taken to activate the state,
    the display will correspond to the current state. 
    Moreover, you are not concerned about adding new transitions.
    
  */
  using namespace ifsm;
  StateMachine playerLogic(
    
    OnEvent("quit", [&loop](){loop=false;}),
  
    State("stopped", initialTag,
    
      OnEntry(
        [&gui](){
          gui.showPlay();
          gui.enableStop(false);
        }
      ),
      
      Transition(
        OnEvent("play"),
        Target("playing")
      ),

      OnExit(
        [&gui](){
          gui.enableStop(true);
        }
      )
    
    ),
    
    State("playing",
      OnEntry(
        [&gui](){
          gui.showPause();
        }
      ),
      
      Transition(
        OnEvent("pause"),
        Target("paused")
      ),
      
      Transition(
        OnEvent("stop"),
        Target("stopped")
      )
    ),
    
    State("paused",
      OnEntry(
        [&gui](){
          gui.showPlay();
        }
      ),
      
      Transition(
        OnEvent("play"),
        Target("playing")
      ),
      
      Transition(
        OnEvent("stop"),
        Target("stopped")
      )
    )
  
  );
  
  playerLogic.enter();

  while (loop) {

    gui.print();

    std::string command;
    std::getline(std::cin, command);
    
    playerLogic.pushEvent(command);
  }
  
}

PlayerInterface::PlayerInterface()
: mShowPlay(false)
, mEnableStop(false){
  mPlayGraph = "+--------------------+"
              "|                    |"
              "|  88888             |"
              "|  88888888          |"
              "|  888   8888        |"
              "|  888     8888      |"
              "|  888        8888   |"
              "|  888          888  |"
              "|  888        8888   |"
              "|  888     8888      |"
              "|  888   8888        |"
              "|  88888888          |"
              "|  88888             |"
              "|                    |"
              "+--------------------+";
              
  mPauseGraph = "+--------------------+"
               "|                    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|    888      888    |"
               "|                    |"
               "+--------------------+";
               
  mStopGraph = "+--------------------+"
              "|                    |"
              "|  8888888888888888  |"
              "|  8888888888888888  |"
              "|  888          888  |"
              "|  888          888  |"
              "|  888          888  |"
              "|  888          888  |"
              "|  888          888  |"
              "|  888          888  |"
              "|  888          888  |"
              "|  8888888888888888  |"
              "|  8888888888888888  |"
              "|                    |"
              "+--------------------+";
              
  mEmptyGraph = "+--------------------+"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "|                    |"
              "+--------------------+";
}

void PlayerInterface::print(){
  for (unsigned short line = 0 ; line<GraphHeight ; ++line){
    if (mShowPlay){
      std::for_each(&mPlayGraph[line * GraphWidth], &mPlayGraph[ (line+1) * GraphWidth], [](char c){std::cout << c; });
    } else {
      std::for_each(&mPauseGraph[line * GraphWidth], &mPauseGraph[(line+1) * GraphWidth], [](char c){std::cout << c; });
    }
    
    if (mEnableStop){
      std::for_each(&mStopGraph[line * GraphWidth], &mStopGraph[(line + 1) * GraphWidth], [](char c){std::cout << c; });
    } else {
      std::for_each(&mEmptyGraph[line * GraphWidth], &mEmptyGraph[(line + 1) * GraphWidth], [](char c){std::cout << c; });
    }
    
    std::cout<<std::endl;
  }
  std::cout<<"commands: stop, play, pause, quit"<<std::endl;
}
  
void PlayerInterface::showPlay(){
  mShowPlay=true;
}

void PlayerInterface::showPause(){
  mShowPlay=false;
}

void PlayerInterface::enableStop(bool enable){
  mEnableStop = enable;
}
