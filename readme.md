# Arven Navigation Controls

- IDE: VS Code
- Technology: C
- Hardware: Raspberry Pi Pico W

This repo is for the code to run on the Arven robot and handle navigation, including motor control.

- Receives sensor information from https://github.com/KiaSkretteberg/arven-sensor-controls
- Controls Motors
- Communicate with DWM1001-Dev module
- Brain of Arven (interpolates data received from sensors and determines course of action based on data)
- Sends/receives information with the https://github.com/KiaSkretteberg/arven-app
