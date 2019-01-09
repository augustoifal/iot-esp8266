/*
    This sketch demonstrates how to set up a simple HTTP-like server.
    The server will set a GPIO pin depending on the request
      http://server_ip/gpio/0 will set the GPIO2 low,
      http://server_ip/gpio/1 will set the GPIO2 high
    server_ip is the IP address of the ESP8266 module, will be
    printed to Serial when the module is connected.
*/
#include <NTPClient.h>
#include <ESP8266WiFi.h>

#include <WiFiUdp.h>
#include <Servo.h>


#ifndef STASSID
#define STASSID  "IFAL-INFO"
#define STAPSK   "ifal-1nf0"
#define LED    14 //GPIO14 D5
#define BUZZER  4 //GPIO4 D2
#define SERVOPIN   5 //GPIO5 D1
#endif

Servo servo;

// Date Time vars 
WiFiUDP ntpUDP;
int16_t utc = -3; //UTC -3:00 Brazil
uint32_t currentMillis = 0;
uint32_t previousMillis = 0;
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", utc*3600, 60000);

// WIFI vars
const char* ssid = STASSID;
const char* password = STAPSK;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  // prepare BUZZER
  pinMode(BUZZER, OUTPUT);
  tone(BUZZER, 1500);
  delay(1000);
  noTone(BUZZER);
  
  // prepare LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  Serial.begin(115200);

  // prepare SERVO
  servo.attach(SERVOPIN); 
  servo.write(0);
  delay(2000);
  


  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Conectando WIFI "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi conectado"));

  // Start the server
  server.begin();
  Serial.println(F("Server iniciado"));

  // Print the IP address
  Serial.println(WiFi.localIP());
  
  //time Client
  timeClient.begin();
  timeClient.update();
}

void forceUpdate(void) {
  timeClient.forceUpdate();
}
 
void checkOST(void) {
  currentMillis = millis();//Tempo atual em ms
  //Lógica de verificação do tempo
  if (currentMillis - previousMillis > 1000) {
    previousMillis = currentMillis;    // Salva o tempo atual
    printf("Time Epoch: %d: ", timeClient.getEpochTime());
    Serial.println(timeClient.getFormattedTime());
  }
}
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("novo clientet"));

  client.setTimeout(5000); // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  // Match the request
  int val, servm = 0;

  if (req.indexOf(F("/gpio/0")) != -1) {
    val = 0;
  } else if (req.indexOf(F("/gpio/1")) != -1) {
    val = 1;
  } else if (req.indexOf(F("/gpio/3")) != -1) {
    servm = 1;  
  } else {
    Serial.println(F("invalido request"));
    val = digitalRead(LED);
    Serial.println(val);
  }
  

  // Set LED according to the request
  digitalWrite(LED, val);
  if (val) {
    tone(BUZZER, 500);
    tone(BUZZER, 500);
  }else{
    tone(BUZZER,1500);
  }

  // Set Servo 90 degrees
  if (servm){
      servo.write(90);
      delay(1000);
      Serial.println("SERVO 90");
  } else {
      servo.write(0);
      delay(1000);
      Serial.println("SERVO  0");
   }

  // read/ignore the rest of the request
  // do not client.flush(): it is for output only, see below
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  // Send the response to the client
  // it is OK for multiple small client.print/write,
  // because nagle algorithm will group them into one single packet
  
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html lang='pt'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/>"));
  client.print(F("<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>"));
  client.print(F("<title>IFAL - NodeMCU + BootStrap UI</title></head><body>"));
  client.print(F("<center><div class='container-fluid'>"));
  client.print(F(      "<h2>IFAL - NodeMCU + BootStrap UI</h2>"));
  client.print(F(        "<img src='https://jgamblog.files.wordpress.com/2018/02/esp8266-nodemcu-pinout.png' style='display: block;  margin-left: auto;  margin-right: auto;  width: 200px;'><br>"));
  client.print(F(      "<h3>GPIOS:</h3>"));
  client.print(F(      "<h4>Hora: "));
  client.print( timeClient.getFormattedTime());
  client.print(F(      "</h4>"));
  client.print(F(      "<div class='row'>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><h4 class ='text-left'>D2 - BUZZER "));
  client.print(F(          "<span class='badge'>"));
  client.print(          (val) ? F("LIGADO") : F("DESLIGADO"));
  client.print(F(        "</span></h4>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><form action='http://"));
  client.print(WiFi.localIP());
  client.print(F(            "/gpio/1' method='POST'><button type='button submit' name='D5' value='1' class='btn btn-success btn-lg'>ON</button></form></div>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><form action='http://"));
  client.print(WiFi.localIP());
  client.print(F(            "/gpio/0' method='POST'><button type='button submit' name='D5' value='0' class='btn btn-danger btn-lg'>OFF</button></form></div>"));
  client.print(F("</div></div>"));
  client.print(F(      "<div class='row'>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><h4 class ='text-left'>D5 - LED "));
  client.print(F(          "<span class='badge'>"));
  client.print(          (val) ? F("LIGADO") : F("DESLIGADO"));
  client.print(F(        "</span></h4>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><form action='http://"));
  client.print(WiFi.localIP());
  client.print(F(            "/gpio/1' method='POST'><button type='button submit' name='D5' value='1' class='btn btn-success btn-lg'>ON</button></form></div>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><form action='http://"));
  client.print(WiFi.localIP());
  client.print(F(            "/gpio/0' method='POST'><button type='button submit' name='D5' value='0' class='btn btn-danger btn-lg'>OFF</button></form></div>"));
  client.print(F("</div></div>"));
  client.print(F(      "<div class='row'>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><h4 class ='text-left'>D1 - SERVO "));
  client.print(F(          "<span class='badge'>"));
  client.print(          (servm) ? F("Rota&ccedil&atildeo: 90 graus") : F("Sem Rota&ccedil&atildeo: 0"));
  client.print(F(        "</span></h4>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><form action='http://"));
  client.print(WiFi.localIP());
  client.print(F(            "/gpio/3' method='POST'><button type='button submit' name='D5' value='3' class='btn btn-success btn-lg'>ON</button></form></div>"));
  client.print(F(        "<div class='col-xs-6 col-md-4'><form action='http://"));
  client.print(WiFi.localIP());
  client.print(F(            "/gpio/0' method='POST'><button type='button submit' name='D5' value='0' class='btn btn-danger btn-lg'>OFF</button></form></div>"));
  client.print(F("</div></div></div>"));
  client.print(F("</center></body></html>"));

  // The client will actually be *flushed* then disconnected
  // when the function returns and 'client' object is destroyed (out-of-scope)
  // flush = ensure written data are received by the other side
  Serial.println(F("Desconectando do cliente"));
  noTone(BUZZER);
  checkOST();
}
