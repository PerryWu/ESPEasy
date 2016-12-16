//********************************************************************************
// Interface for Sending to Controllers
//********************************************************************************
boolean sendData(struct EventStruct *event)
{
  LoadTaskSettings(event->TaskIndex);
  if (Settings.UseRules)
    createRuleEvents(event->TaskIndex);

  if (Settings.GlobalSync && Settings.TaskDeviceGlobalSync[event->TaskIndex])
    SendUDPTaskData(0, event->TaskIndex, event->TaskIndex);

  if (!Settings.TaskDeviceSendData[event->TaskIndex])
    return false;

  if (Settings.MessageDelay != 0)
  {
    uint16_t dif = millis() - lastSend;
    if (dif < Settings.MessageDelay)
    {
      uint16_t delayms = Settings.MessageDelay - dif;
      char log[30];
      sprintf_P(log, PSTR("HTTP : Delay %u ms"), delayms);
      addLog(LOG_LEVEL_DEBUG_MORE, log);
      unsigned long timer = millis() + delayms;
      while (millis() < timer)
        backgroundtasks();
    }
  }

  LoadTaskSettings(event->TaskIndex); // could have changed during background tasks.

  if (Settings.Protocol)
  {
    byte ProtocolIndex = getProtocolIndex(Settings.Protocol);
    CPlugin_ptr[ProtocolIndex](CPLUGIN_PROTOCOL_SEND, event, dummyString);
  }
  PluginCall(PLUGIN_EVENT_OUT, event, dummyString);
  lastSend = millis();
}


/*********************************************************************************************\
 * Handle incoming MQTT messages
\*********************************************************************************************/
// handle MQTT messages
void callback(char* c_topic, byte* b_payload, unsigned int length) {
  char log[256];
  char c_payload[256];
  strncpy(c_payload,(char*)b_payload,length);
  c_payload[length] = 0;
  //statusLED(true);
  sprintf_P(log, PSTR("%s%s"), "MQTT : Topic: ", c_topic);
  Serial.println(log);
  addLog(LOG_LEVEL_DEBUG, log);
  sprintf_P(log, PSTR("%s%s"), "MQTT : Payload: ", c_payload);
  addLog(LOG_LEVEL_DEBUG, log);

  struct EventStruct TempEvent;
  TempEvent.String1 = c_topic;
  TempEvent.String2 = c_payload;
  byte ProtocolIndex = getProtocolIndex(Settings.Protocol);
  CPlugin_ptr[ProtocolIndex](CPLUGIN_PROTOCOL_RECV, &TempEvent, dummyString);
}


/*********************************************************************************************\
 * Connect to MQTT message broker
\*********************************************************************************************/
void MQTTConnect()
{
#if FEATURE_MQTT_SSL
  if(Settings.SecureProtocol)
    MQTTclient.setClient(mqtts);
  else
    MQTTclient.setClient(mqtt);    
#else
  MQTTclient.setClient(mqtt);    
#endif  
  
  IPAddress MQTTBrokerIP(Settings.Controller_IP);
  MQTTclient.setServer(MQTTBrokerIP, Settings.ControllerPort);
  MQTTclient.setCallback(callback);

  // MQTT needs a unique clientname to subscribe to broker
  String clientid = clientIdString;
  String subscribeTo = "";

  String LWTTopic = Settings.MQTTsubscribe;
  LWTTopic.replace("/#", "/status");
  LWTTopic.replace("%sysname%", Settings.Name);
  
  for (byte x = 1; x < 3; x++)
  {
    String log = "";
    boolean MQTTresult = false;

    //boolean connect(const char* id);
    //boolean connect(const char* id, const char* user, const char* pass);
    //boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
    //boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);

    if ((SecuritySettings.ControllerUser[0] != 0) && (SecuritySettings.ControllerPassword[0] != 0))
      MQTTresult = MQTTclient.connect(clientid.c_str(), SecuritySettings.ControllerUser, SecuritySettings.ControllerPassword, LWTTopic.c_str(), 0, 0, "Connection Lost");
    else
      MQTTresult = MQTTclient.connect(clientid.c_str(), LWTTopic.c_str(), 0, 0, "Connection Lost");

    if (MQTTresult)
    {
      // Add verify server fingerprint sample codes.
      /*
      #if FEATURE_MQTT_SSL
      const char* fingerprint = F("26 96 1C 2A 51 07 FD 15 80 96 93 AE F7 32 CE B9 0D 01 55 C4");
      boolean verified = secureClient.verify(fingerprint, "servername");
      Serial.print(verified ? "verified tls!" : "unverified tls");
      #endif
      */
      log = F("MQTT : Connected to broker");
      addLog(LOG_LEVEL_INFO, log);
      subscribeTo = Settings.MQTTsubscribe;
      subscribeTo.replace("%sysname%", Settings.Name);
      MQTTclient.subscribe(subscribeTo.c_str());
      log = F("Subscribed to: ");
      log += subscribeTo;
      addLog(LOG_LEVEL_INFO, log);
      break; // end loop if succesfull
    }
    else
    {
      log = F("MQTT : Failed to connected to broker");
      addLog(LOG_LEVEL_ERROR, log);
    }

    delay(500);
  }
}


/*********************************************************************************************\
 * Check connection MQTT message broker
\*********************************************************************************************/
void MQTTCheck()
{
  byte ProtocolIndex = getProtocolIndex(Settings.Protocol);
  if (Protocol[ProtocolIndex].usesMQTT)
    if (!MQTTclient.connected())
    {
      String log = F("MQTT : Connection lost");
      addLog(LOG_LEVEL_ERROR, log);
      connectionFailures += 2;
      MQTTclient.disconnect();
      delay(1000);
      MQTTConnect();
    }
    else if (connectionFailures)
      connectionFailures--;
}


/*********************************************************************************************\
 * Send status info to request source
\*********************************************************************************************/

void SendStatus(byte source, String status)
{
  switch(source)
  {
    case VALUE_SOURCE_HTTP:
      if (printToWeb)
        printWebString += status;
      break;
    case VALUE_SOURCE_MQTT:
      MQTTStatus(status);
      break;
    case VALUE_SOURCE_SERIAL:
      Serial.println(status);
      break;
  }
}


/*********************************************************************************************\
 * Send status info back to channel where request came from
\*********************************************************************************************/
void MQTTStatus(String& status)
{ 
  // XXX: Perry: don't know why oroginal code use Settings.MQTTSubscribe
  // to send bak request. Subscribe should only be used for others to send
  // command to device. So, Change to use MQTTpublish here.
  String pubname = Settings.MQTTpublish;
  pubname.replace("/#", "/status");
  pubname.replace("%sysname%", Settings.Name);
  MQTTclient.publish(pubname.c_str(), status.c_str(),Settings.MQTTRetainFlag);
}



