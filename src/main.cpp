#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define ssid      "Simple"
#define pass "AsdfghjkL"
// #define ssid "netis_FF6112"
// #define pass "password"

#define API_KEY "AIzaSyCwRlBYwaW0TV781ZA51UOxwD1vVRi-ALI"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "smart-agriculture-rmpi"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "user@user.com"
#define USER_PASSWORD "123456"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseData fbdoPump;

FirebaseAuth auth;
FirebaseConfig config;

DHT dht;

int moisture = 0;
float humidity = 0.0;
float temperature = 0.0;
int waterPumpPin = D5;
bool waterPumpSate = false;

void setup() {
  Serial.begin(9600);
  dht.setup(D3);
  pinMode(waterPumpPin, OUTPUT);
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
fbdoPump.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);

#endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);
 fbdoPump.setResponseSize(2048);
    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

}

void loop() {
  moisture = 1024 - analogRead(A0);
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();

  Serial.print("Moisture = ");
  Serial.print(moisture);
  Serial.print("\t Humidity = ");
  Serial.print(humidity);
  Serial.print("\t Temperature = ");
  Serial.println(temperature);



  FirebaseJson content;
  FirebaseJson pumpData;
  String documentPath = "IOT/data";
  String pumpDocumentPath = "IOT/pump";
  content.clear();
  content.set("fields/moisture/integerValue", moisture);
  content.set("fields/humidity/doubleValue", humidity);
  content.set("fields/temperature/doubleValue", temperature);

    if (moisture > 200 && waterPumpSate){
    waterPumpSate = false;
    digitalWrite(waterPumpPin, waterPumpSate);
    pumpData.clear();
  pumpData.set("fields/water/booleanValue", waterPumpSate);
    if (Firebase.Firestore.patchDocument(&fbdoPump, FIREBASE_PROJECT_ID, "", pumpDocumentPath.c_str(), pumpData.raw(), "water" /* updateMask */))
    {
      String res = String(fbdoPump.payload().c_str());
      Serial.printf("ok\n%s\n\n", res.c_str());
      fbdoPump.clear();
      
    }
  else{
      Serial.println("AAAA");
      Serial.println(fbdoPump.errorReason());
  }
  }
  digitalWrite(waterPumpPin, waterPumpSate);

  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "moisture,humidity,temperature" /* updateMask */)){
    String res = String(fbdo.payload().c_str());
    fbdo.clear();
    }
  else{
    Serial.println(fbdo.errorReason());
    
  }
  if (Firebase.Firestore.getDocument(&fbdoPump, FIREBASE_PROJECT_ID, "", pumpDocumentPath.c_str(), "water" /* updateMask */)){
    String res = String(fbdoPump.payload().c_str());
    // Serial.printf("ok\n%s\n\n", res.c_str());
    waterPumpSate = res.indexOf("true") > -1;
    if(waterPumpSate){
      moisture = 1024 - analogRead(A0);
      if(moisture > 200){
        waterPumpSate = false;
      }
    }
    digitalWrite(waterPumpPin, waterPumpSate);
    fbdoPump.clear();
  }

  else{
    Serial.println(fbdo.errorReason());
    
  }
  Serial.print("Pump = ");
  Serial.println(waterPumpSate ? "ON" : "OFF");
  digitalWrite(waterPumpPin, waterPumpSate);
}