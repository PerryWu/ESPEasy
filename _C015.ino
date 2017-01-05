//#######################################################################################################
//########################### Controller Plugin 015: Domoticz MQTT ######################################
//#######################################################################################################

#define CPLUGIN_015
#define CPLUGIN_ID_015         15
#define CPLUGIN_NAME_015       "PillaKloud MQTT"

boolean CPlugin_015(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_015;
        Protocol[protocolCount].usesMQTT = true;
        Protocol[protocolCount].usesTemplate = true;
        Protocol[protocolCount].usesAccount = true;
        Protocol[protocolCount].usesPassword = true;
        Protocol[protocolCount].defaultPort = 1883;
        Protocol[protocolCount].useSecure = true;
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_015);
        break;
      }
      
    case CPLUGIN_PROTOCOL_TEMPLATE:
      {
        uint8_t mac[] = {0, 0, 0, 0, 0, 0};
        uint8_t* macread = WiFi.macAddress(mac);
        char tmp[20];
        sprintf_P(tmp, PSTR("%02x%02x%02x%02x%02x%02x/out"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
        strcpy_P(Settings.MQTTsubscribe, tmp);
        sprintf_P(tmp, PSTR("%02x%02x%02x%02x%02x%02x/in"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
        strcpy_P(Settings.MQTTpublish, tmp);
        break;
      }

    case CPLUGIN_PROTOCOL_RECV:
      {
        char json[512];
        json[0] = 0;
        event->String2.toCharArray(json, 512);

        StaticJsonBuffer<512> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(json);

        Serial.print("Receive Json: ");
        Serial.println(event->String2);

        // format: {idx:"", event:"switch", value:""}
        // {idx:"", event:"pkPmWriteReg", regAddr:"", regData:""}
        if (root.success())
        {
          unsigned int idx = root["idx"];

          // idx is 0 means we have to check all plugins
          if(idx == 0) {
            struct EventStruct TempEvent;
            String action;
            TempEvent.idx = 0;
            TempEvent.Source = VALUE_SOURCE_MQTT;
            TempEvent.root = &root;
            PluginCall(PLUGIN_WRITEJSON, &TempEvent, action);
            break;
          }
          // idx is not 0, do the task only.
          for (int x = 0; x < TASKS_MAX; x++)
          {
            if (Settings.TaskDeviceID[x] == idx)
            {
              String action;
              byte deviceNumber = Settings.TaskDeviceNumber[x];
              struct EventStruct TempEvent;
              TempEvent.idx = idx;
              TempEvent.TaskIndex = x;
              TempEvent.Source = VALUE_SOURCE_MQTT;
              TempEvent.root = &root;
              for (int y = 0; y < PLUGIN_MAX; y++) {
                if (Plugin_id[y] == deviceNumber)  {
                  Plugin_ptr[y](PLUGIN_WRITEJSON, &TempEvent, action);
                  break;
                }
              }
              break;
            }
          }
        }
        break;
      }

    case CPLUGIN_PROTOCOL_SEND:
      {
        StaticJsonBuffer<200> jsonBuffer;

        JsonObject& root = jsonBuffer.createObject();
        // format: {idx:"", source:"timer/device",event:"report", value:{}}
        // {idx:"", source:"timer/device",event:"pkPmRegData", value:{}}

        root["idx"] = event->idx;
        root["source"] = F("device");
        JsonObject& value = root.createNestedObject("value");
        String inputs;
        char str[80];        
        switch (event->sensorType)
        {
          case SENSOR_TYPE_SINGLE:                      // single value sensor, used for Dallas, BH1750, etc
            inputs = toString(UserVar[event->BaseVarIndex],ExtraTaskSettings.TaskDeviceValueDecimals[0]);
            inputs.toCharArray(str, 80);
            root["event"] = event->sensorType;
            value["data"] = str;
            break;
          case SENSOR_TYPE_LONG:                      // single LONG value, stored in two floats (rfid tags)
            root["event"] = event->sensorType;
            inputs = (unsigned long)UserVar[event->BaseVarIndex] + ((unsigned long)UserVar[event->BaseVarIndex + 1] << 16);
            inputs.toCharArray(str, 80);
            value["data"] = str;
            break;
          case SENSOR_TYPE_DUAL:                       // any sensor that uses two simple values
            root["event"] = event->sensorType;
            inputs = toString(UserVar[event->BaseVarIndex ],ExtraTaskSettings.TaskDeviceValueDecimals[0]);
            inputs.toCharArray(str, 80);
            value["data"] = str;
            inputs = toString(UserVar[event->BaseVarIndex + 1],ExtraTaskSettings.TaskDeviceValueDecimals[1]);
            inputs.toCharArray(str, 80);
            value["data2"] = str;
            break;            
          case SENSOR_TYPE_TEMP_HUM:                      // temp + hum + hum_stat, used for DHT11
            root["event"] = event->sensorType;
            inputs = toString(UserVar[event->BaseVarIndex ],ExtraTaskSettings.TaskDeviceValueDecimals[0]);
            inputs.toCharArray(str, 80);
            value["temp"] = str;
            inputs = toString(UserVar[event->BaseVarIndex + 1],ExtraTaskSettings.TaskDeviceValueDecimals[1]);
            inputs.toCharArray(str, 80);
            value["humid"] = str;
            break;
          case SENSOR_TYPE_TEMP_BARO:                      // temp + hum + hum_stat + bar + bar_fore, used for BMP085
            root["event"] = event->sensorType;
            inputs = toString(UserVar[event->BaseVarIndex ],ExtraTaskSettings.TaskDeviceValueDecimals[0]);
            inputs.toCharArray(str, 80);
            value["temp"] = str;
            inputs = toString(UserVar[event->BaseVarIndex + 1],ExtraTaskSettings.TaskDeviceValueDecimals[1]);
            inputs.toCharArray(str, 80);
            value["humid"] = str;
            break;
          case SENSOR_TYPE_TEMP_HUM_BARO:                      // temp + hum + hum_stat + bar + bar_fore, used for BME280
            root["event"] = event->sensorType;
            inputs = toString(UserVar[event->BaseVarIndex ],ExtraTaskSettings.TaskDeviceValueDecimals[0]);
            inputs.toCharArray(str, 80);
            value["temp"] = str;
            inputs = toString(UserVar[event->BaseVarIndex + 1],ExtraTaskSettings.TaskDeviceValueDecimals[1]);
            inputs.toCharArray(str, 80);
            value["humid"] = str;
            inputs = toString(UserVar[event->BaseVarIndex + 2],ExtraTaskSettings.TaskDeviceValueDecimals[2]);
            inputs.toCharArray(str, 80);
            value["bar"] = str;
            break;
          case SENSOR_TYPE_SWITCH:
            root["event"] = "switch";
            if (UserVar[event->BaseVarIndex] == 0)
              value["value"] = 0;
            else
              value["value"] = 1;
            break;
          case SENSOR_TYPE_DIMMER:
            root["event"] = "switchlight";
            if (UserVar[event->BaseVarIndex] == 0)
              value["value"] = "0";
            else
              value["value"] = UserVar[event->BaseVarIndex];
            break;
          case SENSOR_TYPE_CUSTOM:
            // TODO: Add event and value...
            value["TODO"] = "Add handler";
        }

        char json[256];
        root.printTo(json, sizeof(json));
        String log = F("MQTT : ");
        log += json;
        addLog(LOG_LEVEL_DEBUG, json);

        String pubname = Settings.MQTTpublish;

        if (!MQTTclient.publish(pubname.c_str(), json, Settings.MQTTRetainFlag))
        {
          log = F("MQTT publish failed");
          addLog(LOG_LEVEL_DEBUG, json);
          MQTTConnect();
          connectionFailures++;
        }
        else if (connectionFailures)
          connectionFailures--;
        break;
      }

      case CPLUGIN_WEBFORM_LOAD:
      {
        string += F("<TR><TD>USE SSL:<TD>");
        if (Settings.SecureProtocol)
          string += F("<input type=checkbox name='ssl' checked>");
        else
          string += F("<input type=checkbox name='ssl'>");
        break;
      }

      case CPLUGIN_WEBFORM_SAVE:
      {
        String ssl = WebServer.arg("ssl");
        Settings.SecureProtocol = (ssl == F("on"));
        break;
      }
  }
  return success;
}

