# 1 & 2 Button/Switch connected through an esp8266 microcontroller

> Controls lights managed with https://github.com/bren1818/TCPLightingWebInterface

`This Arduino Sketch is dependant on libraries from https://github.com/thomasfredericks/Bounce2/wiki`


The settings are defined by two JSON files:

## config.json 
```
{
  "ssid": "WIFI-Network-Name",
  "password": "WIFI-Password",
  "localhost": "HOSTNAME-Of-Device",
  "fwUrlBase": "http://lighting.local/firmware/", (hostname of server containing Firmware updates)
  "ip": [
    172,
    16,
    33,
    152
  ],
"gw": [
    172,
    16,
    33,
    253
  ],
  "sn": [
    255,
    255,
    255,
    0
  ],
  "ns": [
    172,
    16,
    33,
    4
  ]
}
```

