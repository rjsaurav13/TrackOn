
 #define TINY_GSM_MODEM_SIM7000


#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SIM7000.h>

#define RX_PIN 26
#define TX_PIN 27
#define GPS_BAUD 9600
#define SIM_BAUD 9600
#define APN "airtelgprs.com"
#define SMS_NUMBER "+917300841079"

SoftwareSerial ss(RX_PIN, TX_PIN);
TinyGPSPlus gps;
SIM7000 sim;
String lastSMS;

void setup() {
  Serial.begin(115200);
  ss.begin(GPS_BAUD);
  sim.begin(SIM_BAUD);
  connectToGPRS();
}

void loop() {
  while (ss.available() > 0) {
    if (gps.encode(ss.read())) {
      if (gps.location.isValid()) {
        String location = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
        sendLocationAsSMS(location);
      }
    }
  }
  readSMS();
}

void connectToGPRS() {
  Serial.println("Connecting to GPRS");
  sim.setGPRSNetworkSettings(APN);
  if (sim.enableGPRS()) {
    Serial.println("GPRS connected");
  } else {
    Serial.println("GPRS connection failed");
  }
}

void sendLocationAsSMS(String location) {
  if (lastSMS != location) {
    Serial.println("Sending SMS: " + location);
    sim.sendSMS(SMS_NUMBER, location);
    lastSMS = location;
  }
}

void readSMS() {
  String message;
  if (sim.isSMSPresent(SMS_UNREAD)) {
    sim.readSMS(message, 1);
    Serial.println("Received SMS: " + message);
    sim.deleteSMS(1);
  }
}
