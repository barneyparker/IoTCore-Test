#include <MQTT.h>
#include <MQTTClient.h>

#include "time.h"

#include "secrets.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define ADCPin 36
#define LEDPin  2

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectWiFi() {
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected!");
}

void connectAWS() {
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setOptions(5000, true, 15000);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);


  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void updateTimeNTP() {
  const char* ntpServer = "pool.ntp.org";
  struct tm timeinfo;

  Serial.print("Configuring time via NTP (UTC)");

  while(!getLocalTime(&timeinfo)) {
    Serial.print(".");
    configTime(0, 0, ntpServer);
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

#define upload_period_ms 60000
#define samples_per_period 600
int light[samples_per_period];

void getLight(int index) {
  light[index] = analogRead(ADCPin);
}

void publishMessage(int sensor_reading)
{
  digitalWrite(LEDPin, HIGH);
  
  time_t now;
  time(&now);
  
  StaticJsonDocument<200> doc;
  doc["device"] = WiFi.macAddress();
  doc["uptime"] = (millis() / 1000);
  doc["epoch"]  = now;
  doc["light"] = sensor_reading;
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  Serial.println(jsonBuffer);

  if(!client.connected()) {
    Serial.println("Not Connected");
    while (!client.connect(THINGNAME)) {
      Serial.print(".");
      delay(100);
    }
  } else {
    Serial.println("Connected");
  }

  if(client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer) == false) {
    Serial.println("Publish Failed");
  } else {
    Serial.println("Publish Succeeded");
  }
  digitalWrite(LEDPin, LOW);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  if(doc["indicator"] == 1) {

  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ADCPin,INPUT);
  pinMode(LEDPin, OUTPUT);

  analogSetPinAttenuation(ADCPin, ADC_0db);
  
  connectWiFi();
  updateTimeNTP();
  connectAWS();
}


void loop() {
  int current_index = 0;

  getLight(current_index);
  current_index++;
  
  if(current_index == samples_per_period) {
    unsigned long total_light = 0;
    for(int index = 0; index < samples_per_period; index++) {
      total_light += light[index];
    }
    publishMessage(total_light / samples_per_period);
    current_index = 0;
  }

  client.loop();

  delay(upload_period_ms / samples_per_period);
}