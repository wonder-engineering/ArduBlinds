
/*
todo: add header description ala doxygen format

 */

#include <Stepper.h>


// Intitialize Stepper
#define STEPPER_STEPS_PER_REVOLUTION 2200
#define STEPPER_PIN_1 7
#define STEPPER_PIN_2 5
#define STEPPER_PIN_3 6
#define STEPPER_PIN_4 4
Stepper stepper(STEPPER_STEPS_PER_REVOLUTION, STEPPER_PIN_1,
                                                STEPPER_PIN_2,
                                                STEPPER_PIN_3,
                                                STEPPER_PIN_4);

// Intialize input pins
#define SENSOR_PIN       A7  // Analog in plus pullup
#define UP_BUTTON_PIN    8   // normally open
#define DOWN_BUTTON_PIN  9   // normally open

// Builtin LED indicator pin
#define LED_PIN 13


void setup() {
  stepper.setSpeed(12);
  
  // initialize the serial port:
  Serial.begin(57600);

  // setup the LED output pin
  pinMode(LED_PIN, OUTPUT);

  // setup button pins with internal pullup
  pinMode(UP_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN,INPUT_PULLUP);

  // setup the analog input pin (external pullup)
  pinMode(SENSOR_PIN,INPUT);
}

#define MOTOR_POSITION_OPEN  0                 // at powerup, assumes we're here
#define ONE_TENTH_MOTOR_POSITION_CLOSED 3000   // moves to here if too light

int upButtonState   = 1;     // 1 = open, 0 = pressed
int downButtonState = 1;     // 1 = open, 0 = pressed
int light_sensor_value = 0;  // Dark = 1023; pointing at bulb = 41
#define LIGHT_SENSOR_DARK_THRESHOLD    800  // less than this will trigger motion
#define LIGHT_SENSOR_LIGHT_THRESHOLD   400  // more than this will trigger motion
#define LIGHT_SENSOR_WAIT_PERIOD       200  // must breach threshold for this many consec. cycles
#define LIGHT_SENSOR_DELAY_PER_CYCLE   10   // duration of cycles above
int bright_counter = 0;      
int dark_counter = 0;
int motor_position = 0;            //current motor position
int desired_motor_position = 0;    //current motor position

void loop() {
  //read the buttons first
  upButtonState = digitalRead(UP_BUTTON_PIN);
  downButtonState = digitalRead(DOWN_BUTTON_PIN);
  light_sensor_value = analogRead(SENSOR_PIN);

  if (upButtonState == 0 && downButtonState ==0) {
    Serial.println(F("both buttons pressed!"));
  } else if (upButtonState == 0) {
    stepper.step(1);
    Serial.println("stepping forward");
  } else if (downButtonState == 0) {
    stepper.step(-1);
    Serial.println(F("stepping backward"));
  }
  else if (light_sensor_value < LIGHT_SENSOR_LIGHT_THRESHOLD) { //too bright!
    bright_counter++;
    delay(LIGHT_SENSOR_DELAY_PER_CYCLE);
    if(bright_counter > LIGHT_SENSOR_WAIT_PERIOD) {
      bright_counter = 0;
      Serial.print("LIGHT light sensor value: "); Serial.println(light_sensor_value);
      desired_motor_position = ONE_TENTH_MOTOR_POSITION_CLOSED;
    }
    // move the stepper to the closed position, if it's not already there
  } else if (light_sensor_value > LIGHT_SENSOR_DARK_THRESHOLD) {
    dark_counter++;
    delay(LIGHT_SENSOR_DELAY_PER_CYCLE);
    if(dark_counter > LIGHT_SENSOR_WAIT_PERIOD) {
      dark_counter = 0;
      Serial.print(F("DARK light sensor value: ")); Serial.println(light_sensor_value);
      desired_motor_position = MOTOR_POSITION_OPEN;
    }
  } else {
    bright_counter = 0;
    dark_counter = 0;
  }

  // motor go to desired position
  if (motor_position != desired_motor_position) {
    digitalWrite(LED_PIN, 1);
    Serial.print(F("moving motor to position: "));
    Serial.print(desired_motor_position);
    Serial.print(" ... ");

    // execute 100 slices of the steps I need to go
    for(int i=0; i<100; i++) {
      stepper.step( (desired_motor_position - motor_position)/10 );

      // check the state of the buttons and break out if I'm interrupted
      upButtonState = digitalRead(UP_BUTTON_PIN);
      downButtonState = digitalRead(DOWN_BUTTON_PIN);
      if (upButtonState == 0 || downButtonState ==0) {
        Serial.print(F("Button hit!  Exiting motion at position iteration: ")); Serial.println(i);
        break;
      } //if()
    } //for()

    // 
    motor_position = desired_motor_position; //assume we got there
    Serial.println("Motion DONE!");

    // Shut down all motor coils
    digitalWrite(STEPPER_PIN_1, 0);
    digitalWrite(STEPPER_PIN_2, 0);
    digitalWrite(STEPPER_PIN_3, 0);
    digitalWrite(STEPPER_PIN_4, 0);
  }
  digitalWrite(LED_PIN, 0);

  
}  // loop()

