// ================================================================
// ===                      Libraries                           ===
// ================================================================
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ================================================================
// ===                      Initialise Pins                     ===
// ================================================================
#define LED_PIN 13 // (Arduino is 13)

#define BUZZER_PIN 8
#define DOWN_BUTTON_PIN 9
#define DIR_PIN 10
#define STEP_PIN 11
#define UP_BUTTON_PIN 12
#define ENTER_BUTTON_PIN 13
#define INTERRUPT_PIN 3
#define ANGLE_PIN 2


// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

// Declare custom functions
void store_angle(float *angle);
void mode_2();
void mode_3();
void mode_4();
int up_button_toggle();
int down_button_toggle();
int enter_button_toggle();
void print_ypr();
void sound_buzzer();
void lcd_print();
int update_angle();
void print_mainscreen(int editing_mode, int mode, int sub_mode);
bool blink_value_flag(unsigned long interval);

// LCD Stuff
String excercise_text;
String speed_text;
String reps_text;
String angle_text;

// Servo Stuff
unsigned long prevStepMicros = 0;

// Button Stuffs
int enter_btn_new_state;
int enter_btn_old_state = 0;

int down_btn_new_state;
int down_btn_old_state = 0;

int up_btn_new_state;
int up_btn_old_state = 0;

// Excercise Stuffs
bool excercise_in_progress = false;
float first_angle = -1;
float second_angle = -1;
float angle_range = 0;
int reps_total = 10;
int reps_current = 1;
int speed = 1;
int current_arm_angle = 0;

// For blinking when in editing mode
unsigned long previousBlinkMillis = 0;
int showvalue_flag = 1;

// Default Mode
int mode = 4;

void setup() {

	// initialize the LCD
	lcd.init();
  lcd.backlight();
	lcd.print("Booting...");

  // Initialise buttons
  pinMode(ENTER_BUTTON_PIN, INPUT);
  pinMode(UP_BUTTON_PIN, INPUT);
  pinMode(DOWN_BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ANGLE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================
int count = 0;
void loop() {
    count += 1;

    if (mode == 1)
    {
      print_mainscreen(0,1,1);
      // Check for if user wants to change mode
      if(up_button_toggle() || down_button_toggle())
      {
        // Go into loop and ask for edit
        int choice = 2;

        // Put in a while loop until user makes a choice to edit which mode
        while(1)
        {
          // If upbutton is pressed
          if (up_button_toggle())
          {
            choice += 1;
            if (choice == 5)
              choice = 1;
          }
          
          // If down button is pressed
          else if (down_button_toggle())
          {
            choice -= 1;
            if (choice == 0)
              choice = 4;
          }

          if (enter_button_toggle())
          {
            if (choice == 2)
            {
              mode = 2;
              break;
            }
            else if (choice == 3)
            {
              mode = 3;
              break;
            }
            else if (choice == 4)
            {
              mode = 4;
              break;
            }
          }
          print_mainscreen(2, choice,0);
        }
      }
    }
    else if (mode == 2)
    {
      mode_2();
      mode = 1;
    }

    else if (mode == 3)
    {
      mode_3();
      mode = 1;
    }

    else if (mode == 4)
    {
      mode_4();
      mode = 1;
    }
}

// Input: None
// Output: Sounds buzzer for 0.5s
void sound_buzzer()
{
  digitalWrite(BUZZER_PIN, 1);
  delay(100);
  digitalWrite(BUZZER_PIN, 0);
}

// Input: enter_btn_to_be_checked
// Output: returns int 1 if button is released from being pressed
int enter_button_toggle()
{
  enter_btn_new_state = digitalRead(ENTER_BUTTON_PIN);

  if (enter_btn_old_state == 1 && enter_btn_new_state == 0)
  {
    enter_btn_old_state = enter_btn_new_state;
    sound_buzzer();
    return 1;
  }
  else
  {
    enter_btn_old_state = enter_btn_new_state;
    return 0;
  }
}


// Input: up_btn_to_be_checked
// Output: returns int 1 if button is released from being pressed
int up_button_toggle()
{
  up_btn_new_state = digitalRead(UP_BUTTON_PIN);

  if (up_btn_old_state == 1 && up_btn_new_state == 0)
  {
    up_btn_old_state = up_btn_new_state;
    sound_buzzer();
    return 1;
  }
  else
  {
    up_btn_old_state = up_btn_new_state;
    return 0;
  }
}

// Input: down_btn_to_be_checked
// Output: returns int 1 if button is released from being pressed
int down_button_toggle()
{
  down_btn_new_state = digitalRead(DOWN_BUTTON_PIN);

  if (down_btn_old_state == 1 && down_btn_new_state == 0)
  {
    down_btn_old_state = down_btn_new_state;
    sound_buzzer();
    return 1;
  }
  else
  {
    down_btn_old_state = down_btn_new_state;
    return 0;
  }
}

// Input:
// 1) int editing_mode, Takes in editing mode is 0 = not editing, 1 = editing mode, 2 = choosing mode
// 2) int mode, For selecting which mode to blink when in editing mode
// 3) int rep_in_progress, To change output text of Press enter to start/stop excercise
void print_mainscreen(int editing_mode, int mode, int sub_mode)
{
  excercise_text = "                    ";
  speed_text = "                    ";
  reps_text = "                    ";
  angle_text = "                    ";

  // Clear screen first
  lcd.setCursor(0,0);
  lcd.print(excercise_text);
  lcd.setCursor(0,1);
  lcd.print(speed_text);
  lcd.setCursor(0,2);
  lcd.print(reps_text);
  lcd.setCursor(0,3);
  lcd.print(angle_text);
  String start_excercise_text = "Press ENTER to start";
  String stop_excercise_text = "Press ENTER to stop excercise";
  String speed_prefixtext = "Speed: ";
  String reps_prefixtext = "REPS: ";
  String angle_prefixtext = "Angle: ";

  if (editing_mode && blink_value_flag(1000))
  {
    showvalue_flag = !showvalue_flag;
  }

  if (editing_mode == 0)
  {
    if (mode == 4)
    {
      if (sub_mode == 1)
      {
        excercise_text = "  Enter to capture";
        reps_text = "    first angle     ";
      }
      else if (sub_mode == 2)
      {
        excercise_text = "  Enter to capture";
        reps_text = "    second angle    ";
      }
      // Mode 1 = excercise page
      else if (mode == 1)
      {
        //sub_mode 1 = haven't started excercise
        if (sub_mode == 1)
        {
          excercise_text = start_excercise_text;
          reps_text = reps_prefixtext + String(reps_total);
          angle_text = angle_prefixtext + String(angle_range);
          speed_text = speed_prefixtext + String(speed);
        }

        // sub_mode 2 = started excercise already
        else if (sub_mode == 2)
        {
          excercise_text = stop_excercise_text;
          reps_text = reps_prefixtext + String(reps_current);
          angle_text = angle_prefixtext + String(angle_range);
          speed_text = speed_prefixtext + String(speed);
        }
      }
    }
  }
  else if (editing_mode == 1)
  {
    if (mode==2)
    {
      excercise_text = "Editing Speed";
      reps_text = reps_prefixtext + String(reps_total);
      angle_text = angle_prefixtext + String(angle_range);
      if (showvalue_flag)
        speed_text = speed_prefixtext + String(speed);
      else
        speed_text = speed_prefixtext + "Enter to set";
    }
    else if (mode == 3)
    {
      excercise_text = "Editing REPS";
      speed_text = speed_prefixtext += String(speed);
      angle_text = angle_prefixtext += String(angle_range);
      if (showvalue_flag)
        reps_text = reps_prefixtext += String(reps_total);
      else
        reps_text = reps_prefixtext += "Enter to set";
    }
    else if (mode == 4)
    {
      excercise_text = "Editing angle";
      speed_text = speed_prefixtext += String(speed);
      reps_text = reps_prefixtext += String(angle_range);
      if (showvalue_flag)
        angle_text = angle_prefixtext += String(angle_range);
      else
        angle_text = angle_prefixtext += "Enter to set";
    }
  }

  else if (editing_mode == 2)
  {
    speed_text = speed_prefixtext += String(speed);
    reps_text = reps_prefixtext += String(reps_total);
    angle_text = angle_prefixtext += String(angle_range);
    if (mode==1)
    {
      excercise_text = "Enter to go back";
    }
    
    if (mode==2)
    {
      excercise_text = "Enter to set Speed";
    }
    else if (mode == 3)
    {
      excercise_text = "Enter to set REPS";
    }
    else if (mode == 4)
    {
      excercise_text = "Enter to set angle";
    }
  }

  lcd.setCursor(0,0);
  lcd.print(excercise_text);
  lcd.setCursor(0,1);
  lcd.print(speed_text);
  lcd.setCursor(0,2);
  lcd.print(reps_text);
  lcd.setCursor(0,3);
  lcd.print(angle_text);
}

// need to constantly read the new speed_update variable in the menu to show the changes
// to allow up and down button to change speed variable everytime they are pressed
// enter key to lock in the speed (mapped to 100 for stepper)

void mode_2()
{
  while (1) {
    print_mainscreen(1,2,0);
    if(up_button_toggle()){
      if(0<speed && speed<10){
      speed+=1;
      }
    }
    else if(down_button_toggle()){
      if(0<speed && speed<10){
      speed-=1;
      }
    }
    else if(enter_button_toggle()){
      speed = map(speed, 0,10,0,100);
      break;
    }
  }
}

//change reps by +-, enter to save into "rep" variable

void mode_3()
{
  while (1) {
    print_mainscreen(1,3,0);
    if(up_button_toggle()){
      if(0<reps_total && reps_total<30){
        reps_total+=1;
      }
    }
    else if(down_button_toggle()){
      if(0<reps_total && reps_total<30){
        reps_total-=1;      
      }
    }
    else if(enter_button_toggle()){
      break;
    }
  }
}

void mode_4()
{
  if (first_angle == -1){
    print_mainscreen(0, 4, 1);
    store_angle(&first_angle);
  }
  if (second_angle == -1){
    print_mainscreen(0, 4, 2);
    store_angle(&second_angle);
  }
  angle_range = round(second_angle-first_angle);
}

bool blink_value_flag(unsigned long interval)
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousBlinkMillis >= interval)
  {
    previousBlinkMillis = currentMillis;
  return true;
  }
  else
    return false;
}

// Input: address of angle variable to be updated
// Output: None, updates angle at input address with the current roll_angle
void store_angle(float *angle)
{
 while(1)
  {
    current_arm_angle = update_angle();
    if (enter_button_toggle()){
      *angle = current_arm_angle;
      break;
    }
  }
}

// INPUT: None
// Output: Once function is called, take in a sensor reading and put it in a buffer, and return the average of all readings in the buffer
int update_angle()
{
  int WINDOW_SIZE = 5;
  int INDEX = 0;
  int VALUE = 0;
  int SUM = 0;
  int READINGS[WINDOW_SIZE];
  int AVERAGED = 0;

  SUM = SUM - READINGS[INDEX];
  VALUE = analogRead(ANGLE_PIN);
  READINGS[INDEX] = VALUE;           // Add the newest reading to the window
  SUM = SUM + VALUE;                 // Add the newest reading to the sum
  INDEX = (INDEX+1) % WINDOW_SIZE;   // Increment the index, and wrap to 0 if it exceeds the window size

  AVERAGED = SUM / WINDOW_SIZE;      // Divide the sum of the window by the window size for the result

  return AVERAGED;
}