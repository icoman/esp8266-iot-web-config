/*
 * 
 * ESP8266 demo iot device.
 * 
 * Config over web interface.
 * Config stored as json in SPIFFS.
 * 
 * 
 */

#define DEFAULT_AP_SSID "esp8266wifi"
#define DEFAULT_AP_PASSWD "12345678"

//list of params stored as json in SPIFFS
char *params[] = {"ssid","passwd",//client wireless
        "host","port","wkey","rkey", //iot server, write key, read key
        "tag","rate",NULL}; //channel name, update rate in milli seconds

//press the Flash button to enter in AP mode
const int BUTTON_FLASH = 0;
const int LED = 16; //GPIO16


#include "webcfg.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

int httpsPort;
const char *ssid, *password;
unsigned long int rate = 0;
const char *host, *tag;
const char *write_key, *read_key;

unsigned long int valuescount = 0;
bool flagAP = false, flagLED = LOW;


void blinkLED(){
  for(int i=0;i<100;i++){
    delay(30);
    digitalWrite(LED, flagLED);
    flagLED=!flagLED;
  }
  digitalWrite(LED, HIGH); //LED off
}

void setup(void){
  delay(1000);
  pinMode ( LED, OUTPUT );
  digitalWrite(LED, HIGH); //LED off
  Serial.begin(115200);
  Serial.println("Start.");
  Serial.println("Press the Flash button to enter in AP mode for config.");
  //Serial.print("size int = ");Serial.println(sizeof(int)); //4 bytes
  //Serial.print("size long int = ");Serial.println(sizeof(long int)); //4 bytes
  pinMode (BUTTON_FLASH, INPUT);
  flagAP = 0;
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  delay(1000);
  bool have_config = loadConfig();
  if(!have_config) {
    //force start AP
    flagAP = 1;
    blinkLED();
    start_AP_config(DEFAULT_AP_SSID, DEFAULT_AP_PASSWD);
    return;
  }
  JsonObject root = doc.as<JsonObject>();
  host = (const char *) root["host"];
  if(0 != strlen(host)) {
    httpsPort = atoi(root["port"]);
    //rate = atol(root["rate"] | "10000");
    rate = atol(root["rate"]);
    ssid = (const char *) root["ssid"];
    password = (const char *) root["passwd"];
    write_key = (const char *) root["wkey"];
    read_key = (const char *) root["rkey"];
    tag = (const char *) root["tag"];
    Serial.print("connecting to "); Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      digitalWrite(LED, flagLED);
      flagLED=!flagLED;
      Serial.print(".");
      if (0 == digitalRead(BUTTON_FLASH)){
        if(!flagAP)
        {
          flagAP = 1;
          blinkLED();
          start_AP_config(DEFAULT_AP_SSID, DEFAULT_AP_PASSWD);
          return;
        }
      }
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else Serial.println("No valid config.");
}




uint8_t sendValues(){
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return 0;
  }

/*
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
*/

  String url = "/set/";
  url += tag;
  valuescount++;
  char mybuffer[500];
  //if "datetime" field is not set, then the iot server will provide a value
  sprintf(mybuffer, "?t1=%lu&t2=%lu&datetime=&millis=%lu&cnt=%lu", (unsigned long)random(10, 90),
    (unsigned long)random(1000, 9000), (unsigned long)millis(), (unsigned long)valuescount);
  //Serial.print("buff len=");Serial.println(strlen(mybuffer)); //under 55 bytes
  url += mybuffer;
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "WKEY: " + write_key + "\r\n" +
               "RKEY: " + read_key + "\r\n" +
               "User-Agent: MyNodeMCU_ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("headers received");
      break;
    }
  }
  //my iot server reply with an json document
  String line = client.readStringUntil('}');
  /*
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
  */
  return 1;
}


unsigned long int cnt = 0;
void loop(void){
  delay(1);
  if(rate<10000) rate = 10000;
  if(flagAP) server.handleClient();
  if (0 == digitalRead(BUTTON_FLASH)){
    if(!flagAP){
      blinkLED();
      start_AP_config(DEFAULT_AP_SSID, DEFAULT_AP_PASSWD);
    }
    flagAP = 1;
  }

  if(0 == cnt % rate){
    if(flagAP){
      Serial.print(millis());
      Serial.println(" - AP Mode:");
    }
    else {
      if(!sendValues()) cnt--; //force retry
    }
  }
  cnt++;
}


