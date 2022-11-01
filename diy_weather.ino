#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> //https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "DHT.h"


#define DHTPIN D5     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT11   // there are multiple kinds of DHT sensors
DHT dht(DHTPIN, DHTTYPE);
int rainsensor = D0;
int timeSinceLastRead = 0;
float h, t, f;
int s;
String textin;
String topicin, topicout, macadd;
byte mac[6];                     // the MAC address of your Wifi shield
char topic_in[50], topic_out[50], cid[50];
int pushButton = D7;

const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50000];
int value = 0;

WiFiManager wm;
void callback(char* topic, byte* message, unsigned int length) {
  \
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
  pinMode(pushButton, INPUT);
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);

  dht.begin();
  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running DHT!");
  Serial.println("-------------------------------------");
  pinMode(rainsensor, INPUT);

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("diyweather1", "diyweather1"); // password protected ap

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
    topicin = "/maxelarator/diyweather1/intopic/" + macadd;
    topicout = "/maxelarator/diyweather1/outtopic/" + macadd;
    macadd.toCharArray(cid, 50);
    topicin.toCharArray(topic_in, 50);
    topicout.toCharArray(topic_out, 50);
    Serial.println(topic_in);
    Serial.println(topic_out);
    Serial.println(cid);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }

}
void reconnect() {
  while (!client.connected()) {
    WiFi.reconnect();
    Serial.print("Attempting MQTT connection...");
    if (client.connect("diyplant1")) {
      calc();
      Serial.println("connected");
      client.subscribe(topic_out);
      client.publish(topic_out, "connected");
      client.subscribe(topic_in);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5);
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
  calc();
}
void calc() {

  if (timeSinceLastRead > 2000) {
    h = dht.readHumidity();
    t = dht.readTemperature();
    f = dht.readTemperature(true);
    if (isnan(h) || isnan(t) || isnan(f)) {
      timeSinceLastRead = 0;
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");
    timeSinceLastRead = 0;
  }
  int rainstate = digitalRead(rainsensor);
  int rainstatus;
  if (rainstate == 0){
    rainstatus = 1;
  }
  else{
    rainstatus = 0;
  }

  String humidity = String(h);
  String Temperature = String(t);
  timeSinceLastRead += 50;

  StaticJsonDocument<200> doc;
  doc["Humidity_%"] = humidity;
  doc["Temperature_C"] = Temperature;
  doc["Rain_State"] = rainstatus;
  serializeJson(doc, Serial);
  Serial.println();
  serializeJsonPretty(doc, Serial);
  // The above line prints:
  // {
  //   "Humidity_%": 52,
  //   "Temperature_C": 28,
  //    "Rain_State": 1
  // }
  char out[128];
  int b = serializeJson(doc, out);
  Serial.print("bytes = ");
  Serial.println(b, DEC);
  boolean rc = client.publish(topic_in, out);
  if (rc == true) {
    Serial.println("Msg Successfully sended");
  }
  if (rc == false) {
    Serial.println("Msg failed to be sended");
  }
}
