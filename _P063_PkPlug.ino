//#######################################################################################################
//#################################### Plugin 063: PKPowerPlug ################################################
//#######################################################################################################

#define PLUGIN_063
#define PLUGIN_ID_063         63
#define PLUGIN_NAME_063       "PK Power Plug"
#define PLUGIN_VALUENAME1_063 "PKPowerPlug"

//#define DEBUG_063
#define MAXRECORD 50 // every second records ten times, total 5 seconds
struct powerInfo {
  uint32_t iarms;
};

struct {
  unsigned int idx;
  struct powerInfo records[MAXRECORD];
} Power;

boolean Plugin_063(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

  case PLUGIN_DEVICE_ADD:
  {
    Device[++deviceCount].Number = PLUGIN_ID_063;
    Device[deviceCount].Type = DEVICE_TYPE_DUAL;
    Device[deviceCount].VType = SENSOR_TYPE_CUSTOM;
    Device[deviceCount].Ports = 0;
    Device[deviceCount].PullUpOption = false;
    Device[deviceCount].InverseLogicOption = false;
    Device[deviceCount].FormulaOption = false;
    Device[deviceCount].DecimalsOnly = true;
    Device[deviceCount].ValueCount = 0;
    Device[deviceCount].SendDataOption = true;
    Device[deviceCount].TimerOption = true;
    Device[deviceCount].GlobalSyncOption = true;
    break;
  }

  case PLUGIN_GET_DEVICENAME:
  {
    string = F(PLUGIN_NAME_063);
    break;
  }

  case PLUGIN_GET_DEVICEVALUENAMES:
  {
    strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_063));
    break;
  }

  case PLUGIN_WEBFORM_LOAD:
  {
    // do calibaration stuff...
    uint32_t value;
    char tmpString[128];
    // Show registers
    if(Settings.TaskDevicePin1[event->TaskIndex] != -1 && Settings.TaskDevicePin2[event->TaskIndex] != -1) {
      rn8209_readReg(&value, 0x22, 3);
      sprintf_P(tmpString, PSTR("<TR><TD>IARMS Value:<TD><p>%d</p>"), value);
      string += tmpString;
      rn8209_readReg(&value, 0x24, 3);
      sprintf_P(tmpString, PSTR("<TR><TD>URMS Value:<TD><p>%d</p>"), value);
      string += tmpString;
      rn8209_readReg(&value, 0x25, 2);
      sprintf_P(tmpString, PSTR("<TR><TD>UFREQ Value:<TD><p>%d</p>"), value);
      string += tmpString;
      rn8209_readReg(&value, 0x26, 4);
      sprintf_P(tmpString, PSTR("<TR><TD>POWERP Value:<TD><p>%d</p>"), value);
      string += tmpString;
      rn8209_readReg(&value, 0x29, 3);
      sprintf_P(tmpString, PSTR("<TR><TD>ENERGY Value:<TD><p>%d</p>"), value);
      string += tmpString;
    }

    // Update K values
    sprintf_P(tmpString, PSTR("<TR><TD>IARMS K:<TD><input type='text' name='plugin_063_iarms_k' value='%u'>"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][0]);
    string += tmpString;
    sprintf_P(tmpString, PSTR("<TR><TD>URMS K:<TD><input type='text' name='plugin_063_urms_k' value='%u'>"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][1]);
    string += tmpString;
    sprintf_P(tmpString, PSTR("<TR><TD>UFREQ K:<TD><input type='text' name='plugin_063_ufreq_k' value='%u'>"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][2]);
    string += tmpString;
    sprintf_P(tmpString, PSTR("<TR><TD>POWERP K:<TD><input type='text' name='plugin_063_powerp_k' value='%u'>"), Settings.TaskDevicePluginConfigLong[event->TaskIndex][3]);
    string += tmpString;
    success = true;
    break;
  }

  case PLUGIN_WEBFORM_SAVE:
  {
    Serial.println(event->TaskIndex);
    // do calibaration stuff...
    String input = WebServer.arg("plugin_063_iarms_k");
    Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = input.toInt();
    input = WebServer.arg("plugin_063_urms_k");
    Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = input.toInt();
    input = WebServer.arg("plugin_063_ufreq_k");
    Settings.TaskDevicePluginConfigLong[event->TaskIndex][2] = input.toInt();
    input = WebServer.arg("plugin_063_powerp_k");
    Settings.TaskDevicePluginConfigLong[event->TaskIndex][3] = input.toInt();
    success = true;
    break;
  }

  case PLUGIN_READ:
  {
    success = true;
    break;
  }

  case PLUGIN_INIT:
  {
    if (swSer == NULL) {
      swSer = new SoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex], false, 256);
    } else {
      delete swSer;
      swSer = new SoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex], false, 256);
    }
    Serial.println("Baud Rate 4800");
    swSer->begin(4800);
    swSer->setParity(1);

    if(Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] == 0) {
      // basic setup.
      Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = 1;
      Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = 1;
      Settings.TaskDevicePluginConfigLong[event->TaskIndex][2] = 1;
      Settings.TaskDevicePluginConfigLong[event->TaskIndex][3] = 1;
    }
    success = true;
    break;
  }

  case PLUGIN_ONCE_A_SECOND:
  {
    break;
  }

  case PLUGIN_TEN_PER_SECOND:
  {
    if(WiFi.status() != WL_CONNECTED || !MQTTclient.connected()) {
      //Serial.println("Failed to conect to remote");
      break;
    }

    static int index = 0;

    Power.idx = Settings.TaskDeviceID[event->TaskIndex];
    rn8209_readReg(&Power.records[index].iarms, 0x22, 3);
    Power.records[index].iarms *= Settings.TaskDevicePluginConfigLong[event->TaskIndex][0];
    if(index == MAXRECORD -1) {
      MQTTStatusBinary((char *)&Power, sizeof(Power));
      index = 0;
    } else 
      index++;
    success = true;
    break;
  }

  case PLUGIN_WRITEJSON:
  {
    JsonObject& root = *(event->root);
    if(event->root == NULL)
      break;
    bool hit = false;
    String command = (const char *)root["event"];
    if(command == F("pkpmwritereg")) {
      hit = true;
      string = "pkpmwritereg,";
      string += (const char *)root["regAddr"];
      string += ",";
      string += (const char *)root["regData"];
    }
    if(command == F("pkpmreadreg")) {
      hit = true;
      string = "pkpmreadreg,";
      string += (const char *)root["regAddr"];
      string += ",";
      string += (const char *)root["regLen"];
    }
    Serial.print("command: ");
    Serial.println(command);
    Serial.println(string);

    if(hit == false)
      break;
    // Pass through to PLUGIN_WRITE to handle command
  }

  case PLUGIN_WRITE:
  { // string will be formatted: cmd,par1,par2,par3...
    String log = "";
    String command = parseString(string, 1);
    char inputs[80];
    char TmpStr1[80];
    string.toCharArray(inputs, 80);

    Serial.print("Receive Event: PLUGIN_WRITE, with string: ");
    Serial.print(string);
    Serial.print(" and command: ");
    Serial.println(command);

    if (command == F("pkpmwritereg"))
    {
      char regAddr = 0;
      uint32_t regData;
      int regLen;

      if (GetArgv(inputs, TmpStr1, 2)) regAddr = strtoul(TmpStr1, NULL, 16);
      //if (GetArgv(inputs, TmpStr1, 3)) regLen = strToHexStr(TmpStr1, regData);
      regData = event->Par2;
      regLen = event->Par3;

      if(regLen) {  
        rn8209_writeEnable();
        rn8209_writeReg(regData, regAddr, regLen, true);
        rn8209_writeDisable();
        log = String(F("PKPowerPlug   : RegAddr ")) + String(regAddr, HEX) + String(F(" Set to ")) + String(regData, HEX);
        addLog(LOG_LEVEL_INFO, log);

        uint32_t readData;
        int readLen = 0;
        readLen = rn8209_readReg(&readData, regAddr, regLen);
        if(readLen) {
          String tmp = "";
          tmp += F("{\"regAddr\":\"");
          tmp += String(regAddr);
          tmp += F("\", \"regData\":\"");
          tmp += String(readData, HEX);
          tmp += F("\"}");
          SendStatus(event->Source, getReportJson(event->idx, "pkPmRegData", (char *)tmp.c_str()));
        } else {
          SendStatus(event->Source, getReportJson(event->idx, "error", "{\"event\": \"pkPmWriteReg\"}"));
        }
      }
      // TODO: user SendStatus to report status
      success = true;
    }

    if (command == F("pkpmreadreg"))
    {
      char regAddr = 0;
      int regLen = 0;

      if (GetArgv(inputs, TmpStr1, 2)) regAddr = strtoul(TmpStr1, NULL, 16);
      //if (GetArgv(inputs, TmpStr1, 3)) regLen = strtoul(TmpStr1, NULL, 10);
      regLen = event->Par2;

      uint32_t readData;
      int readLen = 0;
      Serial.print("regAddr: ");
      Serial.print(regAddr, HEX);
      Serial.print(" regLen: ");
      Serial.println(regLen);

      readLen = rn8209_readReg(&readData, regAddr, regLen);
      if(readLen) {
        String tmp = "";
        tmp += F("{\"regAddr\":\"");
        tmp += String(regAddr, HEX);
        tmp += F("\", \"regData\":\"");
        tmp += String(readData, HEX);
        tmp += F("\"}");
        SendStatus(event->Source, getReportJson(event->idx, "pkPmRegData", (char *)tmp.c_str()));
      } else {
        SendStatus(event->Source, getReportJson(event->idx, "error", "{\"event\": \"pkPmReadReg\"}"));
      }
      success = true;
    }
    break;
  }

  }
  return success;
}

void rn8209_writeEnable() {
  char data[4] = {0xE5,0,0,0};
  rn8209_writeReg(*(uint32_t *) data, 0xEA, 1, 0);
}

void rn8209_writeDisable() {
  char data[4] = {0xDC,0,0,0};
  rn8209_writeReg(*(uint32_t *) data, 0xEA, 1, 0);
}

int rn8209_writeReg(uint32_t srcData, char regAddr, int regLen, bool verify) {
  int retryCnt = 2;
  int bufLen;
  char cksum;
  uint32_t writeData = htonl(srcData);
  char *src, *dataPtr = (char *)&writeData;
  src = dataPtr + 4 - regLen;

#ifdef DEBUG_063
  Serial.print("[rn8209_writeReg] regAddr: ");
  Serial.print(regAddr, HEX);
  Serial.print(" regLen: ");
  Serial.print(regLen);

  Serial.print(" data: ");
  printBuf(src, regLen);
#endif

  regAddr |= 0x80;

  while (retryCnt-- > 0) {
    // initialize variables
    bufLen = 0;
    cksum = regAddr;
    swSer->write(regAddr);
    delay(1);
    for (int i = 0; i < regLen; i++) {
      swSer->write(src[i]);
      cksum += src[i];
      delay(1);
    }
    cksum = ~cksum;
    swSer->write(cksum);
    delay(1);
#ifdef DEBUG_063
    Serial.print("[rn8209_writeReg] cksum: ");
    Serial.println(cksum, HEX);
#endif

    if (verify) {
      uint32_t readData;
      int readLen;
      readLen = rn8209_readReg(&readData, regAddr & 0x7f, regLen);
      if (readLen != regLen) {
        Serial.println("[rn8209_writeReg] failed to get data to verify");
        continue;
      } else {
        if(readData != srcData) {
#ifdef DEBUG_063        
          Serial.println("[rn8209_writeReg] setValue != readValue");
          Serial.print("srcData: ");
          Serial.print(srcData, HEX);
          Serial.print(" readData: ");
          Serial.println(readData, HEX);
#endif
          continue;
        }
      }
    }
    return true;
  }
}

int rn8209_readReg(uint32_t *dstData, char regAddr, int regLen) {
  int retryCnt = 2;
  int bufLen;
  char cksum, readByte;
  char *dst = ((char *) dstData) + 4 - regLen;
  *dstData = 0; // reset

  //return regLen;

#ifdef DEBUG_063
  Serial.print(millis());
  Serial.print("[rn8209_readReg] regAddr: ");
  Serial.print(regAddr, HEX);
  Serial.print(" regLen: ");
  Serial.println(regLen);
#endif
  while (retryCnt-- > 0) {
    // initialize variables
    bufLen = 0;
    cksum = regAddr;
    //ESP.wdtFeed();
    swSer->flush();
    swSer->listen();
    swSer->write(regAddr);
    delay(20);

    swSer->stopListening();
    while (swSer->available() > 0) {
      yield();
      readByte = swSer->read();
      cksum += readByte;
      if(bufLen < regLen)
        dst[bufLen] = readByte;
      bufLen++;
      if (bufLen >= regLen + 1) // including checksum
        break;
    }
    if (bufLen != regLen + 1) {
      Serial.println("[rn8209_readReg] Get fewer bytes");
      continue;
    }
    cksum = ~cksum;
    if (!cksum) {
      //Serial.println("[rn8209_readReg] checksum is correct");
      // Change the order
      *dstData = htonl(*dstData);
      return regLen;
    } else {
      Serial.println("[rn8209_readReg] checksum is NOT correct");
      continue;
    }
  }
  Serial.println("[rn8209_readReg] read was failed");
  return 0;
}
