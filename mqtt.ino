void MQTTsend() // send all data to MQTT-Server
{
  // StaticJsonBuffer<300> JSONbuffer;
  DynamicJsonBuffer JSONbuffer;

  JsonObject& JSONencoder = JSONbuffer.createObject();
  if (client.connect(ChipID, MQTT_USER, MQTT_PASSWD))
  {
    client.subscribe(ChipID);
    commSerial.println("Publishing ... ");
    TRACE;

    JSONencoder["Total Voltage"] = (float)packBasicInfo.Volts / 1000.0;
    JSONencoder["Current"] = (float)packBasicInfo.Amps / 1000;

    JSONencoder["Ah"] = (float)packBasicInfo.CapacityRemainAh / 1000;
    JSONencoder["Percent"] = (float)packBasicInfo.CapacityRemainPercent;
    JSONencoder["Temp1"] = (float)packBasicInfo.Temp1 / 10;
    JSONencoder["Temp2"] = (float)packBasicInfo.Temp2 / 10;

    JSONencoder["Cell Max"] = (float)packCellInfo.CellMax / 1000.0;
    JSONencoder["Cell Min"] =  (float)packCellInfo.CellMin / 1000.0;
    JSONencoder["Cell Delta"] = (float)packCellInfo.CellDiff;

    char JSONmessageBuffer[1000];
    JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    commSerial.println("Sending message to MQTT topic..");
    commSerial.println(JSONmessageBuffer);
    client.publish("SmartBMS", JSONmessageBuffer);



  }
  else
  {
    commSerial.println("Error connecting to MQTT broker");
  }
  /* Close MQTT client cleanly */
  client.disconnect();
  delay(5000);
}
