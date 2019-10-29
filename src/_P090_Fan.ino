#ifdef USES_P090

#include "src/DataStructs/PinMode.h"
#include <Arduino.h>
#include <iostream>   // std::cout
#include <string>


// #######################################################################################################
// #################################### Plugin 090: Input Switch #########################################
// #######################################################################################################

/**************************************************\
   CONFIG
   TaskDevicePluginConfig settings:
   0: PWM

   TaskDevicePluginConfigFloat settings:
   0: lowest_speed (percent*10)
   1: fullspeed_Temp (temp*10)
   2: speed_up_Temp (temp*10) 
   2: frequecy :pwm frequcy
   4: full_speed_pwm (100*10) 1000

\**************************************************/

#define PLUGIN_090
#define PLUGIN_ID_090         90
#define PLUGIN_NAME_090       "PWM fan control"
#define PLUGIN_VALUENAME1_090 "State"
#ifdef USE_SERVO
Servo servo1;
Servo servo2;
#endif // USE_SERVO
// Make sure the initial default is a switch (value 0)
#define PLUGIN_090_TYPE_PWM_FANCONTROL           0

#define P90_Nlines 8        // The number of different lines which can be displayed
#define P90_Nchars 64
// FIXME TD-er: needed to store values for switch plugin which need extra data like PWM.
typedef uint16_t portStateExtra_t;
std::map<uint32_t, portStateExtra_t> p090_MapPortStatus_extras;


boolean Plugin_090_read_switch_state(struct EventStruct *event) {
  byte pinNumber     = CONFIG_PIN1;
  const uint32_t key = createKey(PLUGIN_ID_090, pinNumber);

  if (existPortStatus(key)) {
    return Plugin_090_read_switch_state(pinNumber, globalMapPortStatus[key].mode);
  }
  return false;
}

boolean Plugin_090_read_switch_state(byte pinNumber, byte pinMode) {
  bool canRead = false;

  switch (pinMode)
  {
    case PIN_MODE_UNDEFINED:
    case PIN_MODE_INPUT:
    case PIN_MODE_INPUT_PULLUP:
    case PIN_MODE_OUTPUT:
      canRead = true;
      break;
    case PIN_MODE_PWM:
      break;
    case PIN_MODE_SERVO:
      break;
    case PIN_MODE_OFFLINE:
      break;
    default:
      break;
  }

  if (!canRead) { return false; }

  // Do not read from the pin while mode is set to PWM or servo.
  // See https://github.com/letscontrolit/ESPEasy/issues/2117#issuecomment-443516794
  return digitalRead(pinNumber) == HIGH;
}

boolean Plugin_090(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  // static byte switchstate[TASKS_MAX];
  // static byte outputstate[TASKS_MAX];
  // static int8_t PinMonitor[GPIO_MAX];
  // static int8_t PinMonitorState[GPIO_MAX];

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number           = PLUGIN_ID_090;
      Device[deviceCount].Type               = DEVICE_TYPE_ANALOG;
      Device[deviceCount].VType              = SENSOR_TYPE_SINGLE;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = 1;
      Device[deviceCount].SendDataOption     = false;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].TimerOptional      = true;
      Device[deviceCount].GlobalSyncOption   = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_090);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_090));
      break;
    }

    case PLUGIN_GET_DEVICEGPIONAMES:
    {
      // FIXME TD-er: This plugin is handling too much.
      // - switch/dimmer input
      // - PWM output
      // - switch output (relays)
      // - servo output
      // - sending pulses
      // - playing tunes
      event->String1 = formatGpioName_bidirectional("");
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      // @giig1967g: set current task value for taking actions after changes in the task gpio
      const uint32_t key = createKey(PLUGIN_ID_090, CONFIG_PIN1);

      if (existPortStatus(key)) {
        globalMapPortStatus[key].previousTask = event->TaskIndex;
      }

   //TaskDevicePluginConfigFloat settings:
   //0: lowest_speed (percent*10)
   //1: fullspeed_Temp (temp*10)
   //2: speed_up_Temp (temp*10) 
   //3: frequecy :pwm frequcy
   //4: full_speed_pwm (100*10) 1000
      addFormNumericBox(F("最低转速(百分比)"),
                        F("p090_lowest_speed"),
                        round(PCONFIG(0)),0,100);
      addFormNumericBox(F("满转速温度℃ "),
                        F("p090_fullspeed_temp"),
                        round(PCONFIG(1)),0,100);
      addFormNumericBox(F("加速起始温度℃ "),
                        F("p090_speed_up_temp"),
                        round(PCONFIG(2)),0,100);
      addFormNumericBox(F("风扇调速频率 HZ "),
                        F("p090_frequency"),
                        round(PCONFIG(3)),1,50000);

      // TO-DO: add Extra-Long Press event
      // addFormCheckBox(F("Extra-Longpress event (20 & 21)"), F("p001_elp"), PCONFIG_LONG(1));
      // addFormNumericBox(F("Extra-Longpress min. interval (ms)"), F("p001_elpmininterval"), PCONFIG_LONG(2), 500, 2000);
        {
          String strings[P90_Nlines];
          LoadCustomTaskSettings(event->TaskIndex, strings, P90_Nlines, P90_Nchars);
          for (byte varNr = 0; varNr < 8; varNr++)
          {
            addFormTextBox(String(F("[温度传感器名称#值]")) + (varNr + 1), getPluginCustomArgName(varNr), strings[varNr], 64);
          }
        }


      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      PCONFIG(0) = getFormItemInt(F("p090_lowest_speed"));
      PCONFIG(1) = getFormItemInt(F("p090_fullspeed_temp"));
      PCONFIG(2) = getFormItemInt(F("p090_speed_up_temp"));
      PCONFIG(3) = getFormItemInt(F("p090_frequency"));
      // TO-DO: add Extra-Long Press event
      // PCONFIG_LONG(1) = isFormItemChecked(F("p001_elp"));
      // PCONFIG_LONG(2) = getFormItemInt(F("p001_elpmininterval"));

        char deviceTemplate[P90_Nlines][P90_Nchars];
        String error;
        for (byte varNr = 0; varNr < P90_Nlines; varNr++)
        {
          if (!safe_strncpy(deviceTemplate[varNr], WebServer.arg(getPluginCustomArgName(varNr)), P90_Nchars)) {
            error += getCustomTaskSettingsError(varNr);
          }
        }
        if (error.length() > 0) {
          addHtmlError(error);
        }
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
      //SaveCustomTaskSettings(event->TaskIndex, (byte*)&strncpy, 64);


      // check if a task has been edited and remove 'task' bit from the previous pin
      for (std::map<uint32_t, portStatusStruct>::iterator it = globalMapPortStatus.begin(); it != globalMapPortStatus.end(); ++it) {
        if ((it->second.previousTask == event->TaskIndex) && (getPluginFromKey(it->first) == PLUGIN_ID_090)) {
          globalMapPortStatus[it->first].previousTask = -1;
          removeTaskFromPort(it->first);
          break;
        }
      }
      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      // apply INIT only if PORT is in range. Do not start INIT if port not set in the device page.
      if ((CONFIG_PIN1 >= 0) && (CONFIG_PIN1 <= PIN_D_MAX))
      {
        portStatusStruct newStatus;
        const uint32_t   key = createKey(PLUGIN_ID_090, CONFIG_PIN1);

        // Read current status or create empty if it does not exist
        newStatus = globalMapPortStatus[key];

        // read and store current state to prevent switching at boot time
        newStatus.state                                          = Plugin_090_read_switch_state(event);
        newStatus.output                                         = newStatus.state;
        (newStatus.task < 3) ? newStatus.task++ : newStatus.task = 3; // add this GPIO/port as a task

        // setPinState(PLUGIN_ID_001, CONFIG_PIN1, PIN_MODE_INPUT, switchstate[event->TaskIndex]);
        //  if it is in the device list we assume it's an input pin
        if (Settings.TaskDevicePin1PullUp[event->TaskIndex]) {
            #if defined(ESP8266)

          if (CONFIG_PIN1 == 16) {
            pinMode(CONFIG_PIN1, INPUT_PULLDOWN_16);
          }
          else {
            pinMode(CONFIG_PIN1, INPUT_PULLUP);
          }
            #else // if defined(ESP8266)
          pinMode(CONFIG_PIN1, INPUT_PULLUP);
            #endif // if defined(ESP8266)
          newStatus.mode = PIN_MODE_INPUT_PULLUP;
        } else {
          pinMode(CONFIG_PIN1, INPUT);
          newStatus.mode = PIN_MODE_INPUT;
        }
        // if boot state must be send, inverse default state
        // this is done to force the trigger in PLUGIN_TEN_PER_SECOND

        // set initial UserVar of the switch
        if (Settings.TaskDevicePin1Inversed[event->TaskIndex]) {
          UserVar[event->BaseVarIndex] = !newStatus.state;
        } else {
          UserVar[event->BaseVarIndex] = newStatus.state;
        }

        // counters = 0
        PCONFIG(0)      = 10; // doubleclick counter
        PCONFIG(1)      = 60; // doubleclick counter
        PCONFIG(2)      = 30; // doubleclick counter
        PCONFIG(3)      = 25000; // doubleclick counter
          //PCONFIG(0) = getFormItemInt(F("p090_lowest_speed"));
          //PCONFIG(1) = getFormItemInt(F("p090_fullspeed_speed"));
          //PCONFIG(2) = getFormItemInt(F("p090_speed_up_temp"));
          //PCONFIG(3) = getFormItemInt(F("p090_frequency"));

        savePortStatus(key, newStatus);
      }
      success = true;
      break;
    }

    case PLUGIN_REQUEST:
    {
      // String device = parseString(string, 1);
      // String command = parseString(string, 2);
      // String strPar1 = parseString(string, 3);

      // returns pin value using syntax: [plugin#gpio#pinstate#xx]
      if ((string.length() >= 13) && string.substring(0, 13).equalsIgnoreCase(F("gpio,pinstate")))
      {
        int par1;

        if (validIntFromString(parseString(string, 3), par1)) {
          string = digitalRead(par1);
        }
        success = true;
      }
      break;
    }

    case PLUGIN_MONITOR:
    {
      // port monitoring, generates an event by rule command 'monitor,gpio,port#'
      const uint32_t key                   = createKey(PLUGIN_ID_090, event->Par1);
      const portStatusStruct currentStatus = globalMapPortStatus[key];

      // if (currentStatus.monitor || currentStatus.command || currentStatus.init) {
      byte state = Plugin_090_read_switch_state(event->Par1, currentStatus.mode);

      if ((currentStatus.state != state) || currentStatus.forceMonitor) {
        if (!currentStatus.task) { globalMapPortStatus[key].state = state; // do not update state if task flag=1 otherwise it will not be
                                                                           // picked up by 10xSEC function
        }

        if (currentStatus.monitor) {
          globalMapPortStatus[key].forceMonitor = 0; // reset flag
          String eventString = F("GPIO#");
          eventString += event->Par1;
          eventString += '=';
          eventString += state;
          rulesProcessing(eventString);
        }
      }

      // }

      break;
    }

    case PLUGIN_TEN_PER_SECOND:
    {
    }
    case PLUGIN_ONCE_A_SECOND:
      {
      }

    case PLUGIN_READ:
      {
      String log     = "";
        String strings[P90_Nlines];
        LoadCustomTaskSettings(event->TaskIndex, strings, P90_Nlines, P90_Nchars);
        for (byte x = 0; x < P90_Nlines; x++)
        {
          if (strings[x].length())
          {
            String stemp100 = parseTemplate(strings[x], P90_Nchars );
            //int inttemp100 = stemp100.toInt();
            //std::stoi (stemp100,nullptr,10);
              log  = F("SW   : read temp*100 ");
              log += stemp100;
          }
        }
        success = false;
        break;
      }
    case PLUGIN_WRITE:
    {
      String log     = "";
      String command = parseString(string, 1);

      // WARNING: don't read "globalMapPortStatus[key]" here, as it will create a new entry if key does not exist

      if (command == F("pwmfan")) {
        success = true;

        if ((event->Par1 >= 0) && (event->Par1 <= PIN_D_MAX))
        {
          analogWriteFreq(event->Par3);
          portStatusStruct tempStatus;

          // FIXME TD-er: PWM values cannot be stored very well in the portStatusStruct.
          const uint32_t key = createKey(PLUGIN_ID_090, event->Par1);

          // WARNING: operator [] creates an entry in the map if key does not exist
          // So the next command should be part of each command:
          tempStatus = globalMapPortStatus[key];
          portStateExtra_t psExtra = p090_MapPortStatus_extras[key];

            #if defined(ESP8266)
          pinMode(event->Par1, OUTPUT);
            #endif // if defined(ESP8266)

            #if defined(ESP8266)
          analogWrite(event->Par1, event->Par2);
            #endif // if defined(ESP8266)
            #if defined(ESP32)
          analogWriteESP32(event->Par1, event->Par2);
            #endif // if defined(ESP32)

          // setPinState(PLUGIN_ID_001, event->Par1, PIN_MODE_PWM, event->Par2);
          tempStatus.mode    = PIN_MODE_PWM;
          tempStatus.state   = event->Par2;
          tempStatus.output  = event->Par2;
          tempStatus.command = 1; // set to 1 in order to display the status in the PinStatus page

          psExtra                        = event->Par2;
          p090_MapPortStatus_extras[key] = psExtra;


          savePortStatus(key, tempStatus);
          log  = F("SW   : GPIO ");
          log += event->Par1;
          log += F(" Set FAN control PWM to ");
          log += event->Par2;

          if (event->Par3 != 0) {
            log += F(" frequency to: ");
            log += event->Par3;
          }
          addLog(LOG_LEVEL_INFO, log);
          SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, key, log, 0);

          // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_001, event->Par1, log, 0));
        }
      }
      break;
    }

    case PLUGIN_TIMER_IN:
    {

      // setPinState(PLUGIN_ID_001, event->Par1, PIN_MODE_OUTPUT, event->Par2);
      portStatusStruct tempStatus;

      // WARNING: operator [] creates an entry in the map if key does not exist
      const uint32_t key = createKey(PLUGIN_ID_090, event->Par1);
      tempStatus = globalMapPortStatus[key];

      tempStatus.state = event->Par2;
      tempStatus.mode  = PIN_MODE_OUTPUT;
      savePortStatus(key, tempStatus);
      break;
    }
  }
  return success;
}

#if defined(ESP32)
void analogWriteESP32(int pin, int value)
{
  // find existing channel if this pin has been used before
  int8_t ledChannel = -1;

  for (byte x = 0; x < 16; x++) {
    if (ledChannelPin[x] == pin) {
      ledChannel = x;
    }
  }

  if (ledChannel == -1)             // no channel set for this pin
  {
    for (byte x = 0; x < 16; x++) { // find free channel
      if (ledChannelPin[x] == -1)
      {
        int freq = 5000;
        ledChannelPin[x] = pin; // store pin nr
        ledcSetup(x, freq, 10); // setup channel
        ledcAttachPin(pin, x);  // attach to this pin
        ledChannel = x;
        break;
      }
    }
  }
  ledcWrite(ledChannel, value);
}

#endif // if defined(ESP32)

#endif // USES_P001
