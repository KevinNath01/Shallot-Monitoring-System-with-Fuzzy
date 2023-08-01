# How to Install Node-RED on Raspberry Pi
1. Execute the command on Raspberry Pi cli <br />
   ```bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)```
2. To make node-RED auto start on boot, execute this command<br />
```sudo systemctl enable nodered.service```
3. Open Node-RED on ```http://<hostname>:1880``` at any pc that connected in the same network
4. Import [JSON File](ShalotMonitoringSystem.json).
5. Open the dashboard on ```http://<hostname>:1880/ui```


# Node-RED Configuration
This is the flow of node-RED which can be installed in Raspberry Pi and can be accessed using Raspberry Pi IP Address (localhost:1880).

Main Features : 
1. Monitoring System
2. pH Balancer System
3. Warning Automation System
4. Fuzzy Warning System

Additional Features :
1. Manual Operating System
2. Data Logging in SQLite Database
3. Chart Hourly, Daily and Weekly
4. Whatsapp Warning Notification

Please change the configuration below :
1. MQTT IP Address Node in and out (MQTT Node)
2. Whatsapp API Binding (Whatsapp Node)
