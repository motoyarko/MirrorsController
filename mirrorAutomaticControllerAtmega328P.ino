// /!\DO NOT RUN IN THE SAME TIME/!\
// motorLeftUP and motorRightDOWN
// motorLeftDOWN and motorRightUP
// otherwise driver will be destroyed!
//-----------------------------------
//you can change delays for mirrows by Serial port
//Serial port speed = 115200
//-------SERIAL PORT COMMANDS:-------
// left_motor_up=value
// left_motor_down=value
// right_motor_up=value
// right_motor_down=value
// WRITE
//-------values range:---------------
// 32767 >= value > 0
//-------responces:------------------
//DONE - operation is completed successfully
//ERROR - operation isn't completed successfully
//-----------------------------------


#include <EEPROM.h>
#include "timer-api.h"

//define pins
#define reverseON 4
#define leftMotorCommon 12
#define rightMotorCommon 8
#define leftMotor 10
#define rightMotor 6
#define rightMotorEnable 9
#define leftMotorEnable 11

//define eeprom addresses for run delays of motors
#define left_motor_up_address 0
#define left_motor_down_address 2
#define right_motor_up_address 4
#define right_motor_down_address 6

//variables
int left_motor_up;
int left_motor_down;
int right_motor_up;
int right_motor_down;
//Need to add to EEPROM delay timer between motions UP and Down
int motor_pause = 500;
int reverse_on_delay = 1500;
int reverse_off_delay = 3000;

void setup() {
  //configure pins
  pinMode(reverseON, INPUT);
  pinMode(leftMotorCommon, OUTPUT);
  pinMode(rightMotorCommon, OUTPUT);
  pinMode(leftMotor, OUTPUT);
  pinMode(rightMotor, OUTPUT);
  pinMode(rightMotorEnable, OUTPUT);
  pinMode(leftMotorEnable, OUTPUT);

  motorLeft(0); //disable let motor
  motorRight(0);//disable right motor

  //com port initialization
  Serial.begin(115200);
  Serial.println("Pins are initialized");

  //read motors run delays from eeprom
  byte lowByte;
  byte highByte;

  lowByte = EEPROM.read(left_motor_up_address);
  highByte = EEPROM.read(left_motor_up_address + 1);
  left_motor_up = ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);

  lowByte = EEPROM.read(left_motor_down_address);
  highByte = EEPROM.read(left_motor_down_address + 1);
  left_motor_down = ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);

  lowByte = EEPROM.read(right_motor_up_address);
  highByte = EEPROM.read(right_motor_up_address + 1);
  right_motor_up = ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);

  lowByte = EEPROM.read(right_motor_down_address);
  highByte = EEPROM.read(right_motor_down_address + 1);
  right_motor_down = ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
  
  //print eeprom data to serial
  Serial.print("left_motor_up = ");
  Serial.println(left_motor_up);
  Serial.print("left_motor_down = ");
  Serial.println(left_motor_down);
  Serial.print("right_motor_up = ");
  Serial.println(right_motor_up);
  Serial.print("right_motor_down = ");
  Serial.println(right_motor_down);

  timer_init_ISR_100Hz(TIMER_DEFAULT);
}

void loop() {

// work with serial. receive delays data, save changes in eeprom    
  serialEEPROM();
}

void timer_handle_interrupts(int timer) {
  static boolean reverseONdelayFinished = false;
  static int reverseONdelayTimer = 0;
  static boolean reverseOFFdelayFinished = false;
  static int reverseOFFdelayTimer = 0;

  static boolean leftMotorRunDown;
  static int leftMotorRunDownTime;
  static boolean leftMotorRunDownCompleted;
  
  static boolean leftMotorRunUp;
  static int leftMotorRunUpTime;
  static boolean leftMotorRunUpCompleted = true;

  static boolean rightMotorRunDown;
  static int rightMotorRunDownTime;
  static boolean rightMotorRunDownCompleted;
  
  static boolean rightMotorRunUp;
  static int rightMotorRunUpTime;
  static boolean rightMotorRunUpCompleted = true;
  
//  static int leftMotorPause;

  if (digitalRead(reverseON) == HIGH){
//if left motor UP motion isn't completed, need to finish it
      if (leftMotorRunUp == true){
        if (leftMotorRunUpTime < left_motor_up){
          leftMotorRunUpTime += 10;
        }
        else{
          leftMotorRunUpTime = 0;
          motorLeft(0);
          leftMotorRunUp = false;
          leftMotorRunUpCompleted = true;
          leftMotorRunDownCompleted = false;
        }
      }
//if right motor UP motion isn't completed, need to finish it      
      if (rightMotorRunUp == true){
        if (rightMotorRunUpTime < right_motor_up){
          rightMotorRunUpTime += 10;
        }
        else{
          rightMotorRunUpTime = 0;
          motorRight(0);
          rightMotorRunUp = false;
          rightMotorRunUpCompleted = true;
          rightMotorRunDownCompleted = false;
        }
      }      
//End of checking motor UP motion
//Start checking for motor Down motion    
    if (reverseONdelayFinished == false){  // delay before left motor down
      if (reverseONdelayTimer >= reverse_on_delay){
        reverseONdelayFinished = true;
        reverseOFFdelayFinished = false;
        reverseONdelayTimer = 0;
      }
    else reverseONdelayTimer += 10;
    }
    if (reverseONdelayFinished == true){
      if (leftMotorRunDown == true){ // is left motor runing now?
        if (leftMotorRunDownTime < left_motor_down){ //if yes and left mirir isn't fully moved down
          leftMotorRunDownTime += 10; // increase timer
        }
        else{ // if left mirror is fully moved down 
          leftMotorRunDownTime = 0; //reset timer
          motorLeft(0); //stop left motor
          leftMotorRunDown = false; // set motor run down flag to false
          leftMotorRunDownCompleted = true; // set motor is fully down flag to true
          leftMotorRunUpCompleted = false; // set motor is fully UP flag to false. it needs for enabling motor up function when reverse will be OFF
        }
      }
      if (leftMotorRunDown == false) { // first start of motor down 
        if (leftMotorRunDownCompleted == false){ //just to check if we have permissions to start it. this flag is configurable in the motor up section
          motorLeft(2);
          leftMotorRunDown = true;
        }
      }
// same as before for right motor
      if (rightMotorRunDown == true){ // is right motor runing now?
        if (rightMotorRunDownTime < right_motor_down){ //if yes and right mirir isn't fully moved down
          rightMotorRunDownTime += 10; // increase timer
        }
        else{ // if right mirror is fully moved down 
          rightMotorRunDownTime = 0; //reset timer
          motorRight(0); //stop right motor
          rightMotorRunDown = false; // set motor run down flag to false
          rightMotorRunDownCompleted = true; // set motor is fully down flag to true
          rightMotorRunUpCompleted = false; // set motor is fully UP flag to false. it needs for enabling motor up function when reverse will be OFF
        }
      }
      if (rightMotorRunDown == false) { // first start of motor down 
        if (rightMotorRunDownCompleted == false){ //just to check if we have permissions to start it. this flag is configurable in the motor up section
          motorRight(2);
          rightMotorRunDown = true;
        }
      }      
    }
  }

  if (digitalRead(reverseON) == LOW) {
//chevk i left motor is running down    
      if (leftMotorRunDown == true){
        if (leftMotorRunDownTime < left_motor_down){
          leftMotorRunDownTime += 10;
        }
        else{
          leftMotorRunDownTime = 0;
          motorLeft(0);
          leftMotorRunDown = false;
          leftMotorRunDownCompleted = true;
          leftMotorRunUpCompleted = false;
        }
      }
//check if right motor running down
      if (rightMotorRunDown == true){
        if (rightMotorRunDownTime < right_motor_down){
          rightMotorRunDownTime += 10;
        }
        else{
          rightMotorRunDownTime = 0;
          motorRight(0);
          rightMotorRunDown = false;
          rightMotorRunDownCompleted = true;
          rightMotorRunUpCompleted = false;
        }
      }      
//check delay of reverse off          
     if (reverseOFFdelayFinished == false) {
      if (reverseOFFdelayTimer >= reverse_off_delay){
        reverseOFFdelayFinished = true;
        reverseONdelayFinished = false;
        reverseOFFdelayTimer = 0;
      }
    else reverseOFFdelayTimer += 10;
    }
    if (reverseOFFdelayFinished == true){
//left motor checks      
      if (leftMotorRunUp == true){
        if (leftMotorRunUpTime < left_motor_up){
          leftMotorRunUpTime += 10;
        }
        else{
          leftMotorRunUpTime = 0;
          motorLeft(0);
          leftMotorRunUp = false;
          leftMotorRunUpCompleted = true;
          leftMotorRunDownCompleted = false;
        }
      }
      if (leftMotorRunUp == false) {
        if (leftMotorRunUpCompleted == false){
          motorLeft(1);
          leftMotorRunUp = true;
        }
      }
// right motor checks
      if (rightMotorRunUp == true){
        if (rightMotorRunUpTime < right_motor_up){
          rightMotorRunUpTime += 10;
        }
        else{
          rightMotorRunUpTime = 0;
          motorRight(0);
          rightMotorRunUp = false;
          rightMotorRunUpCompleted = true;
          rightMotorRunDownCompleted = false;
        }
      }
      if (rightMotorRunUp == false) {
        if (rightMotorRunUpCompleted == false){
          motorRight(1);
          rightMotorRunUp = true;
        }
      }      
    }
  }     
}

void motorLeft(int work) { //0 - STOP, 1 - UP, 2 - DOWN
  if (work == 0){
    digitalWrite(leftMotorEnable, LOW);    
    digitalWrite(leftMotor, LOW);
    digitalWrite(leftMotorCommon, LOW);
  }
  if (work == 1) {
    digitalWrite(leftMotor, HIGH);
    digitalWrite(leftMotorCommon, LOW);
    digitalWrite(leftMotorEnable, HIGH);
  }
  if (work == 2) {
    digitalWrite(leftMotor, LOW);
    digitalWrite(leftMotorCommon, HIGH);
    digitalWrite(leftMotorEnable, HIGH);
  }
}

void motorRight(int work) { //0 - STOP, 1 - UP, 2 - DOWN
  if (work == 0) {
    digitalWrite(rightMotorEnable, LOW);    
    digitalWrite(rightMotor, LOW);
    digitalWrite(rightMotorCommon, LOW);
  }
  if (work == 1) {
    digitalWrite(rightMotor, HIGH);
    digitalWrite(rightMotorCommon, LOW);
    digitalWrite(rightMotorEnable, HIGH);
  }
  if (work == 2) {
    digitalWrite(rightMotor, LOW);
    digitalWrite(rightMotorCommon, HIGH);
    digitalWrite(rightMotorEnable, HIGH);    
  }
}

void serialEEPROM() {
  if (Serial.available() > 0 ) {
    String str = Serial.readString();
    if(str.indexOf("left_motor_up=", 0) > -1){
      String str1 = "left_motor_up=";
      //get value for left_motor_up=
      String value = str.substring(str1.length());
      if ((32767 >= value.toInt()) && (value.toInt() > 0)){ //if value can be integer
        left_motor_up = value.toInt();
        Serial.println("DONE");
      }else Serial.println("ERROR");
    }
    else if(str.indexOf("left_motor_down=", 0) > -1){
            String str1 = "left_motor_down=";
            //get value for left_motor_down=
            String value = str.substring(str1.length());
            if ((32767 >= value.toInt()) && (value.toInt() > 0)){ //if value can be integer
              left_motor_down = value.toInt();
              Serial.println("DONE");
            }else Serial.println("ERROR");
          }
    else if(str.indexOf("right_motor_up=") > -1){
            String str1 = "right_motor_up=";
            //get value for right_motor_up=
            String value = str.substring(str1.length());
            if ((32767 >= value.toInt()) && (value.toInt() > 0)){ //if value can be integer
              right_motor_up = value.toInt();
              Serial.println("DONE");
            }else Serial.println("ERROR");
          }
    else if(str.indexOf("right_motor_down=") > -1){
            String str1 = "right_motor_down=";
            //get value for right_motor_up=
            String value = str.substring(str1.length());
            if ((32767 >= value.toInt()) && (value.toInt() > 0)){ //if value can be integer
              right_motor_down = value.toInt();
              Serial.println("DONE");
            }else Serial.println("ERROR");
          }
    else if(str.substring(0) == "WRITE"){
            //write data to EEPROM
            byte hi;
            byte low;
            hi = highByte(left_motor_up);
            low = lowByte(left_motor_up);
            EEPROM.update(left_motor_up_address, low);
            EEPROM.update(left_motor_up_address + 1, hi);
            hi = highByte(left_motor_down);
            low = lowByte(left_motor_down);            
            EEPROM.update(left_motor_down_address, low);
            EEPROM.update(left_motor_down_address + 1, hi);
            hi = highByte(right_motor_up);
            low = lowByte(right_motor_up);            
            EEPROM.update(right_motor_up_address, low);
            EEPROM.update(right_motor_up_address + 1, hi);
            hi = highByte(right_motor_down);
            low = lowByte(right_motor_down);            
            EEPROM.update(right_motor_down_address, low);
            EEPROM.update(right_motor_down_address + 1, hi);
            Serial.println("DONE");
          }
    else Serial.println("ERROR");
  }
}

