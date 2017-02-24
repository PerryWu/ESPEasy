//#######################################################################################################
//#################################### Plugin 062: Analog PIR############################################
//#######################################################################################################
// Some case, PIR output fails to pull high digital input. We can use ADC to check.

#define PLUGIN_062
#define PLUGIN_ID_062         62
#define PLUGIN_NAME_062       "Analog input - PIR"
#define PLUGIN_VALUENAME1_062 "Analog-PIR"

static byte pirstate[TASKS_MAX];

#define PIR_THRESHOLD 500

boolean Plugin_062(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_062;
        Device[deviceCount].Type = DEVICE_TYPE_ANALOG;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_062);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_062));
        break;
      }
      
    case PLUGIN_INIT:
      {
        int value = analogRead(A0);
        byte state = 0;
        if (value > PIR_THRESHOLD)
          state = 1;
        pirstate[event->TaskIndex] = state;
        success = true;
        break;
      }
    case PLUGIN_TEN_PER_SECOND:
      {
        int value = analogRead(A0);
        byte state = 0;
        if (value > PIR_THRESHOLD)
          state = 1;
        if (state != pirstate[event->TaskIndex])
        {
          // State change!
          pirstate[event->TaskIndex] = state;
          UserVar[event->BaseVarIndex] = state;
          event->sensorType = SENSOR_TYPE_SWITCH;
          String log = F("ADCPIR   : State ");
          log += state;
          addLog(LOG_LEVEL_INFO, log);
          sendData(event);
        }
        success = true;
        break;
      }
  }
  return success;
}
