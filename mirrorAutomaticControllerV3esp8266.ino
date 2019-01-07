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

int left_motor_up;
int left_motor_down;
int right_motor_up;
int right_motor_down;
int reverse_on_delay;
int reverse_off_delay;

String left_motor_up_status;
String left_motor_down_status;
String right_motor_up_status;
String right_motor_down_status;
String reverse_on_delay_status;
String reverse_off_delay_status;
String eeprom_write_status;

//global variables
boolean setupMode = false;


void setup() {
  Serial.begin(115200);
  delay(2000); //deley for initialising atMega module
  pinMode(setupONpin, INPUT_PULLUP);

  setupON.update();

  //check setup mode on/off
  if (setupON.read() == HIGH) { //for debug. normal mode is == LOW
    //    Serial.println("setupON:");
    setupMode = true;
  }

  //WIfi and access point is turned on only if setup mode is turned on during boot
  if (setupMode == true) {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("mirror_Controller_SetUp", "12345678");
    //    Serial.println("softAP started");

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);

    //my last code for check requests:
    webServer.on("/args", HTTP_GET, handleArgs); //call the handleArgs after GET /args from client
    //end of my code
    webServer.on("/get_values", HTTP_GET, handleValues); //call the handleValues to read values from atmega by serial
    webServer.on("/write_to_eeprom_values", HTTP_GET, handleWriteToEEPROM); //call the handleWriteToEEPROM to write values to eeprom

    // replay to all requests with same HTML
    webServer.onNotFound([]() {
      webServer.send(200, "text/html", responseHTML());
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
void handleArgs() {
  readArgs();
  saveAllValuesToSerial();
  String response = ""
                    "<!DOCTYPE html><html><body><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                    "<h2>Data were saved to RAM</h2><br>"
                    "left_motor_up - " + left_motor_up_status + "<br>"
                    "left_motor_down - " + left_motor_down_status + "<br>"
                    "right_motor_up - " + right_motor_up_status + "<br>"
                    "right_motor_down - " + right_motor_down_status + "<br>"
                    "reverse_on_delay - " + reverse_on_delay_status + "<br>"
                    "reverse_off_delay - " + reverse_off_delay_status + "<br>"
                    "<br><br>"
                    "<form action=\"/\"><input type=\"submit\" value=\"Go to Settings page\"></form>"
                    "</body></html>";
  webServer.send(200, "text/html", response);

}

void handleValues() {
  readValues();
  webServer.send(200, "text/html", "<!DOCTYPE html><html><body><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><h2>Data were readed</h2><br><form action=\"/\"><input type=\"submit\" value=\"Go to Settings page\"></form></body></html>");
}

void handleWriteToEEPROM() {
  writeValuesToEEPROM();
  webServer.send(200, "text/html", "<!DOCTYPE html><html><body><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><h2>Data were writed to EEPROM</h2><br>" + eeprom_write_status + "<br><form action=\"/\"><input type=\"submit\" value=\"Go to Settings page\"></form></body></html>");
}

void readArgs() {
  left_motor_up = readArg("left_motor_up"); //read arguments one by one
  left_motor_down = readArg("left_motor_down");
  right_motor_up = readArg("right_motor_up");
  right_motor_down = readArg("right_motor_down");
  reverse_on_delay = readArg("reverse_on_delay");
  reverse_off_delay = readArg("reverse_off_delay");
}

int readArg(String argument) {
  String value;
  if (webServer.hasArg(argument)) { //check if we have this argument
    value = webServer.arg(argument); //save it value to temp String
    if ((32767 >= value.toInt()) && (value.toInt() > 0)) { //if value can be integer
      return value.toInt(); //save data to global variable
    }
    else return 0;
  }
}

void saveAllValuesToSerial() {
  left_motor_up_status = saveValueToSerial("left_motor_up=" + String(left_motor_up));
  left_motor_down_status = saveValueToSerial("left_motor_down=" + String(left_motor_down));
  right_motor_up_status = saveValueToSerial("right_motor_up=" + String(right_motor_up));
  right_motor_down_status = saveValueToSerial("right_motor_down=" + String(right_motor_down));
  reverse_on_delay_status = saveValueToSerial("reverse_on_delay=" + String(reverse_on_delay));
  reverse_off_delay_status = saveValueToSerial("reverse_off_delay=" + String(reverse_off_delay));
}

String saveValueToSerial(String string) {
  //if data written to serial suucessfully return true, else return false
  String str;
  Serial.println(string); //print to serial variabble and value e.g. left_motor_up=12
  while ( Serial.available() == 0) { //waiting for response
  }
  if (Serial.available() > 0 ) {
    str = Serial.readString(); //respose to temp strinng
  }
  if (str.indexOf("DONE", 0) > -1) return "OK"; //check the response
  else return "FAIL";
}

void writeValuesToEEPROM() {
  String str;
  Serial.println("WRITE"); //print to serial WRITE command
  while ( Serial.available() == 0) { //waiting for response
  }
  if (Serial.available() > 0 ) {
    str = Serial.readString(); //respose to temp strinng
  }
  if (str.indexOf("DONE", 0) > -1) eeprom_write_status = "PASS"; //check the response
  else eeprom_write_status = "FAIL";
}

void readValues() {

  Serial.println("get_left_motor_up");
  left_motor_up = readValueFromSerial("left_motor_up=");

  Serial.println("get_left_motor_down");
  left_motor_down = readValueFromSerial("left_motor_down=");

  Serial.println("get_right_motor_up");
  right_motor_up = readValueFromSerial("right_motor_up=");

  Serial.println("get_right_motor_down");
  right_motor_down = readValueFromSerial("right_motor_down=");

  Serial.println("get_reverse_on_delay");
  reverse_on_delay = readValueFromSerial("reverse_on_delay=");

  Serial.println("get_reverse_off_delay");
  reverse_off_delay = readValueFromSerial("reverse_off_delay=");

}

int readValueFromSerial(String expectedString) {
  String str;
  while ( Serial.available() == 0) {
  }
  if (Serial.available() > 0 ) {
    str = Serial.readString();
    if (str.indexOf(expectedString, 0) > -1) {
      String value = str.substring(expectedString.length());
      if ((32767 >= value.toInt()) && (value.toInt() > 0)) { //if value can be integer
        return value.toInt();
      } else return 0; //return 0 if value is not correct
    } else return 0; //return 0 if response doesn't contain expected string
  }
}

String responseHTML() {
  String responseHTML = ""
                        "<!DOCTYPE html><html><body><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><h2>Mirrors motors settings</h2>"
                        "<form action=\"/args\">"
                        "Left motor UP time:<br>"
                        "<input type=\"number\" name=\"left_motor_up\" value=\"" + String(left_motor_up) + "\" min=\"1\" max=\"32767\" step=\"1\"><br>"
                        "Left motor DOWN time:<br>"
                        "<input type=\"number\" name=\"left_motor_down\" value=\"" + String(left_motor_down) + "\" min=\"1\" max=\"32767\" step=\"1\"><br><br>"
                        "Right motor UP time:<br>"
                        "<input type=\"number\" name=\"right_motor_up\" value=\"" + String(right_motor_up) + "\" min=\"1\" max=\"32767\" step=\"1\"><br>"
                        "Right motor DOWN time:<br>"
                        "<input type=\"number\" name=\"right_motor_down\" value=\"" + String(right_motor_down) + "\" min=\"1\" max=\"32767\" step=\"1\"><br><br>"
                        "Motion DOWN delay:<br>"
                        "<input type=\"number\" name=\"reverse_on_delay\" value=\"" + String(reverse_on_delay) + "\" min=\"1\" max=\"32767\" step=\"1\"><br>"
                        "Motion UP delay:<br>"
                        "<input type=\"number\" name=\"reverse_off_delay\" value=\"" + String(reverse_off_delay) + "\" min=\"1\" max=\"32767\" step=\"1\"><br>"
                        "All operations may take some time. DO NOT reload the page or click on buttons after first click<br><br>"
                        "<input type=\"submit\" value=\"Save to RAM\"></form><br>"
                        "<form action=\"/get_values\"><input type=\"submit\" value=\"Read values from RAM\"></form><br>"
                        "<form action=\"/write_to_eeprom_values\"><input type=\"submit\" value=\"Write to EEPROM\"></form>"
                        "</body></html>";

  return responseHTML;
}

