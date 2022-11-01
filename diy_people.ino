#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> //https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <ArduinoJson.h>

#define trigPin D5
#define echoPin D6
const int trigPin2 = D0;
const int echoPin2 = D7;
#define SOUND_VELOCITY 0.034
long duration;
float distanceCm;
long duration2;
float distanceCm2;
int incount = 0 , outcount = 0;
String textin;
String topicin, topicout, macadd;
byte mac[6];                     // the MAC address of your Wifi shield
char topic_in[50], topic_out[50],cid[50];
int pushButton = D4;

const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50000];
int value = 0;

WiFiManager wm;
void callback(char* topic, byte* message, unsigned int length) {
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

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(trigPin2, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin2, INPUT); // Sets the echoPin as an Input

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("diypeople1", "diypeople1"); // password protected ap

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
    topicin = "/maxelarator/diypeople1/intopic/" + macadd;
    topicout = "/maxelarator/diypeople1/outtopic/" + macadd;
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
    if (client.connect(cid)) {
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
  if (buttonState == 0) {
    wm.resetSettings();
    ESP.restart();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  calc();
  client.publish(topic_out, "connected");
}
void calc() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance1 (cm): ");
  Serial.println(distanceCm);

  // Clears the trigPin
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration2 = pulseIn(echoPin2, HIGH);
  // Calculate the distance
  distanceCm2 = duration2 * SOUND_VELOCITY / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance2 (cm): ");
  Serial.println(distanceCm2);

  if (distanceCm < 30 ) { //&& inflag == 1 && outflag == 0) {
    Serial.print("+++++++++++++++++++++++++++++++++++++++++++++++++");
    incount = incount + 1;
    Serial.print("incount:");
    Serial.println(incount);
  }
  if (distanceCm2 < 30 ) { //&& inflag == 0 && outflag == 1) {
    Serial.print("+++++++++++++++++++++++++++++++++++++++++++++++++");
    outcount = outcount + 1;
    Serial.print("outcount:");
    Serial.println(outcount);
  }
  Serial.print("in:");
  Serial.println(incount);
  Serial.print("out:");
  Serial.println(outcount);
  Serial.print("occupancy:");
  int ocup = incount - outcount;
  if (ocup < 0) {
    ocup = 0;
  }
  else {
    ocup = ocup;
  }
  Serial.println(ocup);
  String T = String(ocup);
  delay(1000);
  StaticJsonDocument<200> doc;
  doc["In_count"] = incount;
  doc["Out_count"] = outcount;
  doc["Occupancy"] = ocup;
  serializeJson(doc, Serial);
  Serial.println();
  serializeJsonPretty(doc, Serial);
  // The above line prints:
  // {
  //   "In_count": 100,
  //   "Out_count": 50,
  //   "Occupancy": 50
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
