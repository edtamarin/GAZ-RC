# GAZ-RC
Remote-controlled GAZ-2705 model car. 

## Idea
I have a few toy cars at home. It seems like an interesting summer project to try and convert one of them into a remote-controlled car. 

## Hardware
The BOM table provided with the repository contains a breakdown of all parts used.

Importantly, an HC-06 module was used instead of the Feasycom BT616.

The car is controlled through a Teensy 4.0. I chose this board partly because it has some supprt for multithreading, and I wanted to separate communication and control so that they do not hinder each other.

## Software
The code is developed in PlatformIO using the Arduino framework. Controlling the car is possible by using the Bluetooth Joystick Control app.