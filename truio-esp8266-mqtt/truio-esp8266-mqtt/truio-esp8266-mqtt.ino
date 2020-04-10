/* 
 * This example uses an ESP8266
 * to connect to truio.live. 
 * 
 * by TruIO Team
 * Project source and readme can be found at: https://github.com/truio-live/truio-esp8266
 * 
 * Credits to the open source libraries that are used in this project.
*/

#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>

const char ssid[] = "";
const char pass[] = "";
#define DEVICE_KEY ""
const char* mqtt_server = "mqtt.truio.live";

WiFiClient net;
MQTTClient client(512);

unsigned long lastMillis = 0;
unsigned long lastMillis1 = 0;
bool ledOnOff = false;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect(DEVICE_KEY, DEVICE_KEY, DEVICE_KEY)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("TruIO/server/" DEVICE_KEY);
  // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(13, INPUT_PULLUP);        //initialize on-board button GPIO13
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  client.begin(mqtt_server, net);
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  if (millis() - lastMillis1 > 500) {
    lastMillis1 = millis();
    ledOnOff = !ledOnOff;
    digitalWrite(LED_BUILTIN, ledOnOff);
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();

    float humidity, temperature;
    humidity = 50;
    temperature = 24;
  
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
}
