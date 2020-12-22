/*
  ESP8266AutoIOT.cpp - Library for initiating a WiFi connection and managing a server.
  Created by Robert Reed, September 25, 2020.
  Released into the public domain.
*/

#include "ESP8266AutoIOT.h"
#include "Arduino.h"
#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Included with core
#include <ESP8266WebServer.h>   // Included with core
#include <WiFiManager.h>        //https://github.com/RobertMcReed/WiFiManager.git#ota
#include <ESP8266mDNS.h>        // Included with core
#include <ArduinoOTA.h>         // Included with core
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson ~v6.x.x
#include <WiFiUdp.h>            // For the below
#include <LittleFS.h>

const unsigned long __CONNECT_TIMEOUT__ = 30; // How long to attempt to connect to saved WiFi before going into AP mode
const unsigned long __AP_TIMEOUT__ = 60; // Wait 60 Seconds in the config portal before trying again the original WiFi creds
// const char* defaultAp = "esp8266";


bool __shouldSaveConfig__ = false;
void (*__onEnterConfig__)() = NULL;

void __handleOnEnterConfig__(WiFiManager *myWiFiManager) {
  Serial.println("[INFO] Starting WiFi Configuration Portal.");
  __onEnterConfig__();
}

void saveConfigCallback () {
  __shouldSaveConfig__ = true;
}

void ESP8266AutoIOT::_readConfig()
{
  //read configuration from FS json
  Serial.println("[INFO] Mounting FS...");

  if (LittleFS.begin()) {
    Serial.println("[INFO] Mounted file system.");
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("[INFO] Reading config file...");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("[INFO] Opened config file:");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        StaticJsonDocument<256> jsonConfig;
        DeserializationError configJsonError = deserializeJson(jsonConfig, buf.get());

        serializeJsonPretty(jsonConfig, Serial);
        Serial.println();
        if (!configJsonError) {
          if (jsonConfig.containsKey("hostname")) {
            strcpy(_accessPoint, jsonConfig["hostname"]);
            Serial.print("[INFO] Setting hostname/access point to: ");
            Serial.println(_accessPoint);
          }

          if (jsonConfig.containsKey("password")) {
            strcpy(_password, jsonConfig["password"]);
            Serial.print("[INFO] Setting password to: ");
            Serial.println(_password);
          }
        } else {
          Serial.print("[ERROR] Failed to load json config: ");
          Serial.println(configJsonError.c_str());
        }
      }
    } else {
      Serial.println("[INFO] /config.json not found. Using default config.");
    }
  } else {
    Serial.println("[ERROR] Failed to mount FS");
  }
  Serial.println();
}

void ESP8266AutoIOT::_writeConfig() {
  StaticJsonDocument<256> configJsonDoc;

  if ((strcmp(_configAccessPoint, _accessPoint) + strcmp(_configPassword, _password)) == 0) {
    Serial.println("[INFO] Config is unchanged. No need to write.");
    return;
  }

  if (_configAccessPoint && _configAccessPoint != _defaultAccessPoint) {
    configJsonDoc["hostname"] = _configAccessPoint;
    strcpy(_accessPoint, _configAccessPoint);
  }

  if (_configPassword && _configPassword != _defaultPassword) {
    configJsonDoc["password"] = _configPassword;
    strcpy(_password, _configPassword);
  }

  Serial.println("[INFO] Saving config to /config.json...");
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("[ERROR] Failed to open config file for writing");
    return;
  }

  serializeJsonPretty(configJsonDoc, Serial);
  Serial.println();

  serializeJson(configJsonDoc, configFile);
  configFile.close();
  __shouldSaveConfig__ = false;
}

void ESP8266AutoIOT::resetConfig() {
  DynamicJsonDocument emptyDoc(0);

  emptyDoc.to<JsonObject>();

  Serial.println("[WARNING] Resetting /config.json...");
  File configFile = LittleFS.open("/config.json", "w");

  if (!configFile) {
    Serial.println("[ERROR] Failed to open config file for writing");
    return;
  }

  serializeJson(emptyDoc, configFile);
  configFile.close();
}

void ESP8266AutoIOT::_setup(bool enableOTA)
{
  _otaEnabled = enableOTA;
  _hasBegun = false;

  if (!_accessPoint)
  {
    strcpy(_accessPoint, (char*)"esp8266");
  }

  if (!_password)
  {
    strcpy(_password, (char*)"newcouch");
  }

  _LED_ON = LOW;
  _LED_OFF = HIGH;
  _LED = LED_BUILTIN;
  _ledEnabled = true;

  _corsOrigin = "*";
  _corsEnabled = false;

  WiFiManager wifiManager;
  server = new ESP8266WebServer(80);
}

ESP8266AutoIOT::ESP8266AutoIOT()
{
  _setup(false);
}

ESP8266AutoIOT::ESP8266AutoIOT(bool enableOTA)
{
  _setup(enableOTA);
}

ESP8266AutoIOT::ESP8266AutoIOT(char* accessPoint, char* password)
{
  strcpy(_accessPoint, accessPoint);
  strcpy(_password, password);

  _setup(true);
}

ESP8266AutoIOT::ESP8266AutoIOT(char* accessPoint, char* password, bool enableOTA)
{
  strcpy(_accessPoint, accessPoint);
  strcpy(_password, password);

  _setup(enableOTA);
}

void ESP8266AutoIOT::_digitalWrite(int value)
{
   if (_ledEnabled)
   {
     digitalWrite(_LED, value);
   }
}

void ESP8266AutoIOT::_sendCorsHeaderIfEnabled()
{
   if (_corsEnabled)
   {
     server->sendHeader("Access-Control-Allow-Origin", _corsOrigin);
   }
}

void ESP8266AutoIOT::_handleGetRequestVoidFn(voidCallback fn)
{
  _ledOn();
  if (server->method() == HTTP_GET) {
    _sendCorsHeaderIfEnabled();
    fn();
    server->send(200, "text/plain", "Success");
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
  _ledOff();
}

void ESP8266AutoIOT::_handleGetRequestStr(String response, bool isHtml)
{
  _ledOn();
  if (server->method() == HTTP_GET) {
    _sendCorsHeaderIfEnabled();
    const char* contentType = isHtml ? "text/html" : "text/plain";
    server->send(200, contentType, response);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
  _ledOff();
}

void ESP8266AutoIOT::_handleGetRequestStrFn(stringCallback fn, bool isHtml)
{
  _ledOn();
  if (server->method() == HTTP_GET) {
    _sendCorsHeaderIfEnabled();
    String response = fn();
    const char* contentType = isHtml ? "text/html" : "application/json";
    server->send(200, contentType, response);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
  _ledOff();
}

void ESP8266AutoIOT::_handlePostRequestVoidFn(voidCallbackStr fn)
{
  _ledOn();
  if (server->method() == HTTP_POST) {
    _sendCorsHeaderIfEnabled();

    if (server->hasArg("plain") == false) {
      server->send(400);
      return;
    }
    server->send(200);
    String body = server->arg("plain");
    fn(body);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
  _ledOff();
}


void ESP8266AutoIOT::_handlePostRequestStrFn(stringCallbackStr fn)
{
  _ledOn();
  if (server->method() == HTTP_POST) {
    _sendCorsHeaderIfEnabled();

    if (server->hasArg("plain") == false) {
      server->send(400);
      return;
    }
    String body = server->arg("plain");
    String response = fn(body);
    server->send(200, "application/json", response);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
  _ledOff();
}

void ESP8266AutoIOT::_handleNotFound()
{
  _ledOn();
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
  _ledOff();
}

String ESP8266AutoIOT::getHostname() {
  return _accessPoint;
}

void ESP8266AutoIOT::get(String path, stringCallback fn, bool isHtml) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handleGetRequestStrFn, this, fn, isHtml));
}

void ESP8266AutoIOT::get(String path, stringCallback fn) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handleGetRequestStrFn, this, fn, false));
}

void ESP8266AutoIOT::get(String path, voidCallback fn) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handleGetRequestVoidFn, this, fn));
}

void ESP8266AutoIOT::get(String path, String response) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handleGetRequestStr, this, response, false));
}

void ESP8266AutoIOT::get(String path, String response, bool isHtml) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handleGetRequestStr, this, response, isHtml));
}

void ESP8266AutoIOT::post(String path, voidCallbackStr fn) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handlePostRequestVoidFn, this, fn));
}

void ESP8266AutoIOT::post(String path, stringCallbackStr fn) {
  server->on(path, std::bind(&ESP8266AutoIOT::_handlePostRequestStrFn, this, fn));
}

void ESP8266AutoIOT::root(stringCallback fn) {
  _rootHandled = true;
  get("/", fn, true);
}

void ESP8266AutoIOT::root(String response) {
  _rootHandled = true;
  get("/", response, true);
}

void ESP8266AutoIOT::_handleDefaultRoot()
{
  root("Success");
}

void ESP8266AutoIOT::disableLED()
{
  _ledEnabled = false;
}

void ESP8266AutoIOT::enableCors()
{
  _corsEnabled = true;
}

void ESP8266AutoIOT::enableCors(String origin)
{
  _corsEnabled = true;
  _corsOrigin = origin;
}

void ESP8266AutoIOT::setOnConnect(void (*onConnect)())
{
  _onConnect = onConnect;
}

void ESP8266AutoIOT::setOnDisconnect(void (*onDisconnect)())
{
  _onDisconnect = onDisconnect;
}

void ESP8266AutoIOT::setOnEnterConfig(void (*onEnterConfig)())
{
  __onEnterConfig__ = onEnterConfig;
}

void ESP8266AutoIOT::begin()
{
  _hasBegun = true;
  if (_ledEnabled)
  {
    pinMode(_LED, OUTPUT);
  }

  strcpy(_defaultAccessPoint, _accessPoint);
  strcpy(_defaultPassword, _password);
  _readConfig(); // update _accessPoint from stored JSON

  // I've been told this line is a good idea
  WiFi.mode(WIFI_STA);
  
  // Set hostname from settings/default
  #ifdef ESP32
    WiFi.setHostname(_accessPoint);
  #else
    WiFi.hostname(_accessPoint);
  #endif

  if (__onEnterConfig__ != NULL) {
    wifiManager.setAPCallback(__handleOnEnterConfig__);
  }

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConnectTimeout(__CONNECT_TIMEOUT__);
  wifiManager.setTimeout(__AP_TIMEOUT__);
  wifiManager.setCountry("US");

  // WiFiManager custom config
  WiFiManagerParameter custom_hostname("hostname", "AP/Hostname", _accessPoint, 24);
  wifiManager.addParameter(&custom_hostname);
  WiFiManagerParameter custom_password("password", "Set OTA Password", _password, 24);
  wifiManager.addParameter(&custom_password);

  _lastWiFiStatus = false;
  if (!wifiManager.autoConnect(_accessPoint, _password)) {
    // If we've hit the config portal timeout, then retstart
    
    Serial.println("[ERROR] Failed to connect and hit timeout, restarting after 10 seconds...");
    delay(10000);
    ESP.restart();

    // Not sure if this line is necessary
    delay(5000);
  }

  // Update parameters from the new values set in the portal
  if (__shouldSaveConfig__) {
    strcpy(_configAccessPoint, custom_hostname.getValue());
    strcpy(_configPassword, custom_password.getValue());
    _writeConfig();
  }

  Serial.println("[SUCCESS] Connected to WiFi!");

  if (MDNS.begin(_accessPoint))
  {
    Serial.println("[INFO] MDNS responder started");
  }

  if (!_rootHandled)
  {
    _handleDefaultRoot();
  }

  server->onNotFound(std::bind(&ESP8266AutoIOT::_handleNotFound, this));

  server->begin();
  Serial.println("[INFO] HTTP server started!");

  if (_otaEnabled) {
    ArduinoOTA.setPassword(_password);
    ArduinoOTA.begin();
    Serial.println("[INFO] ArduinoOTA enabled!");
  } else {
    Serial.println("[INFO] ArduinoOTA disabled.");
  }
  _ledOff();
}

void ESP8266AutoIOT::loop()
{
  if (_reboot_flagged_at && (millis() - _reboot_flagged_at > 5000))
  {
      ESP.restart();
      delay(5000);
  }

  if (!_hasBegun)
  {
    if (_ledEnabled)
    {
      _ledOn();
      delay(500);
      _ledOff();
      delay(500);
      _ledOn();
      delay(500);
      _ledOff();
    }

    Serial.println("[WARNING] It looks like you forgot to call app.begin(); in setup()...");
    Serial.println("WiFi connectivity is disabled!");

    delay(10000);
    return;
  }
  else
  {
    bool isConnected = WiFi.status() == WL_CONNECTED;
    
    if (isConnected != _lastWiFiStatus) {
      Serial.println("[INFO] WiFi Connectivity change.");

      _lastWiFiStatus = isConnected;
      if (isConnected)
      {
        Serial.println("[INFO] Device is now connected to WiFi.");
        if (_onConnect != NULL) {
          _onConnect();
        }
      } else
      {
        Serial.println("[ERROR] Device has lost it's connection to WiFi.");
        if (_onDisconnect != NULL) {
          _onDisconnect();
        }
      }
    }

    _ledOff();
    server->handleClient();
    _ledOff();
    if (_otaEnabled)
    {
      ArduinoOTA.handle();
    }
  }
}

void ESP8266AutoIOT::_ledOn()
{
  _digitalWrite(_LED_ON);
}

void ESP8266AutoIOT::_ledOff()
{
  _digitalWrite(_LED_OFF);
}

// reset wifi creds (don't reboot)
void ESP8266AutoIOT::resetWiFiCredentials()
{
  resetWiFiCredentials(false);
}

// reset wifi creds and maybe reboot
void ESP8266AutoIOT::resetWiFiCredentials(bool resetEsp)
{
  Serial.println("[WARNING] Resetting WiFi credentials!");
  wifiManager.disconnect();
  delay(500);
  wifiManager.resetSettings();
  delay(500);
  if (resetEsp) 
  {
    _flagReboot();
  }
}

// reset wifi creds and reset ap/pw (don't reboot)
void ESP8266AutoIOT::resetAllSettings() {
  resetWiFiCredentials(false);
}

// reset wifi creds and reset ap/pw and maybe reboot
void ESP8266AutoIOT::resetAllSettings(bool resetEsp) {
  resetConfig();
  delay(1000);
  resetWiFiCredentials(resetEsp);
}

void ESP8266AutoIOT::_flagReboot() {
  _reboot_flagged_at = millis();
  Serial.println("Rebooting in 5 seconds...");
}
