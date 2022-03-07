#include "esphome.h"
#include "Wire.h"
#include "BasicStepperDriver.h"


#define TILT_STEP_MAX 100 //Max-Min = total travelable stepping area (measured in full steps)
#define TILT_STEP_HOME 50 //A value between the Max and Min that will act as the hardware programmed home location.

#define STEPPER_SLEEP_PIN 9
#define STEPPER_STEP_PIN 5
#define STEPPER_DIR_PIN 4

#define CAL_BUTTON_PIN 1
#define UP_BUTTON_PIN 3
#define DOWN_BUTTON_PIN 15
#define HOME_BUTTON_PIN 13

#define STEPS_PER_REVOLUTION 200
#define MICROSTEPS_PER_STEP 1
#define RPM 20

#define DEBOUNCE_MICROS 20000

class Motorized_Blinds : public Component, public CustomAPIDevice {

 public:
  void setup() override {
    // This will be called by App.setup()
	ESP_LOGD("custom", "My setup() called");
	stepper_motor.begin(RPM, STEPS_PER_REVOLUTION);
	ESP_LOGD("custom", "stepper_motor.begin(*,*) completed");
	/*
	stepper_motor.setEnableActiveState(LOW);
	stepper_motor.disable();
	*/
	// Declare a service "calibrate_blinds"
    //  - Service will be called "esphome.<NODE_NAME>_calibrate_blinds" in Home Assistant.
    //  - The service has no arguments
    //  - The function calibrate declared below will attached to the service.
    register_service(&Motorized_Blinds::calibrate, "calibrate_blinds");
	
	// Declare a service "set_blinds_tilt"
    //  - Service will be called "esphome.<NODE_NAME>_set_blinds_tilt" in Home Assistant.
    //  - The service has one argument (type inferred from method definition):
    //  - tilt_value: float
    //  - The function set_blinds_tilt declared below will attached to the service.
    register_service(&Motorized_Blinds::set_blinds_tilt, "set_blinds_tilt", {"tilt_value"});

	// Declare a service "stop_blinds"
    //  - Service will be called "esphome.<NODE_NAME>_stop_blinds" in Home Assistant.
    //  - The service has no arguments
    //  - The function stop_blinds declared below will attached to the service.
    register_service(&Motorized_Blinds::stop_blinds, "stop_blinds");	
	ESP_LOGD("custom", "My setup() completed");
  }
  
  void calibrate(){
	ESP_LOGD("custom", "calibrate called");
	stepper_motor.startMove((int)(-1.05 * ((double) TILT_STEP_MAX)));
	current_step_tilt = 0;
  }
  
  void set_blinds_tilt(float requested_float_tilt) {
	  ESP_LOGD("custom", "set_blinds_tilt called");
    // This will be called every time the user requests a state change.
	  if(requested_float_tilt < 0 || requested_float_tilt > 1){
		return;
	  }
      
	  int requested_step_tilt = (int)(requested_float_tilt * TILT_STEP_MAX * MICROSTEPS_PER_STEP);
	  
	  if(current_step_tilt != requested_step_tilt){ //check whether the motor needs to move by comparing its current position to its requested position
		  if(stepper_motor.getCurrentState() == BasicStepperDriver::STOPPED) {
			stepper_motor.enable();
			stepper_motor.startMove(requested_step_tilt - current_step_tilt);
			current_step_tilt = requested_step_tilt;
		  }
		  else {																//this case is for if the motor is already in the middle of moving due to a previous request
			current_step_tilt -= stepper_motor.stop(); 							//stop the motor where it is and figure out how far it went and correct the current step counter accordingly
			stepper_motor.startMove(requested_step_tilt - current_step_tilt);	// start a new move as requested
			current_step_tilt = requested_step_tilt;
		  }
	  }
  }
  void stop_blinds() {
	  ESP_LOGD("custom", "stop_blinds called");
	  current_step_tilt -= stepper_motor.stop();
	  stepper_motor.disable();
  }
  
  void home_blinds(){
	ESP_LOGD("custom", "home_blinds called");
	stepper_motor.startMove(TILT_STEP_HOME - current_step_tilt);
	current_step_tilt = TILT_STEP_HOME;
  }
  
  
  void loop() override {
	
	unsigned time_stamp_micros = micros();
	/*
	if(time_stamp_micros - next_action_time_micros >= 0){
		unsigned interval_time_micros = stepper_motor.nextAction();
		if(interval_time_micros == 0){
			next_action_time_micros = 0;
			stepper_motor.disable();
		}
		else{
			next_action_time_micros = time_stamp_micros + interval_time_micros;
		}
	}
	*/
	if(time_stamp_micros - next_button_time_micros >= 0){
		next_button_time_micros = 0;
		
		bool current_button1_state = digitalRead(CAL_BUTTON_PIN);
		bool current_button2_state = digitalRead(UP_BUTTON_PIN);
		bool current_button3_state = digitalRead(DOWN_BUTTON_PIN);
		bool current_button4_state = digitalRead(HOME_BUTTON_PIN);
		
		if(current_button1_state && current_button1_state != previous_button1_state)
		{

			//calibrate();
			next_button_time_micros = time_stamp_micros + DEBOUNCE_MICROS;
		}
		
		if(current_button2_state && current_button2_state != previous_button2_state)
		{
			stepper_motor.startMove(TILT_STEP_MAX-current_step_tilt);
			current_step_tilt = TILT_STEP_MAX;
			next_button_time_micros = time_stamp_micros + DEBOUNCE_MICROS;
		}
		else if(!current_button2_state && current_button2_state != previous_button2_state)
		{
			stop_blinds();
			next_button_time_micros = time_stamp_micros + DEBOUNCE_MICROS;
		}
		
		if((current_button3_state && current_button3_state != previous_button3_state))
		{
			stepper_motor.startMove(-1*current_step_tilt);
			current_step_tilt = 0;
			next_button_time_micros = time_stamp_micros + DEBOUNCE_MICROS;
		}
		else if(!current_button3_state && current_button3_state != previous_button3_state)
		{
			stop_blinds();
			next_button_time_micros = time_stamp_micros + DEBOUNCE_MICROS;
		}
		
		if(current_button4_state && current_button4_state != previous_button4_state)
		{
			home_blinds();
			next_button_time_micros = time_stamp_micros + DEBOUNCE_MICROS;
		}
		
		previous_button1_state=current_button1_state;
		previous_button2_state=current_button2_state;
		previous_button3_state=current_button3_state;
		previous_button4_state=current_button4_state;
	}
	
  }
 private:
	int current_step_tilt = 0; //between 0 and TILT_STEP_MAX
	unsigned next_action_time_micros = 0;
	unsigned next_button_time_micros = 0;
	bool previous_button1_state = false;
	bool previous_button2_state = false;
	bool previous_button3_state = false;
	bool previous_button4_state = false;
	BasicStepperDriver stepper_motor = BasicStepperDriver(STEPS_PER_REVOLUTION, STEPPER_DIR_PIN, STEPPER_STEP_PIN, STEPPER_SLEEP_PIN);
	
	
};