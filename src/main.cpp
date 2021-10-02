// ================================================================
// ===                      Libraries                           ===
// ================================================================
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino.h>
#include <AccelStepper.h>
#include <ResponsiveAnalogRead.h>
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
#define ANGLE_PIN A1


// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

// Declare custom functions
// void store_angle(float *angle, int angle_num);
void mode_1();
void mode_2();
void mode_3();
void mode_4();
int up_button_toggle();
int down_button_toggle();
int enter_button_toggle();
int up_button_press();
int down_button_press();
void print_ypr();
void sound_buzzer();
void lcd_print();
void update_angle();
void print_mainscreen(int editing_mode, int mode, int sub_mode);
bool blink_value_flag(unsigned long interval);
void move_stepper(int direction);
void sound_end_excercise_buzzer();


// LCD Stuff
String excercise_text;
String speed_text;
String reps_text;
String angle_text;

// Accelerometer Stuff
int WINDOW_SIZE = 20;
int INDEX = 0;
int VALUE = 0;
int SUM = 0;
int READINGS[20];
int AVERAGED = 0;
ResponsiveAnalogRead accelerometer(ANGLE_PIN, true);

// Stepper Stuff
// Motor interface type must be set to 1 when using a driver (using step and dir pins)
#define dirPin 10
#define stepPin 11
#define motorInterfaceType 1
int num_steps_travelled = 0;
int direction_motor = 0;
int maxspeed = 1400;
int motor_speed = 800;
int forward_speed = 1400;
int reverse_speed = -1400;
int slow_reverse_speed = -700;
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin); // Creates a new instance of the AccelStepper class:


// Button Stuffs
int enter_btn_new_state;
int enter_btn_old_state = 0;

int down_btn_new_state;
int down_btn_old_state = 0;

int up_btn_new_state;
int up_btn_old_state = 0;

// Excercise Stuffs
float first_angle = -1;
float second_angle = -1;
int reps_total = 10;
int reps_current = 0;
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

	//setting stepper's max speed and acceleration
	stepper.setMaxSpeed(maxspeed);
  stepper.setAcceleration(100);
}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================
int choice = 0;
void loop() {
    Serial.println((String)choice + "    " + (String)mode);
    if (mode == 1)
    {
      print_mainscreen(0,1,1);
      if (enter_button_toggle())
      {
				mode_1();
        reps_current = 0;
        mode = 1;
      }

      // Check for if user wants to change mode
      else if(up_button_toggle() || down_button_toggle())
      {
        // Go into loop and ask for edit
        choice = 2;

        // Put in a while loop until user makes a choice to edit which mode
        while(1)
        {
          Serial.println(choice);
          // If upbutton is pressed
          if (up_button_toggle())
          {
            choice -= 1;
            if (choice == 1)
              choice = 4;
          }

          // If down button is pressed
          else if (down_button_toggle())
          {
            choice += 1;
            if (choice == 5)
              choice = 2;
          }

          else if (enter_button_toggle())
          {
            mode = choice;
            break;
          }
          print_mainscreen(2, choice, 1);
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
  //digitalWrite(BUZZER_PIN, 1);
  delay(100);
  digitalWrite(BUZZER_PIN, 0);
}

void sound_end_excercise_buzzer()
{
  delay(1000);
  sound_buzzer();
  delay(500);
  sound_buzzer();
  delay(250);
  sound_buzzer();
  delay(250);
  sound_buzzer();
  delay(500);
  sound_buzzer();
  delay(1000);
  sound_buzzer();
  delay(500);
  sound_buzzer();
  delay(500);
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

// Input: up_btn_to_be_checked
// Output: returns int 1 if button is pressed and held
int up_button_press()
{
  up_btn_new_state = digitalRead(UP_BUTTON_PIN);

  if (up_btn_old_state == 1 && up_btn_new_state == 1)
  {
    up_btn_old_state = up_btn_new_state;
    return 1;
  }
  else
  {
    up_btn_old_state = up_btn_new_state;
    return 0;
  }
}

// Input: down_btn_to_be_checked
// Output: returns int 1 if button is pressed and held
int down_button_press()
{
  down_btn_new_state = digitalRead(DOWN_BUTTON_PIN);

  if (down_btn_old_state == 1 && down_btn_new_state == 1)
  {
    down_btn_old_state = down_btn_new_state;
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
String wait_text = "Please Wait!        ";
String start_excercise_text = "Press ENTER to start";
String stop_excercise_text = "Press ENTER to stop ";
String finished_excercise_text = "Exercise Complete!  ";
String speed_prefixtext = "Speed: ";
String reps_prefixtext = "REPS: ";
String angle_prefixtext = "Angle: ";
String blank = "                    ";
void print_mainscreen(int editing_mode, int mode, int sub_mode)
{
  if (blink_value_flag(700))
  {
    showvalue_flag = !showvalue_flag;
  }

  if (editing_mode == 0)
  {
    if (mode == 4)
    {
      if (sub_mode == 1)
      {
        excercise_text = wait_text;
      }
    }

    // Mode 1 = excercise page
    else if (mode == 1)
    {
      //sub_mode 1 = haven't started excercise
      if (sub_mode == 1)
      {
				if (showvalue_flag)
          excercise_text = start_excercise_text;
        else
          excercise_text = blank;
        reps_text = reps_prefixtext + String(reps_total) + "          ";
        angle_text = angle_prefixtext + String(second_angle) + "      ";
        speed_text = speed_prefixtext + String(speed) + "            ";
      }

      // sub_mode 2 = started excercise already
      else if (sub_mode == 2)
      {
        excercise_text = stop_excercise_text;
        reps_text = reps_prefixtext + String(reps_current) + '/'+ String(reps_total) + "      ";
        angle_text = angle_prefixtext + String(second_angle) + "      ";
        speed_text = speed_prefixtext + String(speed) + "            ";
      }

			// sub_mode 3 = finished excercise
      else if (sub_mode == 3)
      {
        excercise_text = finished_excercise_text;
        reps_text = reps_prefixtext + String(reps_current) + '/'+ String(reps_total) + "      ";
        angle_text = angle_prefixtext + String(second_angle) + "      ";
        speed_text = speed_prefixtext + String(speed) + "            ";;
      }

			// sub mode 4 (test for lcd screen with updapted reps and moving down @ speed) (shown after one hand is pulled up)
			// angle does not change, stays at second_angle
			else if (sub_mode == 4)
      {
        excercise_text = stop_excercise_text;
        reps_text = reps_prefixtext + String(reps_current) + '/'+ String(reps_total) + "      ";
        angle_text = angle_prefixtext + " Moving Down ";
        speed_text = speed_prefixtext + String(speed);
      }
			// sub mode 5 (test for moving up @ speed) (shown after hand is returned to neutral position)
			// angle does not change, stays at second_angle
			else if (sub_mode == 5)
      {
        excercise_text = stop_excercise_text;
        reps_text = reps_prefixtext + String(reps_current) + '/'+ String(reps_total) + "      ";
        angle_text = angle_prefixtext + " Moving Up  ";
        speed_text = speed_prefixtext + String(speed);
      }


    }
  }
  else if (editing_mode == 1)
  {
    if (mode==2)
    {
      if (sub_mode == 1)
      {
        excercise_text = "Editing Speed       ";
        reps_text = reps_prefixtext + String(reps_total) + "          ";
        angle_text = angle_prefixtext + String(second_angle) + "          ";
        if (showvalue_flag)
          speed_text = speed_prefixtext + String(speed) + "           ";
        else
          speed_text = speed_prefixtext + "                ";
      }
    }
    else if (mode == 3)
    {
      if (sub_mode == 1)
      {
        excercise_text = "Editing REPS        ";
        speed_text = speed_prefixtext + String(speed) + "          ";
        angle_text = angle_prefixtext + String(second_angle) + "          ";
        if (showvalue_flag)
          reps_text = reps_prefixtext + String(reps_total) + "          ";
        else
          reps_text = reps_prefixtext + "                ";
      }
    }
    else if (mode == 4)
    {
      if (sub_mode == 1)
      {
        excercise_text = "   Use up/down to   ";
        speed_text = " Change FIRST angle ";
				reps_text = "   Enter to  Save   ";
				angle_text = "    " + String((int)current_arm_angle) + "  ||  " + "NIL   ";
      }
      else if (sub_mode == 2)
      {
				excercise_text = "   Use up/down to   ";
        speed_text = "Change SECOND  angle";
				reps_text = "   Enter to  Save   ";
				angle_text = "    " + String((int)first_angle) + "  ||  " + String((int)current_arm_angle)+ "  ";
      }
      else if (sub_mode == 3)
      {
				excercise_text = blank;
        speed_text =  "   Invalid angle!   ";
				reps_text =   "     Try again!     ";
				angle_text =  blank;
      }
    }
  }

  else if (editing_mode == 2)
  {
    if (mode==2)
    {
      if (sub_mode == 1)
      {
        excercise_text = "Enter to set Speed  ";
        reps_text = reps_prefixtext + String(reps_total) + "           ";
        angle_text = angle_prefixtext + String(second_angle) + "          ";
        if (showvalue_flag)
          speed_text = speed_prefixtext + String(speed) + "          ";
        else
          speed_text = speed_prefixtext + "          ";
      }
    }
    else if (mode == 3)
    {
      if (sub_mode == 1)
      {
        excercise_text = "Enter to set REPS   ";
        speed_text = speed_prefixtext + String(speed) + "           ";
        angle_text = angle_prefixtext + String(second_angle) + "          ";
        if (showvalue_flag)
          reps_text = reps_prefixtext + String(reps_total) + "          ";
        else
          reps_text = reps_prefixtext + "          ";
      }
    }
    else if (mode == 4)
    {
      if (sub_mode == 1)
      {
        excercise_text = "Enter to set angle  ";
        speed_text = speed_prefixtext + String(speed) + "           ";
        reps_text = reps_prefixtext + String(reps_total) + "          ";
        if (showvalue_flag)
          angle_text = angle_prefixtext + String(second_angle) + "     ";
        else
          angle_text = angle_prefixtext + "          ";
      }
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

void mode_1()
{
	direction_motor = 0;
	while(1){
    update_angle();
    // We need this at the start so that the lcd screen immediately updates when we start the excercise
    if (direction_motor == 0)
      print_mainscreen(0,1,5);
    else if (direction_motor == 1)
      print_mainscreen(0,1,4);
    
    // if reach top
		// one alternative for motor code is to use the return position code to return hand to bottom
    if(current_arm_angle < second_angle + 3 && current_arm_angle > second_angle -3 && direction_motor == 0){
      reps_current += 1; // increase reps count by 1
      sound_buzzer(); // sound the buzzer
      direction_motor = 1; // change direction
			// show lcd screen with rep + 1, change angle text to moving down?
			print_mainscreen(0,1,4);
    }

    //if reach bottom
    if(current_arm_angle < first_angle + 3 && current_arm_angle > first_angle - 3 && direction_motor == 1){
      sound_buzzer(); // sound the buzzer
      direction_motor = 0; // change direction
			//  change angle text to moving up
			print_mainscreen(0,1,5);
    }

		while(current_arm_angle < second_angle && direction_motor == 0){
			update_angle();
      //run stepper motor forward (direction_motor: 0 is forward)
			move_stepper(direction_motor);

			//press enter to stop function
			if(enter_button_toggle()){
				direction_motor = 2;
				//return motor to starting position
				move_stepper(direction_motor);
			}
		}

		while(current_arm_angle > first_angle && direction_motor == 1){
      update_angle();
			//run stepper motor backward (direction_motor: 1 is forward)
			move_stepper(direction_motor);
			// print_mainscreen(0,1,2);

			//press enter to stop function
			if(enter_button_toggle()){
				direction_motor = 2;
				//return motor to starting position
				move_stepper(direction_motor);
			}
		}

		if(direction_motor == 2){
      print_mainscreen(0,1,3);
			//sound buzzer end
      sound_end_excercise_buzzer();
      delay(3000);
			break;
		}

    if(reps_current == reps_total){
      delay(100);
			print_mainscreen(0,1,3);
      sound_end_excercise_buzzer();
      reps_current = 0;
			delay(3000);
			break;
		}
	}
}



// need to constantly read the new speed_update variable in the menu to show the changes
// to allow up and down button to change speed variable everytime they are pressed
// enter key to lock in the speed (mapped to 100 for stepper)
void mode_2()
{
  while (1) {
    print_mainscreen(1,2,1);
    if(up_button_toggle()){
      if(1<=speed && speed<10){
      speed+=1;
      }
    }
    else if(down_button_toggle()){
      if(2<speed && speed<=10){
      speed-=1;
      }
    }
    else if(enter_button_toggle()){
			//motor_speed is speed variable to use in motor, speed is display text
			motor_speed = map(speed, 1,10,800,1400);
      break;
    }
  }
}

//change reps by +-, enter to save into "rep" variable
void mode_3()
{
  while (1) {
    print_mainscreen(1,3,1);
    if(up_button_toggle()){
      if(0<=reps_total && reps_total<30){
        reps_total+=1;
      }
    }
    else if(down_button_toggle()){
      if(1<reps_total && reps_total<=30){
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
  int exit_flag = 1;
  while(exit_flag)
  {
    while(1)
    {
      print_mainscreen(1,4,1);
      update_angle();

      while(up_button_press())
      {
        // steps per second
        stepper.setSpeed(forward_speed);
        // Step the motor with a constant speed as set by setSpeed():
        stepper.runSpeed();
        update_angle();
		  }

      while(down_button_press())
      {
        //print_mainscreen(1, 4, 1);
        // lcd.setCursor(3,9);
        // lcd.print(current_arm_angle);
        stepper.setSpeed(reverse_speed);
        // Step the motor with a constant speed as set by setSpeed():
        stepper.runSpeed();
        update_angle();
      }

      if (enter_button_toggle())
      {
        first_angle = current_arm_angle;
        print_mainscreen(1,4,1);
        
        //setting stepper's current position as 0
        stepper.setCurrentPosition(0);
        break;
      }
	  }

    while(1)
    {
      print_mainscreen(1,4,2);
      update_angle();

      while(up_button_press())
      {
        stepper.setSpeed(forward_speed);
        // Step the motor with a constant speed as set by setSpeed():
        stepper.runSpeed();
        update_angle();
      }

      while(down_button_press())
      {
        stepper.setSpeed(reverse_speed);
        // Step the motor with a constant speed as set by setSpeed():
        stepper.runSpeed();
        update_angle();
      }

      if (enter_button_toggle())
      {
        second_angle = current_arm_angle;
        print_mainscreen(1,4,2);
        
        if (first_angle >= second_angle)
        {
          print_mainscreen(1,4,3);
          delay(2000);
        }
        else
        {
          exit_flag = 0;
        }
        break;
      }
    }
  }
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

// INPUT: None
// Output: Once function is called, take in a sensor reading and put it in a buffer, and return the average of all readings in the buffer

void update_angle()
{
  accelerometer.update();
  SUM = SUM - READINGS[INDEX];
  VALUE = accelerometer.getValue();
  READINGS[INDEX] = VALUE;           // Add the newest reading to the window
  SUM = SUM + VALUE;                 // Add the newest reading to the sum
  INDEX = (INDEX+1) % WINDOW_SIZE;   // Increment the index, and wrap to 0 if it exceeds the window size

  AVERAGED = SUM / WINDOW_SIZE;      // Divide the sum of the window by the window size for the result
  current_arm_angle = constrain(map(AVERAGED,277,413,0,180),0,180);
  
  //current_arm_angle = AVERAGED; //for calibration
}

void move_stepper(int direction)
{ //forward direction = 0
	if(direction == 0){
		// steps per second
		stepper.setSpeed(motor_speed);
	  // Step the motor with a constant speed as set by setSpeed():
	  stepper.runSpeed();
	}

	// backward direction = 1
	else if(direction == 1){
			// steps per second
			stepper.setSpeed(-motor_speed);
			// Step the motor with a constant speed as set by setSpeed():
			stepper.runSpeed();
	}

	// direction = 2 for reset motor to original position
	else if(direction == 2){
		stepper.moveTo(0);
    while (stepper.distanceToGo() != 0){
			stepper.setSpeed(slow_reverse_speed); //negative cos should be returning to original position (reverse motor)
			stepper.runToPosition();
		}
	}

	// return to original position if break is called

}
