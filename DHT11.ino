#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>

#define FONA_RX 9
#define FONA_TX 8
#define FONA_RST 4
#define DHT_PIN 11

#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

/************************* WiFi Access Point *********************************/
/*Optionally configure a GPRS APN, username, and password.
You might need to do this to access your network's GPRS/data
network.  Contact your provider for the exact APN, username,
and password values.  Username and password are optional and
can be removed, but APN is required.*/
#define FONA_APN       "safaricom"
#define FONA_USERNAME  "saf"
#define FONA_PASSWORD  "data"

//************************* Adafruit.io Setup *********************************/
// Under the Adafruit.io you can use the io.adafruit.com broker or your own personal broker
// In our case we will use the strathmore MQTT broker
#define AIO_SERVER      "156.0.232.201"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "iotlab"
#define AIO_KEY         "80d18b0d"

/************ Global State (you don't need to change this!) ******************/
// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT,"IOTLAB",AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab above for the source!
boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

/****************************** Feeds ***************************************/ 
// Here we publish to a combined topic
Adafruit_MQTT_Publish TESTING = Adafruit_MQTT_Publish(&mqtt,"dht11");


void setup() {
  // put your setup code here, to run once:
  while (!Serial);

  //Watchdog is optional!
  //Watchdog.enable(8000);
  
  Serial.begin(9600);
  
  Serial.println(F("Tuko Sawa"));
  
  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
  
  // Initialise the FONA module
  while (! FONAconnect(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD))) {
    Serial.println(F("Retrying FONA"));
  }
  
  Serial.println(F("Connected to Cellular!"));
  
  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();  

  dht.begin();
}

void loop() {
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();
  /*Ensure the connection to the MQTT server is alive (this will make the first
  connection and automatically reconnect when disconnected).  See the MQTT_connect
  function definition further below.*/
  MQTT_connect();
  Watchdog.reset();

  Serial.print(F("The Humidity is : "));
  Serial.print(dht.readHumidity());
  Serial.println(" %");

  Serial.print(F("The Temperature is : "));
  Serial.print(dht.readTemperature());
  Serial.println(" Degrees Celsius");

  //Battery Voltage
  uint16_t vbat;
  fona.getBattVoltage(&vbat);
  Serial.print(F("VBat = ")); 
  Serial.print(vbat); 
  Serial.println(F("mV"));
  float bat = vbat;

  //Battery percentage
  fona.getBattPercent(&vbat);
  Serial.print(F("VPct = ")); 
  Serial.print(vbat); 
  Serial.println(F("%"));
  float batP = vbat;

// Create a JSON object and serialize the data
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["temperature"]= dht.readTemperature();
  jsonDoc["humidity"]= dht.readHumidity();
  jsonDoc["volt"]= bat;
  jsonDoc["percentage"]= batP;
  
  //Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonDoc,jsonString);

  //Publishing
  //Publish the JSON payload to the combined topic
  TESTING.publish(jsonString.c_str());
  delay(5000);
  Watchdog.reset(); 
}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print(F("Connecting to MQTT... "));

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(F("Retrying MQTT connection in 5 seconds..."));
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println(F("MQTT Connected!"));
}