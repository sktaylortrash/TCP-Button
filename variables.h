// Blank TCP Device Definitions
String spbttnNm = "";
String spdvcnm1 = "";
String spdvctyp1 = "";
String spdvcid1 = "";
String spdvcnm2 = "";
String spdvctyp2 = "";
String spdvcid2 = "";
String spfngrprnt = "";
String sphost = "";
int spport;
String spNdMdl = "";
String spNdMdlDesc = "";

// Blank WiFi Definitions.
String spssid = "";
String sppassword = "";
String splocalhost = ""; // Defines mDNS host name .local is appended
String spfwUrlBase = "";
//End Customization

//Blank IP Variables
int spip[4];
int spgw[4];
int spsn[4];
int spns[4];


// ESP8266 Variables
String url = "";
int devstate = LOW;
int devstate2 = LOW;
File fsUploadFile;              // a File object to temporarily store the received file
String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

// the timer object
SimpleTimer timer;

//Definitions used for the Up Time Logger
long Day=0;
int Hour =0;
int Minute=0;
int Second=0;
int HighMillis=0;
int Rollover=0;

// Instantiate a Bounce object :
Bounce debouncer1 = Bounce(); 

// Instantiate another Bounce object
Bounce debouncer2 = Bounce(); 


