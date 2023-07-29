# Shallot-Monitoring-System-with-Fuzzy
This is a repository of my final project which is designing a monitoring system that can help farmers to prevent fusarium wilt using IoT and fuzzy logic by monitoring 4 parameters which is Air Temperature, Air Humidity, Soil Humidity and Soil pH. The devices in the field called "ESP-Slave" will send the data usign ESP-NOW Protocol to ESP-Master to be processed in Raspberry Pi using Node-RED as the programming language and give output in a form of whatsapp notification and solenoic water valve opening.
Folder named ESP-Master, ESP-Slave Output, ESP-Slave contains the code needed to run the program and it is uploaded to the ESP32 Microcontroller.
Folder named Node-RED contains JSON file that is a flow of Node-RED programming and can be imported to Node-RED
Folder named Information contains extra information that may be usefull for further development
