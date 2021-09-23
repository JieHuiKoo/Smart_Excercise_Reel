// ================================================================
// ===                      Libraries                           ===
// ================================================================
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ================================================================
// ===                      Initialise Pins                     ===
// ================================================================
#define INTERRUPT_PIN 12 // use pin 2 on Arduino Uno & most boards for interrupt, INT
#define LED_PIN 13 // (Arduino is 13)
#define SERVO_PIN 9
#define ENTER_BUTTON_PIN 3
#define UP_BUTTON_PIN 1
#define DOWN_BUTTON_PIN 2


// ================================================================
// ===                      MPU Stuffs                          ===
// ================================================================
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

MPU6050 mpu;

#define OUTPUT_READABLE_YAWPITCHROLL

bool blinkState = false;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// packet structure for InvenSense teapot demo
uint8_t teapotPacket[14] = { '$', 0x02, 0,0, 0,0, 0,0, 0,0, 0x00, 0x00, '\r', '\n' };

// Interrupt Detection Routine
volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}

// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

// Declare custom functions
void update_mpu();
void move_servo(int speed);
int angle_to_speed(float angle);
void store_angle(float angle);
void mode_4();
void print_ypr();
String speed_to_text();
bool blink_value_flag(unsigned long interval);


// Servo Stuff
Servo reel_servo;

// Button Stuffs
int new_state;
int old_state = 0;

// Excercise Stuffs
bool excercise_in_progress = false;
float first_angle = -1;
float second_angle = -1;
int reps_total = 10;
int reps_current = 0;
int speed = 0;

// Default Mode
int mode = 4;

// For blinking when in editing mode
unsigned long previousMillis = 0;

// Initiating Variables for Mode 2 (speed_updating)
int speed_update = 5;

// Initiating Variables for Mode 3 (rep_updating)
int rep_update = 10;
int rep = 10;

void setup() {

	// initialize the LCD
	lcd.init();

	// Turn on the blacklight and print a message.
	lcd.backlight();
	lcd.print("VV big test");

    // Initialise servo
    reel_servo.attach(SERVO_PIN);

    // Initialise buttons
    pinMode(ENTER_BUTTON_PIN, INPUT);
    pinMode(UP_BUTTON_PIN, INPUT);
    pinMode(DOWN_BUTTON_PIN, INPUT);

    // Initialise LED
    pinMode(LED_PIN, OUTPUT);

    // ========== MPU Initialisation ==========
    // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    // (115200 chosen because it is required for Teapot Demo output, but it's
    // really up to you depending on your project)
    Serial.begin(9600);
    while (!Serial); // wait for Leonardo enumeration, others continue immediately

    // NOTE: 8MHz or slower host processors, like the Teensy @ 3.3V or Arduino
    // Pro Mini running at 3.3V, cannot handle this baud rate reliably due to
    // the baud timing being too misaligned with processor ticks. You must use
    // 38400 or slower in these cases, or use some kind of external separate
    // crystal solution for the UART timer.

    // initialize device
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);

    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // Calibration Time: generate offsets and calibrate our MPU6050
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);
        mpu.PrintActiveOffsets();
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
        Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
        Serial.println(F(")..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop() {
    // If MPU initialisation failed, do not do anything
    if (!dmpReady)
    {
      lcd.print("MPU Initialisation failed.");
      return;
    }

    if (mode == 1)
    {

    }
    else if (mode == 2)
    {

    }

    else if (mode == 3)
    {

    }

    else if (mode == 4)
    {
      //mode_4();
    }

    update_mpu();
    print_ypr();
	lcd.setCursor(0,0);
    lcd.print(ypr[2]*180/M_PI);
	// lcd.print("First Angle is: "+ (String)first_angle);
    // lcd.println("Second Angle is: "+ (String)second_angle);
    delay(1000);
}

// Input: float roll_angle
// Output: int speed of motor
int angle_to_speed(float roll_angle)
{
  int speed = (int)roll_angle+90;
  if ((speed > 0) && (speed < 180))
  {
    return roll_angle + 90;
  }
  else
    return 90;

}

// Input: int speed of servo
// Output: None, makes the servo move at input speed
void move_servo(int speed)
{
  reel_servo.write(speed);
  Serial.print(speed);
}

// Input: int button_pin of button to be checked
// Output: returns int 1 if button is released from being pressed
int button_toggle(int button_pin)
{
  new_state = digitalRead(button_pin);

  if (old_state == 1 && new_state == 0)
  {
    old_state = new_state;
    return 1;
  }
  else
  {
    old_state = new_state;
    return 0;
  }
}

// Input: None
// Output: None, updates ypr value when called
void update_mpu()
{
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer))
    {
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    }
}

// Input: address of angle variable to be updated
// Output: None, updates angle at input address with the current roll_angle
void store_angle(float *angle)
{
 while(1)
  {
    update_mpu();
    if (button_toggle(ENTER_BUTTON_PIN)){
      *angle = ypr[2] * 180/M_PI;
      Serial.println("Button Pressed: " + String(*angle));
      break;
    }
  }
}

// Input: None
// Output: None, prints ypr on a single line
void print_ypr()
{
  Serial.print("ypr\t");
  Serial.print(ypr[0] * 180/M_PI);
  Serial.print("\t");
  Serial.print(ypr[1] * 180/M_PI);
  Serial.print("\t");
  Serial.println(ypr[2] * 180/M_PI);
}

// Input:
// 1) int editing_mode, Takes in whether editing mode is 1 or 0, to turn on blinking
// 2) int mode, For selecting which mode to blink when in editing mode
// 3) int rep_in_progress, To change output text of Press enter to start/stop excercise
String output_text = ""; //20 char only!!!!!!!
String excercise_text = "";
String start_excercise_text = "Press ENTER to start";
String stop_excercise_text = "Press ENTER to stop excercise";
String speed_text = "";
String speed_prefixtext = "Speed: ";
String reps_text = "";
String reps_prefixtext = "REPS: ";
String angle_text = "";
String angle_prefixtext = "Angle: ";

int showvalue_flag = 1;
void print_mainscreen(bool editing_mode, int mode, bool excercise_in_progress)
{
  if (editing_mode && blink_value_flag(500))
  {
    showvalue_flag = !showvalue_flag;
  }

  if (!editing_mode)
  {
    if (excercise_in_progress)
      excercise_text = stop_excercise_text;
    else
      excercise_text = start_excercise_text;
    speed_text = speed_prefixtext += speed_to_text();
    reps_text = reps_prefixtext += String(reps_total);
    angle_text = angle_prefixtext += String(round(second_angle - first_angle));
  }
  else
  {
    if (mode==2)
    {
      if (showvalue_flag)
        speed_text = speed_prefixtext += speed_to_text();
      else
        speed_text = speed_prefixtext;
    }
    else if (mode == 3)
    {
      if (showvalue_flag)
        reps_text = reps_prefixtext += String(reps_total);
      else
        reps_text = reps_prefixtext;
    }
    else if (mode == 4)
    {
      if (showvalue_flag)
        angle_text = angle_prefixtext += "Enter to edit";
      else
        angle_text = angle_prefixtext;
    }
  }

  output_text = excercise_text + "\n" + speed_text +  "\n" + reps_text +  "\n" + angle_text;
}

// need to constantly read the new speed_update variable in the menu to show the changes
// to allow up and down button to change speed variable everytime they are pressed
// enter key to lock in the speed (mapped to 100 for stepper)

void mode_2()
{
  while (1) {
    if(button_toggle(UP_BUTTON_PIN)){
      if(0<speed_update && speed_update<10){
      speed_update+=1;
      }
    }
    if(button_toggle(DOWN_BUTTON_PIN)){
      if(0<speed_update && speed_update<10){
      speed_update-=1;
      }
    }
    if(button_toggle(ENTER_BUTTON_PIN)){
      speed = map(speed_update, 0,10,0,100);
      break;
    }
  }
}

//change reps by +-, enter to save into "rep" variable

void mode_3()
{
  while (1) {
    if(button_toggle(UP_BUTTON_PIN)){
      if(0<rep_update && rep_update<30){
      rep_update+=1;
      }
    }
    if(button_toggle(DOWN_BUTTON_PIN)){
      if(0<rep_update && rep_update<30){
      rep_update-=1;
      }
    }
    if(button_toggle(ENTER_BUTTON_PIN)){
      rep = rep_update;
      break;
    }
  }
}


void mode_4()
{
  if (first_angle == -1){
    Serial.print("Waiting for button press 1\n");
    store_angle(&first_angle);
  }
  if (second_angle == -1){
    Serial.print("Waiting for button press 2\n");
    store_angle(&second_angle);
  }
}

String speed_to_text()
{
  int speed1 = 5;
  int speed2 = 10;
  int speed3 = 15;
  if (speed==speed1)
    return "LOW";
  else if (speed==speed2)
    return "MED";
  else if (speed==speed3)
    return "HIGH";
  else
    return "";
}

bool blink_value_flag(unsigned long interval)
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
  return true;
  }
  else
    return false;
}