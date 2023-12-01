#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ArduinoJson.h>

#define buzzer 5           // Pin for the buzzer
#define motionSensor 4     // Pin for motion sensor
#define trigPin 3         // Pin for ultrasonic sensor's trigger
#define echoPin 2          // Pin for ultrasonic sensor's echo
#define redLED 15          //Pin for red LED
#define blueLED 16        //Pin for blue LED
#define button 17          //Pin for button
#define Password_Length 8  // Length of the password

char Data[Password_Length];                 // Array to store user input for the password
char Master[Password_Length] = "123A456B";  // Array to store the correct password
byte data_count = 0;                        // Counter to keep track of the number of characters entered

const byte ROWS = 4;  // Number of rows in the keypad
const byte COLS = 4;  // Number of columns in the keypad

char hexaKeys[ROWS][COLS] = {  // Keypad layout
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 13, 12, 11, 10 };  // Row pins for the keypad
byte colPins[COLS] = { 9, 8, 7, 6 };      // Column pins for the keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);  // Keypad object
LiquidCrystal_I2C lcd(0x27, 16, 2);                                                // LCD display object

char customKey;                  // Variable to store the current key pressed on the keypad
bool isPasswordCorrect = false;  // Indicate whether the entered password is correct or not
float duration, distance;        // Variables for ultrasonic sensor measurements
int buttonPressed = 0;           // Variable to store the state of a button

// Flame sensor range
const int flameSensorMin = 0;
const int flameSensorMax = 1024;

unsigned long interval1 = 100;      // interval for loop 1 (in milliseconds)
unsigned long previousMillis1 = 0;  // Variable to store the previous millis for data logging

void setup() {
  isPasswordCorrect = false;  // Initialize the password

  Serial.begin(115200);  // Initialize serial communication

  lcd.init();       // Initialize the LCD
  lcd.backlight();  // Turn on the backlight

  // Set pin modes
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motionSensor, INPUT);
  pinMode(button, INPUT);
}

void loop1(int motionValue, int distance_int, int range) {

  // SEND DATA TO ESP8266
  StaticJsonDocument<200> jsonDoc;  // JSON document for sensor data

  // Populate JSON document with sensor values
  jsonDoc["motion"] = motionValue == 1 ? "Detected" : "Nothing detected";
  jsonDoc["distance"] = distance_int;
  jsonDoc["flame"] = range == 0 ? "Detected" : "Nothing detected";

  String jsonString;                   // String to store JSON data
  serializeJson(jsonDoc, jsonString);  // Serialize JSON document to a string
  Serial.println(jsonString);          // Print JSON string to serial
}

void loop() {
  // Check if there is data available on the serial port
  if (Serial.available() > 0) { // checking is there any data
    String temp = ""; //store temporary serial data to the string "temp"
    temp = Serial.readString(); //read serial data
    temp.replace("\n", ""); //remove unnesesery characters
    temp.replace("\r", ""); //remove unnesesery characters
    char tempMaster[Password_Length]; //recieved data changing into to char array
    for (int i = 0; i < temp.length(); i++) { 
      tempMaster[i] = toupper(temp[i]);  // Convert characters to uppercase
    }

    // Update master password if it's different from the current one
    if (Master != tempMaster && strlen(tempMaster) == Password_Length) {
      for (int i = 0; i < strlen(tempMaster); i++) {
        Master[i] = tempMaster[i];
      }
      isPasswordCorrect = false;
    }
  }

  // Ultrasonic sensor: generate 10-microsecond pulse to TRIG pin
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);  // Measure duration of pulse from ECHO pin
  distance = 0.017 * duration;        // Convert duration to distance in centimeters
  int distance_int = int(distance);   // Convert distance to an integer (remove decimal places)

  int motionValue = digitalRead(motionSensor);  // Read motion detection from the sensor
  buttonPressed = digitalRead(button);

  // Check if the button is pressed, reset password status
  if (buttonPressed == HIGH) {
    isPasswordCorrect = false;
    digitalWrite(blueLED, LOW);
  }

  // Activate buzzer and red LED if motion is detected or distance is less than 50 cm, and handle password status password
  if (motionValue == HIGH || distance_int < 50) {
    if (isPasswordCorrect == false) {
      digitalWrite(buzzer, HIGH);
      digitalWrite(redLED, HIGH);
      digitalWrite(blueLED, LOW);

    } else {
      digitalWrite(buzzer, LOW);
      digitalWrite(blueLED, HIGH);
      digitalWrite(redLED, LOW);
    }
  }

  // Flame sensor operation
  int sensorReading = analogRead(A0);                                    // Flame detection using analog sensor
  int range = map(sensorReading, flameSensorMin, flameSensorMax, 0, 3);  // Map the sensor range:

  // Check the flame detection range
  switch (range) {
    case 0:  // A fire between 1-5 feet away.
      lcd.setCursor(0, 0);
      digitalWrite(buzzer, HIGH);
      digitalWrite(blueLED, LOW);
      digitalWrite(redLED, HIGH);
      // Serial.println("Flame Detected");
      break;
  }

  // Display "Enter Password" on the LCD
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");

  // Get key input from the keypad
  customKey = customKeypad.getKey();
  if (customKey) {
    Data[data_count] = customKey;  // Store the entered key in the data array
    lcd.setCursor(data_count, 1);
    lcd.print(Data[data_count]);
    data_count++;
  }

  // Check if the password has been fully entered
  if (data_count == Password_Length) {
    lcd.clear();

    // Check if the entered password is correct
    bool isInputCorrect = false;
    for (int i = 0; i < Password_Length; i++) {
      if (Data[i] == Master[i]) {
        isInputCorrect = true;
      } else {
        isInputCorrect = false;
        break;
      }
    }

    // Display result based on password correctness
    if (isInputCorrect) {
      isPasswordCorrect = true;
      lcd.print("Correct!");
      digitalWrite(buzzer, LOW);
      digitalWrite(blueLED, HIGH);
      digitalWrite(redLED, LOW);
      delay(5000);
    } else {
      lcd.print("Incorrect!");
      delay(1000);
    }

    // Clear entered password data
    lcd.clear();
    clearData();
  }

  // Call loop1 function at a specified interval
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval1) {
    loop1(motionValue, distance_int, range);
    previousMillis1 = currentMillis;
  }
}

// Function to clear entered password data
void clearData() {
  while (data_count != 0) {
    Data[data_count--] = 0;  // Clear the data array
  }
  return;
}