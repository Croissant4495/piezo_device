// _____LIBRARIES_____
#include "HX711.h"
#include <LiquidCrystal.h>
#include <MobaTools.h>

// _____CONSTANTS_____
#define button_pin 34
#define button2_pin 14
#define touch_no 3

#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN   5

#define dirPin 22
#define stepPin 21
#define motorInterfaceType 1

#define speed_increment 100
#define position_increment 4000
#define touch_cooldown 1000

// _____VARIABLES_____
int touch_pins[touch_no] = {27, 32, 33};
bool touch_values[touch_no] = {false, false, false};
const int touch_thresh = 20;

LiquidCrystal lcd(19, 23, 18, 17, 16, 15);
bool change_speed = true;

// AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin); 
int speed = 1000;
int target_position = 0;

const int stepsPerRev = 1000;
MoToStepper stepper1(stepsPerRev, STEPDIR);

HX711 scale;
float calibration_factor = 404990.00;
float weight_grams = 0;
float current_max = 0;

// _____FUNCTIONS_____
void update_LCD(int, int, bool, float);
bool button_pressed();
bool button2_pressed();
void read_touch();
void touched();
void update_loadcell();
void stepper_timer_setup();

void setup() {
  lcd.begin(16, 2);
  pinMode(button_pin, INPUT);
  pinMode(button2_pin, INPUT);
  Serial.begin(9600);

  lcd.setCursor(0, 0);
  lcd.print("spd: ");
  lcd.setCursor(0, 1);
  lcd.print("pos: ");
  lcd.setCursor(10, 0);
  lcd.print("Weight");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(1000);
  
  scale.set_scale();
  scale.tare();

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

void loop() {
  touched();
  scale.set_scale(calibration_factor);
  //update_loadcell();

  weight_grams = scale.get_units();
  current_max=max(weight_grams,current_max );
  
  Serial.println(weight_grams);
  update_LCD(speed, target_position, change_speed, current_max);

  if(button2_pressed()){
    scale.tare();
  }

  if(button_pressed()){
    stepper_timer_setup();
    reset();
  }
}

void update_LCD(int speed, int position, bool cursor, float weight) {
  static int last_speed = -1;
  static int last_position = -1;
  static float last_weight =-1;
  static bool last_cursor = !cursor; // Invert initial state to force update on first call
  // Check if values have changed
  if (speed != last_speed || position != last_position || cursor != last_cursor || weight != last_weight) {
    lcd.setCursor(4, 0);
    lcd.print("      "); // Clear old value
    lcd.setCursor(4, 0);
    lcd.print(speed);

    lcd.setCursor(4, 1);
    lcd.print("      "); // Clear old value
    lcd.setCursor(4, 1);
    lcd.print(position);

    lcd.setCursor(10, 1);
    lcd.print("     "); // Clear old value
    lcd.setCursor(10, 1);
    lcd.print(weight);

    lcd.setCursor(15, 1);
    lcd.print("K");
    
    if (cursor) {
      lcd.setCursor(9, 0);
      lcd.print("<");
      lcd.setCursor(9, 1);
      lcd.print(" ");
    } else {
      lcd.setCursor(9, 0);
      lcd.print(" ");
      lcd.setCursor(9, 1);
      lcd.print("<");
    }

    // Update last values
    last_speed = speed;
    last_position = position;
    last_cursor = cursor;
    last_weight = weight;
  }
}

bool button_pressed(){
  static bool update = true;
  int reading = digitalRead(button_pin);
  if (update && reading){
    update = false;
    return true;
  }else if(!reading){
    update = true;
  }
  return false;
}

bool button2_pressed(){
  static bool update = true;
  int reading = digitalRead(button2_pin);
  if (update && reading){
    update = false;
    return true;
  }else if(!reading){
    update = true;
  }
  return false;
}

void read_touch() {
  static unsigned long lastTouchTime[touch_no] = {0};
  static bool last_touch_state[touch_no] = {false};
  static unsigned long cooldownEndTime[touch_no] = {0};
  unsigned long currentTime = millis();

  for (int i = 0; i < touch_no; i++) {
    int touchValue = touchRead(touch_pins[i]);
    bool currentState = touchValue < touch_thresh;

    if (currentState && !last_touch_state[i]) {
      // Touch just started
      if (currentTime >= cooldownEndTime[i]) {
        touch_values[i] = true;
        cooldownEndTime[i] = currentTime + touch_cooldown; // 1000ms cooldown
      } else {
        touch_values[i] = false;
      }
    } else if (!currentState) {
      // Touch released
      touch_values[i] = false;
    }

    last_touch_state[i] = currentState;
  }
}

void touched(){
  read_touch();
  if(touch_values[0]){
    change_speed = !change_speed;
     delay(1000);
  }else if(touch_values[1]){
    if(change_speed){
      speed += speed_increment;
    }else{
      target_position += position_increment;
    }
    // delay(500);
  }else if(touch_values[2]){
    if(change_speed){
      speed -= speed_increment;
    }else{
      target_position -= position_increment;
    }
    // delay(500);
  }
}

void stepper_timer_setup(){
  stepper1.attach(stepPin, dirPin);
  stepper1.setSpeed(speed);              // 30 rev/min (if stepsPerRev is set correctly)
  stepper1.setRampLen( stepsPerRev / 2); // Ramp length is 1/2 revolution
  stepper1.rotate(1);
}

int max(int x, int y){
    if(x  >  y){
      return x;
    }
    else{
      return y;
    }
}

void reset(){
  current_max=0;
}

// void update_loadcell() {
//   static float last_weight = 0;
//   const int numReadings = 10;       
//   static float weight_buffer[numReadings] = {0};  // Initialize buffer to avoid garbage value 
//   static int index = 0;            
//   static bool buffer_filled = false;

//   const float changeThreshold = 10.0;  // The change necessary to trigger that the value has changed

//   // Read and store the new value in the buffer
//   scale.set_scale(calibration_factor);
//   weight_buffer[index] = scale.get_units();
//   index = (index + 1) % numReadings;

//   if (index == 0) buffer_filled = true;

//   // Compute moving average
//   float sum = 0;
//   int count = buffer_filled ? numReadings : index;
//   for (int i = 0; i < count; i++) sum += weight_buffer[i];
//   float avg_weight = sum / count;

//   // Update only if a significant change occurs
//   if (abs(avg_weight - last_weight) > changeThreshold) {
//     last_weight = avg_weight;
//     weight_grams = avg_weight;
//   }
// }