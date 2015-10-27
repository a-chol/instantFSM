
/*
Copyright (c) 2014 - Alexis Chol

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
Cheat Sheet : 

StateMachine myMachine( parallelTag|State|OnEntry|OnExit|OnEvent|Transition ) 
    : instantiate the FSM
  myMachine.enter() : enter the root state
  myMachine.pushEvent(std::string("myEvent")) : push an event
  myMachine.inState(std::string("stateName")) : returns true of the named state is active
  myMachine.leave() : quit all active states
  
State(std::string("stateName"), parallelTag|initialTag|State|OnEntry|OnExit|OnEvent|Transition) : create a state
  
  -> OnEntry( void(void) | void(StateMachine&) ) : callback triggered when parent state is entered
  -> OnExit( void(void) | void(StateMachine&) ) : callback triggered when parent state is exited
  -> OnEvent( std::string("myEvent"), void(void) | void(StateMachine&) ) : callback triggered when the event "myEvent" is pushed while the parent state is active.

Transition( OnEvent|Target|Action|Condition ) : add a transition
  -> OnEvent( std::string("myEvent") ) : event triggering the transition
  -> Target( std::string("stateName") ) : state to activate when the transition is realised
  -> Action( void(void)|void(StateMachine&) ) : callback triggered after leaving the parent state and before the target state is entered.
  -> Condition( bool(void)|bool(StateMachine&) ) : callback preventing the transition from executing when it returns false
  
*/


#ifndef INSTANTFSM_H
#define INSTANTFSM_H

#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <utility>
#include <cstddef>
#include <algorithm>
#include <queue>
#include <list>
#include <type_traits>
#include <memory>
#include <stdexcept>

// Is noexcept supported?
#if !defined(_MSC_FULL_VER) || _MSC_VER > 1800
#  define NOEXCEPT noexcept
#else
#  define NOEXCEPT
#endif

namespace ifsm{
  class StateMachine;

  namespace priv{
    class StateImpl;
  }

  class StateMachineException : public std::logic_error {
  protected:
    StateMachineException(const std::string& pWhat)
    : std::logic_error(pWhat){}
  };

  class AlreadyHasInitial : public StateMachineException{
  public:
    AlreadyHasInitial(const std::string& pState)
      : StateMachineException("State "+pState+" has two children is initialTag parameter set. Only one initial child is permitted.")
      , mName(pState){

    }

    virtual ~AlreadyHasInitial() NOEXCEPT{}

  public:
    const std::string mName;
  };

  class DuplicateStateIdentifier : public StateMachineException{
  public:
    DuplicateStateIdentifier(const std::string& pState)
      : StateMachineException("The StateMachine declares two States named "+pState+". Unique names are required.") 
      , mName(pState){

    }
    virtual ~DuplicateStateIdentifier() NOEXCEPT{}

  public:
    const std::string mName;
  };

  class NoInitialState : public StateMachineException{
  public:
    NoInitialState(const std::string& pState)
      : StateMachineException("State "+pState+" is not parallel and doesn't have any initial child. One initial child is required for non-parallel nested States.")
      , mName(pState){

    }
    virtual ~NoInitialState() NOEXCEPT{}

  public:
    const std::string mName;
  };

  class NoSuchState : public StateMachineException{
  public:
    NoSuchState(const std::string& pState)
      : StateMachineException("A transition targets a State named \""+pState+"\" which doesn't exist in the StateMachine.")
      , mName(pState){

    }

    virtual ~NoSuchState() NOEXCEPT{}

  public:
    const std::string mName;
  };


  class TargetAlreadySpecified : public StateMachineException{
  public:
    TargetAlreadySpecified(const std::string& pTarget)
      : StateMachineException("A Transition defines two Targets. Only one Target() per Transition is allowed") 
      , mTarget(pTarget){

    }

    virtual ~TargetAlreadySpecified() NOEXCEPT{}

  public:
    const std::string mTarget;
  };

  class ActionAlreadySpecified : public StateMachineException{
  public:
    ActionAlreadySpecified()
      : StateMachineException("A transition has two Action parameters defined. Only one is allowed.")
      {}
  };

  class ConditionAlreadySpecified : public StateMachineException{
  public:
    ConditionAlreadySpecified()
      : StateMachineException("A transition has two Condition parameters defined. Only one is allowed.")
      {}
  };

  class EventAlreadySpecified : public StateMachineException{
  public:
    EventAlreadySpecified()
      : StateMachineException("A transition has two OnEvent parameters defined. Only one is allowed.")
      {}
  };
}

namespace ifsm{ 
  namespace priv{

    template <class CallableType, typename... PTypes>
    struct is_callable_with{
      typedef char yes[1];
      typedef char no[2];

      template <typename CType>
      static yes& test(decltype(std::declval<CType>()(std::declval<PTypes>()...))*);

      template <typename>
      static no& test(...);

      static const bool value = sizeof(test<CallableType>(0)) == sizeof(yes);
    };

    template <class CallableType>
    struct is_callable{
      typedef char yes[1];
      typedef char no[2];

      template <typename CType>
      static yes& test(decltype(std::declval<CType>()())*);

      template <typename>
      static no& test(...);

      static const bool value = sizeof(test<CallableType>(0)) == sizeof(yes);
    };

    template <class CallableType, class Ret>
    struct returns{
      typedef char yes[1];
      typedef char no[2];

      template <typename CType, typename B = typename std::enable_if<std::is_same<decltype(std::declval<CType>()()), Ret>::value>::type>
      static yes& test(int);

      template <typename>
      static no& test(...);

      static const bool value = sizeof(test<CallableType>(0)) == sizeof(yes);
    };

    template <class CallableType, class Ret, typename... Params>
    struct returns_with{
      typedef char yes[1];
      typedef char no[2];

      template <typename CType, typename B = typename std::enable_if<std::is_same<decltype(std::declval<CType>()(std::declval<Params>()...)), Ret>::value>::type>
      static yes& test(int);

      template <typename>
      static no& test(...);

      static const bool value = sizeof(test<CallableType>(0)) == sizeof(yes);
    };

    //turn f() into f(SM)
    template<class Callable, typename B = typename std::enable_if<is_callable<Callable>::value>::type>
    std::function<void(StateMachine&) > fixParams(Callable && pCallable){
      return std::function<void(StateMachine&) >([pCallable](StateMachine& ){
        pCallable();
      });
    }

    //turn f(SM) into f(SM) : no change
    template<class Callable, typename B = typename std::enable_if<is_callable_with<Callable, StateMachine&>::value>::type, typename U = void>
    std::function<void(StateMachine&) > fixParams(Callable && pCallable){
      return std::function<void(StateMachine&) >(pCallable);
    }

    //turn bool f() into bool f(SM)
    template<class Callable, typename B = typename std::enable_if<returns<Callable, bool>::value>::type, typename U = void, typename X = void>
    std::function<bool(const StateMachine&)> fixConditionParams(Callable && pCallable){
      return std::function<bool(const StateMachine&)>([pCallable](const StateMachine& ){
        return pCallable();
      });
    }

    //turn bool f(SM) into bool f(SM) : no change
    template<class Callable, typename B = typename std::enable_if<returns_with<Callable, bool, const StateMachine&>::value>::type, typename U = void, typename X = void, typename Y = void>
    std::function<bool(const StateMachine&)> fixConditionParams(Callable && pCallable){
      return std::function<bool(const StateMachine&)>(pCallable);
    }

  }
}

namespace ifsm{
  
  namespace priv{
    class OnEntryAction;
    class OnExitAction;
  }

  template <class FunType>
  priv::OnEntryAction OnEntry(FunType && pFun);

  template <class FunType>
  priv::OnExitAction OnExit(FunType && pFun);

  
}

namespace ifsm{
  namespace priv{
    class OnEntryAction{

      template <class FunType>
      friend OnEntryAction ifsm::OnEntry(FunType && pFun);

      friend class StateDef;
      friend class StateImpl;

    private:

      OnEntryAction(std::function<void(StateMachine&) > && pFun)
        : mFun(std::move(pFun))
      {}

    private:
      void operator()(StateMachine& pRoot){
        mFun(pRoot);
      }

    private:
      std::function<void(StateMachine&) > mFun;

    };

    class OnExitAction{

      template <class FunType>
      friend OnExitAction ifsm::OnExit(FunType && pFun);

      friend class StateDef;
      friend class StateImpl;

      OnExitAction(std::function<void(StateMachine&) > && pFun)
        : mFun(std::move(pFun))
      {}

    private:

      void operator()(StateMachine& pRoot){
        mFun(pRoot);
      }

    private:
      std::function<void(StateMachine&) > mFun;
    };
  }
}

namespace ifsm{
  namespace priv{
    class TransitionTarget;
    class TransitionAction;
    class TransitionCondition;
    class TransitionEvent;
    class TransitionImpl;
    class TransitionDef;
  }

  /*
  Define the target name of a transition
  */
  inline priv::TransitionTarget Target(const std::string& pTargetName);
  template <class FunType>

  /*
  Define the callback that will be call during execution of the transition.
  pAction must be callable in the form 
    void(StateMachine&)
  or
    void(void)

  it is called after leaving the source state and before entering the target state.
  */
  priv::TransitionAction Action(FunType&& pAction);
  template <class FunType>

  /*
  Define a condition for the selection of the transition as a callback callable in the form
    bool(StateMachine&)
  or
    bool(void)
  */
  priv::TransitionCondition Condition(FunType&& pCondition);

  /*
  Define the event that will trigger the transition
  */
  inline priv::TransitionEvent OnEvent(const std::string& pEvent);

  /*
  Creates a new transition when called as a parameter of the State function.
  */
  template <typename... Args>
  priv::TransitionDef Transition(Args&&... pArgs);

  /*
  Shortcut for defining a targetless transition when called as a parameter of the State function.
  */
  template <class FunType>
  priv::TransitionDef OnEvent(const std::string& pEvent, FunType && pFun);

  namespace priv{
    class  TransitionTarget{

      friend TransitionTarget ifsm::Target(const std::string& pTargetName);
      friend class ifsm::priv::TransitionDef;

      inline TransitionTarget(const std::string& pTargetName);

      std::string mTargetName;
    };

    class TransitionAction{

      template <class FunType>
      friend TransitionAction ifsm::Action(FunType&& pAction);
      friend class ifsm::priv::TransitionDef;

      inline TransitionAction(std::function<void(StateMachine&)>&& pFun);

      inline void operator()(StateMachine& pRoot);

      std::function<void(StateMachine&)> mFun;
    };

    class TransitionCondition{

      template <class FunType>
      friend TransitionCondition ifsm::Condition(FunType&& pCondition);
      friend class ifsm::priv::TransitionDef;

      inline TransitionCondition(std::function<bool(const StateMachine&)>&& pCond);

      inline bool operator()(const StateMachine& pRoot);
      std::function<bool(const StateMachine&)> mCond;
    };

    class TransitionEvent{

      friend TransitionEvent ifsm::OnEvent(const std::string& pEvent);
      friend class ifsm::priv::TransitionDef;

      inline TransitionEvent(const std::string& pEvent);

      std::string mEvent;
    };
    
    class TransitionImpl;

    class TransitionDef{
      template <typename... Args>
      friend ifsm::priv::TransitionDef ifsm::Transition(Args&&... pArgs);

      template <class FunType>
      friend ifsm::priv::TransitionDef ifsm::OnEvent(const std::string& pEvent, FunType && pFun);
      
      friend class TransitionImpl;

      friend class StateImpl;

    public:
      inline TransitionDef(TransitionDef&& pRhs); 

    private:
      template <typename... Params>
      inline TransitionDef(Params && ... pParams); 

      inline void addParameter(priv::TransitionTarget && pTarget);

      inline void addParameter(priv::TransitionAction && pAction);

      inline void addParameter(priv::TransitionCondition && pCondition);

      inline void addParameter(priv::TransitionEvent && pEvent);

      template <class T>
      void addParameters(T&& pParam1);

      template <class T, typename... Params>
      void addParameters(T&& pParam1, Params && ... pParams);

    private:
      std::string mTarget;
      std::string mEvent;
      std::function<void(StateMachine&)> mAction;
      std::function<bool(const StateMachine&)> mCondition;
    };
  };

  template <typename... Params>
  priv::TransitionDef Transition(Params && ... pParams);

  namespace priv{
    class TransitionImpl{
      friend class ifsm::StateMachine;
      friend class ifsm::priv::StateImpl;
    
    public:
      inline TransitionImpl(const TransitionDef& pDef);

    private:
      inline void setSource(StateImpl* pSource);

      inline void setTarget(StateImpl* pTarget);

      inline const StateImpl* getSource() const;
      
      inline const StateImpl* getTarget() const;

      inline const std::string& getEvent() const;

      inline bool test(const StateMachine& pRoot) const;

      inline void doAction(StateMachine& pRoot) const;

    
    private:
      StateImpl* mSource;
      StateImpl* mTarget;
      std::string mEvent;
      std::function<void(StateMachine&)> mAction;
      std::function<bool(const StateMachine&)> mCondition;

    };
  }
  
};

namespace ifsm{
  namespace priv{
    struct initialTag_t{};
    struct parallelTag_t{};
  }

  static priv::initialTag_t initialTag;
  static priv::parallelTag_t parallelTag;

  namespace priv{
    template <class SearchType>
    struct StateIterator;
  }
  
}

namespace ifsm{
  namespace priv{
    class StateDef;
    class StateImpl;
  }

  template <typename... Args>
  priv::StateDef State(const std::string& pName, Args&&... pArgs);
  
}

namespace ifsm{

  template <typename... Args>
  ifsm::priv::StateDef State(const std::string& pName, Args&&... pArgs);

  namespace priv{
  
    class StateDef{

      template <typename... Args>
      friend StateDef ifsm::State(const std::string& pName, Args&&... pArgs);

      friend class ifsm::priv::StateImpl;
      friend class ifsm::StateMachine;

    private:

      inline StateDef(const std::string& pName);

      template <typename... Params>
      inline StateDef(const std::string& pName, Params && ... pParameters);

      inline void addParameter(StateDef && pState);

      inline void addParameter(TransitionDef && pTransition);

      inline void addParameter(OnEntryAction && pAction);

      inline void addParameter(OnExitAction && pAction);

      inline void addParameter(initialTag_t& pTag);

      inline void addParameter(parallelTag_t& pTag);

      template <class FirstP>
      void addParameters(FirstP && pFirst);

      template <class FirstP, typename... Params>
      void addParameters(FirstP && pFirst, Params && ... pParameters);

    private:
      std::string                 mName;
      bool                        mIsInitial;
      bool                        mIsParallel;
      std::vector<StateDef>       mChildren;
      std::vector<TransitionDef>  mTransitions;
      std::vector<OnEntryAction>  mOnEntryActions;
      std::vector<OnExitAction>   mOnExitActions;
    };

    class StateImpl{
      //everything is private, only StateMachine is allowed to use a StateImpl
      friend class ifsm::StateMachine;

      template <typename>
      friend struct ifsm::priv::StateIterator;
      
    public:
      typedef std::vector<StateImpl*> ChildrenMap;
      typedef std::multimap<std::string, std::unique_ptr<TransitionImpl>> TransitionsMap;
      
    public:
      inline StateImpl();

      inline virtual ~StateImpl();
      
    protected:
      inline void build(StateMachine* pRoot, StateImpl* pParent, StateDef&& pDef);

      inline const std::string& getName() const;

      inline bool isAtomic() const;

      inline bool isInitial() const;

      inline bool isParallel() const;

      inline virtual bool isActive() const;

      inline void enter();

      inline void leave();

    protected:
      std::string                                               mName;
      StateImpl*                                                mParent;
      StateMachine*                                             mRoot;
      bool                                                      mIsInitial;
      bool                                                      mIsParallel;
      StateImpl*                                                mInitial;
      StateImpl*                                                mActive;
      ChildrenMap                                               mChildren;
      TransitionsMap 	                                          mTransitions;
      std::vector<priv::OnEntryAction>                          mOnEntryActions;
      std::vector<priv::OnExitAction>                           mOnExitActions;
    };
  }

  class StateMachine {
  
    friend class priv::StateImpl;

  public:

    template <typename... Params>
    StateMachine(Params && ... pParams);

  public:
    /*
    start the current state machine by calling State::enter ()
    for all initial children, depth first
    */
    inline void enter();

    /*
    terminate the current state machine
    by calling State::leave() for all active states
    from leaves to root, depth first
    */
    inline void leave();

    /*
    returns whether the state machine is active
    */
    inline virtual bool isActive() const;

    /*
    add an event to the event queue
    */
    inline void pushEvent(const std::string& pEvent);
    
    /*
    returns whether the current configuration has the given state active
    */
    inline bool inState(const std::string& stateName);
    
  private: // functioning primitives
    inline void processEvents();

    inline void processTransitions(const std::string& pEvent);
    
    /*
    browse through the tree of states to select transitions with a matching event
    and a realized condition
    */
    inline std::vector<priv::TransitionImpl*> selectTransitions(const std::string& pEvent);
    
    /*
    remove transitions having conflicting source/target state
    */
    inline std::vector<priv::TransitionImpl*> removeConflicts(std::vector<priv::TransitionImpl*>& pTransitions);
    
    /*
    compute a list of states that will be exited during execution of the transtion pTransition from the current configuration
    */
    inline std::list<priv::StateImpl*> listExitStates(priv::TransitionImpl* pTransition);
    
    /*
    compute a list of states that will be entered to activate pState from the current configuration
    */
    inline std::list<priv::StateImpl*> listEntryStates(priv::StateImpl* pState);
    
    /*
    returns true if pCheck is a descendant of pAgainst
    */
    inline bool isDescendant(priv::StateImpl* pCheck, priv::StateImpl* pAgainst);
    
    /*
    execute exit behavior for each state that must be exited while executing the
    given transitions
    */
    inline void exitStates(const std::vector<priv::TransitionImpl*>& pTransitions);
    
    /*
    execute enter behavior for each state that must be entered while executing the
    given transitions
    */
    inline void enterStates(const std::vector<priv::TransitionImpl*>& pTransitions);

    /*
    trigger State::leave() to each state that needs to be exited when to enter pTarget
    from the first active ancestor
    */
    inline void exitStates(priv::StateImpl* pTarget);

    /*
    trigger State::enter() to each state that needs to be entered when entering pTarget
    from the first active ancestor
    */
    inline void enterStates(priv::StateImpl* pTarget);

    /*
    get the common ancestor of the source and target states
    */
    inline priv::StateImpl* getTransitionDomain(priv::TransitionImpl* pTransition);

    /*
    find the lowest common ancestor of the two states
    */
    inline priv::StateImpl* findLeastCommonAncestor(priv::StateImpl* pLhs, priv::StateImpl* pRhs);
  private:
    std::unordered_map<std::string, std::unique_ptr<priv::StateImpl>> mAllStates;
    std::queue<std::string> mEvents;
    std::queue<priv::TransitionImpl*> mTransitions;
    bool mIsActive;
    bool mInToplevelProcess;
    priv::StateImpl* mImpl;

  };
}


namespace ifsm{
  namespace priv{

    /**
    Iterates over children of a state
    */
    template <class SearchType>
    struct StateIterator{
      friend class ifsm::priv::StateImpl;
      friend class ifsm::StateMachine;

    private: //only State is allowed to use iterators
      StateIterator();
      StateIterator(ifsm::priv::StateImpl* pFirst);
      StateIterator operator++();
      StateIterator operator++(int);
      priv::StateImpl* operator*() const;
      bool operator==(const StateIterator<SearchType>& pRhs) const;
      bool operator!=(const StateIterator<SearchType>& pRhs) const;
      void setTest(std::function < bool(::ifsm::priv::StateImpl*)>& pTest);
      void setTest(std::function < bool(::ifsm::priv::StateImpl*)>&& pTest);

      void lookupNextValidState();
      
    public:
      const priv::StateImpl::ChildrenMap& getStateChildren(const priv::StateImpl* pState);
    private:
      SearchType mSearch;
      std::function < bool(::ifsm::priv::StateImpl*)> mTest;
    };
    
    struct DepthFirstSearch{
      typedef DepthFirstSearch MyType;
      typedef StateIterator<MyType> MyIterator;

      inline DepthFirstSearch(MyIterator& pIterator, priv::StateImpl* pFirst);
      inline void fetchNext();
      inline priv::StateImpl* getCurrent() const;

      MyIterator& mIterator;
      std::list<priv::StateImpl*> mLifo;
    };
    
    struct WidthFirstSearch{
      typedef WidthFirstSearch MyType;
      typedef StateIterator<MyType> MyIterator;

      inline WidthFirstSearch(MyIterator& pIterator, priv::StateImpl* pFirst);
      inline void fetchNext();
      inline priv::StateImpl* getCurrent() const;

      MyIterator& mIterator;
      std::list<priv::StateImpl*> mFifo;
    };

  }
}

template <class FunType>
ifsm::priv::OnEntryAction ifsm::OnEntry(FunType && pFun){
  using ifsm::priv::is_callable;
  using ifsm::priv::is_callable_with;
  static_assert(is_callable<FunType>::value || is_callable_with<FunType, StateMachine&>::value, "parameter to onEntry must be callable either with no parameter or a StateMachine& parameter");
  return priv::OnEntryAction(priv::fixParams(std::forward<FunType>(pFun)));
}

template <class FunType>
ifsm::priv::OnExitAction ifsm::OnExit(FunType && pFun){
  using ifsm::priv::is_callable;
  using ifsm::priv::is_callable_with;
  static_assert(is_callable<FunType>::value || is_callable_with<FunType, StateMachine&>::value, "parameter to onExit must be callable either with no parameter or a StateMachine& parameter");
  return priv::OnExitAction(priv::fixParams(std::forward<FunType>(pFun)));
}

template <class FunType>
ifsm::priv::TransitionDef ifsm::OnEvent(const std::string& pEvent, FunType && pFun){
  return priv::TransitionDef(OnEvent(pEvent), Action(std::forward<FunType>(pFun)));
}

ifsm::priv::TransitionTarget ifsm::Target(const std::string& pTargetName){
  return priv::TransitionTarget(pTargetName);
}

template <class FunType>
ifsm::priv::TransitionAction ifsm::Action(FunType&& pAction){
  using ifsm::priv::is_callable;
  using ifsm::priv::is_callable_with;
  static_assert(is_callable<FunType>::value || is_callable_with<FunType, StateMachine&>::value,
    "parameter to action must be callable either with no paramater or a 'StateMachine&' parameter");
  return priv::TransitionAction(priv::fixParams(pAction));
}

template <class FunType>
ifsm::priv::TransitionCondition ifsm::Condition(FunType&& pCondition){
  using ifsm::priv::returns;
  using ifsm::priv::returns_with;
  static_assert(returns<FunType, bool>::value || returns_with<FunType, bool, const StateMachine&>::value,
    "parameter to action must be callable either with no paramater or a 'StateMachine&' parameter and must return 'bool'");
  return priv::TransitionCondition(priv::fixConditionParams(pCondition));
}

ifsm::priv::TransitionEvent ifsm::OnEvent(const std::string& pEvent){
  return priv::TransitionEvent(pEvent);
}

ifsm::priv::TransitionTarget::TransitionTarget(const std::string& pTargetName)
  : mTargetName(pTargetName){}
  
ifsm::priv::TransitionAction::TransitionAction(std::function<void(StateMachine&)>&& pFun)
  : mFun(pFun){}

void ifsm::priv::TransitionAction::operator()(StateMachine& pRoot){
  mFun(pRoot);
}

ifsm::priv::TransitionCondition::TransitionCondition(std::function<bool(const StateMachine&)>&& pCond)
  : mCond(pCond)
{}

bool ifsm::priv::TransitionCondition::operator()(const StateMachine& pRoot){
  return mCond(pRoot);
}

ifsm::priv::TransitionEvent::TransitionEvent(const std::string& pEvent)
  : mEvent(pEvent){}

ifsm::priv::TransitionDef::TransitionDef(TransitionDef&& pRhs)
: mTarget(std::move(pRhs.mTarget))
, mEvent(std::move(pRhs.mEvent))
, mAction(std::move(pRhs.mAction))
, mCondition(std::move(pRhs.mCondition)){

}
  
template <typename... Params>
ifsm::priv::TransitionDef::TransitionDef(Params && ... pParams){
  addParameters(std::forward<Params>(pParams)...);
}

void ifsm::priv::TransitionDef::addParameter(priv::TransitionTarget && pTarget){
  if (!mTarget.empty()){
    throw TargetAlreadySpecified(pTarget.mTargetName);
  }

  mTarget = pTarget.mTargetName;
}

void ifsm::priv::TransitionDef::addParameter(priv::TransitionAction && pAction){
  if (mAction){
    throw ActionAlreadySpecified();
  }

  mAction = pAction.mFun;
}

void ifsm::priv::TransitionDef::addParameter(priv::TransitionCondition && pCondition){
  if (mCondition){
    throw ConditionAlreadySpecified();
  }

  mCondition = pCondition.mCond;
}

void ifsm::priv::TransitionDef::addParameter(priv::TransitionEvent && pEvent){
  if (!mEvent.empty()){
    throw EventAlreadySpecified();
  }
  
  mEvent = pEvent.mEvent;
}

template <class T>
void ifsm::priv::TransitionDef::addParameters(T&& pParam1){
  addParameter(std::forward<T>(pParam1));
}

template <class T, typename... Params>
void ifsm::priv::TransitionDef::addParameters(T&& pParam1, Params && ... pParams){
  addParameter(std::forward<T>(pParam1));
  addParameters(std::forward<Params>(pParams)...);
}

template <typename... Params>
ifsm::priv::TransitionDef ifsm::Transition(Params && ... pParams){
  return priv::TransitionDef(std::forward<Params>(pParams)...);
}

ifsm::priv::TransitionImpl::TransitionImpl(const TransitionDef& pDef)
: mSource(nullptr)
, mTarget(nullptr)
, mEvent(std::move(pDef.mEvent))
, mAction(std::move(pDef.mAction))
, mCondition(std::move(pDef.mCondition)){
  
}

void ifsm::priv::TransitionImpl::setSource(StateImpl* pSource){
  mSource = pSource;
}

void ifsm::priv::TransitionImpl::setTarget(StateImpl* pTarget){
 mTarget = pTarget;
}

const ifsm::priv::StateImpl* ifsm::priv::TransitionImpl::getSource() const{
  return mSource;
}

const ifsm::priv::StateImpl* ifsm::priv::TransitionImpl::getTarget() const{
  return mTarget;
}

const std::string& ifsm::priv::TransitionImpl::getEvent() const {
  return mEvent;
}

bool ifsm::priv::TransitionImpl::test(const StateMachine& pRoot) const {
  if (!mCondition){
    return true;
  }
  return mCondition(pRoot);
}

void ifsm::priv::TransitionImpl::doAction(StateMachine& pRoot) const {
  if (mAction) {
    mAction(pRoot);
  }
}

template <typename... Args>
ifsm::priv::StateDef ifsm::State(const std::string& pName, Args&&... pArgs){
  return priv::StateDef(pName, std::forward<Args>(pArgs)...);
}

ifsm::priv::StateDef::StateDef(const std::string& pName)
  : mName(pName)
  , mIsInitial(false)
  , mIsParallel(false){

}

template <typename... Params>
ifsm::priv::StateDef::StateDef(const std::string& pName, Params && ... pParameters)
  : mName(pName)
  , mIsInitial(false)
  , mIsParallel(false){
  addParameters(std::forward<Params>(pParameters)...);
}


void ifsm::priv::StateDef::addParameter(StateDef && pState){
  mChildren.emplace_back(std::move(pState));
}

void ifsm::priv::StateDef::addParameter(TransitionDef && pTransition){
  mTransitions.emplace_back(std::move(pTransition));
}

void ifsm::priv::StateDef::addParameter(priv::OnEntryAction && pAction){
  mOnEntryActions.emplace_back(std::move(pAction));
}

void ifsm::priv::StateDef::addParameter(priv::OnExitAction && pAction){
  mOnExitActions.emplace_back(std::move(pAction));
}

void ifsm::priv::StateDef::addParameter(priv::initialTag_t& ){
  mIsInitial = true;
}

void ifsm::priv::StateDef::addParameter(priv::parallelTag_t& ){
  mIsParallel = true;
}

template <class FirstP>
void ifsm::priv::StateDef::addParameters(FirstP && pFirst){
  addParameter(std::forward<FirstP>(pFirst));
}

template <class FirstP, typename... Params>
void ifsm::priv::StateDef::addParameters(FirstP && pFirst, Params && ... pParameters){
  addParameter(std::forward<FirstP>(pFirst));

  addParameters(std::forward<Params>(pParameters)...);
}

ifsm::priv::StateImpl::StateImpl()
: mParent(nullptr)
, mRoot(nullptr)
, mIsInitial(false)
, mIsParallel(false)
, mInitial(nullptr)
, mActive(nullptr){

}

ifsm::priv::StateImpl::~StateImpl(){
  
}

void ifsm::priv::StateImpl::build(StateMachine* pRoot, StateImpl* pParent, StateDef&& pDef){
  mRoot = pRoot;
  mParent = pParent;
  mName = std::move(pDef.mName);
  mIsInitial = pDef.mIsInitial;
  mIsParallel = pDef.mIsParallel;
  mOnEntryActions = std::move(pDef.mOnEntryActions);
  mOnExitActions = std::move(pDef.mOnExitActions);

  //get children references
  for (auto& lChildDef : pDef.mChildren){
    auto lChild = mRoot->mAllStates[lChildDef.mName].get();
    mChildren.push_back(lChild);
    if (lChildDef.mIsInitial){
      if (mInitial != nullptr){
        throw AlreadyHasInitial(mName);
      }
      mInitial = lChild;
    }
  }

  //test whether this non-parallel non-atomic state has an initial child defined
  if (!mIsParallel && !mChildren.empty() && mInitial == nullptr){
    throw NoInitialState(mName);
  }
  
  //build transitions
  for (auto& lTransitionDef : pDef.mTransitions){
    
    std::unique_ptr<TransitionImpl> lTransition(new TransitionImpl(lTransitionDef));
    if (!lTransitionDef.mTarget.empty()){
      auto lFindTarget = mRoot->mAllStates.find(lTransitionDef.mTarget);
      if (lFindTarget==mRoot->mAllStates.end()){
        throw NoSuchState(lTransitionDef.mTarget);
      }
      lTransition->setTarget(lFindTarget->second.get());
    }
    
    lTransition->setSource(this);
    mTransitions.insert(std::make_pair(lTransition->getEvent(), std::move(lTransition)));
  }
}

const std::string& ifsm::priv::StateImpl::getName() const{
  return mName;
}

bool ifsm::priv::StateImpl::isAtomic() const{
  return mChildren.empty();
}

bool ifsm::priv::StateImpl::isInitial() const{
  return mIsInitial;
}

bool ifsm::priv::StateImpl::isParallel() const{
  return mIsParallel;
}

bool ifsm::priv::StateImpl::isActive() const{
  return mParent == nullptr ? mRoot->isActive() : mParent->mActive == this || (mParent->mIsParallel && mParent->isActive());
}

void ifsm::priv::StateImpl::enter(){
  if (!mIsParallel){
    if (nullptr != mInitial){
      mActive = mInitial;
    }
    else if (!mChildren.empty()){
      throw NoInitialState(mName);
    }
  }

  if (nullptr != mParent && !mParent->mIsParallel){
    mParent->mActive = this;
  }

  for (auto& lAction : mOnEntryActions){
    lAction(*mRoot);
  }
}

void ifsm::priv::StateImpl::leave(){
  if (nullptr != mParent && !mParent->mIsParallel){
    mParent->mActive = nullptr;
  }

  for (auto& lAction : mOnExitActions){
    lAction(*mRoot);
  }
}

template <typename... Params>
ifsm::StateMachine::StateMachine(Params && ... pParams)
: mIsActive(false)
, mInToplevelProcess(false){

  //build the StateDef for the StateMachine's StateImpl construction
  priv::StateDef lCurrentDefinition("root", std::forward<Params>(pParams)...);
  
  //build all children states recursively
  std::list<priv::StateDef*> lWorkingQueue;
  lWorkingQueue.push_back(&lCurrentDefinition);

  while (!lWorkingQueue.empty()){
    priv::StateDef* lDef = lWorkingQueue.front();
    lWorkingQueue.pop_front();

    auto lRes = mAllStates.insert(std::make_pair(lDef->mName, std::unique_ptr<priv::StateImpl>(new priv::StateImpl())));
    if (!lRes.second){
      throw DuplicateStateIdentifier(lDef->mName);
    }

    for (auto& lChild : lDef->mChildren){
      lWorkingQueue.push_back(&lChild);
    }

  }

  mImpl = mAllStates["root"].get();

  //then build them
  //list contains pair<parent, def of child>
  std::list<std::pair<priv::StateImpl*, priv::StateDef*>> lBuildQueue;
  lBuildQueue.push_back(std::make_pair(nullptr, &lCurrentDefinition));

  while (!lBuildQueue.empty()){
    priv::StateImpl* lParent = lBuildQueue.front().first;
    priv::StateDef* lDef = lBuildQueue.front().second;
    lBuildQueue.pop_front();

    priv::StateImpl* lCurrent = mAllStates[lDef->mName].get();
    lCurrent->build(this, lParent, std::move(*lDef));

    for (auto& lChild : lDef->mChildren){
      lBuildQueue.push_back(std::make_pair(lCurrent, &lChild));
    }
  }
#if 0
  std::vector<priv::StateDef*> lDirectChildren;
  gatherStateDefs(lDirectChildren, pParam1, pParams...);

  std::list<priv::StateDef*> lWorkingQueue;
  
  for (auto lChildDef : lDirectChildren){
    auto lRes = mAllStates.insert(std::make_pair(lChildDef->mName, std::unique_ptr<priv::StateImpl>(new priv::StateImpl())));
    if (!lRes.second){
      throw DuplicateStateIdentifier(lChildDef->mName);
    }

    mChildren.push_back(lRes.first->second.get());

    if (lChildDef->mIsInitial){
      if (mIsParallel){
        //parallel should not have initial states
      }
      else {
        if (mInitial != nullptr){
          throw AlreadyHasInitial(mName);
        }
        else {
          mInitial = lRes.first->second.get();
        }
      }
    }

    for (auto& lChild : lChildDef->mChildren){
      lWorkingQueue.push_back(&lChild);
    }
  }

  //instantiate all children states
  while (!lWorkingQueue.empty()){
    priv::StateDef* lDef = lWorkingQueue.front();
    lWorkingQueue.pop_front();
    
    auto lRes = mAllStates.insert(std::make_pair(lDef->mName, std::unique_ptr<priv::StateImpl>(new priv::StateImpl())));
    if (!lRes.second){
      throw DuplicateStateIdentifier(lDef->mName);
    }

    for (auto& lChild : lDef->mChildren){
      lWorkingQueue.push_back(&lChild);
    }
    
  }

  if (!mIsParallel && !mChildren.empty() && mInitial == nullptr) {
    throw NoInitialState(mName);
  }
  
  //then build them
  std::list<std::pair<priv::StateImpl*,priv::StateDef*>> lBuildQueue;

  for (auto lChildDef : lDirectChildren){
    lBuildQueue.push_back(std::make_pair(this, lChildDef));
  }

  while (!lBuildQueue.empty()){
    priv::StateImpl* lParent = lBuildQueue.front().first;
    priv::StateDef* lDef = lBuildQueue.front().second;
    lBuildQueue.pop_front();
    
    priv::StateImpl* lCurrent = mAllStates[lDef->mName].get();
    lCurrent->build(this, lParent, std::move(*lDef));
    
    for (auto& lChild : lDef->mChildren){
      lBuildQueue.push_back(std::make_pair(lCurrent, &lChild));
    }
  }
#endif
}


void ifsm::StateMachine::enter(){
  if (mIsActive) {
    return;
  }

  mIsActive = true;

  //enter children states in  depth first fashion
  std::list<priv::StateImpl*> lToEnter;
  lToEnter.push_back(mImpl);

  while (!lToEnter.empty()) {
    priv::StateImpl* lCurrent = lToEnter.front();
    lToEnter.pop_front();
    lCurrent->enter();

    if (!lCurrent->isParallel()){
      if (lCurrent->mInitial != nullptr){
        lToEnter.push_front(lCurrent->mInitial);
      }
    }
    else {
      std::for_each(lCurrent->mChildren.rbegin(), lCurrent->mChildren.rend(), [&lToEnter](priv::StateImpl* pChild){
        lToEnter.push_front(pChild);
      });
    }

  }
}

void ifsm::StateMachine::leave(){
  if (!mIsActive) {
    return;
  }

  std::list<priv::StateImpl*> lToLeave;

  std::list<priv::StateImpl*> lLifo;
  lLifo.push_back(mImpl);

  while (!lLifo.empty()) {
    priv::StateImpl* lCurrent = lLifo.front();
    lLifo.pop_front();
    lToLeave.push_front(lCurrent);

    if (!lCurrent->isActive()){
      continue;
    }

    if (lCurrent->isParallel()){
      std::for_each(lCurrent->mChildren.rbegin(), lCurrent->mChildren.rend(), [&lLifo](priv::StateImpl* pChild){
        lLifo.push_front(pChild);
      });
    }
    else if (lCurrent->mActive!=nullptr){
      lLifo.push_front(lCurrent->mActive);
    }
  }

  for (priv::StateImpl* lState : lToLeave){
    lState->leave();
  }

  mIsActive = false;
}

bool ifsm::StateMachine::isActive() const{
  return mIsActive;
}

void ifsm::StateMachine::pushEvent(const std::string& pEvent){
  //TO DO : enqueue event, determine dispatch policy
  mEvents.push(pEvent);
  processEvents();
}

bool ifsm::StateMachine::inState(const std::string& stateName){

  auto itFind = mAllStates.find(stateName);

  if (itFind == mAllStates.end()){
    return false;
  }

  if (itFind->second.get() == mImpl){
    return mIsActive;
  }

  if (itFind->second->isActive()){
    return true;
  }

  return false;
}

/**************************************************/
void ifsm::StateMachine::processEvents(){
  if (mInToplevelProcess){
	  return;
  }

  mInToplevelProcess = true;
  while (!mEvents.empty()){
	  const std::string& lEvent = mEvents.front();

    //process transitions linked to the event
    processTransitions(lEvent);
	  mEvents.pop();
  }
  mInToplevelProcess = false;
}

void ifsm::StateMachine::processTransitions(const std::string& pEvent){
  
  std::vector<priv::TransitionImpl*> lTransitions = selectTransitions(pEvent);

  std::vector<priv::TransitionImpl*> lFiltered = removeConflicts(lTransitions);

  exitStates(lFiltered);

  for (priv::TransitionImpl* lTransition : lFiltered){
    lTransition->doAction(*this);
  }

  enterStates(lFiltered);

}

std::vector<ifsm::priv::TransitionImpl*> ifsm::StateMachine::selectTransitions(const std::string& pEvent) {

  //list all atomic active states
  priv::StateIterator<priv::DepthFirstSearch> lAtomicsIt(mImpl);
  lAtomicsIt.setTest(std::function<bool(priv::StateImpl*)>([](priv::StateImpl* pState){
    return pState->isAtomic() && pState->isActive();
  }));

  std::vector<priv::StateImpl*> lAtomics;

  for (; priv::StateIterator<priv::DepthFirstSearch>(nullptr) != lAtomicsIt; ++lAtomicsIt){
    lAtomics.push_back(*lAtomicsIt);
  }

  //look for a valid transition from each active atomic state all the way up to the root
  std::vector<priv::TransitionImpl*> lTransitions;
  for (priv::StateImpl* lState : lAtomics){
    bool lMatched = false;
    while (lState != nullptr && !lMatched){
      auto lRangeMatch = lState->mTransitions.equal_range(pEvent);
      std::for_each(lRangeMatch.first, lRangeMatch.second, [=,&lMatched,&lTransitions](decltype(*lRangeMatch.first) &pTransition){
        if (pTransition.second->test(*this)){
          lTransitions.push_back(pTransition.second.get());
          lMatched = true;
        }
      });
      lState = lState->mParent;
    }
  }

  return lTransitions;
}

std::vector<ifsm::priv::TransitionImpl*> ifsm::StateMachine::removeConflicts(std::vector<priv::TransitionImpl*>& pTransitions) {
  std::vector<priv::TransitionImpl*> lFiltered;
  std::vector<priv::StateImpl*> lIntersect;

  std::vector<priv::TransitionImpl*> lToRemove;
  bool lCheckPreempted = false;

  //for each transition to check
  for (auto lTransitionToCheck : pTransitions){
    lCheckPreempted = false;
    lToRemove.clear();

    priv::StateImpl* lToCheckTarget = lTransitionToCheck->mTarget;

    if (lFiltered.empty() || lToCheckTarget==nullptr){
      lFiltered.push_back(lTransitionToCheck);
      continue;
    }

    std::list<priv::StateImpl*> lToCheckExitsList = listExitStates(lTransitionToCheck);
    std::vector<priv::StateImpl*> lToCheckExits(std::begin(lToCheckExitsList), std::end(lToCheckExitsList));
    std::sort(std::begin(lToCheckExits), std::end(lToCheckExits));

    //check against already filtered transitions
    for (auto lCheckAgainst : lFiltered){

      //compute exit states of both transitions
      priv::StateImpl* lCheckAgainstTarget = lCheckAgainst->mTarget;

      if (lCheckAgainstTarget == nullptr){
        continue;
      }

      std::list<priv::StateImpl*> lCheckAgainstExitsList = listExitStates(lCheckAgainst);
      std::vector<priv::StateImpl*> lCheckAgainstExits(std::begin(lCheckAgainstExitsList), std::end(lCheckAgainstExitsList));
      std::sort(std::begin(lCheckAgainstExits), std::end(lCheckAgainstExits));
      lIntersect.insert(lIntersect.end(), lToCheckExits.size() + lCheckAgainstExits.size(), nullptr);

      //compute intersection of exit states
      std::set_intersection(std::begin(lToCheckExits), std::end(lToCheckExits),
        std::begin(lCheckAgainstExits), std::end(lCheckAgainstExits),
        std::begin(lIntersect));

      if (!lIntersect.empty()){
        if (isDescendant(lToCheckTarget, lCheckAgainstTarget)){
          lToRemove.push_back(lCheckAgainst);
        }
        else {
          lCheckPreempted = true;
          break;
        }
      }
    }

    if (!lCheckPreempted){
      for (auto lToRemoveTsn : lToRemove){
        auto lDel = std::remove(std::begin(lFiltered), std::end(lFiltered), lToRemoveTsn);
        lFiltered.erase(lDel, std::end(lFiltered));
      }
      lFiltered.push_back(lTransitionToCheck);
    }
  }

  return lFiltered;
}

std::list<ifsm::priv::StateImpl*> ifsm::StateMachine::listExitStates(priv::TransitionImpl* pTransition){
  std::list<priv::StateImpl*> lToExitList;

  priv::StateImpl* lCommonAncestor = findLeastCommonAncestor(pTransition->mSource, pTransition->mTarget);

  std::list<priv::StateImpl*> lFifo;

  if (lCommonAncestor->isActive()){
    lFifo.push_back(lCommonAncestor);
  }

  while (!lFifo.empty()){
    priv::StateImpl* lCurrent = lFifo.front();
    lFifo.pop_front();

    for (auto lChild : lCurrent->mChildren){
      if (lChild->isActive()){
        lFifo.push_back(lChild);
        lToExitList.push_front(lChild);
      }
    }
  }
  
  return lToExitList;
}

std::list<ifsm::priv::StateImpl*> ifsm::StateMachine::listEntryStates(priv::StateImpl* pState){
  std::list<priv::StateImpl*> lToEnterList;

  if (pState == nullptr){
    return lToEnterList;
  }

  lToEnterList.push_back(pState);

  //children of target should be entered after target
  std::list<priv::StateImpl*> lFifo;
  lFifo.push_back(pState);
  while (!lFifo.empty()){
    priv::StateImpl* lCurrent = lFifo.front();
    lFifo.pop_front();

    if (lCurrent->mIsParallel){
      for (auto lChild : lCurrent->mChildren){
        lFifo.push_back(lChild);
        lToEnterList.push_back(lChild);
      }
    }
    else if (nullptr != lCurrent->mInitial){
      lFifo.push_back(lCurrent->mInitial);
      lToEnterList.push_back(lCurrent->mInitial);
    }

  }

  //ancestors of target should be entered before target
  //until the first active state
  priv::StateImpl* lActive = pState->mParent;
  while (nullptr != lActive && !lActive->isActive()){
    lToEnterList.push_front(lActive);
    lActive = lActive->mParent;
  }

  //check whether an ancestor is a parallel, in which case its children must be added in correct order
  auto lEnterListIt = lToEnterList.begin();
  while (lEnterListIt != lToEnterList.end()){
    if ((*lEnterListIt)->isParallel()){
      auto lAlreadyInserted = lEnterListIt;
      ++lAlreadyInserted;
      for (auto lChild : (*lEnterListIt)->mChildren){
        if (lChild == *lAlreadyInserted){
          ++lEnterListIt;
          continue;
        }
        lEnterListIt = lToEnterList.insert(++lEnterListIt, lChild);
      }
    }
    ++lEnterListIt;
  }

  return lToEnterList;
}

bool ifsm::StateMachine::isDescendant(priv::StateImpl* pCheck, priv::StateImpl* pAgainst){
  priv::StateImpl* lStart = pCheck;
  while (lStart != pAgainst && lStart != nullptr){
    lStart = lStart->mParent;
  }

  if (lStart != nullptr){
    return true;
  }

  return false;
}

void ifsm::StateMachine::exitStates(const std::vector<priv::TransitionImpl*>& pTransitions){
  std::vector<priv::StateImpl*> lToExit;

  for (auto lTransition : pTransitions) {
    if (lTransition->mTarget == nullptr){
      continue;
    }
    std::list<priv::StateImpl*> lCurrentExit = listExitStates(lTransition);
    lToExit.insert(std::end(lToExit), std::begin(lCurrentExit), std::end(lCurrentExit));
  }

  for (auto lState : lToExit){
    lState->leave();
  }
}

void ifsm::StateMachine::enterStates(const std::vector<priv::TransitionImpl*>& pTransitions){
  std::vector<priv::StateImpl*> lToEnter;

  for (auto lTransition : pTransitions) {
    if (lTransition->mTarget == nullptr){
      continue;
    }
    std::list<priv::StateImpl*> lCurrentEnter = listEntryStates(lTransition->mTarget);
    lToEnter.insert(std::end(lToEnter), std::begin(lCurrentEnter), std::end(lCurrentEnter));
  }

  for (auto lState : lToEnter){
    lState->enter();
  }
}

void ifsm::StateMachine::exitStates(priv::StateImpl* pTarget){
  std::list<priv::StateImpl*> lToExit;

  std::list<priv::StateImpl*> lFifo;


  if (pTarget->isActive()){
	  lToExit.push_front(pTarget);
	  lFifo.push_front(pTarget);
  }
  else {

	//browse up to the first active ancestor
    priv::StateImpl* lActive = pTarget->mParent;
	while (nullptr != lActive && !lActive->isActive()){
	  lActive = lActive->mParent;
	}

	if (nullptr == lActive){
	  //TODO, we shouldn't ever get there
	  return;
	}

	lFifo.push_front(lActive);
  }

  //now we've got the first active ancestor, 
  //let's follow active states to know what states must be exited
  while (!lFifo.empty()){
    priv::StateImpl* lActive = lFifo.front();
	  lFifo.pop_front();

	  if (lActive->mIsParallel){
	    for (auto lChild : lActive->mChildren){
		  lFifo.push_back(lChild);
		  lToExit.push_front(lChild);
	    }
	  }
	  else if (nullptr != lActive->mActive){
	    lFifo.push_back(lActive->mActive);
	    lToExit.push_front(lActive->mActive);
	  }
  }

  //we've got our exit list, let's exit then
  for (auto lExit : lToExit){
	  lExit->leave();
  }

}

void ifsm::StateMachine::enterStates(priv::StateImpl* pTarget){
  std::list<priv::StateImpl*> lToEnter;
  lToEnter.push_back(pTarget);

  //children of target should be entered after target
  std::list<priv::StateImpl*> lFifo;
  lFifo.push_back(pTarget);
  while (!lFifo.empty()){
    priv::StateImpl* lCurrent = lFifo.front();
	  lFifo.pop_front();

	  if (lCurrent->mIsParallel){
	    for (auto lChild : lCurrent->mChildren){
		  lFifo.push_back(lChild);
		  lToEnter.push_back(lChild);
	    }
	  }
	  else if (nullptr != lCurrent->mInitial){
	    lFifo.push_back(lCurrent->mInitial);
	    lToEnter.push_back(lCurrent->mInitial);
	  }

  }

  //ancestors of target should be entered before target
  //until the first active state
  priv::StateImpl* lActive = pTarget->mParent;
  while (nullptr != lActive && !lActive->isActive()){
	  lToEnter.push_front(lActive);
	  lActive = lActive->mParent;
  }

  for (auto lEnter : lToEnter){
	  lEnter->enter();
  }

}

ifsm::priv::StateImpl* ifsm::StateMachine::getTransitionDomain(priv::TransitionImpl* pTransition){
  if (pTransition->mTarget == nullptr){
    return pTransition->mSource;
  }
  else {
    return findLeastCommonAncestor(pTransition->mSource, pTransition->mTarget);
  }
}

ifsm::priv::StateImpl* ifsm::StateMachine::findLeastCommonAncestor(priv::StateImpl* pLhs, priv::StateImpl* pRhs) {
  std::vector<priv::StateImpl*> lLhsParents;
  priv::StateImpl* lCurrentParent = pLhs->mParent;
  lLhsParents.push_back(lCurrentParent);

  while (lCurrentParent != nullptr){
    lCurrentParent = lCurrentParent->mParent;
    lLhsParents.push_back(lCurrentParent);
  }

  for (auto lState : lLhsParents){
    if (isDescendant(pRhs, lState)){
      return lState;
    }
  }

  return mImpl;
}

template <class SearchType>
ifsm::priv::StateIterator<SearchType>::StateIterator(){

}

template <class SearchType>
ifsm::priv::StateIterator<SearchType>::StateIterator(priv::StateImpl* pFirst)
: mSearch(*this, pFirst)
, mTest([](priv::StateImpl*){return true; }){
  lookupNextValidState();
}

template <class SearchType>
ifsm::priv::StateIterator<SearchType> ifsm::priv::StateIterator<SearchType>::operator++(){
  
  mSearch.fetchNext();
  lookupNextValidState();

  return *this;
}

template <class SearchType>
ifsm::priv::StateImpl* ifsm::priv::StateIterator<SearchType>::operator*() const{
  return mSearch.getCurrent();
}

template <class SearchType>
bool ifsm::priv::StateIterator<SearchType>::operator==(const StateIterator<SearchType>& pRhs) const{
  return mSearch.getCurrent() == pRhs.mSearch.getCurrent();
}

template <class SearchType>
bool ifsm::priv::StateIterator<SearchType>::operator!=(const StateIterator<SearchType>& pRhs) const{
  return !(*this == pRhs);
}

template <class SearchType>
void ifsm::priv::StateIterator<SearchType>::setTest(std::function < bool(priv::StateImpl*)>& pTest){
  mTest = pTest;
  lookupNextValidState();
}

template <class SearchType>
void ifsm::priv::StateIterator<SearchType>::setTest(std::function < bool(::ifsm::priv::StateImpl*)>&& pTest){
  setTest(pTest);
}

template <class SearchType>
void ifsm::priv::StateIterator<SearchType>::lookupNextValidState(){
  while (mSearch.getCurrent() != nullptr && !mTest(mSearch.getCurrent())){
    mSearch.fetchNext();
  }
}


template <class SearchType>
const ifsm::priv::StateImpl::ChildrenMap& ifsm::priv::StateIterator<SearchType>::getStateChildren(const priv::StateImpl* pState){
  return pState->mChildren;
}

ifsm::priv::DepthFirstSearch::DepthFirstSearch(MyIterator& pIterator, priv::StateImpl* pFirst)
: mIterator(pIterator){
  if (nullptr == pFirst){
    return;
  }

  mLifo.push_front(pFirst);
}

void ifsm::priv::DepthFirstSearch::fetchNext(){
  if (mLifo.size() <= 0){
    return;
  }
  ifsm::priv::StateImpl* lCurrent = mLifo.front();
  mLifo.pop_front();

  priv::StateImpl::ChildrenMap::const_reverse_iterator lIt = mIterator.getStateChildren(lCurrent).crbegin(),
    lItEnd = mIterator.getStateChildren(lCurrent).crend();
  for (; lIt != lItEnd; ++lIt){
    mLifo.push_front(*lIt);
  }
}

ifsm::priv::StateImpl* ifsm::priv::DepthFirstSearch::getCurrent() const{
  return (mLifo.size()>0)? mLifo.front():nullptr;
}

ifsm::priv::WidthFirstSearch::WidthFirstSearch(MyIterator& pIterator, priv::StateImpl* pFirst)
: mIterator(pIterator){
  if (nullptr == pFirst){
    return;
  }

  mFifo.push_front(pFirst);
}

void ifsm::priv::WidthFirstSearch::fetchNext(){
  if (mFifo.size() <= 0){
    return;
  }
  ifsm::priv::StateImpl* lCurrent = mFifo.front();
  mFifo.pop_front();
  /*
  
  State::ChildrenMap::const_reverse_iterator lIt = mIterator.getStateChildren(lCurrent).crbegin();
  State::ChildrenMap::const_reverse_iterator lItEnd = mIterator.getStateChildren(lCurrent).crend();
  for (; lIt != lItEnd; ++lIt){
    mFifo.push_back(lIt->second);
  }
  */
  for (auto& lChild : mIterator.getStateChildren(lCurrent)){
    mFifo.push_back(lChild);
  }
}


#endif //INSTANTFSM_H
