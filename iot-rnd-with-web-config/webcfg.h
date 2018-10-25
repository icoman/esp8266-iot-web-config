
#define MAX_CFG_SIZE 256


#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>



#include <ArduinoJson.h>
#include "FS.h"


extern char *params[];
extern bool loadConfig();
extern void start_AP_config(char *ssid, char *password);
extern ESP8266WebServer server;
extern DynamicJsonDocument doc;


