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

const char ssid[] = "";   // insert your WiFi SSID
const char pass[] = "";   // insert your WiFi password
#define DEVICE_KEY ""     // insert your device key
#define PAYLOAD_SIZE 512  // 512 bytes of maximum payload
#define PUB_INTERVAL 5000 // 5 seconds interval of publish

#define mqtt_server "mqtt.truio.live" // do not change this
WiFiClient net;
MQTTClient client(PAYLOAD_SIZE);

unsigned long lastMillis = 0;
bool subReceived = false; // flag to indicate subscribe payload received
char JSONpayload[PAYLOAD_SIZE];
StaticJsonDocument<PAYLOAD_SIZE> doc;
JsonArray tag;

void connect()
{
  Serial.print("Checking wifi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nConnecting...");
  while (!client.connect(DEVICE_KEY, DEVICE_KEY, DEVICE_KEY))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nConnected!");

  client.subscribe("TruIO/server/" DEVICE_KEY);
  // client.unsubscribe("/hello");
}

StaticJsonDocument<512> subDoc;

void messageReceived(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(subDoc, payload);

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  // serializeJsonPretty(subDoc, Serial);
  subReceived = true;
}

float truio_readTag(const char *tagName)
{
  for (int i = 0; i < subDoc["tag"].size(); i++)
  {
    // Fetch tag element.
    JsonObject subTag = subDoc["tag"][i];
    // If tag name is "relay", execute triggerRelay function.
    if (!strcmp(subTag["name"], tagName))
    {
      return subTag["value"];
    }
    // Add checking for other tag names with the functions to execute here
  }
}

void initJSON()
{
  memset(JSONpayload, 0, sizeof(JSONpayload));
  doc["writeKey"] = DEVICE_KEY;
  doc["op"] = "write";
  tag = doc.createNestedArray("tag");
}

void truio_prepareTag(const char *tagName, float tagValue, int timestamp)
{
  JsonObject key = tag.createNestedObject();
  key["name"] = tagName;
  key["value"] = tagValue;
  if (timestamp > 0)
  {
    key["time"] = timestamp;
  }
}

void truio_publishTag()
{
  serializeJson(doc, JSONpayload);
  serializeJsonPretty(doc, Serial);
  client.publish("TruIO/device/" DEVICE_KEY, JSONpayload);

  // clear after publish
  memset(JSONpayload, 0, sizeof(JSONpayload));
  doc.clear();
  initJSON();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an JSONpayload
  pinMode(13, INPUT_PULLUP);    //initialize on-board button GPIO13
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  client.begin(mqtt_server, net);
  client.onMessage(messageReceived);

  connect();
  initJSON();
}

void loop()
{
  client.loop();
  delay(10); // <- fixes some issues with WiFi stability

  if (!client.connected())
  {
    connect();
  }

  // publish a message roughly every X seconds.
  if (millis() - lastMillis > PUB_INTERVAL)
  {
    lastMillis = millis();
    truio_prepareTag("humidity", 45, 0);
    truio_prepareTag("temperature", 21, 0);
    truio_publishTag();
  }

  // subscribed messaged to be processed here.
  if (subReceived == true)
  {
    subReceived = false;
    if (truio_readTag("button") > 0)
    {
      digitalWrite(LED_BUILTIN, false);
    }
    else
    {
      digitalWrite(LED_BUILTIN, true);
    }
    subDoc.clear();
  }
}