/*
  ESP_IOT.h - Library for simplifying OTA and WiFi Station settings.
  Created by Itai Liba, 28 Feb 2016.
  Released into the public domain.
*/
#ifndef ESP_IOT_h
#define ESP_IOT_h

#include "Arduino.h"
#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #define WebServer ESP8266WebServer
  #include <ESP8266mDNS.h>
#endif

#include <DNSServer.h>

const byte DNS_PORT = 53;

class ESP_IOT
{
  public:
    static ESP_IOT& Create();
    
    ESP_IOT();
    
    bool writeFile(String filename, String data);
    bool writeFile(String filename, int data);
    bool readFile(String filename, String& data);
    bool readFile(String filename, int& data);

    bool initIOT(String OTAPassword, String deviceName);
    bool initIOT(String APssid, String APpsk, String OTAPassword, String deviceName);

    bool handle();

    static WebServer server;

  private:
    bool startWebServer();
    void startOTA(String OTAPassword, String deviceName);
    void startSoftAP();

    String m_ssid;
    String m_psk;
    String m_APssid;
    String m_APpsk;
    bool m_wasDisconnected;
    unsigned long m_lastMillis;
    DNSServer dnsServer;
    bool m_noSoftAP;
};

extern ESP_IOT IOT;

#endif