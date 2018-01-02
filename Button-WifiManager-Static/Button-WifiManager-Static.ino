#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <WiFiClientSecure.h>
#include <Bounce2.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1
char tcp_host[40] = "lighting.local"; //TCPLightingWebInterface Host Name
char https_port[] = "443";
char button_name[33] = "Button Name";
char device_type[7] = "Device"; // Room, Device or Scene
char tcp_id[20] = "1234567890987654321"; //Set To TCPLightingWebInterface Device ID
char localhost[20] = "tcpbutton"; // Defines mDNS host name .local is appended
// Use web browser to view and copy SHA1 fingerprint of the certificate
char fingerprint[60] = "2d 55 4b dc 65 fa a7 4a a7 d0 28 52 03 85 6e 74 11 75 f0 6f";
//default custom static IP
char static_ip[16] = "172.16.33.160";
char static_gw[16] = "172.16.33.253";
char static_sn[16] = "255.255.255.0";
char static_ns[16] = "172.16.33.4";

// ESP8266 Variables
#define BUTTON_PIN D7
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
int ledState = HIGH;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//WebPage
String getPage() {
  String page = "<html><head>";
  page += "<title>TCPLightingWebInterface Button</title>";
  page += "</head><body><h1>TCPLightingWebInterface Button:";
  page += button_name;
  page += "</h1>";
  page += "<h3>Host Name:";
  page += localhost;
  page += ".local</h3>";
  page += "<a href=http://";
  page += localhost;
  page += ".local/update>Update Firmware</a>";
  page += "</body></html>";
  return page;
}

void handleRoot() {
  httpServer.send ( 200, "text/html", getPage() );
}

// Instantiate a Bounce object :
Bounce debouncer = Bounce();

void HTTPUpdateConnect() {
  httpUpdater.setup(&httpServer);

  // Start TCP (HTTP) server
  httpServer.on ( "/", handleRoot );
  httpServer.begin();
  Serial.println("TCP server started");
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(localhost)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void switchoff() {
  int httpsport = atoi(https_port);
  String tcpid = tcp_id;

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(tcp_host);
  Serial.println(httpsport);
  if (!client.connect(tcp_host, httpsport)) {
    Serial.println("connection failed");
    return;
  }
  if (client.verify(fingerprint, tcp_host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  if (device_type == "Scene" || device_type == "scene") {
    String url = "/api.php?fx=scene&type=off&uid=" + tcpid;
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tcp_host + "\r\n" +
                 "User-Agent: TCPLightingEPS8266\r\n" +
                 "Connection: close\r\n\r\n");
  }
  else if (device_type == "Device" || device_type == "Device") {
    String url = "/api.php?fx=toggle&type=device&uid=" + tcpid + "&val=0";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tcp_host + "\r\n" +
                 "User-Agent: TCPLightingEPS8266\r\n" +
                 "Connection: close\r\n\r\n");
  }
  else {
    String url = "/api.php?fx=toggle&type=room&uid=" + tcpid + "&val=0";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tcp_host + "\r\n" +
                 "User-Agent: TCPLightingEPS8266\r\n" +
                 "Connection: close\r\n\r\n");
  }

  Serial.println("request sent");
}

void switchon() {
  int httpsport = atoi(https_port);
  String tcpid = tcp_id;

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(tcp_host);
  if (!client.connect(tcp_host, httpsport)) {
    Serial.println("connection failed");
    return;
  }
  if (client.verify(fingerprint, tcp_host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  if (device_type == "Scene" || device_type == "scene") {
    String url = "/api.php?fx=scene&type=on&uid=" + tcpid;
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tcp_host + "\r\n" +
                 "User-Agent: TCPLightingEPS8266\r\n" +
                 "Connection: close\r\n\r\n");
  }
  else if (device_type == "Device" || device_type == "Device") {
    String url = "/api.php?fx=dim&type=device&uid=" + tcpid + "&val=100";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tcp_host + "\r\n" +
                 "User-Agent: TCPLightingEPS8266\r\n" +
                 "Connection: close\r\n\r\n");
  }
  else {
    String url = "/api.php?fx=dim&type=room&uid=" + tcpid + "&val=100";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + tcp_host + "\r\n" +
                 "User-Agent: TCPLightingEPS8266\r\n" +
                 "Connection: close\r\n\r\n");
  }
  Serial.println("request sent");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Setup the button with an internal pull-up :
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // After setting up the button, setup the Bounce instance :
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(200);


  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(tcp_host, json["tcp_host"]);
          strcpy(https_port, json["https_port"]);
          strcpy(button_name, json["button_name"]);
          strcpy(device_type, json["device_type"]);
          strcpy(tcp_id, json["tcp_id"]);
          strcpy(localhost, json["localhost"]);
          strcpy(fingerprint, json["fingerprint"]);


          if (json["ip"]) {
            Serial.println("setting custom ip from config");
            //static_ip = json["ip"];
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
//            strcpy(static_ns, json["nameserver"]);
            //strcat(static_ip, json["ip"]);
            //static_gw = json["gateway"];
            //static_sn = json["subnet"];
            Serial.println(static_ip);
            /*            Serial.println("converting ip");
                        IPAddress ip = ipFromCharArray(static_ip);
                        Serial.println(ip);*/
          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  Serial.println(tcp_host);
  Serial.println(https_port);
  Serial.println(button_name);
  Serial.println(device_type);
  Serial.println(tcp_id);
  Serial.println(localhost);
  Serial.println(fingerprint);


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_tcp_host("host", "tcp host", tcp_host, 40);
  WiFiManagerParameter custom_https_port("port", "tcp port", https_port, 6);
  WiFiManagerParameter custom_button_name("bname", "button name", "button_name", 33);
  WiFiManagerParameter custom_device_type("dtype", "device type", "device_type", 7);
  WiFiManagerParameter custom_tcp_id("tcpid", "tcp id", "tcp_id", 40);
  WiFiManagerParameter custom_localhost("lhost", "local host", "localhost", 20);
  WiFiManagerParameter custom_fingerprint("fprint", "sha1 print", "fingerprint", 60);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //IPAddress _ip, _gw, _sn, _ns;
    IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  //_ns.fromString(static_ns);

//  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn, _ns);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  //add all your parameters here
  wifiManager.addParameter(&custom_tcp_host);
  wifiManager.addParameter(&custom_https_port);
  wifiManager.addParameter(&custom_button_name);
  wifiManager.addParameter(&custom_device_type);
  wifiManager.addParameter(&custom_tcp_id);
  wifiManager.addParameter(&custom_localhost);
  wifiManager.addParameter(&custom_fingerprint);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality(40);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("TCPButtonSetup", "lighting")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(tcp_host, custom_tcp_host.getValue());
  strcpy(https_port, custom_https_port.getValue());
  strcpy(button_name, custom_button_name.getValue());
  strcpy(device_type, custom_device_type.getValue());
  strcpy(tcp_id, custom_tcp_id.getValue());
  strcpy(localhost, custom_localhost.getValue());
  strcpy(fingerprint, custom_fingerprint.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["tcp_host"] = tcp_host;
    json["https_port"] = https_port;
    json["button_name"] = button_name;
    json["device_type"] = device_type;
    json["tcp_id"] = tcp_id;
    json["localhost"] = localhost;
    json["fingerprint"] = fingerprint;

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(localhost);
  HTTPUpdateConnect();

}

void loop() {
  // Start the webserver
  httpServer.handleClient();

  // Update the Bounce instance :
  debouncer.update();

  // Call code if Bounce fell (transition from HIGH to LOW) :
  if ( debouncer.fell() ) {

    // Toggle LED state :
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    Serial.println("An interrupt has occurred.");
    Serial.println(ledState);
    if (ledState == 0) {
      switchon();
    }
    else {
      switchoff();
    }
  }

}
