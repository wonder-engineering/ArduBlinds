
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

#define LIMIT_PIN 3


void setup() {
  stepper.setSpeed(12);
  
  // initialize the serial port:
  Serial.begin(57600);

  // setup the LED output pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);

  // setup button pins with internal pullup
  pinMode(UP_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN,INPUT_PULLUP);
  pinMode(LIMIT_PIN,      INPUT_PULLUP);

  // setup the analog input pin (external pullup)
  pinMode(SENSOR_PIN,INPUT);
}

#define MOTOR_POSITION_OPEN  0                   // at powerup, assumes we're here
#define ONE_HUNDREDTH_MOTOR_POSITION_CLOSED 300  // moves to here if too light

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

enum ShadeState {bright, neutral, dark};
ShadeState sensor_state   = dark;   // the desired position
ShadeState shade_position = dark; // the actual position
bool led_state = false;

void loop() {
  // Read the buttons first
  upButtonState = digitalRead(UP_BUTTON_PIN);
  downButtonState = digitalRead(DOWN_BUTTON_PIN);
  light_sensor_value = analogRead(SENSOR_PIN);


  // Clear sensor counters and state when you press any button
  if(upButtonState == 0 && downButtonState ==0){
    sensor_state   = neutral;
    bright_counter = 0;
    dark_counter   = 0;
  }

  // Act on Button and sensor data
  if (upButtonState == 0 && downButtonState ==0) {
    Serial.println(F("Both buttons pressed. Setting shade position to 'dark' position!"));
    shade_position = dark;
  } else if (upButtonState == 0) {
    Serial.println("Stepping from dark to light position. ('up', 'blinds closed')");
    while (digitalRead(UP_BUTTON_PIN) == 0)
      stepper.step(1);
  } else if (downButtonState == 0) {
    Serial.println(F("Stepping from light toward dark position. ('down', 'blinds open')"));
    Serial.println(F(" To calibrate the string position to the 'blinds open' position,"));
    Serial.println(F("   press both buttons at once."));
    while (digitalRead(DOWN_BUTTON_PIN) == 0)
      stepper.step(-1);
  }
  else if (light_sensor_value < LIGHT_SENSOR_LIGHT_THRESHOLD) { //too bright!
    bright_counter++;
    dark_counter = 0;
    if(bright_counter > LIGHT_SENSOR_WAIT_PERIOD &&
       sensor_state != bright) {
      bright_counter = 0;
      sensor_state = bright;
      Serial.print("LIGHT condition detected! Last sensor value: "); 
      Serial.println(light_sensor_value);
    }
    // move the stepper to the closed position, if it's not already there
  } else if (light_sensor_value > LIGHT_SENSOR_DARK_THRESHOLD) {
    dark_counter++;
    bright_counter = 0;
    if(dark_counter > LIGHT_SENSOR_WAIT_PERIOD  &&
              sensor_state != dark) {
      dark_counter = 0;
      sensor_state = dark;
      Serial.print(F("DARK condition detected! Last sensor value: ")); 
      Serial.println(light_sensor_value);
    }
  } else {
    bright_counter = 0;
    dark_counter   = 0;
    sensor_state   = neutral;
  }


  // Move the motor if my position does not match my state
  if (sensor_state != neutral && shade_position != sensor_state) {
    digitalWrite(LED_PIN, 1);  // indicate motion with built in LED
    Serial.println(F("Moving motor!"));
    
    if(sensor_state == dark) {
      int iterator = 1;
      while(digitalRead(LIMIT_PIN) == 0) {
        stepper.step( (-1)*ONE_HUNDREDTH_MOTOR_POSITION_CLOSED );
        iterator ++;
        Serial.print(".");
        if (iterator % 10 == 0)
          Serial.println(iterator);
      }
    } else {
      // execute 100 slices of the steps I need to go
      for(int i=0; i<100; i++) {
  
        // Move in correct direction
        if (sensor_state == bright)
          stepper.step( ONE_HUNDREDTH_MOTOR_POSITION_CLOSED );
        else
          
  
        // blink the LED
        led_state = !led_state;
        if(led_state)
          digitalWrite(LED_PIN, 0);
        else
          digitalWrite(LED_PIN, 1);
  
        // check the state of the buttons and break out if I'm interrupted
        if (digitalRead(UP_BUTTON_PIN) == 0 ||
            digitalRead(DOWN_BUTTON_PIN) ==0) {
          Serial.print(F("Button hit!  Exiting motion at position iteration: ")); Serial.println(i);
          break;
        } else {
          if (i > 0 && i%10 == 0) {
            Serial.print(i); 
            Serial.println(F("\%"));
          }
          else if (i > 0){
            Serial.print(".");
          }
        }
      } //for()
    }

    
    Serial.println("Motion DONE!");
    shade_position = sensor_state;

    // Shut down all motor coils
    digitalWrite(STEPPER_PIN_1, 0);
    digitalWrite(STEPPER_PIN_2, 0);
    digitalWrite(STEPPER_PIN_3, 0);
    digitalWrite(STEPPER_PIN_4, 0);
  }
  digitalWrite(LED_PIN, 0);

  delay(LIGHT_SENSOR_DELAY_PER_CYCLE);
      
}  // loop()

