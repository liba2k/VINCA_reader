#include "ESP_IOT.h"
#include <ArduinoOTA.h>
#include <FS.h>
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

ESP_IOT IOT;

WebServer ESP_IOT::server(80);

ESP_IOT::ESP_IOT(): m_ssid(""), m_psk(""), m_APssid("ESP_IOT"), m_APpsk("password"), m_wasDisconnected(false), m_lastMillis(0), dnsServer(), m_noSoftAP(false)
{
}

bool ESP_IOT::writeFile(String filename, String data)
{
  File configFile = LittleFS.open(filename, "w");
  if (!configFile)
  {
    Serial.println("Failed to open " + filename + " for writing");
    return false;
  }
  configFile.println(data);
  configFile.close();
  return true;
}
bool ESP_IOT::writeFile(String filename, int data)
{
  return writeFile(filename, String(data));
  
}
bool ESP_IOT::readFile(String filename, String& data)
{
  File configFile = LittleFS.open(filename, "r");
  if (!configFile)
  {
    Serial.println("Failed to open " + filename + " for reading");
    return false;
  }
  data = configFile.readString();
  data.trim();
  configFile.close();
  return true;
}
bool ESP_IOT::readFile(String filename, int& data)
{
  File configFile = LittleFS.open(filename, "r");
  if (!configFile)
  {
    Serial.println("Failed to open " + filename + " for reading");
    return false;
  }
  data = configFile.readString().toInt();
  configFile.close();
  return true;
}

bool ESP_IOT::startWebServer()
{
  server.on("/wifi", [](){
    String s = "<html><body>";
    s += "<form action=\"setwifi\">Network name:<br><input type=\"text\" name=\"ssid\" value=\"\"><br>Password:<br><input type=\"text\" name=\"psk\" value=\"\"><br><br><input type=\"submit\" value=\"Submit\"></form>";
    s += "</body></html>";
    server.send(200, "text/html", s);
  });

  server.on("/setwifi", [](){
    if (server.arg("ssid") == "" || server.arg("psk") == "")
    {
      server.send(200, "text/plain", "Missing ssid or psk");
    }
    else
    {
      if (IOT.writeFile("/ssid",server.arg("ssid")) && IOT.writeFile("/psk", server.arg("psk")))
        server.send(200, "text/html", "<html><body><h1>Configuration saved</h1><form action=\"reboot\"><input type=\"submit\" value=\"Reboot\"></form> </body></html>");
      else
        server.send(200, "text/plain", "Configuration save failed");
    }
  });
  server.on("/wifi_info", [](){
    String ssid;

    if (IOT.readFile("/ssid",ssid))
      server.send(200, "text/plain", "ssid="+ ssid);
    else
      server.send(200, "text/plain", "Couldn't load config");
    
  });
  server.on("/reboot", [](){
    server.send(200, "text/plain", "bye");
    ESP.restart();
    
  });
  
  server.onNotFound([](){server.send(404, "text/plain", "Page not found");});
  server.begin();
  return true;
}

bool ESP_IOT::initIOT(String APssid, String APpsk, String OTAPassword, String deviceName)
{
    m_APssid = APssid;
    m_APpsk = APpsk;
    return initIOT(OTAPassword, deviceName);
}
void ESP_IOT::startSoftAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(m_APssid.c_str(), m_APpsk.c_str());
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("No WiFi params, softAP");
}
bool ESP_IOT::initIOT(String OTAPassword, String deviceName)
{
#ifdef ARDUINO_ARCH_ESP32
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
#else
  if (!LittleFS.begin()) {
#endif
    Serial.println("LittleFS mount failed");
    return false;
  }
    
    if ((!IOT.readFile("/ssid",m_ssid)) or (!IOT.readFile("/psk",m_psk)))
    {
        startSoftAP();
    }
    else
    {
        WiFi.mode(WIFI_STA);       
        WiFi.begin(m_ssid.c_str(), m_psk.c_str());
        if (WiFi.waitForConnectResult() == WL_CONNECTED)
        {
            m_noSoftAP = true;
            Serial.print("connected ");
            Serial.println(WiFi.localIP());
        }
        else
        {
          startSoftAP();
        }
        
    }
    delay(100);
    if (!startWebServer())
        return false;
    startOTA(OTAPassword, deviceName);
    MDNS.addService("http", "tcp", 80);
    return true;
}

bool ESP_IOT::handle()
{
    if (WiFi.getMode() == WIFI_STA)
         ArduinoOTA.handle();
    server.handleClient();
    dnsServer.processNextRequest();
    
    unsigned long l_millis = millis();
    if(l_millis < m_lastMillis + 10000)
        return true;
    m_lastMillis = l_millis;
    if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_DISCONNECTED)
    {
      static int count=0;
      // If we couldn't connect to AP and after waiting 30 secs for the AP the boot
      // we will enable soft AP.
      if (m_noSoftAP == false && m_wasDisconnected && count++ > 3)
      {
        count = 0;
        m_wasDisconnected = false;
        Serial.println("Can't connect, softAP");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(m_APssid.c_str(), m_APpsk.c_str());
        dnsServer.setTTL(300);
        dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      }
      else
      {
          m_wasDisconnected = true;
          // WiFi.mode(WIFI_AP_STA);
          Serial.println("Reconnecting ");
          // WiFi.begin(m_ssid.c_str(), m_psk.c_str());
      }
    }
    else if (m_wasDisconnected)
    {
      m_wasDisconnected = false;
      m_noSoftAP = true;
      WiFi.mode(WIFI_STA);
      Serial.print("Reconnected IP address: ");
      Serial.println(WiFi.localIP());
    }
    return true;
}


void ESP_IOT::startOTA(String OTAPassword, String deviceName)
{

  ArduinoOTA.setHostname(deviceName.c_str());

  //No authentication by default
  ArduinoOTA.setPassword(OTAPassword.c_str());

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}