#include <Arduino.h>
#include <Wire.h>
#include "TeensyThreads.h"

volatile int motorCommand = 0;
volatile int phoneCommand = 0;
int led = 13;

// address of the motor
#define motorAddrW 0x65
#define motorAddrR 0x66

void readBT();
String sendBT(String cmd);
void runServo();
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
  pinMode(led,OUTPUT);
  // register threads
  threads.addThread(setMotor);
  threads.addThread(readBT);

}

void loop() {

}

/* BT CONTROL */

void readBT(){
  while(1){
    // read data from the BT module, if any
    while (Serial2.available()>=3){ // While there is more to be read, keep reading.
        phoneCommand = (int)Serial2.read();
        
    }
    switch (phoneCommand){
      case 48: // stop the car
        motorCommand = 0b000000000;
        break;
      case 49: // forward
        motorCommand = 0b110010001;
        break;
      case 50: // back
        motorCommand = 0b110010010;
        break;
      default: // stop the car otherwise
        motorCommand = 0b000000000;
        break;
    }
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
    int speedPercent = motorCommand & 0b111111100;
    int cmd = motorCommand & 0b000000011;
    uint8_t speed = map(speedPercent,1,100,6,63);
    // shift in the command to form the register
    uint8_t data = (speed << 2) | cmd;
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
  Wire.requestFrom(motorAddrR, 1);
  while(Wire.available()) {
    num = Wire.read();
  }
  Serial.println(num, BIN);
}

