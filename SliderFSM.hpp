/*****************************************************************************
== Physics Anonymous CC-BY 2017 ==
******************************************************************************/

#ifndef SLIDERFSM_H
#define SLIDERFSM_H

//We will need a pointer to the AbstractStates, but we don't need to define
//them here.
class AbstractState;

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
