/***************************************************
Predix Edge Sample 1

Cheap Temperature probe to MQTT to bus in Predix Edge 2.0 format
Data can be consumed by Predix Edge MQTT Adapter and sent to
Predix Time Series via Predix Edge Cloud Gateway

- Feb 2019 : Blayne Watt (blayne.watt@ge.com)

Released under MIT License

Arduino : ESP8266 NodeMCU
    $8.39 (Feb 2019)
    https://smile.amazon.com/gp/product/B010O1G1ES

Temperature Probe : Adafruit MCP9808
    $4.95 (Feb 2019)
    https://www.adafruit.com/product/1782
    
OLED Display : 128x64 SH1106 OLED Display
    $12.99 (Feb 2019) -- Optional
    https://smile.amazon.com/gp/product/B016HVG0MM


 ****************************************************/
 
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "Adafruit_MCP9808.h"
#include <WiFiUdp.h>
#include <EasyNTPClient.h>
#include <Arduino.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

// Create the MCP9808 temperature sensor object
//
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       ""
#define WLAN_PASS       ""

/************************* MQTT Broker Setup *********************************/

#define PE_SERVER      "192.168.1.212"
#define PE_SERVERPORT  9883 // custom for predix second mqtt
#define PE_USERNAME    ""
#define PE_KEY         ""

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
//
WiFiClient client;

// use WiFiFlientSecure for SSL
//
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, PE_SERVER, PE_SERVERPORT, PE_USERNAME, PE_KEY);

/****************************** Feeds ***************************************/

Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, "external_mqtt_data");

/*************************** Sketch Code ************************************/

WiFiUDP ntpUDP;

EasyNTPClient ntpClient(ntpUDP, "pool.ntp.org", 1); 

void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  u8x8.begin();
  u8x8.setPowerSave(0);

  Serial.println(F("MQTT temperature reader"));

  // Connect to WiFi access point.
  //
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,0,"WiFi connected");

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  ntpUDP.begin(123);

  // Make sure the MCP9808 is found
  //
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
    while (1);
  }
}

void loop() {
  int counter = 0;
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  char buf [1024];

  // Read and print out the temperature, then convert to °F
  float c = tempsensor.readTempC();
  float f = c * 9.0 / 5.0 + 32;
  int ts = ntpClient.getUnixTime();

  char bufDebug [80];
  sprintf(bufDebug, "Sending temperature val : %3.2f°C    %3.2f°F    at timestamp %d", c, f, ts);
  Serial.println(bufDebug);

  u8x8.drawString(0,0,"Temperature     ");
  String cString = String(" " + String(c) + " *C           ");
  cString.toCharArray(buf, cString.length()+1);
  u8x8.drawString(0,1,buf);
  memset(buf, 0, 1024);
  String fString = String(" " + String(f) + " *F           ");
  fString.toCharArray(buf, fString.length()+1);
  u8x8.drawString(0,2,buf);
  memset(buf, 0, 1024);

  sprintf (buf, "{ \"messageId\": \"%d\", \"body\":[{ \"name\":\"feather1.temp\", \"datapoints\":[[ %d, %f, 3 ]]}]}", counter, ntpClient.getUnixTime(), c);
  
  if (! temp.publish(buf)) {
    Serial.println(F("MQTT Publish Failed"));
  } else {
    Serial.println(F("MQTT Publish OK"));
  }
  memset(buf, 0, 1024);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  //
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
  counter++;
  delay(5000);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       u8x8.drawString(0,1,"MQTT failed");
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         Serial.println("Wait 30 seconds before retrying");
         // basically die and wait for WDT to reset me
         delay(30000);  // wait 30 seconds
         retries = 3; // reset to 3 retries
       }
  }
  Serial.println("MQTT Connected!");
  u8x8.drawString(0,1,"MQTT Connected!");
}
