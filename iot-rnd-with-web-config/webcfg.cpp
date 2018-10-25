
#include "webcfg.h"

ESP8266WebServer server(80);
DynamicJsonDocument doc(MAX_CFG_SIZE);


bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  Serial.print("Config file size:"); Serial.println(size);
  if(size > MAX_CFG_SIZE){
    Serial.println("Config file is too big.");
    return false;
  }
#if 0
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  Serial.println("Config file:");
  Serial.println(buf.get());
  Serial.println("---------");
  configFile.seek(0L, SeekSet);
#endif

  JsonObject root = doc.as<JsonObject>();
  DeserializationError error = deserializeJson(doc, configFile);
  if (error)
    Serial.println("Failed to read file, using default configuration");

  configFile.close();
#if 0
    Serial.println("Test read");
    Serial.print("cfg ssid = ");
    Serial.println((const char *)root["ssid"]);
#endif
  return true;
}


void sendResponse(String& message, bool flag=true){
  String ret = "<html><head><title>web config</title></head><body>";
  if(flag)
    ret += "<a href=\"/\">Home</a><br>";
  ret += message;
  ret += "</body></html>";
  server.send(200, "text/html", ret);
}

void handleRoot() {
  Serial.println("Start page");
  String message = "<h1>Start page</h1>";
  message += "<a href=\"/cfg\">Config</a><br>";
  message += "<a href=\"/for\">Format flash</a><br>";
  sendResponse(message, false);
}

void handleConfig(){
  Serial.println("Config page");
  bool have_config = loadConfig();
  String message = "<h1>Config</h1>";
  message += "<form method=\"POST\" action=\"save\"><table>";
  JsonObject root = doc.as<JsonObject>();
  for(byte i=0;params[i] != NULL;i++){    
    message += "<tr><th>"+String(params[i])+"</th>";
    message += "<td><input type=\"text\" name=\""+String(params[i])+"\" value=\"";
    if(have_config)
      message += (const char *)root[params[i]];
    else
      message += "?";
    message += "\"></td></tr>";
  }
  message += "<tr><td></td><td><input type=\"Submit\" value=\"Save\"></td></tr>";
  message += "</table></form>";
  sendResponse(message);
}

void handleSave(){
  bool flag = true;
  JsonObject root = doc.to<JsonObject>();
  Serial.println("Save page");
  String message = "<pre>Saved!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    root[server.argName(i)] = server.arg(i);
  }

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    flag = false;
  }

  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to file");
    flag = false;
  }

  // Close the file (File's destructor doesn't close the file)
  configFile.close();

  if(flag) message += "\nSave OK.";
  else message += "\nError.";
  message += "</pre>";
  sendResponse(message);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void start_AP_config(char *ssid, char *password){
  Serial.println("Start AP.");
  Serial.print("SSID: ");Serial.println(ssid);
  Serial.print("Password: ");Serial.println(password);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/cfg", handleConfig);
  server.on("/save", handleSave);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });
  server.on("/for", [](){
    Serial.print("Start format:");Serial.println(millis());
    SPIFFS.format();
    Serial.print("End format:");Serial.println(millis());
    server.send(200, "text/plain", "Flash formatted!");
  });
  server.onNotFound(handleNotFound);      
  server.begin();
  Serial.println("HTTP server started");
}


