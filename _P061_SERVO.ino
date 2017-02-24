//#######################################################################################################
//#################################### Plugin 061: Servo ############################################
//#######################################################################################################

#define PLUGIN_061
#define PLUGIN_ID_061         61
#define PLUGIN_NAME_061       "Servo"
#define PLUGIN_VALUENAME1_061 "Servo"

boolean Plugin_061(byte function, struct EventStruct *event, String& string)
{
    boolean success = false;

    switch (function)
    {

    case PLUGIN_DEVICE_ADD:
    {
        Device[++deviceCount].Number = PLUGIN_ID_061;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].GlobalSyncOption = true;
        break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
        string = F(PLUGIN_NAME_061);
        break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_061));
        break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[2];
        options[0] = F("1");
        options[1] = F("2");
        int optionValues[2];
        optionValues[0] = 1;
        optionValues[1] = 2;
        string += F("<TR><TD>Servo Index:<TD><select name='plugin_061_servoid'>");
        for (byte x = 0; x < 2; x++)
        {
            string += F("<option value='");
            string += optionValues[x];
            string += "'";
            if (choice == optionValues[x])
                string += F(" selected");
            string += ">";
            string += options[x];
            string += F("</option>");
        }
        string += F("</select>");

        success = true;
        break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
        String plugin1 = WebServer.arg("plugin_061_servoid");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        success = true;
        break;
    }

    case PLUGIN_INIT:
    {
        success = true;
        break;
    }

    case PLUGIN_WRITEJSON:
    {
        if(Settings.TaskDeviceNumber[event->TaskIndex] != PLUGIN_ID_061) {
            // Not our task...
            break;
        }
        
        JsonObject& root = *(event->root);
        //Serial.println("PLUGIN_WRITEJSON");
        if (event->root == NULL)
            break;
        String command = (const char *)root["e"];
        //Serial.println(command);
        if (command == F("servo")) {
            int angle = (int) root["v"];
            //Serial.println(angle);
            //int angle = value.toInt();
            int servoId = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
            int pin = Settings.TaskDevicePin1[event->TaskIndex];
            /*
            Serial.print("Pin:");
            Serial.print(pin);
            Serial.print(" servoId:");
            Serial.print(servoId);
            Serial.print(" angle:");
            Serial.println(angle);
            */

            if (angle != -1 && (angle < 0 || angle > 180)) {
                Serial.println("The value was incorrect");
                break;
            }

            switch (servoId)
            {
            case 1:
                if (angle == -1) {
                    if (myservo1.attached())
                        myservo1.detach();
                } else {
                    myservo1.attach(pin);
                    myservo1.write(angle);
                }
                break;
            case 2:
                if (angle == -1) {
                    if (myservo2.attached())
                        myservo2.detach();
                } else {
                    myservo2.attach(pin);
                    myservo2.write(angle);
                }
                break;
            }
            String log = "";
            setPinState(PLUGIN_ID_061, pin, PIN_MODE_SERVO, angle);
            log = String(F("SW   : SERVO ")) + String(pin) + String(F(" Servo set to ")) + String(angle);
            addLog(LOG_LEVEL_INFO, log);
            String rst = "\"angle\":" + String(angle);
            SendStatus(event->Source, getReportJson(event->idx, "servo", (char *)rst.c_str()));

            success = true;
            break;
        }
        break;
    }

    } // end switch cases
    return success;
}
