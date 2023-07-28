//Library yang diperlukan
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>

//Deklarasi Pin
#define pHPin 34
#define LEDPin 14
#define sensor_pin 32

#define BOARD_ID 1
#define MAX_CHANNEL 11
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;        //BME I2C
int moisture,sensor_analog; //Nilai ADC Sensor Kelembapan Tanah
int pHsensorValue = 0;      //Nilai ADC Sensor pH
float pHoutputValue = 0.0;

//MAC Address ESP-Master
uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xD9, 0x28, 0xF4};

//Struct kirim data ke ESP-Master
typedef struct struct_message {
    char msgType;
    int id; 
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

struct_message myData;  //Data yng dikirim
struct_pairing pairingData; //Data AutoPairing

//Deklarasi AutoPairing
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
const long interval = 3000;         //mengatur jarak pengiriman data
unsigned long start;                //mengukur lama terkoneksi 

//Deklarasi pH Meter
unsigned long int avgval;
int buffer_arr[20], temp; 

//Fungsi ketika ada pengiriman data
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  digitalWrite(LEDPin, LOW);
  if(status == ESP_NOW_SEND_FAIL){
      digitalWrite(LEDPin, HIGH);
     delay(300);
     digitalWrite(LEDPin, LOW);
  }else if(status == ESP_NOW_SEND_SUCCESS){
      digitalWrite(LEDPin, HIGH);
  }
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

void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  Serial.print("Packet received from: ");
  printMAC(mac_addr);
  Serial.println();
  Serial.print("data size = ");
  Serial.println(sizeof(incomingData));
  uint8_t type = incomingData[0];
  switch (type) {
  case DATA :      
    Serial.print("We receive Data");
    break;

  case PAIRING:    //Menerima perintah PAIRING dari ESP-Master
    memcpy(&pairingData, incomingData, sizeof(pairingData));
    if (pairingData.id == 0) {              //Pesan dari ESP-Master dengan ID=0
      printMAC(mac_addr);
      Serial.print("Pairing done for ");
      printMAC(pairingData.macAddr);
      Serial.print(" on channel " );
      Serial.print(pairingData.channel);    //Channel yang digunakan
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

PairingStatus autoPairing(){
  switch(pairingStatus) {
    case PAIR_REQUEST:{
    Serial.print("Pairing request on channel "  );
    Serial.println(channel);

    //Mengatur channel WiFi untuk ESP-NOW 
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
  
    //Mengatur data untuk pairing
    pairingData.msgType = PAIRING;
    pairingData.id = BOARD_ID;     
    pairingData.channel = channel;

    //Tambahkan peer dan kirim permintaan pairing
    addPeer(broadcastAddress, channel);
    esp_now_send(broadcastAddress, (uint8_t *) &pairingData, sizeof(pairingData));
    previousMillis = millis();
    pairingStatus = PAIR_REQUESTED;
    break;}

    case PAIR_REQUESTED:{
    currentMillis = millis();
    if(currentMillis - previousMillis > 250) {
      previousMillis = currentMillis;
      //Looping channel wifi 1-11
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
 
//Fungsi ambil nilai pH dengan rata-rata
void getPH() {
 for (int i = 0; i < 20; i++) {
 buffer_arr[i] = analogRead(pHPin);
 delay(30);
 }
 for (int i = 1; i <19; i++) {
 for (int j = i + 1; j < 20; j++) {
 if (buffer_arr[i] > buffer_arr[j]) {
 temp = buffer_arr[i];
 buffer_arr[i] = buffer_arr[j];
 buffer_arr[j] = temp;
 }
 }
 }
 avgval = 0;
 for (int i = 5; i < 16; i++)
 avgval += buffer_arr[i];
 avgval = avgval/11;
 Serial.println(avgval);
 pHoutputValue = (-0.017102944*avgval)+18.12020339;
 memset(buffer_arr,0, sizeof(buffer_arr));
}

void setup() {
  Serial.begin(115200);
  pinMode(14, OUTPUT);
  // Buat device sebagai wifi station
  Serial.print("Client Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

   #ifdef SAVE_CHANNEL 
    EEPROM.begin(10);
    lastChannel = EEPROM.read(0);
    Serial.println(lastChannel);
    if (lastChannel >= 1 && lastChannel <= MAX_CHANNEL) {
      channel = lastChannel; 
    }
    Serial.println(channel);
  #endif  
  pairingStatus = PAIR_REQUEST;
  bool status;

  status = bme.begin(0x76);
    if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
   
}
 
void loop() {
  getPH(); //Nilai pH
  sensor_analog = analogRead(sensor_pin); //Nilai analog sensor kelembapan tanah
  moisture = ( 100 - ( (sensor_analog/4095.00) * 100 ) ); //Penghitungan nilai kelembapan tanah
  while (autoPairing() == PAIR_PAIRED) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      //Mengatur jarak pengiriman data
      previousMillis = currentMillis;
      //Mengatur nilai yang dikirim
      myData.id = 2;
      myData.msgType = DATA;
      myData.temp = bme.readTemperature();  //Data temperatur udara BME280
      myData.humid = bme.readHumidity();    //Data humiditas udara BME280
      myData.soilhumid = moisture;
      myData.pH = pHoutputValue;
      //Kirim data via ESP-NOW
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
        Serial.println(myData.temp);
        Serial.println(myData.humid);
        Serial.println(myData.soilhumid);
        Serial.println(myData.pH);
      }
      else {
        Serial.println("Error sending the data");
      }
    }
    delay(900);
  }

}
