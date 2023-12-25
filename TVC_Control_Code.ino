#include <PWMServo.h> //servo library
#include <SoftwareSerial.h> //wifi communication library

SoftwareSerial hc06(0, 1);  //initializing TX and RX ports 
String input="";

PWMServo Xservo;  // creating servo objects
PWMServo Yservo; 
//Pyro pins
int PY1 = 8;
int PY2 = 7;
//LED pins
int Red1 = 10;
int Red2 = 11;
int Green1 = 12;
//Center angle for x and y servos
int YangleInitial = 67.5;
int XangleInitial = 105;
//variables for keeping track of time
unsigned long previousMillis = 0;
unsigned long startFire1 = 0;
unsigned long startFire2 = 0;
const unsigned long interval = 100;

bool servoRunning = false;
bool Fire_Away_1 = false;
bool Fire_Away_2 = false;

enum State {
  Idle,
  Unpowered_TVC,
  Powered_TVC
};

State CurrentState = Idle;

void setup() {
  //Initialize Serial Monitor
 	Serial.begin(9600);
 	//Initialize Bluetooth Serial Port
 	hc06.begin(9600);
  //Initialize Servo ports
  Yservo.attach(23); //1 cross
  Xservo.attach(18); //2 cross
  //Initialize Pyro ports
  pinMode(PY1, OUTPUT);  
  pinMode(PY2, OUTPUT);
  //Initialize LED ports
  pinMode(Red1, OUTPUT);  
  pinMode(Red2, OUTPUT);
  pinMode(Green1, OUTPUT);

}

//defining common functions

void setLEDsHigh() {
  digitalWrite(Red1, HIGH);
  digitalWrite(Red2, HIGH);  
  digitalWrite(Green1, HIGH);
}

void setLEDsLow() {
  digitalWrite(Red1, LOW);
  digitalWrite(Red2, LOW);  
  digitalWrite(Green1, LOW);
}

void LED321() {
  digitalWrite(Red1, HIGH);
  delay(1000);
  digitalWrite(Red2, HIGH);
  delay(1000);
  digitalWrite(Green1, HIGH);
  delay(1000);
  setLEDsLow();
}

void LEDsOnOff() {
  setLEDsHigh();
  delay(1000);
  setLEDsLow();
  delay(1000);
}

void TVC_Control() {
  // using sin and cosine functions to calculate positions for servo y and x in order to have it spin the motor in a circle

  for (float i = 0; i < 360; i += 0.5) {  // Adding 0.5 degress every milisecond
    float Yangle = YangleInitial + 20 * sin(radians(i));  // Calculate the angle using sine function
    float Xangle = XangleInitial + 20 * cos(radians(i));  // Calculate the angle using cosine function
    Yservo.write(Yangle);  // Move the servo to the calculated angle
    Xservo.write(Xangle);
    delay(1);  // Delay before the next movement
  }
}

void loop() {
  
 	//Read data from HC06
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read data from HC06
    while (hc06.available() > 0) {
      input += (char)hc06.read();
    }

    // State triggers from serial data - 1 blink for idle, 2 for unpowered, 3 for powered
    if (input != "") {
      if (input == "Idle State") {
        CurrentState = Idle;
        hc06.println("State is Idle");
        
        LEDsOnOff();

      } else if (input == "Unpowered TVC") {
        CurrentState = Unpowered_TVC;
        hc06.println("Unpowered TVC Started");

        LEDsOnOff();
        LEDsOnOff();

      } else if (input == "Powered TVC") {
        CurrentState = Powered_TVC;
        hc06.println("Powered TVC Started\n mission is a go.\n Good luck, sir.");

        LEDsOnOff();
        LEDsOnOff();
        LEDsOnOff();
        LED321();

      } else if (input == "Fire Pyro 1") { // Activate Pyro 1
        Fire_Away_1 = true;
        startFire1 = millis(); // Record the start time
        hc06.println("Ignition 1 Started");

      } else if (input == "Fire Pyro 2") { // Activate Pyro 2
        Fire_Away_2 = true;
        startFire2 = millis(); // Record the start time
        hc06.println("Ignition 2 Started");

      } else if (input == "Abort Fire") { // Emergyency Pyro Deactivation
        Fire_Away_1 = Fire_Away_2 = false;
        setLEDsLow(); // Turn off LEDs
        digitalWrite(PY1, LOW); // Turn off both pyros
        digitalWrite(PY2, LOW);

        hc06.println("Ignition Aborted");
        // on the fly adjustments to ensure motor is centered, doesn't work as well as I would like in practice though
      } else if (input == "XServo-") {
        XangleInitial = XangleInitial - 1;
        Xservo.write(XangleInitial);
        hc06.println(XangleInitial);
        delay(10);

      } else if (input == "XServo+") {
        XangleInitial = XangleInitial + 1;
        Xservo.write(XangleInitial);
        hc06.println(XangleInitial);
        delay(10);

      } else if (input == "YServo-") {
        YangleInitial = YangleInitial - 1;
        Yservo.write(YangleInitial);
        hc06.println(YangleInitial);
        delay(10);

      } else if (input == "YServo+") {
        YangleInitial = YangleInitial + 1;
        Yservo.write(YangleInitial);
        hc06.println(YangleInitial);
        delay(10);

      } else if (input == "Center TVC") {
        Yservo.write(YangleInitial);
        Xservo.write(XangleInitial);
        delay(25);
        hc06.println("Servo Centred");

      } else {
        hc06.println("Syntax Error");
      }
      input = "";
    }
  }

  //State system for controlling the mount (for static-fire tests only at the moment), currently rudimentary and needs to have a PID added for actual powered flight

  switch (CurrentState) {
    case Idle:
      break; 

    case Unpowered_TVC:
      TVC_Control();
      break;

    case Powered_TVC:
      //Pyro Channel 1
      if (Fire_Away_1) { //checking if fire is a go
          if (millis() - startFire1 < 600) { //Pyro will be on for 600ms
          setLEDsHigh();  // visual feedback
          digitalWrite(PY1, HIGH);
        } else {
          setLEDsLow(); 
          digitalWrite(PY1, LOW);
        }
      }
      //Pyro Channel 2
      if (Fire_Away_2) {
          if (millis() - startFire2 < 600) {
          setLEDsHigh();
          digitalWrite(PY2, HIGH);
        } else {
          setLEDsLow();
          digitalWrite(PY2, LOW);
        }
      }
      TVC_Control();
      break;

    default: 
      break;
  }
}