/*
 ESP8266 MQTT & Si7020 connect to TruIO broker/server
 Version: 1.0
 Date: 21/1/2020
 
 This sketch is a simple example on how to connect to TruIO platform using ESP8266 via MQTT.

 It connects to an TruIO MQTT server then:
  - publishes Si7020 temperature and humidity to the topic "TruIO/device/<user email address>" every two seconds
  - subscribes to the topic "TruIO/server/<user email address>/<device key>", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

  Project source and readme can be found at: https://github.com/truio-live/truio-esp8266

  Credits to the open source libraries that are used in this project.

*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Tasker.h"
#include "Adafruit_Si7021.h"
#include <ArduinoJson.h>

Adafruit_Si7021 sensor = Adafruit_Si7021();
Tasker tasker;

// Update these with values suitable for your network.

const char* ssid = "Insert your WiFi SSID";
const char* password = "Insert your WiFi password";
#define DEVICE_KEY "Insert your Device Key"

const char* mqtt_server = "mqtt.truio.space";
#define MQTT_PORT 1883

void callback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(mqtt_server, MQTT_PORT, callback, espClient);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  StaticJsonDocument<512> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  serializeJsonPretty(doc, Serial);
  // Loop through all tags
  for(int i=0;i<doc["tag"].size();i++) {
    // Fetch tag element.
    JsonObject tag = doc["tag"][i];
    // If tag name is "relay", execute triggerRelay function.
    if(!strcmp(tag["name"],"led")) {
      triggerLED(tag["value"]);
    }
    // Add checking for other tag names with the functions to execute here
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(DEVICE_KEY, DEVICE_KEY, DEVICE_KEY)) {
      Serial.println("connected");
      Serial.print("Subscribing to ");
      Serial.println("TruIO/server/" DEVICE_KEY);
      client.subscribe("TruIO/server/" DEVICE_KEY);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
bool setup_si7021() {
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    return false;
  }

  Serial.print("Found model ");
  switch(sensor.getModel()) {
    case SI_Engineering_Samples:
      Serial.print("SI engineering samples"); break;
    case SI_7013:
      Serial.print("Si7013"); break;
    case SI_7020:
      Serial.print("Si7020"); break;
    case SI_7021:
      Serial.print("Si7021"); break;
    case SI_UNKNOWN:
    default:
      Serial.print("Unknown");
      return false;
  }
  Serial.print(" Rev(");
  Serial.print(sensor.getRevision());
  Serial.print(")");
  Serial.print(" Serial #"); Serial.print(sensor.sernum_a, HEX); Serial.println(sensor.sernum_b, HEX);
  return true;
}

void publishSensor() {
  float humidity, temperature;
  humidity = sensor.readHumidity();
  temperature = sensor.readTemperature();

  char output[512];
  memset(output,0,sizeof(output));
  
  StaticJsonDocument<512> doc;
  doc["writeKey"] = DEVICE_KEY;
  doc["op"] = "write";
  JsonArray tag = doc.createNestedArray("tag");
  JsonObject tagobj1 = tag.createNestedObject();
  tagobj1["name"] = "humidity";
  tagobj1["value"] = humidity;
  JsonObject tagobj2 = tag.createNestedObject();
  tagobj2["name"] = "temperature";
  tagobj2["value"] = temperature;
  
  serializeJson(doc, output);
  serializeJsonPretty(doc, Serial);

  client.publish("TruIO/device/" DEVICE_KEY, output);
}

ICACHE_RAM_ATTR void publishButton() {
  //publish button state
  char output[512];
  memset(output,0,sizeof(output));
  
  StaticJsonDocument<512> doc;
  doc["writeKey"] = DEVICE_KEY;
  JsonArray tag = doc.createNestedArray("tag");
  JsonObject tagobj1 = tag.createNestedObject();
  tagobj1["name"] = "button";
  tagobj1["value"] = !digitalRead(13);

  serializeJson(doc, output);
  serializeJsonPretty(doc, Serial);

  client.publish("TruIO/device/" DEVICE_KEY, output);
}

void triggerLED(bool value) {
  digitalWrite(BUILTIN_LED, !value);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(13, INPUT_PULLUP);        //initialize on-board button GPIO13
  Serial.begin(115200);
  setup_wifi();
  if(setup_si7021()) {
    // call my function 'publishSensor' every 5 seconds
    tasker.setInterval(publishSensor, 5000);
  }
  
  attachInterrupt(digitalPinToInterrupt(13), publishButton, CHANGE);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  tasker.loop();
}
