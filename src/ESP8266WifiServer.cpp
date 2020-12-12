/*
  ESP8266WifiServer.cpp - Library for initiating a WiFi connection and managing a server.
  Created by Robert Reed, September 25, 2020.
  Released into the public domain.
*/

#include "ESP8266WifiServer.h"
#include "Arduino.h"
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>

void ESP8266WifiServer::_setup()
{
  _hasBegun = false;

  if (!_port)
  {
    _port = 80;
  }

  if (!_accessPoint)
  {
    _accessPoint = (char*)"esp8266";
  }

  if (!_password)
  {
    _password = (char*)"newcouch";
  }

  _LED_ON = LOW;
  _LED_OFF = HIGH;
  _LED = LED_BUILTIN;
  _ledEnabled = true;

  _corsOrigin = "*";
  _corsEnabled = false;

  WiFiManager wifiManager;
  server = new ESP8266WebServer(_port);
}

ESP8266WifiServer::ESP8266WifiServer()
{
  _setup();
}

ESP8266WifiServer::ESP8266WifiServer(int port)
{
  _port = port;
  _setup();
}

ESP8266WifiServer::ESP8266WifiServer(char* accessPoint, char* password)
{
  _accessPoint = accessPoint;
  _password = password;
  _setup();
}

ESP8266WifiServer::ESP8266WifiServer(int port, char* accessPoint, char* password)
{
  _accessPoint = accessPoint;
  _password = password;
  _port = port;
  _setup();
}

void ESP8266WifiServer::_digitalWrite(int value)
{
   if (_ledEnabled)
   {
     digitalWrite(_LED, value);
   }
}

void ESP8266WifiServer::_sendCorsHeaderIfEnabled()
{
   if (_corsEnabled)
   {
     server->sendHeader("Access-Control-Allow-Origin", _corsOrigin);
   }
}

void ESP8266WifiServer::_handleGetRequestVoidFn(voidCallback fn)
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

void ESP8266WifiServer::_handleGetRequestStr(String response, bool isHtml)
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

void ESP8266WifiServer::_handleGetRequestStrFn(stringCallback fn, bool isHtml)
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

void ESP8266WifiServer::_handlePostRequestVoidFn(voidCallbackStr fn)
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


void ESP8266WifiServer::_handlePostRequestStrFn(stringCallbackStr fn)
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

void ESP8266WifiServer::_handleNotFound()
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

void ESP8266WifiServer::get(String path, stringCallback fn, bool isHtml) {
  server->on(path, std::bind(&ESP8266WifiServer::_handleGetRequestStrFn, this, fn, isHtml));
}

void ESP8266WifiServer::get(String path, stringCallback fn) {
  server->on(path, std::bind(&ESP8266WifiServer::_handleGetRequestStrFn, this, fn, false));
}

void ESP8266WifiServer::get(String path, voidCallback fn) {
  server->on(path, std::bind(&ESP8266WifiServer::_handleGetRequestVoidFn, this, fn));
}

void ESP8266WifiServer::get(String path, String response) {
  server->on(path, std::bind(&ESP8266WifiServer::_handleGetRequestStr, this, response, false));
}

void ESP8266WifiServer::get(String path, String response, bool isHtml) {
  server->on(path, std::bind(&ESP8266WifiServer::_handleGetRequestStr, this, response, isHtml));
}

void ESP8266WifiServer::post(String path, voidCallbackStr fn) {
  server->on(path, std::bind(&ESP8266WifiServer::_handlePostRequestVoidFn, this, fn));
}

void ESP8266WifiServer::post(String path, stringCallbackStr fn) {
  server->on(path, std::bind(&ESP8266WifiServer::_handlePostRequestStrFn, this, fn));
}

void ESP8266WifiServer::root(stringCallback fn) {
  _rootHandled = true;
  get("/", fn, true);
}

void ESP8266WifiServer::root(String response) {
  _rootHandled = true;
  get("/", response, true);
}

void ESP8266WifiServer::_handleDefaultRoot()
{
  root("Success");
}

void ESP8266WifiServer::disableLED()
{
  _ledEnabled = false;
}

void ESP8266WifiServer::enableCors()
{
  _corsEnabled = true;
}

void ESP8266WifiServer::enableCors(String origin)
{
  _corsEnabled = true;
  _corsOrigin = origin;
}

void ESP8266WifiServer::begin()
{
  _hasBegun = true;
  if (_ledEnabled)
  {
    pinMode(_LED, OUTPUT);
  }

  wifiManager.autoConnect(_accessPoint, _password);
  Serial.println("Connected! to WiFi!");

  if (MDNS.begin(_accessPoint))
  {
    Serial.println("MDNS responder started");
  }

  if (!_rootHandled)
  {
    _handleDefaultRoot();
  }

  server->onNotFound(std::bind(&ESP8266WifiServer::_handleNotFound, this));

  server->begin();
  Serial.println("HTTP server started");
  _ledOff();
}

void ESP8266WifiServer::loop()
{
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

    Serial.println("It looks like you forgot to call app.begin(); in setup()");
    Serial.println("WiFi connectivity is disabled!");

    delay(10000);
  }
  else
  {
    _ledOff();
    server->handleClient();
    _ledOff();
  }
}

void ESP8266WifiServer::_ledOn()
{
  _digitalWrite(_LED_ON);
}

void ESP8266WifiServer::_ledOff()
{
  _digitalWrite(_LED_OFF);
}

void ESP8266WifiServer::resetCredentials()
{
  wifiManager.resetSettings();
}
