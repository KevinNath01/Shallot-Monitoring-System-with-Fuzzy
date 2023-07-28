# ESP-Master Configuration

ESP-Master act as a data receiving hub that receive sensor reading data from ESP-Slave on fields using ESP-NOW. The wifi is required to send data to Raspberry Pi Via MQTT. The processed data from raspberry pi also being received and send to ESP-Slave output here.

Please change the parameter below :
1. Wifi (Line 13 and 14)
2. ESP-Slave Output ESP32 MAC Address (Line 15)
3. MQTT Address (Line 222)
4. MQTT Topic Subscribe (Line 165)
