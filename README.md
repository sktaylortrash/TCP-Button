# 1 & 2 Button/Switch connected through an esp8266 microcontroller

> Controls lights managed with https://github.com/bren1818/TCPLightingWebInterface

All test were performed with a ModeMCU development board but should be reproducable on most any cucuit built with an esp8266. The NodeMCU board was chosen as it already has a 5v --> 3.3v convertor and breadboad sized pinouts attached
![](https://github.com/sktaylortrash/TCP-Button/raw/master/Images/NodeMCUPNG-small.png)

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

`This Arduino Sketch is dependant on libraries from https://github.com/thomasfredericks/Bounce2/wiki`
