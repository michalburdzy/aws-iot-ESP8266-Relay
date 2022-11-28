# ESP8266 D1 Mini Relay Lamp

Board: D1 Mini (ESP8266): [link to official board documentation](https://www.wemos.cc/en/latest/d1/d1_mini.html)
Relay shield: [link to official board documentation](https://www.wemos.cc/en/latest/d1_mini_shield/relay.html)

:information_source: D1 mini compatible board replacements work fine (tested)

## MQTT message payload

```
{
  "light_on": true,
  "red": 0,
  "green": 0,
  "blue": 0
}
```

```
{
  "message": "getConfig"
}
```

## Photo / Video Documentation

![screenshot 1:](./screenshots/photo1.jpg)

![screenshot 2:](./screenshots/photo2.jpg)

![screenshot 3:](./screenshots/photo3.jpg)

![recording 1 (AWS CLI integration):](./screenshots/OK.gif)

![recording 2 (AWS IoT MQTT client integration):](./screenshots/OK2.gif)

![recording 3 (self-served website integration):](./screenshots/OK3.gif)
