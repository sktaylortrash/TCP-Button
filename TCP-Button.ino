#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Bounce2.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// TCP Specific Variable Definitions.
const char* host = "lighting.local"; //TCPLightingWebInterface Host Name
const int httpsPort = 443;
const char* ButtonName = "Hallway";
String DeviceType = "Device"; // Room, Device or Scene 
String TCP_ID = "111111111"; //Set To TCPLightingWebInterface Device ID

//const int OnType = 1; //If on for room or device 0 = Last State, 1 = 100%

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "2d 55 4b dc 65 fa a7 4a a7 d0 28 52 03 85 6e 74 11 75 f0 6f";


// WiFi Definitions.
IPAddress ip(172, 16, 33, 153);
IPAddress gateway(172, 16, 33, 253);
IPAddress subnet(255, 255, 255, 0);
IPAddress DNS(172, 16, 33, 4);
const char* ssid = "SSID"; //Wi-Fi Network Name
const char* password = "PASSWORD"; //Wi-Fi Network Password
const char* localhost = "esphall"; // Defines mDNS host name .local is appended

// ESP8266 Variables
#define BUTTON_PIN D7
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
int ledState = HIGH;

//WebPage
String getPage(){
  String page = "<html><head>";
  page += "<title>TCPLightingWebInterface Button</title>";
  page += "</head><body><h1>TCPLightingWebInterface Button:";
  page += ButtonName;
  page += "</h1>";
  page += "<h3>Host Name:";
  page += localhost;
  page += ".local</h3>";
  page += "</body></html>";
  return page;
}

void handleRoot(){ 
    httpServer.send ( 200, "text/html", getPage() );
}

// Instantiate a Bounce object :
Bounce debouncer = Bounce(); 

void setup() {
  initHardware();
  connectWiFi();
  HTTPUpdateConnect();
}

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
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void switchoff() {
    // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  if (DeviceType == "Scene" || DeviceType == "scene") {
    String url = "/api.php?fx=scene&type=off&uid=" + TCP_ID;
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  } 
  else if (DeviceType == "Device" || DeviceType == "Device"){
    String url = "/api.php?fx=toggle&type=device&uid=" + TCP_ID + "&val=0";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
  else {
    String url = "/api.php?fx=toggle&type=room&uid=" + TCP_ID + "&val=0";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }

    Serial.println("request sent");
}

void switchon() {
    // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  if (DeviceType == "Scene" || DeviceType == "scene") {
    String url = "/api.php?fx=scene&type=on&uid=" + TCP_ID;
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  } 
  else if (DeviceType == "Device" || DeviceType == "Device"){
    String url = "/api.php?fx=dim&type=device&uid=" + TCP_ID + "&val=100";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
  else {
    String url = "/api.php?fx=dim&type=room&uid=" + TCP_ID + "&val=100";
    Serial.print("requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
  Serial.println("request sent");
}

void connectWiFi() {
  Serial.println();
  Serial.print("Initializing Button: ");
  Serial.println(ButtonName);
  Serial.println();
  Serial.println();
  WiFi.config(ip, gateway, subnet, DNS);
  delay(100);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void initHardware() {
  Serial.begin(115200);

  // Setup the button with an internal pull-up :
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  
  // After setting up the button, setup the Bounce instance :
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(200);

  // Setup the LED :
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,ledState);

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
     digitalWrite(LED_BUILTIN,ledState);
     Serial.println("An interrupt has occurred.");
     Serial.println(ledState);    
      if (ledState == 0) {
        switchon();
      }
      else{
        switchoff();
      }
   }
  
}

