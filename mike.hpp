#ifndef MIKE_H
#define MIKE_H

//TODO: #include <AccelStepper.h>
#include "config.hpp"

/****************************************************************************/
enum STATES {
  NO_STATE=0,
  IDLE,
  FIRST_HOME, 
  FIRST_ADJUST,
  FIRST_END_MOVE,
  SECOND_ADJUST,
  SECOND_HOME,
  WAIT,
  EXECUTE,
  ERROR,
  MAX_STATES
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

/*** Class declarations for the states **************************************/
class StateIdle;
class StateFirstHome;
class StateFirstAdjust;
class StateFirstEndMove;
class StateSecondAdjust;
class StateSecondHome;
class StateWait;
class StateExecute;

class SliderFSM;
/****************************************************************************/

//avr-gcc uses unsigned int
//typedef unsigned int size_t;
//while gcc uses long unsigned int.
typedef long unsigned int size_t;

/****************************************************************************/
// This class is the abstract base class inherited by our states
class AbstractState {
  public:
    virtual ~AbstractState();

    //State classes override this function to be called at regular intervals
    //in the main loop.  (default to nothing)
    virtual void run_loop();

    //Action to take in this state when the red "go" or "set" button is pressed
    //(default to no ignore)
    virtual void go_button();

    //Action to take when the "home" stop is activated (default to ignore)
    virtual void home_stop();

    //Action to take when the "end" stop is activated (default to ignore)
    virtual void end_stop();

    //By default, allow all transitions
    virtual bool transition_allowed(STATES new_state);

    //actions to take when exiting this state (cleanup, etc.)
    virtual void exit_state();

    //actions to take when entering this state
    virtual void enter_state();

    //Just returns the enum associated with the current state
    virtual STATES get_state_as_enum();

    void* operator new(size_t sz);

    void operator delete(void* p);

  protected:
    SliderFSM* m_machine; 
    //Protected constructor to prevent accidental instantiation of abstract
    AbstractState(SliderFSM* machine) : m_machine(machine) {};
};
/****************************************************************************/


/****************************************************************************/
// Create a Finite State Machine
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

/*** Idle state *************************************************************/
class StateIdle : public AbstractState {
  public:
    virtual void run_loop();
    virtual void go_button();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual STATES get_state_as_enum();
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateIdle(SliderFSM* machine);
};
/****************************************************************************/

/*** First home state *******************************************************/
class StateFirstHome : public AbstractState {
  public:
    virtual void run_loop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual void home_stop();
    virtual void end_stop();
    virtual STATES get_state_as_enum();
    virtual void go_button();
    //Default (no action) on: exit_state
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateFirstHome(SliderFSM* machine);
};
/****************************************************************************/

/*** First adjust state *******************************************************/
class StateFirstAdjust : public AbstractState {
  public:
    virtual void run_loop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual STATES get_state_as_enum();
    virtual void go_button();
    //Default (no action) on: exit_state
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateFirstAdjust(SliderFSM* machine);
    int m_update_counter;
};
/****************************************************************************/

/*** First end move state ***************************************************/
class StateFirstEndMove : public AbstractState {
  public:
    virtual void run_loop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual void home_stop();
    virtual void end_stop();
    virtual STATES get_state_as_enum();
    virtual void go_button();
    //Default (no action) on: exit_state
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateFirstEndMove(SliderFSM* machine);
};
/****************************************************************************/

/*** Second adjust state *******************************************************/
class StateSecondAdjust : public AbstractState {
  public:
    virtual void run_loop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual STATES get_state_as_enum();
    virtual void go_button();
    //Default (no action) on: exit_state
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateSecondAdjust(SliderFSM* machine);
    int m_update_counter;
};
/****************************************************************************/

/*** Second home state ******************************************************/
class StateSecondHome : public AbstractState {
  public:
    virtual void run_loop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual void home_stop();
    virtual void end_stop();
    virtual STATES get_state_as_enum();
    virtual void go_button();
    //Default (no action) on: exit_state
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateSecondHome(SliderFSM* machine);
};
/****************************************************************************/

/*** Wait state *************************************************************/
class StateWait : public AbstractState {
  public:
    virtual void run_loop();
    virtual void go_button();
    virtual bool transition_allowed(STATES new_state);
    virtual STATES get_state_as_enum();
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateWait(SliderFSM* machine);
};
/****************************************************************************/

/*** Execute state **********************************************************/
class StateExecute : public AbstractState {
  public:
    virtual void run_loop();
    virtual void go_button();
    virtual void end_stop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual STATES get_state_as_enum();
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateExecute(SliderFSM* machine);
};
/****************************************************************************/

/*** Error state ************************************************************/
class StateError : public AbstractState {
  public:
    virtual void run_loop();
    virtual bool transition_allowed(STATES new_state);
    virtual void enter_state();
    virtual STATES get_state_as_enum();
  protected:
    friend SliderFSM; //Only let the state machine construct us
    StateError(SliderFSM* machine);
};
/****************************************************************************/

#endif
