#include <FS.h> // Include the SPIFFS library
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Bounce2.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Begin Customization Section
// TCP Specific Variable Definitions.
const char* host = "lighting.local"; //TCPLightingWebInterface Host Name
const int httpsPort = 443;
const char* ButtonName = "Jenny";
String DeviceType = "Scene"; // Room, Device or Scene 
String TCP_ID = "3068675309"; //Set To TCPLightingWebInterface Device ID

//const int OnType = 1; //If on for room or device 0 = Last State, 1 = 100%

// Use web browser to view and copy SHA1 fingerprint of the certificate
const char* fingerprint = "2d 55 4b dc 65 fa a7 4a a7 d0 28 52 03 85 6e 74 11 75 f0 6f";


// WiFi Definitions.
IPAddress ip(172, 16, 33, 154);
IPAddress gateway(172, 16, 33, 253);
IPAddress subnet(255, 255, 255, 0);
IPAddress DNS(172, 16, 33, 4);
const char* ssid = "......";
const char* password = "......";
const char* localhost = "jenny"; // Defines mDNS host name .local is appended

//End Customization

//Don't Change After Here Unless You Know What You Are Doing

// ESP8266 Variables
#define BUTTON_PIN1 D7
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
int ledState = HIGH;
File fsUploadFile;              // a File object to temporarily store the received file
String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS

// Instantiate a Bounce object :
Bounce debouncer = Bounce(); 

void setup() {
  initHardware();
  connectWiFi();
  SPIFFS.begin();                           // Start the SPI Flash Files System
  HTTPUpdateConnect();
}

void serveindex(){
  server.send ( 200, "text/html", IndexPage() );
}

void HTTPUpdateConnect() {  
  httpUpdater.setup(&server);

  // Start HTTP server
  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                    // Receive and save the file
  );
  server.on("/off", HTTP_GET, httpoff);                 // Call the 'httpoff' function when a GET request is made to URI "/off"
  server.on("/on", HTTP_GET, httpon);                 // Call the 'httpon' function when a GET request is made to URI "/on"
  server.on("/", HTTP_GET, serveindex);
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
    });
  server.begin();
  Serial.println("HTTP server started");
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

bool handleFileRead(String path){  // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
//  if(path.endsWith("/")) path += "index.html";           // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){  // If the file exists, either as a compressed archive, or normal
    if(SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);
  return false;                                          // If the file doesn't exist, return false
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}
String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void httpoff() {                          // If a GET request is made to URI /off
  ledState = !ledState;
  digitalWrite(LED_BUILTIN,ledState);
  switchoff();                              // Turn Light Off
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void httpon() {                          // If a GET request is made to URI /on
  ledState = !ledState;
  digitalWrite(LED_BUILTIN,ledState);
  switchon();                              // Turn Light On
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
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
  pinMode(BUTTON_PIN1,INPUT_PULLUP);
  
  // After setting up the button, setup the Bounce instance :
  debouncer.attach(BUTTON_PIN1);
  debouncer.interval(200);

  // Setup the LED :
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,ledState);

}

void loop() {
  // Start the webserver
  server.handleClient();
  
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

String IndexPage(){
  String page = "<html lang='en'><head><meta charset='utf-8'>";
  page += "<link rel='stylesheet' href='site.css'><title>";
  page += ButtonName;
  page += " Button Controller</title></head><body>";
  page += "<div class='main'>";
  page +=   "<div class='header'>";
  page +=     "<img src='/logo-trans.png' class='headerimg'>";
  page +=   "</div>";
  page +=   "<div class='left'>";
  page +=     "<br>&nbsp;<h1>";
  page +=     ButtonName;
  page +=     ": Button Controller</h1><br>";
  page +=     "<div id='wrapper'>";
  page +=     "<div><form action='/on'><input type='submit' class='btn-style' value='Light On' /></form></div>";
  page +=     "<div id='flexcenter'>&nbsp</div>";
  page +=     "<div><form action='/off'><input type='submit' class='btn-style' value='Light Off' /></form></div></div>";
  page +=     "<h3>Device Maintenance</h3>";
  page +=     "<a href='/update'>Upgrade Programming</a><br>";
  page +=     "<a href='/upload'>Upload File to Storage</a><br>&nbsp;";
  page += "</div></div>";
  page += "</body></html>";
  return page;
}
