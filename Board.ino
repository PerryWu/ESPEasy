/********************************************************************************************\
* Initialize specific board setings
\*********************************************************************************************/

void boardInit()
{
  bool hit = false;
  uint8_t mac[] = {0, 0, 0, 0, 0, 0};
  uint8_t* macread = WiFi.macAddress(mac);
  char tmp[20];

  // Maximum clientId in mqtt is 23 bytes. so name(10) + mac(13) = 23.
  if(strlen(Settings.Name) >= 10) {
    clientIdString = String(Settings.Name).substring(0,9);
  } else {
    clientIdString = Settings.Name;
  }
  sprintf_P(tmp, PSTR("-%02x%02x%02x%02x%02x%02x"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
  clientIdString += tmp;

  if(Settings.BoardInited == true)
    return;

  if(strcasecmp_P(Settings.Name, PSTR("pkPowerPlug")) == 0) {
    //Settings.Protocol = 0;
    // Have LED
    Settings.Pin_status_led = 16;
    Settings.Protocol = 15;

    // Task 0 should be pkpowerplug
    Settings.TaskDeviceNumber[0] = 63;
    Settings.TaskDevicePin1[0] = 14;
    Settings.TaskDevicePin2[0] = 12;
    Settings.TaskDevicePin3[0] = -1;
    Settings.TaskDevicePin1PullUp[0] = false;
    Settings.TaskDevicePin1Inversed[0] = false;
    Settings.TaskDeviceSendData[0] = true;
    Settings.TaskDeviceTimer[0] = 60;
    Settings.BoardInited = true;

    LoadTaskSettings(0);
    strcpy_P(ExtraTaskSettings.TaskDeviceName, PSTR("Power Plug"));
    SaveTaskSettings(0);
    
    // Task 1 should be a pkswitch

    hit = true;
  }

  if(hit) {
    Serial.println("This device is a pillakloud predefined product.");
    SaveSettings();
    delay(2000);
    ESP.reset();
  } else {
    Settings.BoardInited = true;
    SaveSettings();
  }
  return;
}

