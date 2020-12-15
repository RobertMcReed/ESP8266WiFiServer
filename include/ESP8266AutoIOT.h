/*
  ESP8266AutoIOT.h - Library for initiating a WiFi connection and managing a server.
  Created by Robert Reed, September 25, 2020.
  Released into the public domain.
*/

#ifndef ESP8266AutoIOT_h
#define ESP8266AutoIOT_h

#include "Arduino.h"
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson ~v6.x.x
#include <WiFiUdp.h>             // For the below
#include <LittleFS.h>

typedef void (*voidCallback) ();
typedef void (*voidCallbackStr) (String);
typedef String (*stringCallback) ();
typedef String (*stringCallbackStr) (String);

class ESP8266AutoIOT
{
  public:
    ESP8266AutoIOT();
    ESP8266AutoIOT(bool enableOTA);
    ESP8266AutoIOT(char* accessPoint, char* password);
    ESP8266AutoIOT(char* accessPoint, char* password, bool enableOTA);

    ESP8266WebServer* server;
    WiFiManager wifiManager;

    void loop();
    void begin();
    
    void disableLED();
    
    void enableCors();
    void enableCors(String corsOrigin);
    
    void softReset();
    void hardReset();
    void resetCredentials();
    
    void root(String response);
    void root(stringCallback fn);
    
    void get(String path, String response);
    void get(String path, voidCallback fn);
    void get(String path, stringCallback fn);
    void get(String path, String response, bool isHtml);
    void get(String path, stringCallback fn, bool isHtml);
    
    void post(String path, voidCallbackStr fn);
    void post(String path, stringCallbackStr fn);

    void setOnConnect(voidCallback);
    void setOnDisconnect(voidCallback);
    void setOnEnterConfig(voidCallback);

  private:
    void _readConfig();
    void _writeConfig();
    void _setup(bool enableOTA);

    void _ledOn();
    void _ledOff();
    void _enableOTA();
    void _disableOTA();
    void _digitalWrite(int value);

    void _handleNotFound();
    void _handleDefaultRoot();
    
    void _sendCorsHeaderIfEnabled();

    void _handleGetRequestVoidFn(voidCallback fn);
    void _handleGetRequestStr(String response, bool isHtml);
    void _handleGetRequestStrFn(stringCallback fn, bool isHtml);

    void _handlePostRequestVoidFn(voidCallbackStr fn);
    void _handlePostRequestStrFn(stringCallbackStr fn);
    
    void (*_onConnect)() = NULL;
    void (*_onDisconnect)() = NULL;

    bool _hasBegun;
    bool _ledEnabled;
    bool _otaEnabled;
    bool _corsEnabled;
    bool _rootHandled;
    bool _lastWiFiStatus;

    char _password[40];
    char _accessPoint[40];
    char _configPassword[40];
    char _defaultPassword[40];
    char _configAccessPoint[40];
    char _defaultAccessPoint[40];

    String _corsOrigin;
    
    int _LED_ON = LOW;
    int _LED_OFF = HIGH;
    int _LED = LED_BUILTIN;
};

#endif
