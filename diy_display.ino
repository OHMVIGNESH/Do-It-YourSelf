#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> //https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
//#include <ESP32Ping.h>

//Vcc - Vcc
//Gnd - Gnd
//Din - Mosi (Pin GPIO23)
//Cs  - SS (Pin GPIO22)
//Clk - Sck (Pin GPIO18)
const int pinCS = D4;
const int numberOfHorizontalDisplays = 8;
const int numberOfVerticalDisplays = 1;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
const int wait = 40;
const int spacer = 1;
const int width = 5 + spacer; // Ancho de la fuente a 5 pixeles
String textin;
String topicin, topicout, macadd;
byte mac[6];                     // the MAC address of your Wifi shield
char topic_in[50], topic_out[50], cid[50];
int pushButton = D0;

const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50000];
int value = 0;

WiFiManager wm;
void callback(char* topic, byte* message, unsigned int length) {
  textin = "";
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    textin += (char)message[i];
  }
  Serial.println();
  textin = textin;

  Serial.println(textin);
  // Feel free to add more if statements to control more GPIOs with MQTT


}
void setup() {
  digitalWrite(pinCS, LOW);
  pinMode(pushButton, INPUT);
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);

  textin = "No Internet";
  matrix. setIntensity ( 15 ) ;  // Adjust the brightness between 0 and 15
  matrix. setPosition ( 0 ,  0 ,  0 ) ;  // The first display is at <0, 0>
  matrix. setPosition ( 1 ,  1 ,  0 ) ;  // The second display is at <1, 0>
  matrix. setPosition ( 2 ,  2 ,  0 ) ;  // The third display is in <2, 0>
  matrix. setPosition ( 3 ,  3 ,  0 ) ;  // The fourth display is at <3, 0>
  matrix. setPosition ( 4 ,  4 ,  0 ) ;  // The fifth display is at <4, 0>
  matrix. setPosition ( 5 ,  5 ,  0 ) ;  // The sixth display is at <5, 0>
  matrix. setPosition ( 6 ,  6 ,  0 ) ;  // The seventh display is at <6, 0>
  matrix. setPosition ( 7 ,  7 ,  0 ) ;  // The eighth display is in <7, 0>
  matrix. setPosition ( 8 ,  8 ,  0 ) ;  // The ninth display is at <8, 0>
  matrix. setRotation ( 0 ,  1 ) ;     // Display position
  matrix. setRotation ( 1 ,  1 ) ;     // Display position
  matrix. setRotation ( 2 ,  1 ) ;     // Display position
  matrix. setRotation ( 3 ,  1 ) ;     // Display position
  matrix. setRotation ( 4 ,  1 ) ;     // Display position
  matrix. setRotation ( 5 ,  1 ) ;     // Display position
  matrix. setRotation ( 6 ,  1 ) ;     // Display position
  matrix. setRotation ( 7 ,  1 ) ;     // Display position
  matrix. setRotation ( 8 ,  1 ) ;     // Display position

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("WiFiDisplay1", "WiFiDisplay1"); // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println(WiFi.SSID());
    Serial.println(WiFi.psk());
    Serial.println(WiFi.localIP());
    WiFi.macAddress(mac);
    String id1 = String(mac[0], HEX);
    String id2 = String(mac[1], HEX);
    String id3 = String(mac[2], HEX);
    String id4 = String(mac[3], HEX);
    String id5 = String(mac[4], HEX);
    String id6 = String(mac[5], HEX);
    macadd = id1 + id2 + id3 + id4 + id5 + id6;
    Serial.println(macadd);
    topicin = "/maxelarator/diydisplay1/intopic/" + macadd;
    topicout = "/maxelarator/diydisplay1/outtopic/" + macadd;
    macadd.toCharArray(cid, 50);
    topicin.toCharArray(topic_in, 50);
    topicout.toCharArray(topic_out, 50);
    Serial.println(topic_in);
    Serial.println(topic_out);
    Serial.println(cid);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    textin = "Waiting for Input     ";
  }

}
void reconnect() {
  while (!client.connected()) {
    /*
      String sssid = WiFi.SSID();
      char ssid[50];
      sssid.toCharArray(ssid, 50);
      String passs =  WiFi.psk();
      char pass[50];
      passs.toCharArray(pass, 50);
      WiFi.begin(ssid, pass);*/
    disp();
    WiFi.reconnect();
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // String clientId = "ESP8266Client-";
    // clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(cid)) {
      disp();
      Serial.println("connected");
      client.subscribe(topic_out);
      client.publish(topic_out, "connected");
      client.subscribe(topic_in);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      disp();
      delay(5000);
    }
  }
}

void loop() {
  int buttonState = digitalRead(pushButton);
  if (buttonState == 1) {
    wm.resetSettings();
    ESP.restart();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //String string = textin;
  //Serial.print(string + "\n");
  disp();
  //client.publish("outTopic", "connected");
}
void disp() {
  Serial.println("disp= ");
  Serial.println(textin);
  long int time = millis();

  for (int i = 0; i < width * textin.length() + matrix.width() - 1 - spacer; i++) {
    matrix.fillScreen(LOW);
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // Centrar el texto
    while (x + width - spacer >= 0 && letter >= 0) {
      if (letter < (textin.length())) {
        matrix.drawChar(x, y, textin[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Muestra loscaracteres
    delay(wait);
  }
}
