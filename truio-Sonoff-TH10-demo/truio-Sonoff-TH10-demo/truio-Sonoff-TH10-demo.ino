/*
 Sonoff TH10 with AM2301 sensor connecting to TruIO broker/server
 Version: 1.0
 Date: 31/1/2020
 
 This sketch is an  example on how to send sensor data and control the relay of an Itead Sonoff TH10/16 to TruIO platform using ESP8266 via MQTT.

 It connects to the TruIO MQTT server then:
  - syncs the latest relay state from the server.
  - sends temperature and humidity data from the AM2301 sensor every 5 seconds.
  - manipulates the relay state based on the toggle button widget on the dashboard.
 
 It will reconnect to the server if the connection is lost using a non-blocking
 reconnect function.

 On the dashboard:
 - Add 2 gauge widgets with tags "temperature" & "humidity" as the data source.
 - Create a new tag named "relay".
 - Add a toggle button widget and set tag "relay" as its data source. 

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

  Project source and readme can be found at: https://github.com/truio-live/truio-esp8266

  Credits to the open source libraries that are used in this project.

*/
#define RELAY 12

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Tasker.h>
#include <ArduinoJson.h>
// The TroykaDHT library requires modification to make it work with the Sonoff AM2301 sensor.
// Replace line 34 of TroykaDHT.cpp with delayMicroseconds(500);
// Ignore if using copy of library provided with this example.
#include <TroykaDHT.h>

// Update these with values suitable for your network.

const char* ssid = "Insert your WiFi SSID";
const char* password = "Insert your WiFi password";
#define DEVICE_KEY "Insert your Device Key"

const char* mqtt_server = "mqtt.truio.live";
#define MQTT_PORT 1883

Tasker tasker;
// create an object of class DHT
// pass the pin number to which the sensor is connected and the type of sensor
// Sensor type: DHT11, DHT21, DHT22 (AM2301 is similar to DHT21)
DHT dht(14, DHT21);
//variables to store humidity & temperature values
float humidity, temperature;

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
    //blink the blue LED while connecting
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  }
  
  randomSeed(micros());
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(BUILTIN_LED, LOW);
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
    if(!strcmp(tag["name"],"relay")) {
      triggerRelay(tag["value"]);
    }
    // Add checking for other tag names with the functions to execute here
  }
}

void reconnect() {
  //boolean to store sync state
  static bool firstConnect = true;
  // Loop until we're reconnected
  
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(DEVICE_KEY, DEVICE_KEY, DEVICE_KEY)) {
    Serial.println("connected");
    Serial.print("Subscribing to ");
    Serial.println("TruIO/server/" DEVICE_KEY);
    client.subscribe("TruIO/server/" DEVICE_KEY);
    //sync the latest relay state stored in server
    if(firstConnect) {
      syncTag("relay");
      firstConnect = false;
    }   
  } 
  else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" trying again in 5 seconds");
    // Retrying in 5 seconds
    tasker.setTimeout(reconnect, 5000);
  }
}

void publishSensor() {

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

void syncTag(char * tagName) {
  char output[512];
  memset(output,0,sizeof(output));
  
  StaticJsonDocument<512> doc;
  doc["writeKey"] = DEVICE_KEY;
  doc["op"] = "read";
  JsonArray tag = doc.createNestedArray("tag");
  JsonObject tagobj1 = tag.createNestedObject();
  tagobj1["name"] = tagName;
  
  serializeJson(doc, output);
  serializeJsonPretty(doc, Serial);

  client.publish("TruIO/device/" DEVICE_KEY, output);
}

void triggerRelay(bool value) {
  digitalWrite(RELAY, value);
}

void readSensor() {
  // perform a read
  dht.read();
  // check status of data
  switch(dht.getState()) {
    // всё OK
    case DHT_OK:
      // store the temperature & humidity in their variables and print on serial
      temperature = dht.getTemperatureC();
      Serial.print("Temperature = ");
      Serial.print(temperature);
      Serial.println(" C \t");
      humidity = dht.getHumidity();
      Serial.print("Humidity = ");
      Serial.print(humidity);
      Serial.println(" %");
      break;
    case DHT_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    case DHT_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    case DHT_ERROR_NO_REPLY:
      Serial.println("Sensor not connected");
      break;
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(RELAY, OUTPUT);        //initialize on-board relay control pin as output
  Serial.begin(115200);
  setup_wifi();
  dht.begin();

  // call my function 'publishSensor' every 5 seconds
  tasker.setInterval(publishSensor, 5000);
  // read the TH sensor every 2 seconds
  tasker.setInterval(readSensor, 2000);
}

void loop() {
  if (!client.connected() && (tasker.scheduledIn(reconnect) == 0)) {
    //set to reconnect in 100ms
    tasker.setTimeout(reconnect, 100);
    //reconnect();
  }
  client.loop();
  tasker.loop();
}
