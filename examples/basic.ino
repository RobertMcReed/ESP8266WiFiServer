/*
  ESP8266AutoIOT - Library for initiating a WiFi connection and managing a server.
  Created by Robert Reed, September 25, 2020.
  Released into the public domain.
*/

#include <ArduinoJson.h>
#include <ESP8266AutoIOT.h>

// Instantiate the app

// default AP: "esp8266"
// default PW: "newcouch"
ESP8266AutoIOT app;

// explicitly enable/disable ArduinoOTA (false by default if no pw provided)
// ESP8266AutoIOT app(true);

// Specify access point and password
// ESP8266AutoIOT app("my_ap", "my_pw"); // OTA enabled by default if pw provided

// ESP8266AutoIOT app("my_ap", "my_pw", false); // do it all (set ap/pw and disable OTA)

// You can access the ESP8266WebServer directly via app.server
// You can access the WifiManager directly via app.wifiManager

// You can return a String as a get response
String response = "Welcome to ESP8266!";

// Or you can provide a function that returns a String
String getHtml() {
 return "<h1>Hello, World!</h1>";
}

// Or you can provide a function with no return ("Success" is returned)
void trigger() {
  Serial.println("Function triggered!");
}

// You can access the raw body of a post request
void post(String body) {
 Serial.print("The body is: ");
 Serial.println(body);
}

// and you can return a dynamic response using that body
String triple(String body) {
  Serial.print("The body is: ");
  Serial.println(body);

  int val = body.toInt();
  int res = val * 3;

  Serial.print("The response is: ");
  Serial.println(res);

  return String(res);
}

// and you can even return json
String maths(String body) {
  Serial.print("The body is: ");
  Serial.println(body);

  int val = body.toInt();
  int square = val * val;

  // make sure you've imported ArduinoJson.h
  const size_t capacity = JSON_OBJECT_SIZE(1);
  DynamicJsonDocument doc(capacity);
  
  doc["square"] = square;
  char output[128];
  
  serializeJson(doc, output);

  Serial.print("The response is: ");
  Serial.println(output);

  return String(output);
}

void handleConnect() {
  Serial.println("The board has connected to WiFi!");
}

void handleDisconnect() {
  Serial.println("The board has DISCONNECTED from WiFi!");
}

void handleConfig() {
  Serial.println("The configuration portal has been started!");
}

void setup() {
  // WiFi information is printed, so it's a good idea to start the Serial monitor
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  delay(1000);

  // To change the WiFi your device connects to, reset WiFi Credentials (but don't reset board)
  // app.resetCredentials();

  // to change the WiFi and erase the file system (with custom hostname/pw) (but don't reset board)
  // app.softReset();

  // to change the WiFi, erase the file system, and reset the board (buggy, usually connects back to same WiFi until you physically reset the board again)
  // app.hardReset

  // If you want to allow CORS, indicate so before calling app.begin();
  // app.enableCors(); // "*" All origins accepted

  // Or specify an origins
  // app.enableCors("http://localhost:3001");

  // disable LED indicator if desired (call before app.begin();)
  // app.disableLED();

  // set callbacks to handle various lifecycle events
  
  // called once each time the board (re-)establishes a WiFi connection
  app.setOnConnect(handleConnect);

  // called once each time the board loses WiFi connection
  app.setOnDisconnect(handleDisconnect);
  
  // called once each time the board starts the Config Portal
  app.setOnEnterConfig(handleConfig);

  // Set an HTML response for GET requests to /   
  app.root(response);

  // You can provide a function that returns a response
  app.get("/hi", getHtml, 'text/html'); // set the content type or it will default to application/json

  // Or a function that triggers a side effect (200, "Success" returned)
  app.get("/trigger", trigger);

  // A body is required when handling post requests
  // You can act on the body and return a default response
  app.post("/post", post);

  // Or you can use the body to return a dynamic response
  app.post("/triple", triple);

  // You must register all of your routes before calling app.begin();
  app.begin();
}

void loop() {
    // You must call app.loop(); to keep the server going...
    app.loop();
}
