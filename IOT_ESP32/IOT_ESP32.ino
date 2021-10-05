#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "ArduinoJson.h"
#include <String.h>
#include <DHT.h>

//khai báo 
//const char* ssid = "Alcohol 4 Team HUST";
#define DHTPIN 35
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);

#define LDR 34 //light sensor

//khai bao chan dieu khien
#define DV_1 4 //GPIO4
int state_dv = 0;
//#define DV_2 2 //GPIO2
//#define DV_3 15 //GPIO15

String key_dv = "AbCy8HT08";

const char* ssid = "anhluamaucam";
const char* password = "maydoanthuxem";
const char* mqttServer = "52.148.71.71";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

//khai bao cac chi so
String formattedDate;
String dayStamp;
String timeStamp;
String realTime;


float temperature ;
float humidity;
unsigned int light;


WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long lastMillis = 0;

// cac ham tu xay dung

//ham lay hum, temp
void readDHT(){
  dht.begin();
  delay(100);
  temperature=dht.readTemperature();
  humidity=dht.readHumidity();
}
//ham l?y date time
void callTime(){
  realTime = "";
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  //Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  realTime += dayStamp;
  realTime += " ";
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  realTime += timeStamp;
  //Serial.println(realTime);
}
//ham xu ly khi co goi tin subscribe t?i
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
  Serial.println("Start control solution!");

  StaticJsonBuffer<300> JSONBuffer;                         //Memory pool
  JsonObject& parsed = JSONBuffer.parseObject(payload);
  if (!parsed.success()) {   //Check for errors in parsing
    // thong bao loi json tra ve
    Serial.println("Loi json!"); // loi json ch? print o Serial r?i debug sau.
    //String f1 = "";
    //f1 += "tung1";
    //client.publish("esp/res", f1.c_str());
    return;
  }
  // x? lý di?u khi?n
  else{
    String cmd_id = parsed["command_id"];
    String key = parsed["key"];
    int device = parsed["pin_id"];
    int cmd = parsed["command"];
    if( key == key_dv){
      if(device == DV_1)  { //|| (device == DV_2) || (device == DV_3)
      //dieu khien
        if (cmd == 0){
          digitalWrite(device, LOW);
          state_dv = 0;
        }
        else{
          digitalWrite(device, HIGH);
          state_dv = 1;
        }
        Serial.println("Thank cong!");
        String ressuc = "";
        ressuc += "{\"command_id\": \"";
        ressuc += cmd_id;
        ressuc += "\", \"data\": ";
        ressuc += cmd;
        ressuc += ",\"create_at\": \"";
        callTime();
        ressuc += realTime;
        ressuc += "\"}";
        Serial.println(ressuc);
        client.publish("command",ressuc.c_str());
      }
      else{
      //bao loi
        Serial.println("Loi chan thiet bi!");
        String resfail = "";
        resfail += "{\"command_id\": \"";
        resfail += cmd_id;
        resfail += "\", \"data\": ";
        resfail += -1;
        resfail += ",\"create_at\": \"";
        callTime();
        resfail += realTime;
        resfail += "\"}";
        Serial.println(resfail);
        client.publish("command",resfail.c_str());
      }
    }
    else{
      return;
    }
  }
}

void reconnect(){
  Serial.println("Start reconnect!");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  Serial.println("Reconnect successfully!");
  client.subscribe("iot/command");
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  //client.publish("esp/pub", "Start test subscribe!");
  //Serial.println("Subscribe!");

  timeClient.begin();
  timeClient.setTimeOffset(25200); //GMT +7
  client.subscribe("iot/command");

  //xet cac thiet bi
  pinMode(DV_1, OUTPUT);
  //pinMode(DV_2, OUTPUT);
  //pinMode(DV_3, OUTPUT);
  digitalWrite(DV_1, HIGH);
  //digitalWrite(DV_2, LOW);
  //digitalWrite(DV_3, LOW);
}


void loop() {
  client.loop();
  while(client.connected() == false){
    reconnect();
  }
  callTime();
  //Serial.println(realTime);
  readDHT();
  //Serial.print("Hum:   ");
  //Serial.println(humidity);
  //Serial.print("Temp:  ");
 // Serial.println(temperature);
  if(isnan(humidity) || isnan(temperature)){
    humidity = 0;
    temperature = 0;
  }
  light = analogRead(LDR);
  // print out the value you read:
  //Serial.print("Light:  ");
  //Serial.println(light);
  //tao string de gui
    String publsh = String("");
    
    publsh += "[{ \"sensor\" : \"";
    publsh += "c24979ba-1696-47d5-aad1-02cd83bbc062"; //put id here
    publsh += "\", \"data\" : ";
    //publsh += "[{  ";
    //publsh += "";
    //publsh += light;
    publsh +=  "[{ \"field\" : \"temperature\" ";
    publsh += ", \"value\" : ";
    publsh += temperature;
    publsh +=  "},{ \"field\" : \"sound\" ";
    publsh += ", \"value\" : ";
    publsh += humidity;
    
    publsh += "}],";
    publsh += " \"create_at\" : \"";
    publsh += realTime;
    publsh += "\"}]";
    //Serial.println("Publish: ");
    //Serial.println(publsh);
    client.publish("subscribe", publsh.c_str());
  
    delay(5000);
}
