#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebSrv.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <WiFi.h>

#define SS_PIN  21
#define RST_PIN 22
#define SERVO_PIN 17

unsigned long lastCardReadTime = 0;
const unsigned long cardReadInterval = 1500;  // Interval in milliseconds (1.5 seconds)

String serverName = "http://morally-alert-polecat.ngrok-free.app/";

// Captive portal address
const IPAddress localIP(8, 8, 4, 4);
const IPAddress gatewayIP(8, 8, 4, 4);
const IPAddress subnetMask(255, 255, 255, 0);

// WiFi credentials
String wifi_ssid = "";
String wifi_pass = "";

bool captivePortalUp = false;

const String localIPURL = "http://cats";      // a string version of the local IP with http, used for redirecting clients to your webpage

String  postNewCatsStringArray[5];
int postNewCatsStringArraySize = 0;

// DNS Server definition
DNSServer dnsServer;

// Async web server definition
AsyncWebServer server(80);

MFRC522 mfrc522(SS_PIN, RST_PIN);  // MFRC522 instance

// Micro servo definition
Servo myservo;
int pos = 90;

// Master RFID tag
const byte masterRfidTag[4] = {0xBC, 0xA0, 0x22, 0x79};

const char index_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html lang="en">
    <head>
      <title>C.A.T.S' Portal</title>
      <style>
          body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
          }

          #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
          }

          h1 {
              color: #333;
          }

          p {
              color: #555;
              margin-bottom: 20px;
          }

          .cat-image {
              max-width: 100%;
              border-radius: 8px;
              margin-bottom: 20px;
          }

          .cta-button {
              display: block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border-radius: 5px;
              transition: background-color 0.3s;
          }

          .cta-button:hover {
              background-color: #2980b9;
          }
        </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
      <div id="container">
        <h1>Welcome to C.A.T.S!</h1>
        <svg width="64px" height="64px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" aria-hidden="true" role="img" class="iconify iconify--noto" preserveAspectRatio="xMidYMid meet">
          <path d="M114.67 70.19C112.71 44.22 94.44 26.3 64 26.3S15.25 45.33 13.45 71.31c-1.05 15.14 4.58 28.63 15.91 36.32c7.46 5.07 17.88 7.88 34.77 7.88c17.18 0 27.03-3.71 34.49-8.73c12.43-8.35 17.18-21.67 16.05-36.59z" fill="#ffc022">
          </path>
          <path d="M53.72 42.6C46.3 23.4 30.1 10.34 23.87 8.39c-2.35-.74-5.3-.81-6.63 1.35c-3.36 5.45-7.66 22.95 1.85 47.78L53.72 42.6z" fill="#ffc022">
          </path>
          <path d="M36.12 34.21c1.54-1.29 2.29-2.55.6-5.16c-2.62-4.05-7.33-8.78-9.16-10.23c-3-2.38-5.32-3.18-6.21.65c-1.65 7.08-1.52 16.69.25 21.99c.62 1.87 2.54 2.86 4.02 1.57l10.5-8.82z" fill="#ffd1d1">
          </path>
          <path d="M54.12 45.02c1.13.96 3.42.82 4.75-.72c1.61-1.87 3.29-8.17 2.24-17.91c-4.67.17-9.09.84-13.21 1.97c3.33 5.46 4.13 14.88 6.22 16.66z" fill="#ff9b31">
          </path>
          <path d="M73.88 45.02c-1.13.96-3.42.82-4.75-.72c-1.61-1.87-3.29-8.17-2.24-17.91c4.67.17 9.09.84 13.21 1.97c-3.33 5.46-4.13 14.88-6.22 16.66z" fill="#ff9b31">
          </path>
          <path d="M79.9 29.22c8.08-12.41 19.38-20.75 24.07-22.24c2.32-.74 5.02-.62 6.34 1.55c3.32 5.45 6.13 22.24-.42 45.75L85.96 42.74L79.9 29.22z" fill="#ffc022">
          </path>
          <path d="M97.55 38.23c2.43 2.43 4.41 4.06 5.84 5.61c.95 1.03 2.69.56 2.97-.82c2.45-11.8 1.67-21.86 0-24.5c-.8-1.26-2.29-1.59-3.65-1.13c-2.44.81-8.66 5.45-13.05 12.22c-.51.79-.32 1.85.46 2.38c1.58 1.07 4.34 3.14 7.43 6.24z" fill="#ffd1d1">
          </path>
          <path d="M55.67 77.75c-.05-3.08 4.37-4.55 8.54-4.62c4.18-.07 8.68 1.29 8.73 4.37c.05 3.08-5.22 7.13-8.54 7.13c-3.31 0-8.67-3.81-8.73-6.88z" fill="#000000">
          </path>
          <g fill="none" stroke="#9e9e9e" stroke-width="3" stroke-linecap="round" stroke-miterlimit="10">
              <path d="M6.7 71.03c.34.41 4.41.35 14.36 5.07">
              </path>
              <path d="M2.9 82.86s6.42-2.24 17.46-.28">
              </path>
              <path d="M8.81 92.29s2.74-1.38 12.67-2.25">
              </path>
              <g>
                  <path d="M120.87 67.51s-3.41.33-13.94 6.34">
                  </path>
                  <path d="M122.42 78.49s-5.09-.36-16.05 1.97">
                  </path>
                  <path d="M120.45 89.05s-4.83-1.71-14.78-2.25">
                  </path>
              </g>
          </g>
          <path d="M96.09 66.37c-.34 5.51-3.76 8.54-7.65 8.54s-7.04-3.88-7.04-8.66s3.28-8.71 7.65-8.47c5.07.29 7.32 4.09 7.04 8.59z" fill="#000200">
          </path>
          <path d="M46 65.81c.78 5.61-1.58 9.03-5.49 9.82c-3.91.79-7.26-1.84-8.23-6.64c-.98-4.81.9-9.32 5.34-9.97c5.15-.75 7.74 2.2 8.38 6.79z" fill="#000200">
          </path>
          <path d="M44.99 85.16c-2.57 1.67.47 5.54 2.25 6.85c1.78 1.31 4.98 2.92 9.67 2.44c5.54-.56 7.13-4.69 7.13-4.69s1.97 4.6 8.82 4.79c6.95.19 9.1-3.57 10.04-4.69c.94-1.13 1.88-4.04.28-5.16c-1.6-1.13-2.72.28-4.41 2.63c-1.69 2.35-5.16 3.66-8.54 2.06s-3.57-7.04-3.57-7.04l-4.79.28s-.75 4.69-2.91 6.19s-7.32 1.88-9.48-1.41c-.95-1.46-2.33-3.66-4.49-2.25z" fill="#000000">
          </path>
        </svg>
        <p>What do you want to do?</p>
        <a href="wifi-config" class="cta-button">Configure WiFi</a>
        <a href="new-cat" class="cta-button">Add a new cat</a>
        <a href="/exit" class="cta-button">Exit</a>
      </div>
    </body>
  </html>
)=====";

const char wifi_config[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WiFi Configuration</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            form {
              display: flex;
              flex-direction: column;
              align-items: center;
              margin-top: 20px;
            }

            label {
              display: block;
              margin-bottom: 8px;
              color: #555;
            }

            input {
              width: 100%;
              padding: 8px;
              margin-bottom: 20px;
              border: 1px solid #ccc;
              border-radius: 5px;
              box-sizing: border-box;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }

            .cta-button-reset {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #737373;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button-reset:hover {
              background-color: #B4B4B4;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>WiFi Configuration</h1>
          <form method="get" action="/saved-wifi-config">
              <label for="ssid">WiFi SSID:</label>
              <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi SSID" required>
              <label for="password">WiFi Password:</label>
              <input type="password" id="password" name="password" placeholder="Enter WiFi Password" required>
              <button type="submit" class="cta-button">Connect to WiFi</button>
              <button type="reset" class="cta-button-reset">Reset</button>
          </form>
      </div>
    </body>
  </html>
)=====";

const char saved_wifi_config[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WiFi Configuration</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>WiFi Configuration</h1>
          <p>New WiFi configuration successfully saved. Closing captive portal...</p>
          <a href="/" class="cta-button">Home</a>
      </div>
    </body>
  </html>
)=====";

const char new_cat_html[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>New cat</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            form {
              display: flex;
              flex-direction: column;
              align-items: center;
              margin-top: 20px;
            }

            label {
              display: block;
              margin-bottom: 8px;
              color: #555;
            }

            input {
              width: 100%;
              padding: 8px;
              margin-bottom: 20px;
              border: 1px solid #ccc;
              border-radius: 5px;
              box-sizing: border-box;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }

            .cta-button-reset {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #737373;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button-reset:hover {
              background-color: #B4B4B4;
            }

            .rfid-input {
              width: calc(20% - 4px);
              margin-right: 4px;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>New cat</h1>
          <form method="post" action="/new-cat">
              <label for="catname">Name:</label>
              <input type="text" id="catname" name="catname" placeholder="Enter cat name" required>
              <label for="rfidTag">RFID Tag:</label>
              <div>
                <input type="number" class="rfid-input" id="rfid1" name="rfid1" placeholder="1" min="0" max="255" required>
                <input type="number" class="rfid-input" id="rfid2" name="rfid2" placeholder="2" min="0" max="255" required>
                <input type="number" class="rfid-input" id="rfid3" name="rfid3" placeholder="3" min="0" max="255" required>
                <input type="number" class="rfid-input" id="rfid4" name="rfid4" placeholder="4" min="0" max="255" required>
              </div>
              <button type="submit" class="cta-button">Save cat</button>
              <button type="reset" class="cta-button-reset">Reset</button>
          </form>
      </div>
    </body>
  </html>
)=====";

const char saved_cat_html[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WiFi Configuration</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>Cat added!</h1>
          <p>You have added a new cat. Press the button below to return to the home page.</p>
          <a href="/" class="cta-button">Home</a>
      </div>
    </body>
  </html>
)=====";

const char error_save_cat_html[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WiFi Configuration</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>Error!</h1>
          <p>An error occurred while trying to add the new cat. Press the button below to return to the home page.</p>
          <a href="/" class="cta-button">Home</a>
      </div>
    </body>
  </html>
)=====";

const char error_save_cat_cap_html[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WiFi Configuration</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>Error!</h1>
          <p>In order to add a new cat you have to reset the device. Press the button below to return to the home page.</p>
          <a href="/" class="cta-button">Home</a>
      </div>
    </body>
  </html>
)=====";

const char logout[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WiFi Configuration</title>
        <style>
            body {
              font-family: 'Arial', sans-serif;
              background-color: #f5f5f5;
              margin: 0;
              padding: 0;
              display: flex;
              align-items: center;
              justify-content: center;
              height: 100vh;
            }

            #container {
              text-align: center;
              padding: 20px;
              background-color: #fff;
              border-radius: 10px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            }

            h1 {
              color: #333;
            }

            .cta-button {
              display: inline-block;
              padding: 10px 20px;
              margin: 6px 0px;
              font-size: 16px;
              text-decoration: none;
              background-color: #3498db;
              color: #fff;
              border: none;
              border-radius: 5px;
              transition: background-color 0.3s;
              outline: none;
            }

            .cta-button:hover {
              background-color: #2980b9;
            }
        </style>
    </head>
    <body>
      <div id="container">
          <h1>Bye!</h1>
          <p>Leaving the portal...</p>
      </div>
    </body>
  </html>
)=====";

// Function to send a POST request with JSON payload to the server
int postJSON(String payload) {
  HTTPClient http;   
  String path = serverName + "pets";
     
  http.begin(path);  
  http.addHeader("Content-Type", "application/json");

  // Print the JSON payload to the Serial Monitor
  Serial.println(payload);

  // Send the POST request and return the HTTP response code
  return http.POST(payload);
}

// Function to save new cat data received from the web server
void saveNewCatData(AsyncWebServerRequest *request) {
  // Create a JSON document for the new cat data
  DynamicJsonDocument newCat(1024);

  // Set the cat's name from the HTTP request parameters
  newCat["name"] = request->getParam(0)->value();

  // Create a nested array for the RFID tag
  JsonArray rfidArray = newCat.createNestedArray("rfid");

  // Extract RFID values from the request parameters and add them to the array
  rfidArray.add(request->getParam(1)->value());
  rfidArray.add(request->getParam(2)->value());
  rfidArray.add(request->getParam(3)->value());
  rfidArray.add(request->getParam(4)->value());

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(newCat, jsonString);

  // Clear the JSON document and store the JSON string for later use
  newCat.clear();
  postNewCatsStringArray[postNewCatsStringArraySize] = jsonString;
  postNewCatsStringArraySize++;
}

// Function to set up the DNS server
void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
  // Define the DNS interval in milliseconds between processing DNS requests
  #define DNS_INTERVAL 30

	// Set the TTL for DNS response and start the DNS server
	dnsServer.setTTL(3600);
	dnsServer.start(53, "*", localIP);
}

// Function to set up the web server with various routes and redirects
void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
  // Define various routes and redirects for the captive portal
	server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });	// windows 11 captive portal workaround
	server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });

	server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });		      // android captive portal redirect
	server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			        // microsoft redirect
	server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });   // apple call home
	server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });	      // firefox captive portal call home
	server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					            // firefox captive portal call home
	server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			        // windows call home

	// return 404 to webpage icon
	server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });

  // Set up routes for different web pages and handle requests

  // Serve the basic HTML page for the default route
	server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
		response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);
		Serial.println("Served Basic HTML Page");
	});

  // Serve the WiFi configuration page
  server.on("/wifi-config", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", wifi_config);
    response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);
		Serial.println("WiFi configuration page");
  });

  // Serve the saved WiFi configuration page and save new configuration if provided
  server.on("/saved-wifi-config", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", saved_wifi_config);
    response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);

    // Wait for 2 seconds before processing parameters (configuring delay)
    delay(2000);

    // Check if there are parameters in the request
    if(request->params() > 0) {
      // Extract WiFi SSID and password from parameters
      wifi_ssid = request->getParam(0)->value();
      wifi_pass = request->getParam(1)->value();
      captivePortalUp = false;
      digitalWrite(LED_BUILTIN, LOW);
    }

    Serial.println("WiFi config saved");
  });

  // Serve the new cat HTML page
  server.on("/new-cat", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", new_cat_html);
    response->addHeader("Cache-Control", "public,max-age=31536000");
		request->send(response);
		Serial.println("New pet HTML Page");
  });

  // Handle the form submission for adding a new cat
  server.on("/new-cat", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response;

    // Check if the number of saved cats is less than 5
    if (postNewCatsStringArraySize < 5) {
      // Save new cat data and serve the saved cat HTML page
      saveNewCatData(request);
      response = request->beginResponse(200, "text/html", saved_cat_html);
      Serial.println("Saved cat HTML Page");
    }
    else {
      // Serve an error page if the limit of saved cats is reached
      response = request->beginResponse(200, "text/html", error_save_cat_cap_html);
      Serial.println("Error save cat cap HTML Page");
    }

    // Add cache control header and send the response
    response->addHeader("Cache-Control", "public,max-age=31536000");
    request->send(response);
  });

  // Serve the exit HTML page and deactivate the captive portal
  server.on("/exit", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", logout);
    Serial.println("Exit HTML Page");
    response->addHeader("Cache-Control", "public,max-age=31536000");
    request->send(response);
    captivePortalUp = false;
    digitalWrite(LED_BUILTIN, LOW);
  });

  // Handle not found requests by redirecting to the local IP
	server.onNotFound([](AsyncWebServerRequest *request) {
		request->redirect(localIPURL);
		Serial.print("onnotfound ");
		Serial.print(request->host());
		Serial.print(" ");
		Serial.print(request->url());
		Serial.print(" sent redirect to " + localIPURL + "\n");
	});
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
  // Define the maximum number of clients that can connect to the server
  #define MAX_CLIENTS 4
  // Define the WiFi channel to be used (channel 6 in this case)
  #define WIFI_CHANNEL 6

	// Set the WiFi mode to access point and station
	WiFi.mode(WIFI_MODE_AP);

	// Configure the soft access point with a specific IP and subnet mask
	WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

	// Start the soft access point with the given ssid, password, channel, max number of clients
	WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);
}

// Function to start the captive portal
void startCaptivePortal() {
  // Wait until Serial is ready
  while (!Serial);

  // Captive portal ssid and password
  const char *ssid = "CaptiveCats";
  const char *password = "cats1234";
  const IPAddress subnetMask(255, 255, 255, 0);

  // Start the soft access point for captive portal
	startSoftAccessPoint(ssid, password, localIP, gatewayIP);

  // Set up DNS server for captive portal
	setUpDNSServer(dnsServer, localIP);

  // Set up web server for captive portal
	setUpWebserver(server, localIP);
	server.begin();

  // Turn on built-in LED to indicate the captive portal is up
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Set captivePortalUp flag to true
  captivePortalUp = true;

  // Print startup information
	Serial.print("\n");
  Serial.print("Starting captive test.");
	Serial.print("\n");
	Serial.print("Startup Time: ");	// should be somewhere between 270-350 for Generic ESP32 (D0WDQ6 chip, can have a higher startup time on first boot)
	Serial.print(millis());
	Serial.print("\n");
}

// Function to wait for a WiFi connection and handle captive portal activation
void waitForConnection() {
  // Try to connect to WiFi for up to 10 seconds
  for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
    delay(1000);
    Serial.print(".");
  }

  // If not connected, start the captive portal
  if (WiFi.status() != WL_CONNECTED) {
    startCaptivePortal();
  }
  // If connected, print the IP address and perform additional actions
  else {
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    // Blink the built-in LED to indicate successful connection
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    // Set captivePortalUp flag to false
    captivePortalUp = false;

    // If there are new cat data to post, iterate and send POST requests
    if (postNewCatsStringArraySize != 0) {
      for(int i = 0; i < postNewCatsStringArraySize; i++) {
        Serial.print("***POSTING ADDED CATS***");
        postJSON(postNewCatsStringArray[i]);
      }
      postNewCatsStringArraySize = 0;
    }
  }
}

// Function to initialize WiFi in station mode and connect to a saved network
void initWiFi() {
  // Set WiFi mode to station and disconnect
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();

  // Print connecting message
  Serial.println("Connecting...");

  // Begin connecting to WiFi
  WiFi.begin();

  // Wait for connection
  waitForConnection();
}

// Function to initialize WiFi with custom credentials
void initWiFi(String ssid, String pass) {
  // Set WiFi mode to station and disconnect
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();

  // Print connecting message
  Serial.println("Connecting...");

  // Begin connecting to WiFi with custom credentials
  WiFi.begin(ssid, pass);

  // Wait for connection
  waitForConnection();
}

// Function to send a GET request to the server and retrieve pets data
String requestPets() {
  // Initialize HTTP client and payload string
  HTTPClient http;
  String payload = "";

  // Create the path for the GET request
  String path = serverName + "pets";

  // Begin the HTTP request
  http.begin(path.c_str());

  // Perform the GET request and handle the response
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.print("Response: ");
    Serial.println(payload);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // End the HTTP request
  http.end();

  // Return the received payload
  return payload;
}

// Function to compare two RFID tags
bool compareRfidTags(const byte* tag1, const byte* tag2) {
  // Compare each byte of the RFID tags
  for (int i = 0; i < 4; i++) {
    if (tag1[i] != tag2[i]) {
      return false;
    }
  }
  // If all bytes are equal, return true
  return true;
}

void setup() {
  // Begin serial communication
	Serial.begin(9600);
  while(!Serial);

  // Set pin mode for built-in LED
  pinMode(LED_BUILTIN, OUTPUT);

  // Servo setup
	myservo.setPeriodHertz(50);
	myservo.attach(SERVO_PIN, 500, 2400);
  myservo.write(pos);

  digitalWrite(LED_BUILTIN, LOW);

  // WiFi connection
  initWiFi();

  // Init SPI bus
	SPI.begin();

  // Init MFRC522
	mfrc522.PCD_Init();
	delay(4);
	mfrc522.PCD_DumpVersionToSerial();
}

void loop() {
  // Check if captive portal is active
  if (captivePortalUp) {
    dnsServer.processNextRequest();
	  delay(DNS_INTERVAL);
    return;
  }

  // Check if WiFi is not connected
  if(WiFi.status() != WL_CONNECTED) {
    if(wifi_ssid != "" && wifi_pass != "")
      initWiFi(wifi_ssid, wifi_pass);
    else
      initWiFi();
    return;
  }

  if (millis() - lastCardReadTime < cardReadInterval) {
    return;
  }

  lastCardReadTime = millis();

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if (!mfrc522.PICC_IsNewCardPresent())
		return;

	// Select one of the cards
	if (!mfrc522.PICC_ReadCardSerial())
		return;

  // Read the RFID card data
  byte readCard[4];
  for (int i = 0; i < mfrc522.uid.size; i++) {
    readCard[i] = mfrc522.uid.uidByte[i];
  }

  // Check if the scanned card matches the master RFID tag
  if (compareRfidTags(readCard, masterRfidTag)) {
    WiFi.disconnect();
    startCaptivePortal();
  }
  else {
    // Fetch pets data from the server
    DynamicJsonDocument doc(1024);
    String jsonString = requestPets();
    deserializeJson(doc, jsonString);

    // Extract pet information from the JSON
    JsonArray pets = doc["pets"];
    for (JsonObject pet : pets) {
      JsonArray rfidTag = pet["rfid"];
      byte targetCard[mfrc522.uid.size];
      for (int i = 0; i < mfrc522.uid.size; i++) {
        targetCard[i] = rfidTag[i];
      }

      // Check if the scanned card matches a pet's RFID tag
      if (compareRfidTags(readCard, targetCard)) {
        digitalWrite(LED_BUILTIN, HIGH);

        // Move the servo from 90 to 180 degrees
        for (pos = 90; pos <= 180; pos += 1) {
		      myservo.write(pos);
		      delay(15);
	      }

        // Print pet information
        Serial.print("Hello ");
        String petName = pet["name"];
        Serial.print(petName);
        Serial.println("!");

        // Delay for 5 seconds
        delay(5000);

        // Move the servo from 180 to 90 degrees
        for (pos = 180; pos >= 90; pos -= 1) {
		      myservo.write(pos);
		      delay(15);
	      }

        // Turn off the LED
        digitalWrite(LED_BUILTIN, LOW);

        // Exit the loop once the pet is found
        break;
      }
    }

    // Clear the JSON document
    doc.clear();
  }
}
