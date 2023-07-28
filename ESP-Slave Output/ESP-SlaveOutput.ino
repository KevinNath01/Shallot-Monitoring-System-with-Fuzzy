//Library yang diperlukan
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>

//Pin yang terkoneksi
#define RELAY_PIN  15
#define LED_PIN 14
#define sensorPin 13

#define BOARD_ID 1
#define MAX_CHANNEL 11

//Deklarasi untuk Waterflow sensor
volatile long pulse;
unsigned long lastTime;
float volume;
int state = 0;

uint8_t broadcastAddress[] = {0xC0, 0x49, 0xEF, 0x6A, 0x1F, 0x38};

// Struct data yang diterima dari ESP-Master
typedef struct struct_manual {   
    const char* msgType;
    int jumlahair;
} struct_manual;

typedef struct struct_pairing {  
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;

// Membuat struct dengan nama myData dan pairing data
struct_manual myData;
struct_pairing pairingData;

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};
PairingStatus pairingStatus = NOT_PAIRED;

enum MessageType {PAIRING, DATA,};
MessageType messageType;

#ifdef SAVE_CHANNEL
  int lastChannel;
#endif  
int channel = 1;

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;  
const long interval = 3000;       
unsigned long start;               
unsigned int readingId = 0;  

// Fungsi ketika menerima data
void OnDataRecv(const uint8_t * mac_addr,const uint8_t *incomingData, int len) {
  Serial.print("Packet received from: ");
  printMAC(mac_addr);
  Serial.println();
  Serial.print("data size = ");
  uint8_t type = incomingData[0];
  Serial.print("msgtype : ");
  Serial.print(type);
  switch (type) {
  case DATA :      //Menerima data dari esp-master
    char macStr[18];
    Serial.print("Packet received from: ");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.printf("Jumlah air : %d \n", myData.jumlahair);
    Serial.println();
    break;

  case PAIRING:    //menerima hasil pairing dari esp master
    memcpy(&pairingData, incomingData, sizeof(pairingData));
    if (pairingData.id == 0) {              
      printMAC(mac_addr);
      Serial.print("Pairing done for ");
      printMAC(pairingData.macAddr);
      Serial.print(" on channel " );
      Serial.print(pairingData.channel);   
      Serial.print(" in ");
      Serial.print(millis()-start);
      Serial.println("ms");
      addPeer(pairingData.macAddr, pairingData.channel); 
      #ifdef SAVE_CHANNEL
        lastChannel = pairingData.channel;
        EEPROM.write(0, pairingData.channel);
        EEPROM.commit();
      #endif  
      pairingStatus = PAIR_PAIRED;        
    }
    break;
  }

}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if(status == ESP_NOW_SEND_FAIL){
     digitalWrite(LED_PIN, HIGH);
     Serial.println("on");
     delay(250);
     digitalWrite(LED_PIN, LOW);
     Serial.println("off");
  }else if(status == ESP_NOW_SEND_SUCCESS){
      digitalWrite(LED_PIN, HIGH);
      Serial.println("on1");
  }
}

void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

void addPeer(const uint8_t * mac_addr, uint8_t chan){
  esp_now_peer_info_t peer;
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_set_channel(chan ,WIFI_SECOND_CHAN_NONE));
  esp_now_del_peer(mac_addr);
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = chan;
  peer.encrypt = false;
  memcpy(peer.peer_addr, mac_addr, sizeof(uint8_t[6]));
  if (esp_now_add_peer(&peer) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  memcpy(broadcastAddress, mac_addr, sizeof(uint8_t[6]));
}

PairingStatus autoPairing(){
  switch(pairingStatus) {
    case PAIR_REQUEST:{
    Serial.print("Pairing request on channel "  );
    Serial.println(channel);

    //Mengatur WiFi Channel  
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
  
    //Mengirim pesan pairing ke server
    pairingData.msgType = PAIRING;
    pairingData.id = BOARD_ID;     
    pairingData.channel = channel;

    addPeer(broadcastAddress, channel);
    esp_now_send(broadcastAddress, (uint8_t *) &pairingData, sizeof(pairingData));
    previousMillis = millis();
    pairingStatus = PAIR_REQUESTED;
    break;}

    case PAIR_REQUESTED:{
    //looping 1-11 untuk channel
    currentMillis = millis();
    if(currentMillis - previousMillis > 500) {
      previousMillis = currentMillis;
      channel ++;
      if (channel > MAX_CHANNEL){
         channel = 1;
      }   
      pairingStatus = PAIR_REQUEST;
    }
    break;}

    case PAIR_PAIRED:{
    break;}
  }
  return pairingStatus;
}  

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
  WiFi.mode(WIFI_STA);   //Atur sebagai wifi station
  WiFi.disconnect();
  // Mengatur ke channel sesuai ESP-NOW
   #ifdef SAVE_CHANNEL 
    EEPROM.begin(10);
    lastChannel = EEPROM.read(0);
    Serial.println(lastChannel);
    if (lastChannel >= 1 && lastChannel <= MAX_CHANNEL) {
      channel = lastChannel; 
    }
    Serial.print("Wifi Channel: ");
    Serial.println(channel);
  #endif  
  pairingStatus = PAIR_REQUEST;
  bool status;
  Serial.println(WiFi.macAddress());
}
 
void loop() {
  //Tutup valve
  while (autoPairing() == PAIR_PAIRED) {
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  if(myData.jumlahair != 0){
    //Buka valve jika data air tidak sama dengan 0
    Serial.print("Perintah diterima : ");
    Serial.print(myData.jumlahair);
    Serial.println(" ml");
    volume = 2.663 * pulse; //fungsi menghitung volume
    Serial.println("on2");
  if (millis() - lastTime > 20000) {
    pulse = 0;
    lastTime = millis();
  }
  while(volume <= myData.jumlahair){
    //looping agar valve tetap terbuka
    state = 1;
    int volume1 = 2.663 * pulse;
    if(volume1 < myData.jumlahair){
      digitalWrite(LED_PIN, HIGH);
      Serial.println("on3");
      delay(300);
      digitalWrite(LED_PIN, LOW);
      Serial.println("off1");
    }
    digitalWrite(RELAY_PIN, LOW);
    Serial.print("Status : ");
    Serial.println(state);
    Serial.print(volume1);
    Serial.println("ml");
    if(volume1 > myData.jumlahair || volume1 < 0){
      //Tutup valve
       digitalWrite(RELAY_PIN, HIGH);
       myData.jumlahair = 0;
       break;
       }
       delay(500);
  }
  }
  digitalWrite(RELAY_PIN, HIGH);
  delay(500);
}}

//Menyimpan pulse waterflow sementara
ICACHE_RAM_ATTR void increase() {
  pulse++;
}
