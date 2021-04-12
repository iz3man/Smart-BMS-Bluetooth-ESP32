# Smart-BMS-Bluetooth-ESP32

This is modified code from others (see below) where I removed code for display and such and added code for Wifi and MQTT.
This code connects to BMSs like these; https://www.lithiumbatterypcb.com/  via Bluetooth, read it's data, then connects to a give MQTT-server and sends this data.

All code needed to be changed is in config.h - just set you Wifi data/credentials and configure your MQTT server.
Then connect ESP32 (tested with Wemos D1 mini 32) to serial and check serial console. The connection process of BT, Wifi and MQTT is logged there. This may help to see if everything works as expected.

based on code (c) Miroslav Kolinsky 2019  https://www.kolins.cz  

known bugs:
* if BLE server is not available during startup, program hangs
* reconnection sort of works, sometimes ESP reboots
* GUI needs to be done

todo:

For me it's not of real relevance, but if you want to see EVERY single cell's voltage you may need to add a routine for this. If you do so I'd be pleased to be informed to add that code to mine.
