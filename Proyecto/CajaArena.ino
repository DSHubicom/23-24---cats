// Captive Portal
#include <AsyncTCP.h>  //https://github.com/me-no-dev/AsyncTCP using the latest dev version from @me-no-dev
#include <DNSServer.h>
#include <ESPAsyncWebSrv.h>	//https://github.com/me-no-dev/ESPAsyncWebServer using the latest dev version from @me-no-dev
#include <esp_wifi.h>			//Used for mpdu_rx_disable android workaround

#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <M5StickCPlus.h>
#include <EEPROM.h>
// Pre reading on the fundamentals of captive portals https://textslashplain.com/2022/06/24/captive-portals/

/*Captive portal Wifi and http configuration*/
const char *ssid = "litterbox";  // FYI The SSID can't have a space in it
const char *password = "cats1234";  // no password

#define MAX_CLIENTS 2
#define WIFI_CHANNEL 6

#define START_ADDRESS 0
#define DEVICE_NAME_ADDRESS sizeof(bool)

const IPAddress localIP(8, 8, 4, 4);		   // the IP address the web server, Samsung requires the IP to be in public space
const IPAddress gatewayIP(8, 8, 4, 4);		   // IP address of the network should be the same as the local IP for captive portals
const IPAddress subnetMask(255, 255, 255, 0);

const String localIPURL = "http://cats";	 // a string version of the local IP with http, used for redirecting clients to your webpage
/*------------------------------------------*/


/*Bluetooth configuration*/

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

#define USE_SERIAL Serial

#define BT_DISCOVER_TIME	10000
/*-----------------------*/


/*MQ-2 sensor pin*/
#define MQ_PIN_A0 36
/*---------------*/
/*Speaker data*/
const int speaker_pin = 26;
int freq            = 50;
int ledChannel      = 0;
int resolution      = 10;
/*------------*/
std::string deviceName;  //Bluetooth device name that the user will input
bool atHome;  //Variable to check if the user is at home at the moment
int sensorValue; //MQ-2 value, it could be 0 or 1
bool start; //False is this device is on captive portal mode, true if the device knows the bluetooth device name and it starts discovering

const char index_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html>
    <head>
      <title>C.A.T.S | Cat Automated Treatment System</title>
      <style>
        body {
          background-color:#61BBFF;
          height: 100vh;
        }
        h1 {color: white;}
        h2 {color: white;}
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
      <h1>Welcome to C.A.T.S!</h1>
      <ul>
        <li><a href="/bluetooth">Add a new device</a></li>
      </ul>
    </body>
  </html>
)=====";

const char new_device_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html>
    <head>
      <title>Add new device - C.A.T.S | Cat Automated Treatment System</title>
      <style>
        body {
          background-color:#61BBFF;
          height: 100vh;
        }
        h1 {color: white;}
        h2 {color: white;}
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
      <h2>Please, enter your bluetooth device's name</h2>
      <form action="/bluetooth" method="post">
        <label for="name">Name: </label>
        <input type="text" id="name" name="name" required><br>
        <input type="submit">
      </form>
    </body>
  </html>
)=====";

const char saved_bluetooth_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html>
    <head>
      <title>Bluetooth device saved - C.A.T.S | Cat Automated Treatment System</title>
      <style>
        body {
          background-color:#61BBFF;
          height: 100vh;
        }
        h1 {color: white;}
        h2 {color: white;}
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
      <h2>You added a new device!</h2>
      <a href="/start">Start litter box</a>
    </body>
  </html>
)=====";

DNSServer dnsServer;
AsyncWebServer server(80);

void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 30

	// Set the TTL for DNS response and start the DNS server
	dnsServer.setTTL(3600);
	dnsServer.start(53, "*", localIP);
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
  // Define the maximum number of clients that can connect to the server
  #define MAX_CLIENTS 2
  // Define the WiFi channel to be used (channel 6 in this case)
  #define WIFI_CHANNEL 6

	// Set the WiFi mode to access point and station
	WiFi.mode(WIFI_MODE_AP);

	// Define the subnet mask for the WiFi network
	const IPAddress subnetMask(255, 255, 255, 0);

	// Configure the soft access point with a specific IP and subnet mask
	WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

	// Start the soft access point with the given ssid, password, channel, max number of clients
	WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

	// Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
	esp_wifi_stop();
	esp_wifi_deinit();
	wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
	my_config.ampdu_rx_enable = false;
	esp_wifi_init(&my_config);
	esp_wifi_start();
	vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay
}


void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
	//======================== Webserver ========================
	// WARNING IOS (and maybe macos) WILL NOT POP UP IF IT CONTAINS THE WORD "Success" https://www.esp8266.com/viewtopic.php?f=34&t=4398
	// SAFARI (IOS) IS STUPID, G-ZIPPED FILES CAN'T END IN .GZ https://github.com/homieiot/homie-esp8266/issues/476 this is fixed by the webserver serve static function.
	// SAFARI (IOS) there is a 128KB limit to the size of the HTML. The HTML can reference external resources/images that bring the total over 128KB
	// SAFARI (IOS) popup browserÂ has some severe limitations (javascript disabled, cookies disabled)

	// Required
	server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });	// windows 11 captive portal workaround
	server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });								// Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

	// Background responses: Probably not all are Required, but some are. Others might speed things up?
	// A Tier (commonly used by modern systems)
	server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });		   // android captive portal redirect
	server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // microsoft redirect
	server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });  // apple call home
	server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });	   // firefox captive portal call home
	server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					   // firefox captive portal call home
	server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // windows call home

	// return 404 to webpage icon
	server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });	// webpage icon

	// Serve Basic HTML Page
	server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
		response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);
		Serial.println("Served Basic HTML Page");
	});

  server.on("/bluetooth", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", new_device_html);
    response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);
		Serial.println("Bluetooth");
  });

    server.on("/bluetooth", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", saved_bluetooth_html);
    response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);
    deviceName = request->arg("name").c_str();
    for (int i = 0; i < deviceName.length(); i++) {
    EEPROM.write(DEVICE_NAME_ADDRESS + i, deviceName[i]);
    }
    EEPROM.write(DEVICE_NAME_ADDRESS + deviceName.length(), '\0');
  });

    server.on("/start", HTTP_GET, [&server](AsyncWebServerRequest *request) {
      start = true;

      // Save start to EEPROM
      EEPROM.write(START_ADDRESS, start);
      EEPROM.commit(); 
      EEPROM.end(); // Free up the RAM
      ESP.restart();
  });

	// the catch all
	server.onNotFound([](AsyncWebServerRequest *request) {
		request->redirect(localIPURL);
		Serial.print("onnotfound ");
		Serial.print(request->host());	// This gives some insight into whatever was being requested on the serial monitor
		Serial.print(" ");
		Serial.print(request->url());
		Serial.print(" sent redirect to " + localIPURL + "\n");
	});
}

void setup() {
	// Set the transmit buffer size for the Serial object and start it with a baud rate of 115200.
	Serial.setTxBufferSize(1024);
	Serial.begin(115200);
  EEPROM.begin(512);  // Initialize 512 bytes of EEPROM
  byte startByte = EEPROM.read(START_ADDRESS);
  if (startByte == 255) {
    start = false;  // First time, so go to the captive portal
  } else {
    start = bool(startByte);
  }

  delay(3000);

  Serial.println(start);

  // Read deviceName from EEPROM
  deviceName = "";  // Clear the deviceName string
  for (int i = 0; i < 20; i++) {  // Assuming deviceName is not longer than 20 characters
    char c = char(EEPROM.read(DEVICE_NAME_ADDRESS + i));
    if (c == '\0') {  // Stop reading at the null character
        break;
    }
    deviceName += c;
}

  Serial.println(deviceName.c_str());

	// Wait for the Serial object to become available.
	while (!Serial);

	// Print a welcome message to the Serial port.
	Serial.println("\n\nCaptive Portal, V0.5.0 compiled " __DATE__ " " __TIME__ " by CD_FER");  //__DATE__ is provided by the platformio ide
	Serial.printf("%s-%d\n\r", ESP.getChipModel(), ESP.getChipRevision());

  if(!start){
	  startSoftAccessPoint(ssid, password, localIP, gatewayIP);

	  setUpDNSServer(dnsServer, localIP);

	  setUpWebserver(server, localIP);
	  server.begin();

	  Serial.print("\n");
	  Serial.print("Startup Time:");	// should be somewhere between 270-350 for Generic ESP32 (D0WDQ6 chip, can have a higher startup time on first boot)
	  Serial.println(millis());
	  Serial.print("\n");
  }
  else{
    for (int i = 0; i < 512; i++) {  // Change 512 to the size of your EEPROM.
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    EEPROM.end(); // Free up the RAM
    pinMode(MQ_PIN_A0, INPUT); //Set the mq-2 pin
    pinMode(speaker_pin, INPUT);
    gpio_pulldown_dis(GPIO_NUM_25); //set the g25 to floating as the g36 is going to be used
    gpio_pullup_dis(GPIO_NUM_25);
    SerialBT.begin("litterbox"); //Bluetooth device name
    M5.begin();
    M5.Lcd.setRotation(0);
    M5.Lcd.setCursor(25, 80, 4);
    M5.Lcd.println("speaker");
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(speaker_pin, ledChannel);
    ledcWrite(ledChannel, 256);  // 0°
  }
}

void loop() {
  if(!start){
	  dnsServer.processNextRequest();	 // I call this atleast every 10ms in my other projects (can be higher but I haven't tested it for stability)
	  delay(DNS_INTERVAL);			 // seems to help with stability, if you are doing other things in the loop this may not be needed
  }
  else{
    int mqValue = analogRead(MQ_PIN_A0);

    Serial.println(mqValue);
  
    atHome = false;
    
    
    Serial.println("Starting discover...");
    BTScanResults *pResults = SerialBT.discover(BT_DISCOVER_TIME);
    if (pResults){
      for (int i = 0; i<pResults->getCount() && atHome == false;i++){
        BTAdvertisedDevice* device = pResults->getDevice(i);
        if(device->getName() == deviceName){
          Serial.println("Owner is at home");
          atHome = true;
        }
      }
    }  
    else{
      Serial.println("Error on BT Scan, no result!");
    }
  
    if(atHome && mqValue > 3000){
      Serial.println("Alarm activation");
      ledcWriteTone(ledChannel, 1250); 
  
    }
    else{
      Serial.println("Alarm deactivation");
      ledcWriteTone(ledChannel, 0); 
    } 
  
    delay(1000);
  }
}
