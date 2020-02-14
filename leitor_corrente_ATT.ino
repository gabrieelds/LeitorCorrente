extern "C" {                                // for deep sleep
#include "user_interface.h"
}

#include <Arduino.h>
#include "EmonLiteESP.h"

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#define DST_IP "80.243.190.58"

#define WLAN_SSID       "GVT-1AF9"
#define WLAN_PASS       "S1EB612518"
//#define WLAN_SSID       "FABRICA"
//#define WLAN_PASS       "FAB*1234"

#define AIO_SERVER      "ia4h4j.messaging.internetofthings.ibmcloud.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "use-token-auth"
#define AIO_KEY         "passw0rd"
#define MQTT_CLIENTID   "d:ia4h4j:Feather:featherhuzzah"
       
WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish mqttconn = Adafruit_MQTT_Publish(&mqtt, "iot-2/evt/bpm/fmt/json");

// Aanalog GPIO on the ESP8266
#define CURRENT_PIN             0
// If you are using a nude ESP8266 board it will be 1.0V, if using a NodeMCU there
// is a voltage divider in place, so use 3.3V instead.
#define REFERENCE_VOLTAGE       1
// Precision of the ADC measure in bits. Arduinos and ESP8266 use 10bits ADCs, but the
// ADS1115 is a 16bits ADC
#define ADC_BITS                10
// Number of decimal positions for the current output
#define CURRENT_PRECISION       3
// This is basically the volts per amper ratio of your current measurement sensor.
// If your sensor has a voltage output it will be written in the sensor enclosure,
// something like "30V 1A", otherwise it will depend on the burden resistor you are
// using.
#define CURRENT_RATIO           73    // based on 30ohm burden resistor and 2000 windings in SCT-013-000
// This version of the library only calculate aparent power, so it asumes a fixes
// mains voltage
#define MAINS_VOLTAGE           220
// Number of samples each time you measure
#define SAMPLES_X_MEASUREMENT   1024

EmonLiteESP power;
int valores [300];
int j = 1;
float consumo = 0;
float total = 0;
String outmsg;

unsigned int currentCallback() {
  return analogRead(CURRENT_PIN);
}

void MQTT_connect();

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
}

uint32_t x=0;

void loop() {
  /* começa aqui */
  power.initCurrent(currentCallback, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);
  power.setPrecision(CURRENT_PRECISION);

  digitalWrite(5, HIGH);
  delay(100);
  digitalWrite(5, LOW);
  for (int i = 0; i < 10; i++) {
    float c = power.getCurrent(SAMPLES_X_MEASUREMENT);
    Serial.println(c);
  }
  float current = power.getCurrent(SAMPLES_X_MEASUREMENT);
  int p = int (current * MAINS_VOLTAGE);
  Serial.print(current);
  Serial.print("\t");
  Serial.print(F("Power now: "));
  Serial.print(p);
  valores[j] = p;
  Serial.println(F("W"));
  Serial.print("\t");
  if (j < 300  ) {
    Serial.println(j);
    Serial.println("\t");
    Serial.println(valores[j]);
    Serial.println(F("\t ainda não"));
    j++;
  } else {
    for(int k = 0; k < 300; k++){
      consumo = consumo + valores[k];
    }
    total = consumo / 300;
    consumo = 0;
    Serial.println(total);
    j = 0;
    MQTT_connect();
    outmsg = "{\"consumo\":";
    outmsg += total; 
    outmsg += "}";
    char charoutmsg[50];
    outmsg.toCharArray(charoutmsg, 50);
    if (mqttconn.publish(charoutmsg)){
      Serial.println(F("Até tentou Enviar!"));
    }else{
      Serial.println(F("tentou enviar e não conseguiu!"));  
    }
    Serial.println(F("OK!"));
  }
  delay(900);
  /*termina aqui*/
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);
       retries--;
       if (retries == 0) {
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
