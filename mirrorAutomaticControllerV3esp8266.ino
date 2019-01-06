#include <Bounce2.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

int setupONpin = 14;

Bounce setupON = Bounce(setupONpin, 10);

int left_motor_up = 100;
int left_motor_down = 200;
int right_motor_up = 300;
int right_motor_down = 400;
int reverse_on_delay = 500;
int reverse_off_delay = 600;

//global variables
boolean setupMode = false;

String responseHTML = ""
                      "<!DOCTYPE html><html><body><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><h2>Mirrors motors settings</h2>"
                      "<form action=\"/args\">"
                      "Left motor UP time:<br>"
                      "<input type=\"number\" name=\"left_motor_up\" value=\"" + String(left_motor_up) + "\" min=\"0\" max=\"10000\" step=\"10\"><br>"
                      "Left motor DOWN time:<br>"
                      "<input type=\"number\" name=\"left_motor_down\" value=\"" + String(left_motor_down) + "\" min=\"0\" max=\"10000\" step=\"10\"><br><br>"
                      "Right motor UP time:<br>"
                      "<input type=\"number\" name=\"right_motor_up\" value=\"" + String(right_motor_up) + "\" min=\"0\" max=\"10000\" step=\"10\"><br>"
                      "Right motor DOWN time:<br>"
                      "<input type=\"number\" name=\"right_motor_down\" value=\"" + String(right_motor_down) + "\" min=\"0\" max=\"10000\" step=\"10\"><br><br>"
                      "Motion DOWN delay:<br>"
                      "<input type=\"number\" name=\"reverse_on_delay\" value=\"" + String(reverse_on_delay) + "\" min=\"0\" max=\"10000\" step=\"10\"><br>"
                      "Motion UP delay:<br>"
                      "<input type=\"number\" name=\"reverse_off_delay\" value=\"" + String(reverse_off_delay) + "\" min=\"0\" max=\"10000\" step=\"10\"><br><br>"
                      "<input type=\"submit\" value=\"Save\"></form></body></html>";

void setup() {
  Serial.begin(115200);
//  Serial.println();

  readValues();
  
  pinMode(setupONpin, INPUT_PULLUP);

  setupON.update();
  
//check setup mode on/off
  if (setupON.read() == HIGH) { //for debug. normal mode is == LOW
    Serial.println("setupON:");
    setupMode = true; 
  }
           
//WIfi and access point is turned on only if setup mode is turned on during boot
  if (setupMode == true) {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("mirror_Controller_SetUp", "12345678");
    Serial.println("softAP started");

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);

//my last code for check requests:
    webServer.on("/args", HTTP_GET, handleRoot); //call the handleRoot after any GET from client
//end of my code
    // replay to all requests with same HTML
    webServer.onNotFound([]() {
      webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();
  }
  else {
    WiFi.mode(WIFI_OFF);
    Serial.println("SetupOFF");
    Serial.println("WiFi off");
  }

}

void loop() {
  //webserver and dns server is needed only if setup mode is on
  if (setupMode == true) { 
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
}

//parse params from /args endPoint
void handleRoot(){
  webServer.send(200, "text/html", "<!DOCTYPE html><html><body><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><h2>Data were saved</h2><br><form action=\"/\"><input type=\"submit\" value=\"Go to Settings page\"></form></body></html>");
}

void readValues(){
  if (Serial.available() > 0 ) {
    Serial.println("GET");
    delay(500);
    String str = Serial.readString();
    Serial.println(str);
  }
}

