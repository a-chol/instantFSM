#include <instantFSM.h> 
#include <string.h>

#include "gtest/gtest.h"

using namespace ifsm;


/**
Canonical
*/
TEST(instantFSM_multiTU, Canonical){
  StateMachine machine;
  machine.enter();
  ASSERT_TRUE(machine.inState("root"));
  machine.leave();
  ASSERT_FALSE(machine.inState("root"));
}

int main(int argc, char** argv){
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}