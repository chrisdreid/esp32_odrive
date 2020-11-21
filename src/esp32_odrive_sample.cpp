/*
 * We are going to use the ESP32 extra serial communication from HardwareSerial
 * ** https://hackaday.com/2017/08/17/secret-serial-port-for-arduinoesp32/
 * ** https://youtu.be/GwShqW39jlE
 * 
 * Much of the code is from the Example file in ODriveArduino Examples: ODriveArduinoTest

 PIN | ESP32 | ODrive
  RX |    16 |  2
  TX |    17 |  1
 GND |   GND | GND

 */

#include <ODriveArduino.h>

// ## I have not tried it yet, but if you need another serial: UART1, redefine pins (see video link above)
#define ESP32_UART2_PIN_TX 17
#define ESP32_UART2_PIN_RX 16

// ODrive uses 115200 baud
#define BAUDRATE 115200

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }


// ODrive object //HardwareSerial Serial1;
ODriveArduino odrive(Serial1);

float vel_limit = 22000.0f;
float current_lim = 11.0f;

void setup() {
  
  // Serial to PC
  Serial.begin(BAUDRATE);
  
  // Serial to the ODrive //UART HWSerial 1 Setup
  // Note: you must also connect GND on ODrive to GND on ESP32!
  Serial1.begin(BAUDRATE, SERIAL_8N1, ESP32_UART2_PIN_TX, ESP32_UART2_PIN_RX);
  // ## You should be able to setup another serial for more motors

  
  while (!Serial) ;  // wait for Arduino Serial 0 Monitor to open
  Serial.println("Serial 0 Ready...");
  while (!Serial1) ; // wait for Arduino Serial 1 
  Serial.println("Serial 1 Ready...");
  // ## You should be able to setup another serial for more motors
  

//  ODriveArduino odrive(Serial1);
// Set current and velocity defaults
  for (int axis = 0; axis < 2; ++axis) {
    Serial1 << "w axis" << axis << ".controller.config.vel_limit " << vel_limit << '\n';
    Serial1 << "w axis" << axis << ".motor.config.current_lim " << current_lim << '\n';
  }

// Some serial out documentation
  Serial.println("Ready!");
  Serial.println("Send the character '0' or '1' to calibrate respective motor (you must do this before you can command movement)");
  Serial.println("Send the character 's' to exectue test move");
  Serial.println("Send the character 'b' to read bus voltage");
  Serial.println("Send the character 'p' to read motor positions in a 10s loop");
  Serial.println("Send the character 'c'(-) or 'C'(+) to raise and lower the current_limit (+- 5)");
  Serial.println("Send the character 'v'(-) or 'V'(+) to raise and lower the velocity_limit (+- 50000)");
  Serial.println("Send the character 'x'(-) or 'X'(+) to switch back and forth to position (+- 5000)");
}

// This is the same as the ODriveArduinoTest
void loop() {

  if (Serial.available()) {
    char c = Serial.read();
    Serial.println((String)" Serial.Read(): " + c  );
    
    // Run calibration sequence
    if (c == '0' || c == '1') {
      int motornum = 0;
      if(c=='1') motornum = 1;
      int requested_state;

      requested_state = ODriveArduino::AXIS_STATE_MOTOR_CALIBRATION;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, false); // don't wait
    }

   // Change Velocity incrementally
    if (c == 'V' || c == 'v'){
      float inc = 50000.0f;
      if(c=='v') inc *= -1;
      vel_limit += inc;
      Serial.println((String)"Velocity Limit: " + vel_limit);
      for (int axis = 0; axis < 2; ++axis) {
        Serial1 << "w axis" << axis << ".controller.config.vel_limit " << vel_limit << '\n';
      }
    }

   // Change Current incrementally
    if (c == 'C' || c == 'c'){
      float inc = 5.0f;
      if(c=='c') inc *= -1;
      current_lim += inc;
       Serial.println((String)"Current Limit: " + current_lim);
      for (int axis = 0; axis < 2; ++axis) {
        Serial1 << "w axis" << axis << ".motor.config.current_lim " << current_lim << '\n';
      }
    }
    
   // Change Positions: Flips position back and forth between + and - value
    if (c == 'X' || c == 'x'){
      int pos = 5000;
      if(c=='x') pos *= -1;
      Serial.println((String)"Position: " + pos);
        odrive.SetPosition(0, -pos);
        odrive.SetPosition(1, pos);
      
    }
    // Sinusoidal test move
    if (c == 's') {
      Serial.println("Executing test move");
      for (float ph = 0.0f; ph < 6.28318530718f; ph += 0.01f) {
        float pos_m0 = 20000.0f * cos(ph);
        float pos_m1 = 20000.0f * sin(ph);
        odrive.SetPosition(0, pos_m0);
        odrive.SetPosition(1, pos_m1);
        delay(5);
      }
    }

    // Read bus voltage
    if (c == 'b') {
      //odrive_serial << "r vbus_voltage\n";
      Serial1 << "r vbus_voltage\n";
      Serial << "Vbus voltage: " << odrive.readFloat() << '\n';
    }

    // print motor positions in a 10s loop
    if (c == 'p') {
      static const unsigned long duration = 10000;
      unsigned long start = millis();
      while(millis() - start < duration) {
        for (int motor = 0; motor < 2; ++motor) {
          //odrive_serial << "r axis" << motor << ".encoder.pos_estimate\n";
          Serial1 << "r axis" << motor << ".encoder.pos_estimate\n";
          Serial << odrive.readFloat() << '\t';
        }
        Serial << '\n';
      }
    }
  }
}