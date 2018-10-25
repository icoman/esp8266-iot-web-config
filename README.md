# ESP8266 iot demo client

To enter into config mode, after device boot, press FLASH button (GPIO 16) 
or any other button configured in program as INPUT. 
The firmware will switch to AP mode and will start a web server.

Configuration is stored as json into SPIFFS.

Tested with Wemos Micro and ESP8266 NodeMCU.

Compiled with Arduino IDE 1.8.5
