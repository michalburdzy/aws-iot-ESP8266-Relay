#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "externs.h"
#include <ArduinoJson.h>
#include "web_server.h"
#include "secrets.h"

bool relayState = 0;
int relayPin = D1;
bool shouldReboot = false;

// using diode with common +, so the values written to it have to be inverted
// e.g. to turn the color on, provide value "0", while value "255" turns it off
uint redLightPin = D6;
uint greenLightPin = D7;
uint blueLightPin = D8;

uint redLightValue = 255;
uint greenLightValue = 255;
uint blueLightValue = 255;

bool shouldSendInitialPing = true;

BearSSL::X509List client_crt(deviceCertificatePemCrt);
BearSSL::PrivateKey client_key(devicePrivatePemKey);
BearSSL::X509List rootCert(awsCaPemCrt);

WiFiClientSecure wiFiClient;
void msgReceived(char *topic, byte *payload, unsigned int len);
PubSubClient pubSubClient(awsEndpoint, 8883, msgReceived, wiFiClient);

int8_t TIME_ZONE = 1;
unsigned long lastPublish;
int msgCount;
time_t now;

void setCurrentTime()
{
  configTime(TIME_ZONE * 3600, 0, "pl.pool.ntp.org", "europe.pool.ntp.org", "pool.ntp.org");

  Serial.print("Waiting for NTP time sync: ");
  now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current GMT time: ");
  Serial.println(asctime(&timeinfo));
}

void pubSubCheckConnect()
{
  if (!pubSubClient.connected())
  {
    Serial.print("PubSubClient connecting to: ");
    Serial.print(awsEndpoint);
    while (!pubSubClient.connected())
    {
      analogWrite(redLightPin, 100);
      analogWrite(blueLightPin, 100);
      Serial.print("PubSubClient state: ");
      Serial.println(pubSubClient.state());
      pubSubClient.connect(thingName);
    }

    Serial.println(" connected");
    pubSubClient.subscribe(thingMqttTopicIn);
  }
  else
  {
    analogWrite(redLightPin, redLightValue);
    analogWrite(greenLightPin, greenLightValue);
  }
  pubSubClient.loop();
}

void writeColors()
{
  analogWrite(redLightPin, redLightValue);
  analogWrite(greenLightPin, greenLightValue);
  analogWrite(blueLightPin, blueLightValue);
}

void redLight()
{
  redLightValue = 0;
  greenLightValue = 255;
  blueLightValue = 255;
  writeColors();
}

void greenLight()
{
  redLightValue = 255;
  greenLightValue = 0;
  blueLightValue = 255;
  writeColors();
}

void blueLight()
{
  redLightValue = 255;
  greenLightValue = 255;
  blueLightValue = 0;
  writeColors();
}

void noColor()
{
  redLightValue = 255;
  greenLightValue = 255;
  blueLightValue = 255;
  writeColors();
}

void publishMqttMessage(char *outputData)
{
  // analogWrite(redLightPin, 200);
  // analogWrite(blueLightPin, 200);
  pubSubClient.publish(thingMqttTopicOut, outputData);
  Serial.println("Published MQTT Message");
  // analogWrite(redLightPin, redLightValue);
  // analogWrite(blueLightPin, blueLightValue);
}

void setup()
{
  pinMode(relayPin, OUTPUT);
  pinMode(redLightPin, OUTPUT);
  pinMode(greenLightPin, OUTPUT);
  pinMode(blueLightPin, OUTPUT);

  redLight();

  Serial.begin(9600);
  Serial.println("AWS IoT MQTT Connection");
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  blueLight();

  // get current time, otherwise certificates are flagged as expired
  setCurrentTime();

  wiFiClient.setClientRSACert(&client_crt, &client_key);
  wiFiClient.setTrustAnchors(&rootCert);

  setupWebServer();

  greenLight();
  delay(500);
  noColor();
}

unsigned long
getTime()
{
  time(&now);
  return now;
}

void sendPing()
{
  unsigned long epochTime;
  epochTime = getTime();
  char outputData[256];
  sprintf(outputData, "{\"timestamp\": %ld, \"message\": \"ping\"}", epochTime);
  publishMqttMessage(outputData);
}

void loop()
{
  if (shouldReboot)
  {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  pubSubCheckConnect();

  if (shouldSendInitialPing || millis() - lastPublish > 60 * 1000)
  {
    sendPing();
    lastPublish = millis();
    cleanupWsClients();
    shouldSendInitialPing = false;
  }
}

void handleMessage(String message)
{
  unsigned long epochTime;
  epochTime = getTime();
  char outputData[256];
  if (message == "getConfig")
  {
    String localIp = WiFi.localIP().toString();
    sprintf(outputData, "{\"timestamp\": %ld, \"localIp\": \"%s\"}", epochTime, localIp.c_str());
    publishMqttMessage(outputData);
  }

  if (message == "ping")
  {
    String localIp = WiFi.localIP().toString();
    sprintf(outputData, "{\"timestamp\": %ld, \"message\": \"pong\"}", epochTime);
    publishMqttMessage(outputData);
  }
}

void toggleRelayStateChange()
{
  digitalWrite(relayPin, relayState);
  Serial.print("Toggle Relay State Change To: ");
  Serial.println(relayState);
}

void msgReceived(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on ");
  Serial.print(topic);
  Serial.print(": ");
  for (uint i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  JsonObject obj = doc.as<JsonObject>();

  bool light_on = obj[F("light_on")];

  if (light_on == true && relayState == LOW)
  {
    relayState = HIGH;
    toggleRelayStateChange();
  }
  if (light_on == false && relayState == HIGH)
  {
    relayState = LOW;
    toggleRelayStateChange();
  }

  if (obj.containsKey("message"))
  {
    String message = obj["message"];

    handleMessage(message);
  }
  if (obj.containsKey("red"))
  {
    redLightValue = obj[F("red")];
  }
  if (obj.containsKey("green"))
  {
    greenLightValue = obj[F("green")];
  }
  if (obj.containsKey("blue"))
  {
    blueLightValue = obj[F("blue")];
  }

  writeColors();
}
