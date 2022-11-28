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

// diode with common +
const int redLightPin = D6;
const int greenLightPin = D7;
const int blueLightPin = D8;

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
      Serial.print("PubSubClient state:");
      Serial.print(pubSubClient.state());
      pubSubClient.connect(thingName);
    }
    Serial.println(" connected");
    pubSubClient.subscribe(thingMqttTopicIn);
  }
  pubSubClient.loop();
}

void blinkColors()
{
  analogWrite(redLightPin, 0);
  analogWrite(greenLightPin, 0);
  analogWrite(blueLightPin, 0);

  delay(1000);

  analogWrite(redLightPin, 255);
  analogWrite(greenLightPin, 255);
  analogWrite(blueLightPin, 255);

  analogWrite(redLightPin, 0);
  delay(500);
  analogWrite(redLightPin, 255);
  analogWrite(blueLightPin, 0);
  delay(500);
  analogWrite(blueLightPin, 255);
  analogWrite(greenLightPin, 0);
  delay(500);
  analogWrite(greenLightPin, 255);
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("AWS IoT MQTT Connection");

  pinMode(relayPin, OUTPUT);
  pinMode(redLightPin, OUTPUT);
  pinMode(greenLightPin, OUTPUT);
  pinMode(blueLightPin, OUTPUT);

  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // get current time, otherwise certificates are flagged as expired
  setCurrentTime();

  wiFiClient.setClientRSACert(&client_crt, &client_key);
  wiFiClient.setTrustAnchors(&rootCert);

  setupWebServer();

  blinkColors();
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
  pubSubClient.publish(thingMqttTopicOut, outputData);

  Serial.println("Published MQTT Ping");
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

  if (millis() - lastPublish > 60 * 15 * 1000)
  {
    sendPing();
    lastPublish = millis();
    cleanupWsClients();
  }
}

void handleMessage(String message)
{
  if (message == "getConfig")
  {
    unsigned long epochTime;
    epochTime = getTime();
    char outputData[256];
    String localIp = WiFi.localIP().toString();
    sprintf(outputData, "{\"timestamp\": %ld, \"localIp\": \"%s\"}", epochTime, localIp.c_str());
    pubSubClient.publish(thingMqttTopicOut, outputData);

    Serial.println("Published MQTT Ping");
  }
}

void toggleRelayStateChange()
{
  digitalWrite(relayPin, relayState);
  Serial.print("Toggle Relay State Change To: ");
  Serial.println(relayState);
}

void writeColors(int red, int green, int blue)
{
  Serial.print("red: ");
  Serial.println(red);
  Serial.print("green: ");
  Serial.println(green);
  Serial.print("blue: ");
  Serial.println(blue);

  analogWrite(redLightPin, red);
  analogWrite(greenLightPin, green);
  analogWrite(blueLightPin, blue);
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
    uint red = obj[F("red")];
    analogWrite(redLightPin, red);
  }
  if (obj.containsKey("green"))
  {
    uint green = obj[F("green")];
    analogWrite(greenLightPin, green);
  }
  if (obj.containsKey("blue"))
  {
    uint blue = obj[F("blue")];
    analogWrite(blueLightPin, blue);
  }
}
