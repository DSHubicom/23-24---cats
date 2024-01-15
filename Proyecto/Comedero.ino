#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h> // Include the Wi-Fi Manager library
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>

#define SS_PIN 21
#define RST_PIN 22

Servo myservo;
int pos = 0;
int weight;

String serverName = "http://morally-alert-polecat.ngrok-free.app/";

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

String requestPets() {
  HTTPClient http;
  String payload = "";

  String path = serverName + "pets";

  http.begin(path.c_str());

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

  http.end();

  return payload;
}

bool requestEat(String name) {
  HTTPClient http;
  String payload = "";

  String path = serverName + "feeding" + "?name=" + name;
  Serial.println("Requesting feeding to: " + path);

  http.begin(path.c_str());

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

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  JsonObject documentRoot = doc.as<JsonObject>();

  for (JsonPair keyValue : documentRoot) {
    Serial.println(keyValue.key().c_str());
  }

  String canEat = doc["canIt"];
  Serial.println(canEat);

  http.end();

  return canEat == String("true");
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
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
  // put your setup code here, to run once:
  Serial.begin(115200);

  // First initiate WiFi connection

  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  // wifiManager.resetSettings();

  // Sets timeout until configuration portal gets turned off
  // Useful to make it all retry or go to sleep in seconds
  wifiManager.setTimeout(180);

  // Starts an access point with the specified name if no known Wi-Fi network is present
  // and goes into a blocking loop, awaiting configuration
  if (!wifiManager.autoConnect("FeederCats")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  // If you get here you have connected to the Wi-Fi network
  Serial.println("Connection stablished");

  // Turn off the light until an RFID is passed near
  digitalWrite(LED_BUILTIN, LOW);

  // Then initiate all remaining stuff

  // Initiate servo
  myservo.attach(14);
  myservo.write(0);

  // RFID
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
}

void loop() {
  // put your main code here, to run repeatedly:
  weight = analogRead(A0);
  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.print(" - Position: ");
  Serial.println(pos);
  delay(1000);

  // If the door is open and there is too much food
  if (weight > 75 && pos == 180){
    // The door is closed
    for (pos = 180; pos > 0; pos -= 1) { // goes from 180 degrees to 0 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
  }
  // If there is no weight and the door is closed
  if (weight <= 75 && pos == 0){

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! rfid.PICC_IsNewCardPresent())
      return;

    // Verify if the NUID has been readed
    if ( ! rfid.PICC_ReadCardSerial())
      return;

    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));

    // Check is the PICC of Classic MIFARE type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("Your tag is not of type MIFARE Classic."));
      return;
    }

    // Read the RFID card data
    byte readCard[4];
    for (int i = 0; i < rfid.uid.size; i++) {
      readCard[i] = rfid.uid.uidByte[i];
    }

    Serial.print(F("New card detected: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    // Fetch pets data from the server
    DynamicJsonDocument doc(1024);
    String jsonString = requestPets();
    deserializeJson(doc, jsonString);

    // Extract pet information from the JSON
    JsonArray pets = doc["pets"];
    for (JsonObject pet : pets) {
      JsonArray rfidTag = pet["rfid"];
      byte targetCard[rfid.uid.size];
      for (int i = 0; i < rfid.uid.size; i++) {
        targetCard[i] = rfidTag[i];
      }

      // Check if the scanned card matches a pet's RFID tag
      if (compareRfidTags(readCard, targetCard)) {
        digitalWrite(LED_BUILTIN, HIGH);
        String name = pet["name"];
        if(requestEat(name)){
          Serial.print(name.c_str());
          Serial.println(" can eat");
          for (pos = 0; pos < 180; pos += 1) { // goes from 0 degrees to 180 degrees
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            delay(15);                       // waits 15ms for the servo to reach the position
          }
        }
        else {
          Serial.print(name.c_str());
          Serial.println(" can't eat yet");
        }
        digitalWrite(LED_BUILTIN, LOW);
        break;
      }
    }
  }
}
