#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// Watson IoT connection details
#define MQTT_HOST "9zlw2x.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define DEVICE_ID "area2"
//Device                 Org    Type    ID
#define MQTT_DEVICEID "d:9zlw2x:ESP8266:area2"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "area2_esp"
#define MQTT_TOPIC_DISPLAY "iot-2/cmd/display/fmt/json"

// Water Flow Sensor IBM_Watson data
char pubTopic1[] = "iot-2/evt/area2_WFS_FLitres/fmt/json";    // Water flow rate
char pubTopic2[] = "iot-2/evt/area2_WFS_TLitres/fmt/json";    // Amount of water consumed

// Soil Moisture Sensor IBM_Watson data
char pubTopic3[] = "iot-2/evt/area2_SMS_moisture/fmt/json";

// Solindoid Valve IBM_Watson data
char pubTopic4[] = "iot-2/evt/area2_SValve/fmt/json";

// WiFi connection information
char ssid[] = "mostafa";      // network SSID (name)
char pass[] = "m1234567";     // network password
//////////////////////////////////////

// Solindoid Valve
#define valve_pin  5
boolean valve_state = false;
///////////////////

// Water Flow Sensor
#define WFS_SENSOR  4
long WFS_currentMillis = 0;
long WFS_previousMillis = 0;
int WFS_interval = 1000;
float WFS_calibrationFactor = 4.5;
volatile byte WFS_pulseCount;
byte WFS_pulse1Sec = 0;
float WFS_flowRate;
unsigned long WFS_flowMilliLitres;
unsigned int WFS_totalMilliLitres;
float WFS_flowLitres;
float WFS_totalLitres;
// To avoid error on interrupt with ESP8266
// we should add IRAM_ATTR in front of the function that called on interrupt.
void IRAM_ATTR pulseCounter()
{
  WFS_pulseCount++;
}
////////////////////// Water Flow Sensor End //////////////////////

// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

// variables to hold data
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
static char msg[50];

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");

  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string
  Serial.println((char *)payload);
}

void setup() {
  ////////// Solindoid Valve
  pinMode(valve_pin, OUTPUT);
  digitalWrite(valve_pin, LOW);
  ///////////////////////////// Soil Moisture Sensor //////////////////////////////
  pinMode(A0, INPUT);
  ///////////////////////////// Soil Moisture End //////////////////////////
  ///////////////////////////// Water Flow Sensor////////////////////////////////////////////
  pinMode(WFS_SENSOR, INPUT_PULLUP);
  WFS_pulseCount = 0;
  WFS_flowRate = 0.0;
  WFS_flowMilliLitres = 0;
  WFS_totalMilliLitres = 0;
  WFS_previousMillis = 0;
  attachInterrupt(digitalPinToInterrupt(WFS_SENSOR), pulseCounter, FALLING);
  ///////////////////////////////Water Flow Sensor End /////////////////////////////
  // Start serial console
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
  Serial.println();
  Serial.println("ESP8266 Sensor Application");

  /////////////// Start WiFi connection /////////////////////////////////
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  //////////////////////////////////////////////////////////////////////////

  // Connect to MQTT - IBM Watson IoT Platform ///////////////////////////
  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
    Serial.println("MQTT Connected");
    mqtt.subscribe(MQTT_TOPIC_DISPLAY);
  } else {
    Serial.println("MQTT Failed to connect!");
    ESP.reset();
  }
  //////////////////////////////////////////////////////////////////////
}
void loop() {
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_DISPLAY);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
  WFS_currentMillis = millis();
  if (WFS_currentMillis - WFS_previousMillis > WFS_interval)
  {
    /////////////////// Water Flow Sensor ////////////////////////////////
    WFS_pulse1Sec = WFS_pulseCount;
    WFS_pulseCount = 0;
    WFS_flowRate = ((1000.0 / (millis() - WFS_previousMillis)) * WFS_pulse1Sec) / WFS_calibrationFactor;
    WFS_previousMillis = millis();
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    WFS_flowMilliLitres = (WFS_flowRate / 60) * 1000;
    WFS_flowLitres = (WFS_flowRate / 60);
    // Add the millilitres passed in this second to the cumulative total
    WFS_totalMilliLitres += WFS_flowMilliLitres;
    WFS_totalLitres += WFS_flowLitres;
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(float(WFS_flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space
    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(WFS_totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(WFS_totalLitres);
    Serial.println("L");

    // Check if any reads failed and exit early (to try again).
    if (isnan(WFS_flowRate) || isnan(WFS_totalLitres)) {
      Serial.println("Failed to read from Water Flow sensor!");
    } else {
      String payload = "{\"d\":{\"Name\":\"" DEVICE_ID "\"";
      payload += ",\"area2_WFS_FLitres\":";
      payload += WFS_flowRate;
      payload += "}}";
      Serial.print("Sending payload: ");
      Serial.println(payload);
      if (mqtt.publish(pubTopic1, (char*) payload.c_str())) {
        Serial.println("Publish ok");
      } else {
        Serial.println("Publish failed");
      }
      String payload1 = "{\"d\":{\"Name\":\"" DEVICE_ID "\"";
      payload1 += ",\"area2_WFS_TLitres\":";
      payload1 += WFS_totalLitres;
      payload1 += "}}";
      if (mqtt.publish(pubTopic2, (char*) payload1.c_str())) {
        Serial.println("Publish ok");
      } else {
        Serial.println("Publish failed");
      }
    }
  }
  ///////////////// Water Flow Sensor End ///////////////////////////////

  ///////////////////////////// Soil Moisture Sensor //////////////////////////////
  float moisture = analogRead(A0);
  moisture = ((1024 - moisture) / 1024) * 100;
  Serial.print("Moisture value :");
  Serial.println(moisture);
  
  if (moisture <= 25) {
    digitalWrite(valve_pin, LOW);
    valve_state = true;
  } else if (moisture >= 50) {
    digitalWrite(valve_pin, HIGH);
    valve_state = false;
  }
  if (isnan(moisture)) {
    Serial.println("Failed to read from Soil Moisture sensor!");
  } else {
    String payload = "{\"d\":{\"Name\":\"" DEVICE_ID "\"";
    payload += ",\"area2_SMS_moisture\":";
    payload += moisture;
    payload += "}}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (mqtt.publish(pubTopic3, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    } else {
      Serial.println("Publish failed");
    }
  }

  /////////// Send state of Solindoid Valve to IBM Watson
  String payload = "{\"d\":{\"Name\":\"" DEVICE_ID "\"";
    payload += ",\"area2_SValve\":";
    payload += valve_state;
    payload += "}}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (mqtt.publish(pubTopic4, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    } else {
      Serial.println("Publish failed");
    }
  ///////////////////////////// Soil Moisture End //////////////////////////

  // Pause - but keep polling MQTT for incoming messages
  for (int i = 0; i < 10; i++) {
    mqtt.loop();
    delay(1000);
  }
}
