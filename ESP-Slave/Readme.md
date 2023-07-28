# ESP-Slave Configuration

ESP-Slave is the ESP32 that connected to the sensors and have a job to retrieving data from sensor and send it to ESP-Master to be processed. The communication to ESP-Master is done with ESP-NOW Protocol so it doesn't need wifi or bluetooth.
This sensor will send JSON data every 3 seconds and the soil pH value is averaged from 20 sensor reading data to ensure a more stable data reading.

The sensor use in this project is : 
1. BME280
2. Local Soil pH sensor
3. Soil MOisture Sensor

Before using please configure this parameter below :
1. Define Max Channel 11/13 according to your country (Line 18)
2. ESP-Master MAC Address (Line 28)
3. pH Function based on your calibration result (Line 212)
4. BoardID (Line 255)
