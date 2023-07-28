//Library yang diperlukan
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <MQTT.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

//Pin yang terkoneksi
#define LEDPIN 15

//Data network
const char WIFI_SSID[] = "LT3 lorong1";
const char WIFI_PASSWORD[] = "10293847";
uint8_t broadcastAddress[] = {0xC0,0x49,0xEF,0x6A,0x1F,0x38}; //Mac address ESP-Slave Output

//Setting MQTT
WiFiClient net;
MQTTClient client;

//Deklarasi AutoPairing
esp_now_peer_info_t slave;
int chan; 
enum MessageType {PAIRING, DATA,};
MessageType messageType;

//Struct penerimaan data dari ESP-Slave
typedef struct struct_message {
    char msgType;
    int board_id; 
    float humid;
    float temp;
    float soilhumid;
    float pH;
} struct_message;

//Struct AutoPairing
typedef struct struct_pairing { 
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;	

struct_message myData;
struct_pairing pairingData;

struct_message board1;
struct_message board2;

esp_now_peer_info_t peerInfo;

//Buat array dengan 2 board
struct_message boardsStruct[2] = {board1, board2};

void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

bool addPeer(const uint8_t *peer_addr) { // Tambahkan peer untuk ESP-NOW
  memset(&slave, 0, sizeof(slave));
  const esp_now_peer_info_t *peer = &slave;
  memcpy(slave.peer_addr, peer_addr, 6);
  
  slave.channel = chan; 
  slave.encrypt = 0; 
  bool exists = esp_now_is_peer_exist(slave.peer_addr); //Tambahkan peer
  if (exists) {
    Serial.print(slave.channel);
    Serial.println("Already Paired");
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK) {
      // Pairing berhasil
      Serial.println("Pair success");
      return true;
    }
    else 
    {
      Serial.println("Pair failed");
      return false;
    }
  }
} 
	
//Fungsi ketika data diterima
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
uint8_t type = incomingData[0]; 

Serial.print("msg type "); //msg type : DATA/PAIRING 
Serial.println(type);
 switch (type) {
  case DATA : 
  {    
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.printf("Board ID %d: %d bytes\n", myData.board_id, len);
    boardsStruct[myData.board_id-1].humid = myData.humid;
    boardsStruct[myData.board_id-1].temp = myData.temp;
    Serial.printf("Humidity : %f \n", boardsStruct[myData.board_id-1].humid);
    Serial.printf("Temperature : %f \n", boardsStruct[myData.board_id-1].temp);
    Serial.println();

    String //Penyusunan data JSON untuk dikirimkan ke MQTT
    jsonEspNowData = "{";
    jsonEspNowData += "\"BoardId\":" + (String)myData.board_id + ",";
    jsonEspNowData += "\"temperature\":" + (String)myData.temp + ",";
    jsonEspNowData += "\"humidity\":" + (String)myData.humid + ",";
    jsonEspNowData += "\"soilhumid\":" + (String)myData.soilhumid + ",";
    jsonEspNowData += "\"pH\":" + (String)myData.pH;
    jsonEspNowData += "}";
    Serial.println("ESP-NOW Node JSON Payload > " + jsonEspNowData + "\n");

    client.publish("DHT/sensor", jsonEspNowData);
    break;
  }
  case PAIRING :                           
  { 
      memcpy(&pairingData, incomingData, sizeof(pairingData));
      Serial.print("msgtype : ");
      Serial.println(pairingData.msgType);
      Serial.print("pairing id : ");
      Serial.println(pairingData.id);
      Serial.print("Pairing request from: ");
      printMAC(mac_addr);
      Serial.println();
      Serial.print("Pairing Channel : ");
      Serial.println(pairingData.channel);
      if (pairingData.id > 0) {     //Mencegah reply ke server
        if (pairingData.msgType == PAIRING) { 
          pairingData.id = 0;       // Server ID= 0
          WiFi.softAPmacAddress(pairingData.macAddr);   
          pairingData.channel = chan;
          Serial.println("send response");
          esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &pairingData, sizeof(pairingData));
          addPeer(mac_addr);
      }  
    }  
    break; 
  }
  }
}
//Fungsi koneksi wifi dan MQTT
void connect(){
    Serial.print("Checking Wifi...");
    while (WiFi.status() != WL_CONNECTED){
      Serial.print(".");
    delay(2000);
    }
    Serial.println("\nWifi Connected !");
    Serial.println(WiFi.macAddress());
    Serial.print("\nconnecting...");
    while (!client.connect("node")){
      Serial.print(".");
      digitalWrite(LEDPIN, HIGH);
      delay(100);
      digitalWrite(LEDPIN, LOW);
      delay(2000);
    }
    Serial.println("\nMQTT Connected !");
    client.subscribe("esp32/output");       //subscribe client MQTT
}

//Fungsi ketika penerimaan data dari Raspberry Pi dan dikirim ke ESP-Slave Output
void messageReceived(String &topic, String &payload){
  Serial.println("incoming: " + topic + " - " + payload);
  deserializeJson(JsonDoc, payload);
  int jumlahair = JsonDoc["jumlahair"];
  Serial.println(payload);
  //Kirim data ke ESP-Slave Output
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &jumlahair, sizeof(jumlahair));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
    Serial.println(jsonEspNowDataout);
  }
  else {
    Serial.println("Error sending the data");
    Serial.println(result);
  }
}

//Cek pengiriman data ke ESP-Slave Output
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  digitalWrite(LEDPIN, LOW);
  if(status == ESP_NOW_SEND_FAIL){
      digitalWrite(LEDPIN, HIGH);
     delay(300);
     digitalWrite(LEDPIN, LOW);
  }else if(status == ESP_NOW_SEND_SUCCESS){
      digitalWrite(LEDPIN, HIGH);
  }
}
  
void setup(){
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //Koneksi Wifi

  while (WiFi.status() != WL_CONNECTED) { 
    digitalWrite(LEDPIN, HIGH);
    delay(300);
    digitalWrite(LEDPIN, LOW);
    delay(1000);
    Serial.println("Setting as a wifi station..");
    }
    digitalWrite(LEDPIN, HIGH);
    Serial.print("Server SOFT AP MAC Address:  ");
    Serial.println(WiFi.softAPmacAddress());
    chan = WiFi.channel();
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Wifi Channel: ");
    Serial.println(WiFi.channel());

    client.begin("192.168.2.22", net); //Koneksi ke MQTT Broker
    client.onMessage(messageReceived);
    connect();
    if(esp_now_init() != 0){
      Serial.println("Error Initializing ESP-NOW");
      return;
      }
    //Menambahkan peer
    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK){
     Serial.println("Failed to add peer");
    return;
    }
     esp_now_register_recv_cb(OnDataRecv);
}

void loop(){
  client.loop();
  delay(10);
  if (!client.connected()){
    connect();
  }
}
