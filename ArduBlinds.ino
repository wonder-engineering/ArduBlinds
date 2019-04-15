
/*

  Arduino Firmware for the WonderTechnologies, LLC ArduBlinds hardware.


  TODO: 
    * Test limit switches with buttons for safety
    * put light sensor in a holder
    * Design circuit
    * wider diameter lugs for board mounting
 */
#include <Arduino.h>
#include <EEPROM.h>

// UI TYPES AND GLOBALS
#define PRINT_PERIOD_MS  2000
#define SERIAL_BAUD      57600

// SENSOR TYPES AND GLOBALS
#define SENSOR_PIN       A7  //  todo: attach sensor!  Analog in plus pullup
#define LIGHT_SENSOR_DARK_THRESHOLD    250  // more than this will close blinds
                                            // trigger motion
#define LIGHT_SENSOR_LIGHT_THRESHOLD   75   // less than this will
                                            // trigger motion
#define LIGHT_SENSOR_WAIT_PERIOD       200  // must breach threshold
                                            // for this many consec. cycles
#define LIGHT_SENSOR_DELAY_PER_CYCLE   10   // duration of cycles above
int bright_counter = 0;
int dark_counter   = 0;
int light_sensor_value = 0;
enum ShadeState {bright, neutral, dark};
ShadeState sensor_state   = dark;           // the desired position
ShadeState shade_position = dark;           // the actual position

// BUILTIN LED TYPES AND GLOBALS
#define LED_PIN 13                          // Builtin LED
bool led_state = false;                     // Builtin LED


// BUTTON TYPES AND GLOBALS
#define OPEN_BUTTON_PIN    9   // normally open (high)
#define CLOSED_BUTTON_PIN  8   // normally open (high)
int openedButtonState    = 1;     // 1 = closed, 0 = pressed
int closedButtonState    = 1;     // 1 = closed, 0 = pressed


// H-BRIDGE CONTROL TYPES AND GLOBALS
#define MOTOR_SPEED                    128 // 0-255 is 0-100% duty cycle
#define CLOSED_LIMIT_PIN               3      // HIGH = not pressed
#define OPEN_LIMIT_PIN                 2      // HIGH = not pressed
#define H_BRIDGE_PIN_A                 5      // IN3; HIGH = power on
#define H_BRIDGE_PIN_B                 4      // IN4; HIGH = OPEN direction
#define H_BRIDGE_PIN_ENA               11     // PWM output
#define MOTOR_CYCLE_DELAY_MS           100    // Delay while holding button closed
#define MOVEMENT_TIMEOUT_MS            10000  // don't try moving for longer than this
enum motor_state {
  motor_forward = 0,
  motor_reverse = 1,
  motor_halt    = 2
};
const int h_bridge_state[][2] = {
  {HIGH, LOW},    // motor_forward
  {LOW, HIGH},    // motor_reverse
  {LOW,  LOW}};   // motor_halt
enum ShadeDirection {opened,closed};


// DATA COLLECTION TYPES AND GLOBALS
#define COLLECT_SENSOR_DATA              false
#define DATA_COLLECTION_INTERVAL_MS      1200000  // 20 min allows 1 full day of data collection
typedef struct DataElement{
  unsigned long timestamp;
  uint16_t light_value;
};
union eeprom_element{
  DataElement element;
  uint8_t bytearray[sizeof(DataElement)];
}data_element;
unsigned long now_time  = 0;
unsigned long last_time = 500000;  // init value causes us to collect data on the first iteration
uint16_t eeprom_index = 0;


///////////////////// FUNCTIONS /////////////////////

void move_toward(ShadeDirection dest) { 
  // set the power relay to the correct direction
  digitalWrite(H_BRIDGE_PIN_A, h_bridge_state[dest][0]);
  digitalWrite(H_BRIDGE_PIN_B, h_bridge_state[dest][1]);

  // indicate motion with built in LED
  Serial.println(F("Moving motor!"));
} //  move_toward()


void stop_motion() { 
  // set the motor to ON
  digitalWrite(H_BRIDGE_PIN_A, h_bridge_state[motor_halt][0]);
  digitalWrite(H_BRIDGE_PIN_B, h_bridge_state[motor_halt][1]);
} //  stop_motion()


void move_to_limit(ShadeDirection dest, int timeout_ms) {

  // select the limit switch
  int limit_pin = (dest == closed) ? CLOSED_LIMIT_PIN : OPEN_LIMIT_PIN;

  // indicate motion with built in LED
  Serial.print(F("Moving motor toward!")); Serial.println(dest);
  Serial.print(F("Reading pin: ")); Serial.println(limit_pin);

  int iterations_to_run = timeout_ms / MOTOR_CYCLE_DELAY_MS;

  // wait till the limit switch is pressed
  for (int iterator=1; iterator <= iterations_to_run; iterator++) {
    // check limit switch
    if(digitalRead(limit_pin) == LOW)
      break;

    // Print to serial
    Serial.print(".");
    if (iterator % 10 == 0)
      Serial.println(iterator);
    
    // blink the LED
    led_state = !led_state;
    if (led_state)
      digitalWrite(LED_PIN, LOW);
    else
      digitalWrite(LED_PIN, HIGH);

    // Check for button presses
    if (digitalRead(OPEN_BUTTON_PIN) == LOW ||
          digitalRead(CLOSED_BUTTON_PIN) == LOW) {
      Serial.print(F("Button hit!  Exiting motion "
                     "at position iteration: "));
      Serial.println(iterator);
        break;
    }
    move_toward(dest);

    // wait 1/2 second
    delay(MOTOR_CYCLE_DELAY_MS);
          
  }  // while()

  // set the motor to OFF
  Serial.println("Motion DONE!");
  digitalWrite(LED_PIN, LOW);

  stop_motion();
}


void dump_eeprom(){
  Serial.println(F("Dumping last data collection from EEPROM"));
  
  // dump all eeprom to serial
  Serial.println(F("TIMESTAMP , Data"));
  Serial.println(F("--------- , ----")); 
  for(int idx=0; idx < EEPROM.length(); idx += sizeof(DataElement)){
    // Read Data Element
    for(int j = 0; j < sizeof(DataElement); j++){
      data_element.bytearray[j] = EEPROM[idx + j];
    }
    Serial.print(data_element.element.timestamp); Serial.print(F(" , ")); Serial.println(data_element.element.light_value);
  }
}

void zeroize_eeprom(){
  Serial.println(F("Zeroizing EEPROM..."));
  // zeroize EEPROM (todo: actually make this a dump, or get EEPROM to roll over
  for(int i=0; i<EEPROM.length(); i++){
    EEPROM.update(i,0);
  }
  Serial.println(F("Zeroize complete."));
}

void setup_header(){
  Serial.print(F("setup(): ")); Serial.print(millis()); Serial.print(": ");
}

void loop_header(){
  Serial.print(F("setup(): ")); Serial.print(millis()); Serial.print(": ");
}

void button_move(ShadeDirection dir){

  int limit_pin  = (dir == closed) ? CLOSED_LIMIT_PIN : OPEN_LIMIT_PIN;
  int button_pin = (dir == closed) ? CLOSED_BUTTON_PIN : OPEN_BUTTON_PIN;
  
  Serial.print("Moving shades with button in direction )"); Serial.println(dir);

  while (digitalRead(button_pin) == LOW &&
         digitalRead(limit_pin)  == HIGH){
    move_toward(opened);
    delay(MOTOR_CYCLE_DELAY_MS);
  }
  stop_motion();
}


void dump_sensor_data(){
  // light sensor
    loop_header(); Serial.print(F("Light Sensor: [ ")); 
      Serial.print(analogRead(SENSOR_PIN)); 
      Serial.print(F(" ]"));
    loop_header(); Serial.print(F("OPEN_LIM[ ")); 
      Serial.print(digitalRead(OPEN_LIMIT_PIN)); 
      Serial.print(F(" ]"));
    loop_header(); Serial.print(F("CLOSED_LIM[ ")); 
      Serial.print(digitalRead(CLOSED_LIMIT_PIN)); 
      Serial.print(F(" ]"));
    loop_header(); Serial.print(F("(HIGH=")); 
      Serial.print(HIGH); 
      Serial.println(F(")"));
}

///////////////////// SETUP /////////////////////

void setup() {

  // initialize the serial port:
  Serial.begin(SERIAL_BAUD);

  setup_header(); Serial.println(F("Setting pin Modes..."));
  // setup the LED output pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // setup button and limit pins with internal pullup
  pinMode(OPEN_BUTTON_PIN,    INPUT_PULLUP);
  pinMode(CLOSED_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(OPEN_LIMIT_PIN,     INPUT_PULLUP);
  pinMode(CLOSED_LIMIT_PIN,   INPUT_PULLUP);
  pinMode(H_BRIDGE_PIN_A,   OUTPUT);
  pinMode(H_BRIDGE_PIN_B,   OUTPUT);
  pinMode(H_BRIDGE_PIN_ENA, OUTPUT);
  analogWrite(H_BRIDGE_PIN_ENA, MOTOR_SPEED);
    
  // setup the analog input pin (external pullup)
  pinMode(SENSOR_PIN, INPUT);

  dump_eeprom();

  if(COLLECT_SENSOR_DATA){
    setup_header(); Serial.println(F("Waiting 30 seconds before erasing last data collection..."));
    delay(30000);
    zeroize_eeprom();
  } else {
    setup_header(); Serial.println(F("NOT Collecting sensor data..."));
  }

  setup_header(); Serial.println(F("Moving to Open (opened) position."));
  move_to_limit(opened, 20000);

  setup_header(); Serial.println(F("Init complete!"));
}


///////////////////// LOOP /////////////////////


void loop() {
  // Read the buttons first
  openedButtonState  = digitalRead(OPEN_BUTTON_PIN);
  closedButtonState  = digitalRead(CLOSED_BUTTON_PIN);
  light_sensor_value = analogRead(SENSOR_PIN);


  // Clear sensor counters and state when you press any button
  if (openedButtonState == LOW && closedButtonState == LOW) {
    sensor_state   = neutral;
    bright_counter = 0;
    dark_counter   = 0;
  }

  // Act on Button and sensor data
  if (openedButtonState == 0) {
    button_move(opened);
  } else if (closedButtonState == 0) {
    button_move(closed);
  } else if (light_sensor_value < LIGHT_SENSOR_LIGHT_THRESHOLD) {
    bright_counter++;
    dark_counter = 0;
    if (bright_counter > LIGHT_SENSOR_WAIT_PERIOD &&
       sensor_state != bright) {
      bright_counter = 0;
      sensor_state = bright;
      loop_header(); Serial.print("LIGHT condition detected! Last sensor value: ");
      Serial.println(light_sensor_value);
    }
    // move the stepper to the openedd position, if it's not already there
  } else if (light_sensor_value > LIGHT_SENSOR_DARK_THRESHOLD) {
    dark_counter++;
    bright_counter = 0;
    if (dark_counter > LIGHT_SENSOR_WAIT_PERIOD  &&
              sensor_state != dark) {
      dark_counter = 0;
      sensor_state = dark;
      loop_header(); Serial.print(F("DARK condition detected! Last sensor value: "));
      Serial.println(light_sensor_value);
    }
  } else {  // neither bright nor dark, reset sensor state
    bright_counter = 0;
    dark_counter   = 0;
    sensor_state   = neutral;
  }

  // Make motor move, if needed
  if (sensor_state != neutral && shade_position != sensor_state) {
    if(sensor_state == dark)
      move_to_limit(opened, MOVEMENT_TIMEOUT_MS);
    else
      move_to_limit(closed, MOVEMENT_TIMEOUT_MS);
    shade_position = sensor_state;
  }

  // Record the current light sensor value to EEPROM, if it's time
  now_time  = millis();
  static unsigned long delta_time = 0;
  // Get delta, handling rollover
  if(now_time < last_time)
    delta_time = 4294967295 - last_time + now_time;
  else
    delta_time = now_time - last_time;


  // Check whether it's time
  if(delta_time > DATA_COLLECTION_INTERVAL_MS && COLLECT_SENSOR_DATA){
    last_time = now_time;  // reset the timer

    // Collect data
    data_element.element.timestamp = now_time;
    data_element.element.light_value = light_sensor_value;


    // if EEPROM is not full, write a data element to it (indexed by 
    if(eeprom_index + sizeof(data_element) < EEPROM.length()){
      for(int i=0; i<sizeof(data_element); i++){
        // todo: check for data overflow here (for now, just stop and don't overwrite)
        EEPROM.update(eeprom_index++,data_element.bytearray[i]);
      }
      //todo: add a delimiter (or checksum) here, if I find I need it
      loop_header(); Serial.println(F("Light data collected:"));
    }
    else{
      loop_header(); Serial.println(F("EEPROM is full - skipping data collection for point:"));
    }
    Serial.println(data_element.element.timestamp);
    Serial.println(data_element.element.light_value);

  }

  if(millis() % PRINT_PERIOD_MS < 10)
    dump_sensor_data();

  delay(LIGHT_SENSOR_DELAY_PER_CYCLE);
}  // loop()
