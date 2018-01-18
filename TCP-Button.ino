#include <FS.h> // Include the SPIFFS library
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Bounce2.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>
#include "variables.h"

const int FW_VERSION = 1046;
//Button GPIO Definitions
#define BUTTON_PIN1 D7
#define BUTTON_PIN2 D6

//Don't Change After Here Unless You Know What You Are Doing

void serveindex(){
  server.send ( 200, "text/html", IndexPage() );
}

void clearconfig() {
  ESP.eraseConfig();
  ESP.reset();
}

void reboot(){
  server.sendHeader("Location","/reboot.html");
  server.send(303);
  timer.setTimeout(1500, clearconfig);
}

void modelinfo() {
  if (spNdMdl == "BTN") {
    spNdMdlDesc = "1 Button Light Controller";
  }
  else if (spNdMdl == "2BTN"){
    spNdMdlDesc = "2 Button Light Controller";
  }
  else if (spNdMdl == "PIR"){
    spNdMdlDesc = "Motion Sensing Light Controller";
  }
  else {
    spNdMdlDesc = "Undefined Model";
  }
}

void serveinfo(){
  modelinfo();
  uptime();
  server.send ( 200, "text/html", InfoPage() );
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
  server.on("/off1", HTTP_GET, httpoff1);                 // Call the 'httpoff' function when a GET request is made to URI "/off"
  server.on("/on1", HTTP_GET, httpon1);                 // Call the 'httpon' function when a GET request is made to URI "/on"
  server.on("/off2", HTTP_GET, httpoff2);                 // Call the 'httpoff2' function when a GET request is made to URI "/off2"
  server.on("/on2", HTTP_GET, httpon2);                 // Call the 'httpon2' function when a GET request is made to URI "/on2"
  server.on("/info", HTTP_GET, serveinfo);
  server.on("/reboot", HTTP_GET, reboot);
  server.on("/", HTTP_GET, serveindex);
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
    });
  server.begin();
  Serial.println(F("HTTP server started"));
  // Set up mDNS responder:
  if (!MDNS.begin(splocalhost.c_str())) {
    Serial.println(F("Error setting up MDNS responder!"));
    while(1) { 
      delay(1000);
    }
  }
  Serial.println(F("mDNS responder started"));
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

bool handleFileRead(String path){  // send the right file to the client (if it exists)
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){  // If the file exists, either as a compressed archive, or normal
    if(SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
      path += ".gz";                                       // Use the compressed version
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
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
    Serial.print(F("handleFileUpload Name: ")); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print(F("handleFileUpload Size: ")); Serial.println(upload.totalSize);
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

void httpoff1() {                           // If a GET request is made to URI /off1
  switchoff(spdvctyp1, spdvcid1);
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
  devstate = !devstate;
}

void httpon1() {                             // If a GET request is made to URI /on1
  switchon(spdvctyp1, spdvcid1);
  server.sendHeader("Location","/");        
  server.send(303);                         
  devstate = !devstate;
}

void httpoff2() {                           // If a GET request is made to URI /off2
  switchoff(spdvctyp2, spdvcid2);
  server.sendHeader("Location","/");        
  server.send(303);                         
  devstate2 = !devstate2;
}

void httpon2() {                            // If a GET request is made to URI /on2
  switchon(spdvctyp2, spdvcid2);
  server.sendHeader("Location","/");        
  server.send(303);                         
  devstate2 = !devstate2;
}

void switchoff(String dtype, String did) {
  WiFiClientSecure client;
  if (!client.connect(sphost.c_str(), spport)) {
    Serial.println(F("connection failed"));
    return;
  }
  if (client.verify(spfngrprnt.c_str(), sphost.c_str())) {
  } else {
    Serial.println(F("certificate doesn't match"));
  }
  if (dtype == "Scene" || dtype == "scene") {
    String url = "/api.php?fx=scene&type=off&uid=" + did;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  } 
  else if (dtype == "Device" || dtype == "device"){
    String url = "/api.php?fx=toggle&type=device&uid=" + did + "&val=0";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
  else {
    String url = "/api.php?fx=toggle&type=room&uid=" + did + "&val=0";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
}


void switchon(String dtype, String did) {
    // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  if (!client.connect(sphost.c_str(), spport)) {
    Serial.println(F("connection failed"));
    return;
  }
  if (client.verify(spfngrprnt.c_str(), sphost.c_str())) {
  } else {
    Serial.println(F("certificate doesn't match"));
  }
  if (dtype == "Scene" || dtype == "scene") {
    String url = "/api.php?fx=scene&type=on&uid=" + did;
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  } 
  else if (dtype == "Device" || dtype == "device"){
    String url = "/api.php?fx=dim&type=device&uid=" + did + "&val=100";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
  else {
    String url = "/api.php?fx=dim&type=room&uid=" + did + "&val=100";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
  }
}


void connectWiFi() {
  IPAddress ip(spip[0], spip[1], spip[2], spip[3]);
  IPAddress gateway(spgw[0], spgw[1], spgw[2], spgw[3]);
  IPAddress subnet(spsn[0], spsn[1], spsn[2], spsn[3]);
  IPAddress DNS(spns[0], spns[1], spns[2], spns[3]);
  Serial.println();
  Serial.print(F("Initializing Button: "));
  Serial.println(spbttnNm);
  Serial.println();
  Serial.println();
  WiFi.config(ip, gateway, subnet, DNS);
  delay(100);
  Serial.println();
  Serial.print(F("connecting to "));
  Serial.println(spssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(spssid.c_str(), sppassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println("");
  Serial.print(F("WiFi connected to: "));
  Serial.println(WiFi.SSID());
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
}


void initHardware() {
  Serial.begin(115200);
  // Setup the buttons with an internal pull-up :
  pinMode(BUTTON_PIN1,INPUT_PULLUP);
  pinMode(BUTTON_PIN2,INPUT_PULLUP);
  // After setting up the button, setup the Bounce instances :
  debouncer1.attach(BUTTON_PIN1);
  debouncer1.interval(200);
  debouncer2.attach(BUTTON_PIN2);
  debouncer2.interval(200);
}

void uptime(){
//** Making Note of an expected rollover *****//   
  if(millis()>=3000000000){ 
    HighMillis=1;
  }
//** Making note of actual rollover **//
  if(millis()<=100000&&HighMillis==1){
    Rollover++;
    HighMillis=0;
  }
  long secsUp = millis()/1000;
  Second = secsUp%60;
  Minute = (secsUp/60)%60;
  Hour = (secsUp/(60*60))%24;
  Day = (Rollover*50)+(secsUp/(60*60*24));  //First portion takes care of a rollover [around 50 days]
};

String getMAC()
{
  uint8_t mac[6];
  char result[14];
  WiFi.macAddress(mac);

 snprintf( result, sizeof( result ), "%02x%02x%02x%02x%02x%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );

  return String( result );
}

void checkForUpdates() {
  String mac = getMAC();
  String fwURL = String( spfwUrlBase );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( mac );
  fwVersionURL.concat( ".version" );

  Serial.println(F( "Checking for firmware updates." ));
  Serial.print(F( "MAC address: " ));
  Serial.println( mac );
  Serial.print(F( "Firmware version URL: " ));
  Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    Serial.print(F( "Current firmware version: " ));
    Serial.println( FW_VERSION );
    Serial.print(F( "Available firmware version: " ));
    Serial.println( newFWVersion );

    int newVersion = newFWVersion.toInt();

    if( newVersion > FW_VERSION ) {
      Serial.println(F( "Preparing to update" ));

      String fwImageURL = fwURL;
      fwImageURL.concat( newFWVersion );
      fwImageURL.concat( ".bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
          break;
      }
    }
    else {
      Serial.println(F( "Already on latest version" ));
    }
  }
  else {
    Serial.print(F( "Firmware version check failed, got HTTP response code " ));
    Serial.println( httpCode );
  }
  httpClient.end();
}

void GetState() {
  WiFiClientSecure client;
  if (!client.connect(sphost.c_str(), spport)) {
    Serial.println(F("connection failed"));
    return;
  }
  if (client.verify(spfngrprnt.c_str(), sphost.c_str())) {
  } else {
    Serial.println(F("certificate doesn't match"));
  }
  if (spdvctyp1 == "Scene" || spdvctyp1 == "scene") {
    url = "/api.php?fx=getSceneState&uid=" + spdvcid1;
  }
  else if (spdvctyp1 == "Device" || spdvctyp1 == "device"){
    url = "/api.php?fx=getDeviceState&uid=" + spdvcid1;
  }
  else {
    url = "/api.php?fx=getRoomState&uid=" + spdvcid1;
  }
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
    while(client.connected()){
      String line = client.readStringUntil('\r');
      if (line.endsWith("1")) {
        devstate = 1;
      } 
      else {
        devstate = 0;
      }
    url = "";
     }
   Serial.print("Device1: ");
   Serial.println(devstate);
}

void GetState2() {
  WiFiClientSecure client;
  if (!client.connect(sphost.c_str(), spport)) {
    Serial.println(F("connection failed"));
    return;
  }
  if (client.verify(spfngrprnt.c_str(), sphost.c_str())) {
  } else {
    Serial.println(F("certificate doesn't match"));
  }
  if (spdvctyp2 == "Scene" || spdvctyp2 == "scene") {
    url = "/api.php?fx=getSceneState&uid=" + spdvcid2;
  }
  else if (spdvctyp2 == "Device" || spdvctyp2 == "device"){
    url = "/api.php?fx=getDeviceState&uid=" + spdvcid2;
  }
  else {
    url = "/api.php?fx=getRoomState&uid=" + spdvcid2;
  }
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + sphost.c_str() + "\r\n" +
               "User-Agent: TCPLightingEPS8266\r\n" +
               "Connection: close\r\n\r\n");
    while(client.connected()){
      String line = client.readStringUntil('\r');
      if (line.endsWith("1")) {
        devstate2 = 1;
      } 
      else {
        devstate2 = 0;
      }
    url = "";
     }
   Serial.print("Device2: ");
   Serial.println(devstate2);
  
}

void GetStates() {
  GetState();
  GetState2();
}

String InfoPage(){
  String page = "<html lang='en'><head><meta charset='utf-8' http-equiv='refresh' content='10'>";
  page += "<link rel='stylesheet' href='site.css'><script src='/w3.js'></script><title>";
  page += spbttnNm;
  page += F(" Button Controller</title></head><body>");
  page += F("<div class='main'>");
  page +=   F("<div class='header'>");
  page +=     "<img src='/logo-trans.png' class='headerimg'>";
  page +=   "</div>";
  page +=   "<div class='left'>";
  page +=     "<h3>Software Info</h3>";
  page +=     F("<dl>");
  page +=       F("<dt>Firmware Version</dt><dd>");
  page +=         FW_VERSION;
  page +=     F("</dd>");
  page +=       F("<dt>Model Version</dt><dd>");
  page +=         spNdMdlDesc;
  page +=     F("</dd></dl>");
  page +=     "<h3>Device Info</h3>";
  page +=     F("<dl>");
  page +=       F("<dt>Chip ID</dt><dd>");
  page +=         ESP.getChipId();
  page +=       F("</dd>");
  page +=       F("<dt>Flash Chip ID</dt><dd>");
  page +=         ESP.getFlashChipId();
  page +=       F("</dd>");
  page +=       F("<dt>IDE Flash Size</dt><dd>");
  page +=         ESP.getFlashChipSize();
  page +=       F(" bytes</dd>");
  page +=       F("<dt>Real Flash Size</dt><dd>");
  page +=         ESP.getFlashChipRealSize();
  page +=       F(" bytes</dd>");
  page +=       F("<dt>Uptime</dt><dd>");
  page +=         Day;
  page +=         F(" Days ");
  page +=         Hour;
  page +=         F(" Hours ");
  page +=         Minute;
  page +=         F(" Minutes ");
  page +=         Second;
  page +=         F(" Seconds");
  page +=       F("</dd></dl>");
  page +=     "<h3>Network Info</h3>";
  page +=     F("<dl>");
  page +=       F("<dt>Station MAC</dt><dd>");
  page +=         WiFi.macAddress();
  page +=       F("</dd>");
  page +=       F("<dt>IP Address</dt><dd>");
  page +=         spip[0];
  page +=       F(".");
  page +=         spip[1];
  page +=       F(".");
  page +=         spip[2];
  page +=       F(".");
  page +=         spip[3];
  page +=       F("</dd>");
  page +=       F("<dt>Default Gateway</dt><dd>");
  page +=         spgw[0];
  page +=       F(".");
  page +=         spgw[1];
  page +=       F(".");
  page +=         spgw[2];
  page +=       F(".");
  page +=         spgw[3];
  page +=       F("</dd>");
  page +=       F("<dt>Subnet Mask</dt><dd>");
  page +=         spsn[0];
  page +=       F(".");
  page +=         spsn[1];
  page +=       F(".");
  page +=         spsn[2];
  page +=       F(".");
  page +=         spsn[3];
  page +=       F("</dd>");
  page +=       F("<dt>DNS Server</dt><dd>");
  page +=         spns[0];
  page +=       F(".");
  page +=         spns[1];
  page +=       F(".");
  page +=         spns[2];
  page +=       F(".");
  page +=         spns[3];
  page +=       F("</dd>");
  page +=       F("<dt>SSID</dt><dd>");
  page +=         spssid;
  page +=       F("</dd>");
  page +=       F("<dt>Password</dt><dd>");
  page +=         sppassword;
  page +=       F("</dd>");
  page +=       F("<dt>HostName</dt><dd>");
  page +=         splocalhost;
  page +=       F(".local</dd></dl>");
  page +=     F("</div>");
  page +=     "<div id='wrapper'>";
  page +=     "<div><form action='/reboot'><input type='submit' class='btn-style' value='Reboot' /></form></div>";
  page +=     "<div id='flexcenter'>&nbsp</div>";
  page +=     "<div>&nbsp;</div>";
  page +=     "</div>";
  page +=     "<div w3-include-html='footer.html'></div> ";
  page +=   "</div>";
  page +=   "<script>w3.includeHTML();</script>";
  page += "</body></html>";
  return page;
};

String IndexPage(){
  String page = "<html lang='en'><head><meta charset='utf-8'>";
  page += "<link rel='stylesheet' href='site.css'><script src='/w3.js'></script><title>";
  page += spbttnNm;
  page += " Button Controller</title></head><body>";
  page += "<div class='main'>";
  page +=   "<div class='header'>";
  page +=     "<img src='/logo-trans.png' class='headerimg'>";
  page +=   "</div>";
  page +=   "<div class='left'>";
  page +=     "<h1>";
  page +=     spbttnNm;
  page +=     ": Button Controller</h1><h3>";
  page +=     spdvcnm1;
  page +=     "</h3><div id='wrapper'>";
  page +=     "<div><form action='/on1'><input type='submit' class='btn-style' value='Light On' /></form></div>";
  page +=     "<div id='flexcenter'>&nbsp</div>";
  page +=     "<div><form action='/off1'><input type='submit' class='btn-style' value='Light Off' /></form></div></div>";
  if (spNdMdl == "2BTN") {
  page +=     "<h3>";
  page +=     spdvcnm2;
  page +=     "</h3><div id='wrapper'>";
  page +=     "<div><form action='/on2'><input type='submit' class='btn-style' value='Light On' /></form></div>";
  page +=     "<div id='flexcenter'>&nbsp</div>";
  page +=     "<div><form action='/off2'><input type='submit' class='btn-style' value='Light Off' /></form></div></div>";
  }
  page +=   "</div><br>";
  page +=     "<div w3-include-html='footer.html'></div> ";
  page +=   "</div>";
  page +=   "<script>w3.includeHTML();</script>";
  page += "</body></html>";
  return page;
}


bool loadtcpConfig() {
  File tcpconfigFile = SPIFFS.open("/tcp.json", "r");
  if (!tcpconfigFile) {
    Serial.println(F("Failed to open TCP config file"));
    return false;
  }

  size_t size = tcpconfigFile.size();
  if (size > 1024) {
    Serial.println(F(" TCP Config file size is too large"));
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  tcpconfigFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println(F("Failed to parse TCP config file"));
    return false;
  }

  spbttnNm = json["bttnNm"].as<String>();
  spdvcnm1 = json["dvcnm1"].as<String>();
  spdvctyp1 = json["dvctyp1"].as<String>();
  spdvcid1 = json["dvcid1"].as<String>();
  spdvcnm2 = json["dvcnm2"].as<String>();
  spdvctyp2 = json["dvctyp2"].as<String>();
  spdvcid2 = json["dvcid2"].as<String>();
  spfngrprnt = json["fngrprnt"].as<String>();
  sphost = json["tcphost"].as<String>();
  spport = json["port"];
  spNdMdl = json["NdMdl"].as<String>();
  spfwUrlBase = json["fwUrlBase"].as<String>();
  
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("Failed to open config file"));
    return false;
  }
  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println(F("Config file size is too large"));
    return false;
  }
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println(F("Failed to parse config file"));
    return false;
  }
  spssid = json["ssid"].as<String>();
  sppassword = json["password"].as<String>();
  splocalhost = json["localhost"].as<String>();
 
  //IP Address
  spip[0]=json["ip"][0];
  spip[1]=json["ip"][1];
  spip[2]=json["ip"][2];
  spip[3]=json["ip"][3];

  //Gateway Address
  spgw[0]=json["gw"][0];
  spgw[1]=json["gw"][1];
  spgw[2]=json["gw"][2];
  spgw[3]=json["gw"][3];

  //DNS Address
  spns[0]=json["ns"][0];
  spns[1]=json["ns"][1];
  spns[2]=json["ns"][2];
  spns[3]=json["ns"][3];

  //Subnet Mask
  spsn[0]=json["sn"][0];
  spsn[1]=json["sn"][1];
  spsn[2]=json["sn"][2];
  spsn[3]=json["sn"][3];

  int ip0=json["ns"][0];
  int ip1=json["ns"][1];
  int ip2=json["ns"][2];
  int ip3=json["ns"][3];
  char ipa[15];
 return true;
}

void setup() {
  initHardware();
  SPIFFS.begin();                           // Start the SPI Flash Files System
  loadtcpConfig();
  loadConfig();    
  connectWiFi();
  HTTPUpdateConnect();
  timer.setInterval(600000, checkForUpdates);
  if (spNdMdl == "2BTN") {
    timer.setInterval(60000,  GetStates );
  }
  else {
  timer.setInterval(60000,  GetState );
  }
}

void loop() {
  // Start the webserver
  server.handleClient();
  timer.run();

  // Update the Bounce instance :
   debouncer1.update();
   debouncer2.update();
   
   // Call code if Bounce fell (transition from HIGH to LOW) :
   if ( debouncer1.fell() ) {
      if (devstate == 0) {
        switchon(spdvctyp1, spdvcid1);
        Serial.println(F("Turning On"));
        devstate = !devstate;
      }
      else{
        switchoff(spdvctyp1, spdvcid1);
        Serial.println(F("Turning Off"));
        devstate = !devstate;
      }
   }
   // Call code if Bounce2 fell (transition from HIGH to LOW) :
   if ( debouncer2.fell() ) {
      if (devstate2 == 0) {
        switchon(spdvctyp2, spdvcid2);
        Serial.println(F("Turning 2 On"));
        devstate2 = !devstate2;
      }
      else{
        switchoff(spdvctyp2, spdvcid2);
        Serial.println(F("Turning 2 Off"));
        devstate2 = !devstate2;
      }
   }
}

