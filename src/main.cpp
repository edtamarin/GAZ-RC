#include <Arduino.h>
#include <Wire.h>
#include "TeensyThreads.h"
#include <Servo.h>

volatile int motorCommand = 0;
int led = 13;

// servo
Servo steerServo;
int turnAngle = 5;
// set up limits on how far the servo will turn
const int turnLimit = 45;

// address of the motor
#define motorAddrW 0x65
#define motorAddrR 0x66

void readBT();
String sendBT(String cmd);
void turnServo(int angle);
void setMotor();
void clearFault();
void readFault();

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial2.begin(9600);
  // BT module set-up
  sendBT("AT\r\n");
  sendBT("AT+VERSION\r\n");
  sendBT("AT+NAMEGAZ-RC\r\n");
  sendBT("AT+PIN1567\r\n");
  // servo setup
  steerServo.attach(11);
  pinMode(led,OUTPUT);
  // register threads
  threads.addThread(setMotor);
  threads.addThread(readBT);

}

// all the work is done in threads
void loop() {}

/* BT CONTROL */

void readBT(){
  int dirAngle = 0;
  int carSpeed = 0;
  while(1){
    // read data from the BT module, if any
    while (Serial2.available()>0){ // While there is more to be read, keep reading.
        String bluetoothCommand = Serial2.readStringUntil('#');\
        dirAngle = bluetoothCommand.substring(0,3).toInt();
        carSpeed = bluetoothCommand.substring(3,6).toInt();
    }
    // check if the car is moving forwards or backwards
    boolean carForward = (dirAngle >= 0) && (dirAngle < 180);
    // control motors based on command
    if (carSpeed == 0){
      // if there is no speed, stop the car
      motorCommand = 0b000000000;
    }else if (carForward){
      // move the car forward with a given speed
      motorCommand = (carSpeed << 2) | 0b01;
    }else if (!carForward){
      // move the car backwards
      motorCommand = (carSpeed << 2) | 0b10;
    }
    // control steering servo
    turnServo(dirAngle);
    threads.yield();
  }
}

String sendBT(String cmd){
  String command = "";
  // if a command has benn provided, send it to module
  Serial2.print(cmd);
  delay(1000);
  while (Serial2.available()>=4){ // While there is more to be read, keep reading.
    for(int i=0;i<4;i++){
      command += (char)Serial2.read();
    }
  }
  return command;
}

/* SERVO CONTROL */

/**
 * Turn the servo to steer the car
 * Input: angle from the joystick in degrees
 */
void turnServo(int angle){
  // to make the turning more robust to noise, only about a 60 degree angle
  // is used in each quadrant of the joystick. This checks if we are in such
  // a region and so if we need to turn the servo.
  boolean isInTurnRegion = (abs(cos(radians(angle)))<0.965) && (abs(cos(radians(angle)))>0.258);
  if (isInTurnRegion){
    // check if it is a left or right turn
    boolean isLeftTurn = (angle > 90) && (angle < 270);
    // how hard the turn is can be determined by the sine of the joystick angle
    int sinTransform = floor(abs(sin(radians(angle)))*1000);
    // servo is limited by how much it should turn, set up through a constant
    int servoAngle = map(sinTransform,961,275,0,turnLimit);
    if (isLeftTurn){
      steerServo.write(90-servoAngle);
    }else{
      steerServo.write(90+servoAngle);
    }
  }
}

/* MOTOR CONTROL */

/**
 * Set the motor speed and status
 * Speed in percentage
 * Commands: coast (0), fwd (1), rev (2), brake (3)
 * Command is provided as a byte since TeensyThreads allows for only 1 param
 */
void setMotor(){
  while(1){
    /* DRV8830 has the motor votage values in steps
      from 0x06 to 0x3F, so a remap is needed */
    int speedPercent = motorCommand >> 2;
    int cmd = motorCommand & 0b000000011;
    uint8_t speed = map(speedPercent,1,100,6,63);
    // shift in the command to form the register
    uint8_t data = (speed << 2) | cmd;
    readFault();
    clearFault();
    Wire.beginTransmission(motorAddrW);
    Wire.write(byte(0x00));
    Wire.write(data);
    Wire.endTransmission();
    threads.yield();
  }
}

/**
 * Clear the FAULT register
 */
void clearFault(){
  Wire.beginTransmission(motorAddrW);
  Wire.write(byte(0x01));
  Wire.write(byte(0x80));
  Wire.endTransmission();
}

/**
 * Read the FAULT register
 */
void readFault(){
  uint8_t num = 0;
  Wire.beginTransmission(motorAddrW);
  Wire.write(byte(0x01));
  Wire.requestFrom(motorAddrW, 1);
  num = Wire.read();
  Wire.endTransmission();
  // if FAULT bit is set check condition
  if (num & 0x1){
    if (num & 0x2) Serial.println("OVERCURRENT");
    if (num & 0x4) Serial.println("UNDERVOLTAGE");
    if (num & 0x8) Serial.println("OVERTEMPERATURE");
    if (num & 0x8) Serial.println("EXTENDED CURRENT LIMIT");
  }
}

