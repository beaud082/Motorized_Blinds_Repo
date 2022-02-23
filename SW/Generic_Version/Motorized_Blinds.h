#include "esphome.h"
#include "BasicStepperDriver.h"

#define TILT_STEP_MAX 100 //Max-Min = total travelable stepping area (measured in full steps)
#define TILT_STEP_HOME 50 //A value between the Max and Min that will act as the hardware programmed home location.

#define STEPPER_SLEEP_PIN
#define STEPPER_STEP_PIN
#define STEPPER_DIR_PIN
#define STEPS_PER_REVOLUTION 200
#define MICROSTEPS_PER_STEP 1
#define RPM 20

class Motorized_Blinds : public Component, public CustomAPIDevice {

 public:
  void setup() override {
    // This will be called by App.setup()
    
	stepper_motor.begin(RPM, STEPS_PER_REVOLUTION);
	stepper_motor.setEnableActiveState(LOW);
	stepper_motor.enable();
	
	// Declare a service "calibrate_blinds"
    //  - Service will be called "esphome.<NODE_NAME>_calibrate_blinds" in Home Assistant.
    //  - The service has no arguments
    //  - The function calibrate declared below will attached to the service.
    register_service(&Motorized_Blinds::calibrate, "calibrate_blinds");
	
  }
  
  void calibrate(){
	stepper_motor.startMove(-1*TILT_STEP_MAX);
  }
  
  void set_blinds_tilt(float requested_float_tilt) {
    // This will be called every time the user requests a state change.
	  if(requested_float_tilt < 0 || requested_float_tilt > 1){
		return;
	  }
      
	  int requested_step_tilt = (int)(requested_float_tilt * TILT_STEP_MAX * MICROSTEPS_PER_STEP);
	  
	  if(current_step_tilt != requested_step_tilt){ //check whether the motor needs to move by comparing its current position to its requested position
		  if(stepper_motor.state() == STOPPED) {
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
	// User requested cover stop
	  current_step_tilt -= stepper_motor.stop();
	  stepper_motor.disable();
  }
  
  void loop() override {
	  unsigned time_stamp_micros = micros();
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

  }
 private:
	int current_step_tilt = 0; //between 0 and TILT_STEP_MAX
	unsigned next_action_time_micros = 0;
	
	BasicStepperDriver stepper_motor(STEPS_PER_REVOLUTION, STEPPER_DIR_PIN, STEPPER_STEP_PIN, STEPPER_SLEEP_PIN);
	
	
};