/*
  ESP8266WifiServer.h - Library for initiating a WiFi connection and managing a server.
  Created by Robert Reed, September 25, 2020.
  Released into the public domain.
*/

#ifndef ESP8266WifiServer_h
#define ESP8266WifiServer_h

#include "Arduino.h"
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>

typedef void (*voidCallback) ();
typedef void (*voidCallbackStr) (String);
typedef String (*stringCallback) ();
typedef String (*stringCallbackStr) (String);

class ESP8266WifiServer
{
  WiFiManager wifiManager;
  ESP8266WebServer* server;

  public:
    ESP8266WifiServer();
    ESP8266WifiServer(int port);
    ESP8266WifiServer(char* accessPoint, char* password);
    ESP8266WifiServer(int port, char* accessPoint, char* password);

    void loop();
    void begin();
    
    void disableLED();
    
    void enableCors();
    void enableCors(String corsOrigin);
    
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

  private:
    void _setup();

    void _ledOn();
    void _ledOff();
    void _digitalWrite(int value);

    void _handleNotFound();
    void _handleDefaultRoot();
    
    void _sendCorsHeaderIfEnabled();

    void _handleGetRequestVoidFn(voidCallback fn);
    void _handleGetRequestStr(String response, bool isHtml);
    void _handleGetRequestStrFn(stringCallback fn, bool isHtml);

    void _handlePostRequestVoidFn(voidCallbackStr fn);
    void _handlePostRequestStrFn(stringCallbackStr fn);

    bool _hasBegun;
    bool _ledEnabled;
    bool _corsEnabled;
    bool _rootHandled;

    char* _password;
    char* _accessPoint;
    String _corsOrigin;
    
    int _port;
    int _LED_ON = LOW;
    int _LED_OFF = HIGH;
    int _LED = LED_BUILTIN;
};

#endif
