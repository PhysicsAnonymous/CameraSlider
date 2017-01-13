#ifndef MIKE_H
#define MIKE_H

#include "config.hpp"

//We need size_t defined locally for some reason
typedef unsigned int size_t;

#define ENDWARD SLIDER_MAX_POSITION / SLIDER_MAX_POSITION
#define HOMEWARD ENDWARD * -1

/****************************************************************************/
enum STATES {
  NO_STATE=0,
  FIRST_HOME, 
  ADJUST,
  FIRST_END_MOVE,
  SECOND_HOME,
  WAIT,
  EXECUTE,
  ERROR,
  REVERSE_EXECUTE
};

enum SWITCH_STATE{
  VIDEO_MODE,
  LAPSE_MODE,
  PROGRAM_MODE,
  INVALID
};

enum ERROR_T{
  NONE=0,
  UNKNOWN,
  CANCEL,
  SOFTWARE
};
/****************************************************************************/

class SliderFSM;
/****************************************************************************/

/****************************************************************************/
// This class is the abstract base class inherited by our states
class AbstractState {
  public:
    virtual ~AbstractState(){};

    //State classes override this function to be called at regular intervals
    //in the main loop.  (default to nothing)
    virtual void run_loop()=0;

    //Action to take in this state when the red "go" or "set" button is pressed
    //(default to no ignore)
    virtual void go_button()=0;

    //Action to take when the "home" stop is activated (default to ignore)
    virtual void home_stop()=0;

    //Action to take when the "end" stop is activated (default to ignore)
    virtual void end_stop()=0;

    //Limit transitions to ones we approve of in advance
    virtual bool transition_allowed(STATES new_state)=0;

    //actions to take when exiting this state (cleanup, etc.)
    virtual void exit_state()=0;

    //actions to take when entering this state
    virtual void enter_state()=0;

    //Just returns the enum associated with the current state
    virtual STATES get_state_as_enum()=0;

    void* operator new(size_t sz);

    void operator delete(void* p);

  protected:
    void setSoftwareError();
    void setUnknownError();
    void setCancel();
    //transitions to state if provided direction matches NEXT_DIRECTION,
    //transitions to error otherwise.
    void transitionOrError(const int direction, const STATES state);

    AbstractState(SliderFSM* machine) : m_machine(machine){};
    SliderFSM* m_machine; 

};

/****************************************************************************/
// This class is the concrete instantiation of our states - one template
// specialization per state.
template <STATES state>
class ConcreteState : public AbstractState {
  public:
    virtual ~ConcreteState(){};

    virtual void run_loop(){};
    virtual void go_button(){};
    virtual void home_stop(){};
    virtual void end_stop(){};
    virtual bool transition_allowed(STATES new_state); //We can't make this
                                   //virtual, because the compiler can't be
                                   //sure we won't instantiate a template
                                   //that does not have it defined.  However,
                                   //leaving it blank will mean the linker
                                   //complains if template specializations
                                   //do not define it.
    virtual void exit_state(){};
    virtual void enter_state(){};
    virtual STATES get_state_as_enum(){return state;};

  protected:
    //Protected constructor to prevent accidental instantiation of abstract
    ConcreteState(SliderFSM* machine) : AbstractState(machine) {};
    friend SliderFSM;
};
/****************************************************************************/

/*** Class declarations for the states **************************************/

typedef ConcreteState<STATES::FIRST_HOME> StateFirstHome;
typedef ConcreteState<STATES::ADJUST> StateAdjust;
typedef ConcreteState<STATES::FIRST_END_MOVE> StateFirstEndMove;
typedef ConcreteState<STATES::SECOND_HOME> StateSecondHome;
typedef ConcreteState<STATES::WAIT> StateWait;
typedef ConcreteState<STATES::EXECUTE> StateExecute;
typedef ConcreteState<STATES::ERROR> StateError;
typedef ConcreteState<STATES::REVERSE_EXECUTE> StateReverseExecute;

/****************************************************************************/

/*** SliderFSM **************************************************************/
class SliderFSM {
  public:
    SliderFSM();
    void run_loop();
    void go_button();
    void home_stop();
    void end_stop();
    void change_state(STATES new_state);
  private:
    AbstractState* m_state;
    STATES m_target_state;
    void update_state();
};
/****************************************************************************/


#endif
