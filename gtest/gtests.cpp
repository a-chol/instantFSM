#include <instantFSM.h> 
#include <string.h>

#include "gtest/gtest.h"

using namespace ifsm;

/**
Canonical
*/
TEST(instantFSM, Canonical){
  StateMachine machine;
  ASSERT_FALSE(machine.inState("root"));
  machine.enter();
  ASSERT_TRUE(machine.inState("root"));
  machine.leave();
  ASSERT_FALSE(machine.inState("root"));
}

/**
RootOnEntry
*/
TEST(instantFSM, RootOnEntry){
  bool flag = false;
  StateMachine machine(
    OnEntry([&flag](){
      flag = true;
    })
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_TRUE(flag);
  
}
/**
RootOnExit
*/
TEST(instantFSM, RootOnExit){
  bool flag = false;
  StateMachine machine(
    OnExit([&flag](){
      flag = true;
    })
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.leave();
  
  ASSERT_TRUE(flag);
  
}
/**
RootOnEvent
*/
TEST(instantFSM, RootOnEvent){
  bool flag = false;
  StateMachine machine(
    OnEvent("event",[&flag](){
      flag = true;
    })
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.pushEvent("event");
  
  ASSERT_TRUE(flag);
  
}
/**
ChildOnEntry
*/
TEST(instantFSM, ChildOnEntry){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1", "S1B", "S1Biii" };

  StateMachine machine(
    State("S1", initialTag,
      OnEntry([&lXpResult](){
        lXpResult.push_back("S1");
      }),
      State("S1A",
        OnEntry([&lXpResult](){
          lXpResult.push_back("S1A");
        })
      ),
      State("S1B", initialTag,
        OnEntry([&lXpResult](){
          lXpResult.push_back("S1B");
        }),
        State("S1Bi",
          OnEntry([&lXpResult](){
            lXpResult.push_back("S1Bi");
          })
        ),
        State("S1Bii",
          OnEntry([&lXpResult](){
            lXpResult.push_back("S1Bii");
          })
        ),
        State("S1Biii", initialTag,
          OnEntry([&lXpResult](){
            lXpResult.push_back("S1Biii");
          })
        )
      ),
      State("S1C",
        OnEntry([&lXpResult](){
          lXpResult.push_back("S1C");
        })
      )
    ),
    State("S2",
      OnEntry([&lXpResult](){
        lXpResult.push_back("S2");
      })
    ),
    State("S3",
      OnEntry([&lXpResult](){
        lXpResult.push_back("S3");
      })
    )
  );
  
  ASSERT_TRUE(lXpResult.empty());
  
  machine.enter();
  
  ASSERT_EQ(lXpResult, lRefResult);
  
}
/**
ChildOnExit
*/
TEST(instantFSM, ChildOnExit){
  bool flag = false;
  
  StateMachine machine(
    State("S1", initialTag,
      OnExit([&flag](){
        flag = true;
      })
    )
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.leave();
  
  ASSERT_TRUE(flag);
  
}
/**
ChildOnEvent
*/
TEST(instantFSM, ChildOnEvent){
  bool flag = false;
  
  StateMachine machine(
    State("S1", initialTag,
      OnEvent("event", 
        [&flag](){
          flag = true;
        }
      )
    )
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.pushEvent("event");
  
  ASSERT_TRUE(flag);
  
}
/**
TransitionOnExit
*/
TEST(instantFSM, TransitionOnExit){
  bool flag = false;
  
  StateMachine machine(
    State("S1", initialTag,
      Transition(OnEvent("event"),Target("S2")) ,
      OnExit([&flag](){
        flag = true;
      })
    ) ,
    State("S2")
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.pushEvent("event");
  
  ASSERT_TRUE(flag);
  
}
/**
TransitionAction
*/
TEST(instantFSM, TransitionAction){
  bool flag = false;
  
  StateMachine machine(
    State("S1", initialTag,
      Transition(
        OnEvent("event"),
        Target("S2"), 
        Action([&flag](){
          flag = true;
        })
      )
    ) ,
    State("S2")
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.pushEvent("event");
  
  ASSERT_TRUE(flag);
  
}
/**
TransitionOnEntry
*/
TEST(instantFSM, TransitionOnEntry){
  bool flag = false;
  
  StateMachine machine(
    State("S1", initialTag,
      Transition(OnEvent("event"),Target("S2"))    
    ) ,
    State("S2" ,
      OnEntry([&flag](){
        flag = true;
      })
    )
  );
  
  ASSERT_FALSE(flag);
  
  machine.enter();
  
  ASSERT_FALSE(flag);
  
  machine.pushEvent("event");
  
  ASSERT_TRUE(flag);
  
}
/**
CheckEntryOrder
*/
TEST(instantFSM, CheckEntryOrder){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1 entry", "S1A entry", "S1Ai entry" };


  StateMachine machine(
    State("S1", initialTag,
      OnEntry([&lXpResult](){
        lXpResult.push_back("S1 entry");
      }),
      State("S1A", initialTag,
        OnEntry([&lXpResult](){
          lXpResult.push_back("S1A entry");
        }),
        State("S1Ai", initialTag,
          OnEntry([&lXpResult](){
            lXpResult.push_back("S1Ai entry");
          })
        )
      )
    )
  );

  machine.enter();

  ASSERT_EQ(lRefResult, lXpResult);

}
/**
CheckExitOrder
*/
TEST(instantFSM, CheckExitOrder){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1Ai exit", "S1A exit", "S1 exit" };

  StateMachine machine(
    State("S1", initialTag,
      OnExit([&lXpResult](){
        lXpResult.push_back("S1 exit");
      }),
      State("S1A", initialTag,
        OnExit([&lXpResult](){
          lXpResult.push_back("S1A exit");
        }),
        State("S1Ai", initialTag,
          OnExit([&lXpResult](){
            lXpResult.push_back("S1Ai exit");
          })
        ),
        State("S1Aii",
          OnExit([&lXpResult](){
            lXpResult.push_back("S1Aii exit");
          })
        )
      ),
      State("S1B", 
        OnExit([&lXpResult](){
          lXpResult.push_back("S1B exit");
        }),
        State("S1Bi", initialTag,
          OnExit([&lXpResult](){
            lXpResult.push_back("S1Bi exit");
          })
        ),
        State("S1Bii",
          OnExit([&lXpResult](){
            lXpResult.push_back("S1Bii exit");
          })
        )
      )
    ),
    State("S2", 
      OnExit([&lXpResult](){
        lXpResult.push_back("S2 exit");
      }),
      State("S2A", initialTag,
        OnExit([&lXpResult](){
          lXpResult.push_back("S2A exit");
        }),
        State("S2Ai", initialTag,
          OnExit([&lXpResult](){
            lXpResult.push_back("S2Ai exit");
          })
        ),
        State("S2Aii",
          OnExit([&lXpResult](){
            lXpResult.push_back("S2Aii exit");
          })
        )
      ),
      State("S2B",
        OnExit([&lXpResult](){
          lXpResult.push_back("S2B exit");
        }),
        State("S2Bi", initialTag,
          OnExit([&lXpResult](){
            lXpResult.push_back("S2Bi exit");
          })
        ),
        State("S2Bii",
          OnExit([&lXpResult](){
            lXpResult.push_back("S2Bii exit");
          })
        )
      )
    )
  );


  machine.enter();

  machine.leave();

  ASSERT_EQ(lRefResult, lXpResult);

}
/**
TargetlessTransitions
*/
TEST(instantFSM, TargetlessTransitions){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "targetless in S1" , "OnEvent in S1" , 
    "targetless in S2A", "OnEvent in S2A"
  };


  StateMachine machine(parallelTag,
    State("S1",
      Transition(OnEvent("event"), 
        Action([&lXpResult](){
          lXpResult.push_back("targetless in S1");
        })
      ),
      OnEvent("event",
        [&lXpResult](){
          lXpResult.push_back("OnEvent in S1");
        }
      )
    ),

    State("S2",

      Transition(OnEvent("event"),
        Action([&lXpResult](){
          lXpResult.push_back("targetless in S2");
        })
      ),
      OnEvent("event",
        [&lXpResult](){
          lXpResult.push_back("OnEvent in S2");
        }
      ),

      State("S2A", initialTag,
        Transition(OnEvent("event"),
          Action([&lXpResult](){
            lXpResult.push_back("targetless in S2A");
          })
        ),
        OnEvent("event",
          [&lXpResult](){
            lXpResult.push_back("OnEvent in S2A");
          }
        )
      ),

      State("S2B",
        Transition(OnEvent("event"),
          Action([&lXpResult](){
            lXpResult.push_back("targetless in S2B");
          })
        ),
        OnEvent("event",
          [&lXpResult](){
            lXpResult.push_back("OnEvent in S2B");
          }
        )
      )

    )
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("S2"));
  ASSERT_TRUE(machine.inState("S2A"));
  ASSERT_FALSE(machine.inState("S2B"));

  machine.pushEvent("event");

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("S2"));
  ASSERT_TRUE(machine.inState("S2A"));
  ASSERT_FALSE(machine.inState("S2B"));

  ASSERT_EQ(lRefResult, lXpResult);

}
/**
CheckTransitionOrder
*/
TEST(instantFSM, CheckTransitionOrder){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "transition from S1A" };


  StateMachine machine(
    State("S1", initialTag,
      Transition(OnEvent("event"), Target("S2"), Action([&lXpResult](){
        lXpResult.push_back("transition from S1");
      })),
      State("S1A", initialTag,
        Transition(OnEvent("event"), Target("S2"), Action([&lXpResult](){
          lXpResult.push_back("transition from S1A");
        }))
      )
    ),
    State("S2")
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("S1A"));
  ASSERT_FALSE(machine.inState("S2"));

  machine.pushEvent("event");

  ASSERT_FALSE(machine.inState("S1"));
  ASSERT_FALSE(machine.inState("S1A"));
  ASSERT_TRUE(machine.inState("S2"));

  ASSERT_EQ(lRefResult, lXpResult);

}
/**
CheckParallelTransitionOrder
*/
TEST(instantFSM, CheckParallelTransitionOrder){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "transition from S1", "transition from S2" };


  StateMachine machine(parallelTag,
    State("S1",
      Transition(OnEvent("event"), Action([&lXpResult](){
        lXpResult.push_back("transition from S1");
      }))
    ),
    State("S2",
      Transition(OnEvent("event"), Action([&lXpResult](){
        lXpResult.push_back("transition from S2");
      }))
    )
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("S2"));

  machine.pushEvent("event");

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("S2"));

  ASSERT_EQ(lRefResult, lXpResult);

}
/**
ParallelOnEntry
*/
TEST(instantFSM, ParallelOnEntry){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1", "S2", "S3" };

  StateMachine machine(parallelTag,
    State("S1",
      OnEntry([&lXpResult](){
        lXpResult.push_back("S1");
      })
    ),
    State("AAAAS2",
      OnEntry([&lXpResult](){
        lXpResult.push_back("S2");
      })
    ),
    State("ZZZZS2",
      OnEntry([&lXpResult](){
        lXpResult.push_back("S3");
      })
    )
  );

  machine.enter();

  ASSERT_EQ(lRefResult, lXpResult);
  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("AAAAS2"));
  ASSERT_TRUE(machine.inState("ZZZZS2"));

}

/**
ParallelOnExit
*/
TEST(instantFSM, ParallelOnExit){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S3", "S2", "S1" };

  StateMachine machine(parallelTag,
    State("S1",
      OnExit([&lXpResult](){
        lXpResult.push_back("S1");
      })
    ),
    State("AAAAS2",
      OnExit([&lXpResult](){
        lXpResult.push_back("S2");
      })
    ),
    State("ZZZZS2",
      OnExit([&lXpResult](){
        lXpResult.push_back("S3");
      })
    )
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("AAAAS2"));
  ASSERT_TRUE(machine.inState("ZZZZS2"));

  machine.leave();

  ASSERT_EQ(lRefResult, lXpResult);

}
/**
ParallelOnEvent
*/
TEST(instantFSM, ParallelOnEvent){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1", "S2", "S3" };

  StateMachine machine(parallelTag,
    State("S1",
      OnEvent("event",[&lXpResult](){
        lXpResult.push_back("S1");
      })
    ),
    State("AAAAS2",
      OnEvent("event", [&lXpResult](){
        lXpResult.push_back("S2");
      })
    ),
    State("ZZZZS2",
      OnEvent("event", [&lXpResult](){
        lXpResult.push_back("S3");
      })
    )
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("AAAAS2"));
  ASSERT_TRUE(machine.inState("ZZZZS2"));

  machine.pushEvent("event");

  ASSERT_EQ(lRefResult, lXpResult);

  machine.leave();

  ASSERT_FALSE(machine.inState("S1"));
  ASSERT_FALSE(machine.inState("AAAAS2"));
  ASSERT_FALSE(machine.inState("ZZZZS2"));

}


/**
TransitionToParallelChild
*/
TEST(instantFSM, TransitionToParallelChild){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1_entry", "S1_exit", "S1_to_S2B", "S2_entry", "S2A_entry", "S2B_entry" };

  StateMachine machine(
    State("S1", initialTag,
      OnEntry([&lXpResult](){
        lXpResult.push_back("S1_entry");
      }),
      Transition(
        OnEvent("event"),
        Target("S2B"),
        Action([&lXpResult](){
          lXpResult.push_back("S1_to_S2B");
        })
      ),
      OnExit([&lXpResult](){
        lXpResult.push_back("S1_exit");
      })
    ),
    State("S2", parallelTag,
      OnEntry([&lXpResult](){
        lXpResult.push_back("S2_entry");
      }),
      State("S2A",
        OnEntry([&lXpResult](){
          lXpResult.push_back("S2A_entry");
        })
      ),
      State("S2B",
        OnEntry([&lXpResult](){
          lXpResult.push_back("S2B_entry");
        })
      )
    )
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_FALSE(machine.inState("S2"));
  ASSERT_FALSE(machine.inState("S2A"));
  ASSERT_FALSE(machine.inState("S2B"));

  machine.pushEvent("event");

  ASSERT_FALSE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("S2"));
  ASSERT_TRUE(machine.inState("S2A"));
  ASSERT_TRUE(machine.inState("S2B"));

  ASSERT_EQ(lRefResult, lXpResult);

}

/**
ParallelConflictingTransition
*/
TEST(instantFSM, ParallelConflictingTransition){
  std::vector<std::string> lXpResult;
  std::vector<std::string> lRefResult = { "S1 entry", "SA entry", "SB entry", "SB exit", "SA exit", "S1 exit", "event", "S2 entry" };

  StateMachine machine(
    State("S1", parallelTag, initialTag,
      OnEntry([&lXpResult](){lXpResult.push_back("S1 entry");}),
      OnExit([&lXpResult](){lXpResult.push_back("S1 exit"); }),

      State("SA", //one transition targets S2
        OnEntry([&lXpResult](){lXpResult.push_back("SA entry"); }),
        OnExit([&lXpResult](){lXpResult.push_back("SA exit"); }),
        Transition(OnEvent("event"), Target("S2"), Action([&lXpResult](){
          lXpResult.push_back("event");
        }))
      ),

      State("SB",//while the other targets S3 on the same event
        OnEntry([&lXpResult](){lXpResult.push_back("SB entry"); }),
        OnExit([&lXpResult](){lXpResult.push_back("SB exit"); }),
        Transition(OnEvent("event"), Target("S3"), Action([&lXpResult](){
          lXpResult.push_back("event");
        }))
      )

    ),

    State("S2",
      OnEntry([&lXpResult](){lXpResult.push_back("S2 entry"); }),
      OnExit([&lXpResult](){lXpResult.push_back("S2 exit"); })
    ),

    State("S3",
      OnEntry([&lXpResult](){lXpResult.push_back("S3 entry"); }),
      OnExit([&lXpResult](){lXpResult.push_back("S3 exit"); })
    )
  );

  machine.enter();

  ASSERT_TRUE(machine.inState("S1"));
  ASSERT_TRUE(machine.inState("SA"));
  ASSERT_TRUE(machine.inState("SB"));
  ASSERT_FALSE(machine.inState("S2"));
  ASSERT_FALSE(machine.inState("S3"));

  machine.pushEvent("event");

  ASSERT_FALSE(machine.inState("S1"));
  ASSERT_FALSE(machine.inState("SA"));
  ASSERT_FALSE(machine.inState("SB"));
  ASSERT_TRUE(machine.inState("S2"));
  ASSERT_FALSE(machine.inState("S3"));

  ASSERT_EQ(lRefResult, lXpResult);

}


/**
TransitionBetweenParallel
*/

/**
Throws AlreadyHasInitial
*/

TEST(instantFSM, ThrowsAlreadyHasInitial){
  ASSERT_THROW(
  StateMachine machine(
    State("S1", initialTag),
    
    State("S2", initialTag)
  )
  , ifsm::AlreadyHasInitial);
  
}

/**
Throws DuplicateStateIdentifier
*/
TEST(instantFSM, ThrowsDuplicateStateIdentifier){
  ASSERT_THROW(
  StateMachine machine(
    State("S1", initialTag),
    
    State("S1")
  )
  , ifsm::DuplicateStateIdentifier);
  
}
/**
Throws NoInitialState
*/
TEST(instantFSM, ThrowsNoInitialState){
  ASSERT_THROW(
  StateMachine machine(
    State("S1"),
    
    State("S2")
  )
  , ifsm::NoInitialState);
  
}
/**
Throws NoSuchState
*/
TEST(instantFSM, ThrowsNoSuchState){
  ASSERT_THROW(
  StateMachine machine(
    State("S1", initialTag,
      Transition(
        OnEvent("event"), 
        Target("doesnotexist")
      )
    )
  );
  , NoSuchState);

}
/**
Throws TargetAlreadySpecified
*/
TEST(instantFSM, ThrowsTargetAlreadySpecified){
  ASSERT_THROW(
    StateMachine machine(
      State("S1",
        Transition(
          OnEvent("event"),
          Target("S2"),
          Target("S3")
        )
      ),

      State("S2")
    )
    , ifsm::TargetAlreadySpecified);

}
/**
Throws ActionAlreadySpecified
*/
TEST(instantFSM, ThrowsActionAlreadySpecified){
  ASSERT_THROW(
    StateMachine machine(
      State("S1",
        Transition(
          OnEvent("event"),
          Action(
            std::function < void(void)>()
          ),
          Action(
            std::function < void(void)>()
          )
        )
      ),

      State("S2")
    )
    , ifsm::ActionAlreadySpecified);

}
/**
Throws ConditionAlreadySpecified
*/
TEST(instantFSM, ThrowsConditionAlreadySpecified){
  ASSERT_THROW(
    StateMachine machine(
      State("S1",
        Transition(
          OnEvent("event"),
          Condition(
            std::function < bool(void)>()
          ),
          Condition(
            std::function < bool(void)>()
          )
        )
      ),

      State("S2")
    )
    , ifsm::ConditionAlreadySpecified);

}
/**
Throws EventAlreadySpecified
*/

TEST(instantFSM, ThrowsEventAlreadySpecified){
  ASSERT_THROW(
    StateMachine machine(
      State("S1",
        Transition(
          OnEvent("event"),
          OnEvent("event2")
        )
      ),

      State("S2")
    )
    , ifsm::EventAlreadySpecified);

}

int main(int argc, char** argv){
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}