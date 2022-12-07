#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "externs.h"
#include <ArduinoJson.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
    <head>
        <title>D1-Mini Web Server</title>
        <link rel="shortcut icon" href="https://img.icons8.com/emoji/2x/light-bulb-emoji.png">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="icon" href="data:,">
        <style>
          html {
            font-family: Arial, Helvetica, sans-serif;
            text-align: center;
          }
          h1 {
            font-size: 1.8rem;
            color: white;
          }
          h2{
            font-size: 1.5rem;
            font-weight: bold;
            color: #143642;
          }
          .topnav {
            overflow: hidden;
            background-color: #143642;
          }
          body {
            margin: 0;
          }
          .content {
            padding: 30px;
            max-width: 600px;
            margin: 0 auto;
          }
          .card {
            background-color: #F8F7F9;;
            box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
            padding-top:10px;
            padding-bottom:20px;
          }
          .button {
            padding: 15px 50px;
            font-size: 24px;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #0f8b8d;
            border: none;
            border-radius: 5px;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0,0,0,0);
          }
          /*.button:hover {background-color: #0f8b8d}*/
          .button:active {
            background-color: #0f8b8d;
            box-shadow: 2 2px #CDCDCD;
            transform: translateY(2px);
          }
          .state {
            font-size: 1.5rem;
            color:#8c8c8c;
            font-weight: bold;
          }
          .lamp{
            display: flex;
            flex-direction: column;
            align-items: center;
          }
          .shade{
            width: 15vh; 
            height: 10vh;
            background: #EEE4CF;
            clip-path: polygon(20%% 0, 80%% 0, 100%% 100%%, 0 100%%);
            z-index: 3;
            border-radius: 10px;
            background-image: radial-gradient(closest-corner at 50%% 2%%, #EEE4CF 70%%, transparent 78%%),linear-gradient(to right, transparent 48%%, #ffffff66 58%% 75%%, transparent 88%%),linear-gradient(-253deg, #ffffffcc, #E5CB9A 15%%, #DBB97C 20%%, #EDDBB7 36%%, #ffffff33 54%%, transparent 95%%),linear-gradient(253deg, #F8EFDD 20%%, transparent);
          }
          .leg{
            width: 1vh; 
            height: 25vh;  
            background: #995136;
            top: -1vh; 
            z-index:2;
            background-image: linear-gradient(#00000044, transparent 20%% 99.8%%, #00000033 100%%),linear-gradient(to right, transparent 15%%, #00000055 36%% 44%%,  #00000011 60%%, #ffffff4f 78%% 95%%, #9B5539 95%%);
          }
          .foot{
            width: 11vh; 
            height: 2.5vh;
            background: #995136;  
            top: -2vh; 
            border-radius: 50%%;
            box-shadow: 0 0.4vh #773321;
            background-image: linear-gradient(to right, transparent 11vh, transparent 12vh, transparent 13vh),linear-gradient( 40deg, transparent 5%% 24%%, #0000001f 28%% 36%%, #00000011 37%% 44%%, transparent 50%%, #ffffff1c 56%%, #ffffff44 68%%, #ffffff1c 92%%, transparent);
          }
          input.rounded {
          height: 2rem;
          border: 1px solid #ccc;
          -moz-border-radius: 10px;
          -webkit-border-radius: 10px;
          border-radius: 10px;
          -moz-box-shadow: 2px 2px 3px #666;
          -webkit-box-shadow: 2px 2px 3px #666;
          box-shadow: 2px 2px 3px #666;
          font-size: 20px;
          padding: 4px 7px;
          outline: 0;
          -webkit-appearance: none;
        }
        input.rounded:focus {
          border-color: #339933;
        }
      </style>
        <title>D1-Mini</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="icon" href="data:,">
    </head>
    <body>
        <div class="topnav">
          <h1>D1-Mini Web Server</h1>
        </div>
        <div class="content">
          <div class="lamp">
            <div class="shade"></div>
            <div class="leg"></div>
            <input id="color_input" type="color" class="foot rounded" />
          </div>
          <div class="card">
            <h2>Output - GPIO %LEDPIN%</h2>
            <p class="state">state: <span id="relay_state">%RELAY_STATE%</span></p>
            <p><button id="light_button" class="button">Toggle Light</button></p>
          </div>
        </div>
        <script>
          const gateway = `ws://${window.location.hostname}/ws`;
          let websocket;
          let lastColorChangeMessageTime = 0;
          let colorChangeMessageTimeout = 50;

          const initWebSocket = () => {
            console.log("Trying to open a WebSocket connection...");
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onmessage = onMessage; // <-- add this line
          };
          const onOpen = (event) => {
            console.log("Connection opened");
          };
          const onClose = (event) => {
            console.log("Connection closed");
            setTimeout(initWebSocket, 2000);
          };
          const onMessage = (event) => {
            let state;
            if (event.data == "1") {
              state = "ON";
            } else {
              state = "OFF";
            }
            document.getElementById("relay_state").innerHTML = state;
          };
          const onLoad = (event) => {
            initWebSocket();
            initButton();
            initColorInput();
          };
          const initButton = () => {
            document
              .getElementById("light_button")
              .addEventListener("click", toggle_light, 1000);
          };
          const toggle_light = () => {
            websocket.send("toggle_light");
          };
          const initColorInput = () => {
            document
              .getElementById("color_input")
              .addEventListener("input", onColorChange);
          };
          const onColorChange = (e) => {
            if (!e) return;

            const hexValue = e.target.value;
            const websocketPayload = hexToWebsocketPayload(hexValue);

            const now = new Date().getTime();
            const canSendColorChangeMessage =
              now > lastColorChangeMessageTime + colorChangeMessageTimeout;
            if (canSendColorChangeMessage) {
              lastColorChangeMessageTime = new Date().getTime();
              websocket.send(JSON.stringify(websocketPayload));
            }
          };

          const invertNumberBase255 = (number) => {
            return Math.abs(255 - number);
          };

          const hexToWebsocketPayload = (h) => {
            const payload = {};

            if (h.length == 4) {
              payload.red = invertNumberBase255(parseInt("0x" + h[1] + h[1], "hex"));
              payload.green = invertNumberBase255(parseInt("0x" + h[2] + h[2], "hex"));
              payload.blue = invertNumberBase255(parseInt("0x" + h[3] + h[3], "hex"));
            } else if (h.length == 7) {
              payload.red = invertNumberBase255(parseInt("0x" + h[1] + h[2], "hex"));
              payload.green = invertNumberBase255(parseInt("0x" + h[3] + h[4], "hex"));
              payload.blue = invertNumberBase255(parseInt("0x" + h[5] + h[6], "hex"));
            }

            return payload;
          };

          window.addEventListener("load", onLoad);

        </script>
    </body>
  </html>
)rawliteral";

void notifyClients()
{
  ws.textAll(String(relayState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "toggle_light") == 0)
    {
      relayState = !relayState;
      toggleRelayStateChange();
      notifyClients();
    }
    else
    {
      // Serial.println((char *)data);
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      JsonObject obj = doc.as<JsonObject>();

      bool objContainsAnyColorField = obj.containsKey("red") || obj.containsKey("green") || obj.containsKey("blue");
      if (objContainsAnyColorField)
      {
        if (obj.containsKey("red"))
        {
          redLightValue = obj["red"];
        }
        if (obj.containsKey("green"))
        {
          greenLightValue = obj["green"];
        }
        if (obj.containsKey("blue"))
        {
          blueLightValue = obj["blue"];
        }

        writeColors();
      }
    }
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len)
{
  Serial.println("WS EVENT: ");
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
    Serial.println("WS PONG");
    break;
  case WS_EVT_ERROR:
    Serial.println("WS ERROR");
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}

String processor(const String &var)
{
  Serial.println(var);
  if (var == "RELAY_STATE")
  {
    if (relayState)
    {
      return "ON";
    }
    else
    {
      return "OFF";
    }
  }

  if (var == "LEDPIN")
  {
    return String(relayPin);
  }

  return String();
}

void setupWebServer()
{
  initWebSocket();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"); });
  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request)
      {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          Serial.printf("Update Start: %s\n", filename.c_str());
          Update.runAsync(true);
          if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
          {
            Update.printError(Serial);
          }
        }
        if (!Update.hasError())
        {
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }
        }
        if (final)
        {
          if (Update.end(true))
          {
            Serial.printf("Update Success: %uB\n", index + len);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  server.begin();
}

void cleanupWsClients()
{
  ws.cleanupClients();
}