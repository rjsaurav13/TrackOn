#define TINY_GSM_MODEM_SIM7000
#define SerialMon Serial

#ifndef _AVR_ATmega328P_
#define SerialAT Serial1

#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 1024
#endif

#define TINY_GSM_DEBUG SerialMon

#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

#define GSM_PIN ""

const char apn[]      = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char wifiSSID[] = "";
const char wifiPass[] = "";

const char server[]   = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";

#include <TinyGsmClient.h>

#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

#ifdef USE_SSL&& defined TINY_GSM_MODEM_HAS_SSL
TinyGsmClientSecure      client(modem);
const int                port = 443;
#else
TinyGsmClient  client(modem);
const int      port = 80;
#endif

void setup() {
  SerialMon.begin(115200);
  delay(10);

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  delay(100);
  digitalWrite(4, HIGH);
  delay(100);
  SerialAT.begin(115200, SERIAL_8N1, 26, 27);
  delay(6000);
}

void loop() {
  SerialMon.print("Initializing modem...");
  if (!modem.init()) {
    SerialMon.println(F(" [fail]"));
    SerialMon.println(F("********"));
    SerialMon.println(F(" Is your modem connected properly?"));
    SerialMon.println(F(" Is your serial speed (baud rate) correct?"));
    SerialMon.println(F(" Is your modem powered on?"));
    SerialMon.println(F(" Do you use a good, stable power source?"));
    SerialMon.println(F(" Try using File -> Examples -> TinyGSM -> tools -> AT_Debug to find correct configuration"));
    SerialMon.println(F("********"));
    delay(10000);
    return;
  }
  SerialMon.println(F(" [OK]"));

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif

#if TINY_GSM_USE_WIFI
  SerialMon.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(
          600000L)) { 
    SerialMon.println(F(" [fail]"));
    SerialMon.println(F("********"));
    SerialMon.println(F(" Is your sim card locked?"));
    SerialMon.println(F(" Do you have a good signal?"));
    SerialMon.println(F(" Is antenna attached?"));
    SerialMon.println(F(" Does the SIM card work with your phone?"));
    SerialMon.println(F("********"));
    delay(10000);
    return;
  }
  SerialMon.println(F(" [OK]"));

#if TINY_GSM_USE_GPRS
  SerialMon.print("Connecting to ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(F(" [fail]"));
    SerialMon.println(F("********"));
    SerialMon.println(F(" Is GPRS enabled by network provider?"));
    SerialMon.println(F(" Try checking your card balance."));
    SerialMon.println(F("********"));
    delay(10000);
    return;
  }
  SerialMon.println(F(" [OK]"));
#endif

  IPAddress local = modem.localIP();
  SerialMon.print("Local IP: ");
  SerialMon.println(local);

  SerialMon.print(F("Connecting to "));
  SerialMon.print(server);
  if (!client.connect(server, port)) {
    SerialMon.println(F(" [fail]"));
    delay(10000);
    return;
  }
  SerialMon.println(F(" [OK]"));

  client.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  while (client.connected() && !client.available()) {
    delay(100);
    SerialMon.print('.');
  };
  SerialMon.println();

  client.find("\r\n\r\n");

  uint32_t timeout       = millis();
  uint32_t bytesReceived = 0;
  while (client.connected() && millis() - timeout < 10000L) {
    while (client.available()) {
      char c = client.read();
      bytesReceived += 1;
      timeout = millis();
    }
  }

  client.stop();
  SerialMon.println(F("Server disconnected"));

#if TINY_GSM_USE_WIFI
  modem.networkDisconnect();
  SerialMon.println(F("WiFi disconnected"));
#endif
#if TINY_GSM_USE_GPRS
  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));
#endif

  SerialMon.println();
  SerialMon.println(F("********"));
  SerialMon.print(F(" Received: "));
  SerialMon.print(bytesReceived);
  SerialMon.println(F(" bytes"));
  SerialMon.print(F(" Test:     "));
  SerialMon.println((bytesReceived == 121) ? "PASSED" : "FAILED");
  SerialMon.println(F("********"));

  while (true) { delay(1000); }
}
