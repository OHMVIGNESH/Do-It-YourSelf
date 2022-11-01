#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> //https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
const int trigPin = D5;
const int echoPin = D6;
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701
float tempc;
int pushButton = D4;
float distanceCm;
float distanceInch;
int redled = D0;
int greenled = D7;
int alarm;
long duration;
String topicin, topicout, macadd;
byte mac[6];                     // the MAC address of your Wifi shield
char topic_in[50], topic_out[50], cid[50];

const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50000];
int value = 0;
String textin;
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

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  while (!Serial);
  Serial.println("Adafruit MLX90614 test");
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };
  Serial.print("Emissivity = "); Serial.println(mlx.readEmissivity());
  Serial.println("================================================");
  pinMode(redled, OUTPUT);
  pinMode(greenled, OUTPUT);

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("diytemperature1", "diytemperature1"); // password protected ap

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
    topicin = "/maxelarator/diytemperature1/intopic/" + macadd;
    topicout = "/maxelarator/diytemperature1/outtopic/" + macadd;
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
  /*int buttonState = digitalRead(pushButton);
    if (buttonState == 1) {
    wm.resetSettings();
    ESP.restart();
    }*/
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  calc();
  //client.publish(topic_out, "connected");
}
void calc() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_VELOCITY / 2;
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  if (distanceCm < 15) {
    Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC());
    Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
    Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempF());
    Serial.print("*F\tObject = "); Serial.print(mlx.readObjectTempF()); Serial.println("*F");
    Serial.println();
    delay(500);
    tempc = mlx.readObjectTempF();
    Serial.println(tempc);
    alarm = 0;
    if (tempc < 200) {
      if (tempc < 99) {
        // Blynk.notify("ONE PERSON HIGH TEMPRATURE");
        digitalWrite(redled, LOW);
        delay(1000);                       // wait for a second
        digitalWrite(greenled, HIGH);    // turn the LED off by making the voltage LOW
        delay(1000);
        alarm = 0;
      }
      if (tempc > 99) {
        digitalWrite(greenled, LOW);   // turn the LED on (HIGH is the voltage level)
        delay(1000);                       // wait for a second
        digitalWrite(redled, HIGH);    // turn the LED off by making the voltage LOW
        delay(1000);
        alarm = 1;
      }

      digitalWrite(greenled, LOW);
      digitalWrite(redled, LOW);
      delay(1000);
    }
    if (tempc < 200) {
      StaticJsonDocument<200> doc;
      doc["Temperature_C"] = tempc;
      doc["Alarm"] = alarm;
      serializeJson(doc, Serial);
      Serial.println();
      serializeJsonPretty(doc, Serial);
      // The above line prints:
      // {
      //   "Temperature_C": 100,
      //   "Alarm": 1
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
    else{
      StaticJsonDocument<200> doc;
      doc["Temperature_C"] = "nan";
      doc["Alarm"] = alarm;
      serializeJson(doc, Serial);
      Serial.println();
      serializeJsonPretty(doc, Serial);
      // The above line prints:
      // {
      //   "Temperature_C": 100,
      //   "Alarm": 1
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
  }
}
