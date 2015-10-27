#include <instantFSM.h> 
#include <string.h>

#include "gtest/gtest.h"

using namespace ifsm;

/**
Canonical
*/
TEST(instantFSM_multiTU, Canonical2){
  StateMachine machine;
  machine.enter();
  ASSERT_TRUE(machine.inState("root"));
  machine.leave();
  ASSERT_FALSE(machine.inState("root"));
}