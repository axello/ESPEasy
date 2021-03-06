//#######################################################################################################
//############################# Plugin 052: SenseAir CO2 Sensors ########################################
//#######################################################################################################
/*
  Plugin originally written by: Daniel Tedenljung info__AT__tedenljungconsulting.com
  Rewritten by: Mikael Trieb mikael__AT__triebconsulting.se

  This plugin reads availble values of SenseAir Co2 Sensors.
  Datasheet can be found here:
  S8: http://www.senseair.com/products/oem-modules/senseair-s8/
  K30: http://www.senseair.com/products/oem-modules/k30/
  K70/tSENSE: http://www.senseair.com/products/wall-mount/tsense/

  Circuit wiring
    GPIO Setting 1 -> RX
    GPIO Setting 2 -> TX
    Use 1kOhm in serie on datapins!
*/
#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_052
#define PLUGIN_ID_052         52
#define PLUGIN_NAME_052       "SenseAir"
#define PLUGIN_VALUENAME1_052 ""

boolean Plugin_052_init = false;

#include <SoftwareSerial.h>
SoftwareSerial *Plugin_052_SoftSerial;

boolean Plugin_052(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_052;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_052);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_052));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
          byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
          String options[4];
          options[0] = F("Status");
          options[1] = F("Carbon Dioxide");
          options[2] = F("Temperature");
          options[3] = F("Humidity");
          int optionValues[4];
          optionValues[0] = 0;
          optionValues[1] = 1;
          optionValues[2] = 2;
          optionValues[3] = 3;
          string += F("<TR><TD>Sensor:<TD><select name='plugin_052'>");
          for (byte x = 0; x < 4; x++)
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
          String plugin1 = WebServer.arg(F("plugin_052"));
          Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
          success = true;
          break;
      }

    case PLUGIN_INIT:
      {
        Plugin_052_init = true;
        Plugin_052_SoftSerial = new SoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex],
                                                   Settings.TaskDevicePin2[event->TaskIndex]);
        success = true;
        break;
      }

    case PLUGIN_READ:
      {

        if (Plugin_052_init)
        {

          String log = F("SenseAir: ");
          switch(Settings.TaskDevicePluginConfig[event->TaskIndex][0])
          {
              case 0:
              {
                  int sensor_status = Plugin_052_readStatus();
                  UserVar[event->BaseVarIndex] = sensor_status;
                  log += sensor_status;
                  break;
              }
              case 1:
              {
                  int co2 = Plugin_052_readCo2();
                  UserVar[event->BaseVarIndex] = co2;
                  log += co2;
                  break;
              }
              case 2:
              {
                  float temperature = Plugin_052_readTemperature();
                  UserVar[event->BaseVarIndex] = (float)temperature;
                  log += (float)temperature;
                  break;
              }
              case 3:
              {
                  float relativeHumidity = Plugin_052_readRelativeHumidity();
                  UserVar[event->BaseVarIndex] = (float)relativeHumidity;
                  log += (float)relativeHumidity;
                  break;
              }
          }
          addLog(LOG_LEVEL_INFO, log);

          success = true;
          break;
        }
        break;
      }
  }
  return success;
}

void Plugin_052_buildFrame(byte slaveAddress,
              byte  functionCode,
              short startAddress,
              short numberOfRegisters,
              byte frame[8])
{
  frame[0] = slaveAddress;
  frame[1] = functionCode;
  frame[2] = (byte)(startAddress >> 8);
  frame[3] = (byte)(startAddress);
  frame[4] = (byte)(numberOfRegisters >> 8);
  frame[5] = (byte)(numberOfRegisters);
  // CRC-calculation
  byte checkSum[2] = {0};
  unsigned int crc = Plugin_052_ModRTU_CRC(frame, 6, checkSum);
  frame[6] = checkSum[0];
  frame[7] = checkSum[1];
}

int Plugin_052_sendCommand(byte command[])
{
  byte recv_buf[7] = {0xff};
  byte data_buf[2] = {0xff};
  long value       = -1;

  Plugin_052_SoftSerial->write(command, 8); //Send the byte array
  delay(50);

  // Read answer from sensor
  int ByteCounter = 0;
  while(Plugin_052_SoftSerial->available()) {
    recv_buf[ByteCounter] = Plugin_052_SoftSerial->read();
    ByteCounter++;
  }

  data_buf[0] = recv_buf[3];
  data_buf[1] = recv_buf[4];
  value = (data_buf[0] << 8) | (data_buf[1]);

  return value;
}

int Plugin_052_readStatus(void)
{
  int sensor_status = -1;
  byte frame[8] = {0};
  Plugin_052_buildFrame(0xFE, 0x04, 0x00, 1, frame);
  sensor_status = Plugin_052_sendCommand(frame);
  return sensor_status;
}

int Plugin_052_readCo2(void)
{
  int co2 = 0;
  byte frame[8] = {0};
  Plugin_052_buildFrame(0xFE, 0x04, 0x03, 1, frame);
  co2 = Plugin_052_sendCommand(frame);
  return co2;
}

float Plugin_052_readTemperature(void)
{
  int temperatureX100 = 0;
  float temperature = 0.0;
  byte frame[8] = {0};
  Plugin_052_buildFrame(0xFE, 0x04, 0x04, 1, frame);
  temperatureX100 = Plugin_052_sendCommand(frame);
  temperature = temperatureX100/100;
  return temperature;
}

float Plugin_052_readRelativeHumidity(void)
{
  int rhX100 = 0;
  float rh = 0.0;
  byte frame[8] = {0};
  Plugin_052_buildFrame(0xFE, 0x04, 0x05, 1, frame);
  rhX100 = Plugin_052_sendCommand(frame);
  rh = rhX100/100;
  return rh;
}

// Compute the MODBUS RTU CRC
unsigned int Plugin_052_ModRTU_CRC(byte buf[], int len, byte checkSum[2])
{
  unsigned int crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];          // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  checkSum[1] = (byte)((crc >> 8) & 0xFF);
  checkSum[0] = (byte)(crc & 0xFF);
  return crc;
}
#endif
