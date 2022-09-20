#include "secrets.h" // file with my credentials

#include <ESP8266WiFi.h> 
#include <WiFiClientSecure.h>

//fast wifi connection https://pokewithastick.net/?page_id=72


const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* host = "api.pushbullet.com";
const char* pushBulletAPIKey = PUSHBULLET_API_KEY;
const int httpsPort = 443;

int count = 0;

// the MAC address of your wifi router 
uint8_t home_mac[6] = { 0xEC, 0xA8, 0x1F, 0xF9, 0xA6, 0x8F };   // **
int channel = 11;    // the wifi channel to be used              // **
// SHA1 fingerprint of the certificate
const char* fingerprint = "2C BC 06 10 0A E0 6E B0 9E 60 E5 96 BA 72 C5 63 93 23 54 B3"; //got it using https://www.grc.com/fingerprints.htm 

void setup() {
  Serial.begin(115200);
  int counter=0;
 
// Set your Static IP address
IPAddress local_IP(192, 168, 0, 109);
//IPAddress local_IP(192, 168, 0, 89); lolin
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


  //  Connect to WiFi network  
  //enableWiFiAtBootTime();  // required for release >= 3
  Serial.println(">> Door Alarm with PushBullet <<");
  Serial.println("Starting Wifi connection... ");
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  // so even though no WiFi.connect() so far, check and see if we are connecting.
  // The 1st time sketch runs, this will time-out and THEN it accesses WiFi.connect().
  // After the first time (and a successful connect), next time it connects very fast
  while (WiFi.status() != WL_CONNECTED) {
    delay(5);     // use small delays, NOT 500ms
    if (++counter > 1000) break;     // 5 sec timeout
  }
  //if timed-out, connect the slow-way
  if (counter > 1000) {
    Serial.println("Não conectou automaticamente, indo para o modo lento.");
    launchSlowConnect();
  }


  
  Serial.print("Connected do wifi! ");
  Serial.println(WiFi.localIP());
  
  //WiFiClient client;
  //const int httpPort = 80;

 // Use WiFiClientSecure class to create TLS connection
  //WiFiClientSecure client;
  BearSSL::WiFiClientSecure client; // dica aqui https://forum.arduino.cc/t/https-get-request-not-working/562932
  client.setInsecure();
  client.setTimeout(10000);
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Coundn't connect to host. Backing to deep sleep!");
    ESP.deepSleep(0);
    delay(2000);
  }
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  //if (!client.connect(host, httpPort)) {
  //  Serial.println("Coundn't connect to host. Backing to deep sleep!");
  //  ESP.deepSleep(0);
  //  delay(2000);
  //}

  //Read Battery Voltage
  int adcRead = 0; // value read from the battery
  adcRead = analogRead(A0);
  Serial.print("ADC: ");
  Serial.println(adcRead);
  float batteryVoltage = (float)adcRead;
  batteryVoltage = batteryVoltage / 178.1;  

  //Send data to an endpoint
  String url = "/v2/pushes";
  String messagebody = "{\"body\":\"A porta dos fundos foi aberta ou fechada. A voltagem da bateria é de "+(String)batteryVoltage+"V\",\"title\":\"ALARME Porta dos fundos\",\"type\":\"note\",\"channel_tag\":\"deteccao_porta\"}";
  Serial.println("Sending request... ");
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + pushBulletAPIKey + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " +
               String(messagebody.length()) + "\r\n\r\n");
  client.print(messagebody);
  Serial.println("request sent. ");

  //print the response

  while (client.available() == 0);
  Serial.println("passou pelo client.available() == 0");
  
  // comentei isso para reduzir o tempo
  //while (client.available()) {
  //  String line = client.readStringUntil('\n');
  //  Serial.println(line);
  //}

  
  Serial.println("Everything Ok. Going DeepSleep! ");
  ESP.deepSleep(0);
  delay(2000);
}



void launchSlowConnect() {
  Serial.println("No (or wrong) saved WiFi credentials. Doing a fresh connect.");
    // persistent and autoconnect should be true by default, but lets make sure.
  if (!WiFi.getAutoConnect()) WiFi.setAutoConnect(true);  // autoconnect from saved credentials
  if (!WiFi.getPersistent()) WiFi.persistent(true);     // save the wifi credentials to flash
 
  // Note the two forms of WiFi.begin() below. If the first version is used
  // then no wifi-scan required as the RF channel and the AP mac-address are provided.
  // so next boot, all this info is saved for a fast connect.
  // If the second version is used, a scan is required and for me, adds about 2-seconds
  // to my connect time. (on a cold-boot, ie power-cycle)
 
  WiFi.begin(ssid, password, channel, home_mac, true);
  //  WiFi.begin(ssid, password);
 
  // now wait for good connection, or reset
  int counter=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++counter > 20) {        // allow up to 10-sec to connect to wifi
        Serial.println("wifi timed-out. Rebooting..");
        delay(10);  // so the serial message has time to get sent
        Serial.println("Slow Wifi connection failed. Going DeepSleep! ");
        ESP.deepSleep(0);
        delay(2000);
    }
  }
  Serial.println("WiFi connected and credentials saved");
}


void loop() {
  
}
