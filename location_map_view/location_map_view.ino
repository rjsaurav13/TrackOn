#define TINY_GSM_MODEM_SIM7000
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false


const char apn[]= "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";


const char *broker = "io.adafruit.com";
const char *GPSTopic = "rjsaurav13/feeds/gpsloc/csv";

#include <TinyGsmClient.h>
#include <PubSubClient.h>


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
TinyGsmClient client(modem);
PubSubClient  mqtt(client);


#define uS_TO_S_FACTOR 1000000ULL  
#define TIME_TO_SLEEP  120         

#define UART_BAUD   9600
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define BAT_ADC     35

#define LED_PIN     12

float speed_kph = 0;
float heading = 0;
float speed_mph = 0;
float altitude = 0;
float lat = 0;
float lon = 0;
char Lat[20];
char Lon[20];
char sendbuffer[120];

RTC_DATA_ATTR int bootCount = 0;


int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

void transCoordinates()
{
  while (lat <= 0 || lon <= 0)
  {
    modem.sendAT("+SGPIO=0,4,1,1");
    if (modem.waitResponse(10000L) != 1) {
      Serial.println(" SGPIO=0,4,1,1 false ");
    }
    modem.enableGPS();
    Serial.println("Requesting current GPS/GNSS/GLONASS location");
    if (modem.getGPS(&lat, &lon))
    {
      Serial.println("Latitude: " + String(lat, 8) + "\tLongitude: " + String(lon, 8));
    }
  }
  char *p = sendbuffer;
 
  dtostrf(speed_mph, 2, 6, p);
  p += strlen(p);
  p[0] = ','; p++;

  
  dtostrf(lat, 2, 6, p);
  p += strlen(p);
  p[0] = ','; p++;

 
  dtostrf(lon, 3, 6, p);
  p += strlen(p);
  p[0] = ','; p++;

  
  dtostrf(altitude, 2, 6, p);
  p += strlen(p);

  p[0] = 0;

  Serial.print("Sending: "); Serial.println(sendbuffer); 
  mqtt.publish(GPSTopic, sendbuffer);

}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{

  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();
}

boolean mqttConnect()
{
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);


  boolean status = mqtt.connect("GsmClientName", "rjsaurav13", "x");

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
 
  return mqtt.connected();
}


void setup()
{
 
  Serial.begin(115200);
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);

  Serial.println("\nWait...");

  delay(1000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  if (bootCount  == 0)
  {
 
    Serial.println("Initializing modem...");
    if (!modem.restart()) {
      Serial.println("Failed to restart modem, attempting to continue without restarting");
    }
    bootCount++;
  }
  String name = modem.getModemName();
  DBG("Modem Name:", name);

  String modemInfo = modem.getModemInfo();
  DBG("Modem Info:", modemInfo);


#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }
#endif

  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

}

void loop()
{
  if (!modem.isNetworkConnected())
  {
    SerialMon.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true))
    {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected())
    {
      SerialMon.println("Network re-connected");
    }

#if TINY_GSM_USE_GPRS
    if (!modem.isGprsConnected())
    {
      SerialMon.println("GPRS disconnected!");
      SerialMon.print(F("Connecting to "));
      SerialMon.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected()) {
        SerialMon.println("GPRS reconnected");
      }
    }
#endif
  }

  if (!mqtt.connected())
  {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L)
    {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  mqtt.loop();
  transCoordinates();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();

}