#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "HTTPSRedirect.h"

// WiFi credentials
const char *aSsid = "Mi11Pro";
const char *aPassword = "123456777";
const char *aGScriptId = "AKfycbzwO0SwMz2u97Ws2aDzNw_8JwUkvdjl76ttPTyBGHXmlDbq8iorExrQ-BXkNQNElPoGIg";

// App scrip set up
const char *ahost = "script.google.com";
const int httpsPort = 443;
String url = String("/macros/s/") + aGScriptId + "/exec";
HTTPSRedirect *client = nullptr;  // HTTPSRedirect object


// Time intervals for loops
unsigned long interval1 = 2000;  // interval for loop 1 (in milliseconds)
unsigned long previousMillis1 = 0;
unsigned long interval2 = 10000;  // interval for loop 2 (in milliseconds)
unsigned long previousMillis2 = 0;

String data = "";  // Data storage variable


void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  //Connect to WiFi
  WiFi.begin(aSsid, aPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  // Initialize HTTPSRedirect object
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(false);
  client->setContentTypeHeader("application/json");
  client->connect(ahost, httpsPort);
}

// Post data to App script only once during 2 seconds interval
void postData() {

  // Check time interval and data availability
  unsigned long currentMillis = millis();
  // Check timing interval and data availability
  if (currentMillis - previousMillis1 >= interval1 && data != "") {
    // Perform HTTPS POST request
    client->POST(url, ahost, data);
    
    // Update the timer
    previousMillis1 = currentMillis;
  }
}

//Get password from App script only once during 10 seconds interval
void getPassword() {

  // Check time interval
  unsigned long currentMillis = millis();

  // Check timing interval
  if (currentMillis - previousMillis2 >= interval2) {
    client->GET(url + "?action=getPassword", ahost);  // Perform HTTPS GET request
    DynamicJsonDocument jsonDocument(1024);           // Parse JSON response
    deserializeJson(jsonDocument, client->getResponseBody());

    // Extract password from JSON response
    const char *password = jsonDocument["password"];
    Serial.println(password);

    // Update the timer
    previousMillis2 = currentMillis;
  }
}


// Read data from Arduino Uno board
//This loop waits until everything will be posted
void loop() {

  // Read incoming data from serial
  String tempData = Serial.readStringUntil('\n');

  // Parse the received JSON data
  DynamicJsonDocument jsonDoc(200);
  DeserializationError error = deserializeJson(jsonDoc, tempData);

  // Check for JSON parsing errors
  if (error) {
  } else {
    data = tempData;  // Store parsed data
  }

  // Call loop functions
  postData();
  getPassword();
}