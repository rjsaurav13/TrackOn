#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024
#define SerialAT Serial1


#define DUMP_AT_COMMANDS

#define SMS_TARGET  "+917300841079"


#define GSM_PIN ""

const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";


const char wifiSSID[] = "";
const char wifiPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS  
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR 1000000ULL  
#define TIME_TO_SLEEP  60         

#define UART_BAUD   9600
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4


int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;


void setup()
{
  // Set console baud rate
  Serial.begin(115200);
  delay(10);


  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);



  Serial.println("\nWait...");

  delay(1000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  Serial.println("Initializing modem...");
  if (!modem.restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

}

void loop()
{

  Serial.println("Initializing modem...");
  if (!modem.init()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  delay(500);
  Serial.println("Modem Info: " + modemInfo);

  Serial.println("\n---Starting GPS TEST---\n");
  
  modem.sendAT("+SGPIO=0,4,1,1");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,1 false ");
  }
  modem.enableGPS();
  float lat,  lon;
  while (1) {
    if (modem.getGPS(&lat, &lon)) {
      Serial.printf("lat:%f lon:%f\n", lat, lon);
      break;
    } else {
      Serial.print("getGPS ");
      Serial.println(millis());
    }
    delay(2000);
  }
  modem.disableGPS();

  modem.sendSMS(SMS_TARGET, String("https://www.google.com/maps/@") + lat + String(",") + lon);

  while (true) {
  modem.maintain();
  }
