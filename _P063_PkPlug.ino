//#######################################################################################################
//#################################### Plugin 063: PKPowerPlug ################################################
//#######################################################################################################

#define PLUGIN_063
#define PLUGIN_ID_063         63
#define PLUGIN_NAME_063       "PK Power Plug"
#define PLUGIN_VALUENAME1_063 "PKPowerPlug"
//#define DEBUG_063

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
    // I might use this to do calibaration stuff...
    break;
  }

  case PLUGIN_WEBFORM_SAVE:
  {
    // I might use this to do calibaration stuff...
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
    swSer->begin(4800);
    swSer->setParity(1);
    success = true;
    break;
  }

  case PLUGIN_TEN_PER_SECOND:
  {
    success = true;
    break;
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
      char regData[8];
      int regLen;

      if (GetArgv(inputs, TmpStr1, 2)) regAddr = strtoul(TmpStr1, NULL, 16);
      if (GetArgv(inputs, TmpStr1, 3)) regLen = strToHexStr(TmpStr1, regData);

      if(regLen) {  
        rn8209_writeEnable();
        rn8209_writeReg(regData, regAddr, regLen, true);
        rn8209_writeDisable();
        log = String(F("PKPowerPlug   : RegAddr ")) + String(regAddr, HEX) + String(F(" Set to ")) + hexstrToString(regData, regLen);
        addLog(LOG_LEVEL_INFO, log);

        char data[8];
        int dataLen = 0;
        dataLen = rn8209_readReg(data, regAddr, regLen);
        if(dataLen) {
          String tmp = "";
          tmp += F("{\"regAddr\":\"");
          tmp += String(regAddr);
          tmp += F("\", \"regData\":\"");
          tmp += hexstrToString(data, regLen);
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
      if (GetArgv(inputs, TmpStr1, 3)) regLen = strtoul(TmpStr1, NULL, 10);

      char regData[8];
      int dataLen = 0;
      Serial.print("regAddr: ");
      Serial.print(regAddr, HEX);
      Serial.print(" regLen: ");
      Serial.println(regLen);

      dataLen = rn8209_readReg(regData, regAddr, regLen);
      if(dataLen) {
        String tmp = "";
        tmp += F("{\"regAddr\":\"");
        tmp += String(regAddr, HEX);
        tmp += F("\", \"regData\":\"");
        tmp += hexstrToString(regData, regLen);
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
  char data[1] = {0xE5};
  rn8209_writeReg(data, 0xEA, 1, 0);
}

void rn8209_writeDisable() {
  char data[1] = {0xDC};
  rn8209_writeReg(data, 0xEA, 1, 0);
}

int rn8209_writeReg(char *src, char regAddr, int len, bool verify) {
  int retryCnt = 2;
  int bufLen;
  char cksum;

#ifdef DEBUG_063
  Serial.print("[rn8209_writeReg] regAddr: ");
  Serial.print(regAddr, HEX);
  Serial.print(" regLen: ");
  Serial.print(len);

  Serial.print(" data: ");
  printBuf(src, len);
#endif

  regAddr |= 0x80;

  while (retryCnt-- > 0) {
    // initialize variables
    bufLen = 0;
    cksum = regAddr;
    swSer->write(regAddr);
    delay(1);
    for (int i = 0; i < len; i++) {
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
      char tmp[8];
      int dataLen;
      dataLen = rn8209_readReg(tmp, regAddr & 0x7f, len);
      if (dataLen == 0) {
        Serial.println("[rn8209_writeReg] failed to get data to verify");
        continue;
      } else {
        if((long) *src != (long) *tmp) {
#ifdef DEBUG_063        
          Serial.println("[rn8209_writeReg] setValue != readValue");
          Serial.print("setValue: ");
          Serial.print((long) *src));
          Serial.print(" readValue: ");
          Serial.println((long) *tmp);
#endif
          continue;
        }
      }
    }
    return true;
  }
}

int rn8209_readReg(char *dst, char regAddr, int len) {
  int retryCnt = 2;
  int bufLen;
  char cksum;

#ifdef DEBUG_063
  Serial.print("[rn8209_readReg] regAddr: ");
  Serial.print(regAddr, HEX);
  Serial.print(" regLen: ");
  Serial.println(len);
#endif

  swSer->listen();
  while (retryCnt-- > 0) {
    // initialize variables
    bufLen = 0;
    cksum = regAddr;
    swSer->write(regAddr);
    delay(20);

    while (swSer->available() > 0) {
      dst[bufLen] = swSer->read();
      cksum += dst[bufLen];
      bufLen++;
      if (bufLen >= len + 1) // including checksum
        break;
    }
    if (bufLen != len + 1) {
      Serial.println("[rn8209_readReg] Get fewer bytes");
      continue;
    }
    cksum = ~cksum;
    if (!cksum) {
      //Serial.println("[rn8209_readReg] checksum is correct");
      dst[len] = 0; // remove checksum.
      swSer->stopListening();
      return len;
    } else {
      //Serial.println("[rn8209_readReg] checksum is NOT correct");
      continue;
    }
  }
  swSer->stopListening();
  return 0;
}
