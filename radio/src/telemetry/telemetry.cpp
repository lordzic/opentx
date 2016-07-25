/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

uint8_t telemetryStreaming = 0;
uint8_t telemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];   // Receive buffer. 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
uint8_t telemetryRxBufferCount = 0;

#if defined(WS_HOW_HIGH)
uint8_t wshhStreaming = 0;
#endif

uint8_t link_counter = 0;

#if defined(CPUARM)
uint8_t telemetryState = TELEMETRY_INIT;
#endif

TelemetryData telemetryData;

#if defined(CPUARM)
uint8_t telemetryProtocol = 255;
#endif

#if defined(PCBSKY9X) && defined(REVX)
uint8_t serialInversion = 0;
#endif

#if !defined(CPUARM)
uint16_t getChannelRatio(source_t channel)
{
  return (uint16_t)g_model.frsky.channels[channel].ratio << g_model.frsky.channels[channel].multiplier;
}

lcdint_t applyChannelRatio(source_t channel, lcdint_t val)
{
  return ((int32_t)val+g_model.frsky.channels[channel].offset) * getChannelRatio(channel) * 2 / 51;
}
#endif

#if defined(CPUSTM32)
#define IS_TELEMETRY_INTERNAL_MODULE (g_model.moduleData[INTERNAL_MODULE].rfProtocol != RF_PROTO_OFF)
#else
#define IS_TELEMETRY_INTERNAL_MODULE (false)
#endif
#if defined(CPUARM)
void processTelemetryData(uint8_t data)
{
#if defined(CROSSFIRE)
  if (telemetryProtocol == PROTOCOL_PULSES_CROSSFIRE) {
    processCrossfireTelemetryData(data);
    return;
  }
#endif
#if defined(MULTIMODULE)
  if (telemetryProtocol == PROTOCOL_SPEKTRUM) {
    processSpektrumTelemetryData(data);
    return;
  }
#endif
  processFrskyTelemetryData(data);
}
#endif

void telemetryWakeup()
{
#if defined(CPUARM)
  uint8_t requiredTelemetryProtocol = MODEL_TELEMETRY_PROTOCOL();
#if defined(REVX)
  uint8_t requiredSerialInversion = g_model.moduleData[EXTERNAL_MODULE].invertedSerial;
  if (telemetryProtocol != requiredTelemetryProtocol || serialInversion != requiredSerialInversion) {
    serialInversion = requiredSerialInversion;
#else
   if (telemetryProtocol != requiredTelemetryProtocol) {
#endif
    telemetryInit(requiredTelemetryProtocol);
    telemetryProtocol = requiredTelemetryProtocol;
  }
#endif

#if defined(CPUSTM32)
  uint8_t data;
  if (!telemetryFifo.isEmpty()) {
    LOG_TELEMETRY_WRITE_START();
  }
  while (telemetryFifo.pop(data)) {
    processTelemetryData(data);
    LOG_TELEMETRY_WRITE_BYTE(data);
  }
#elif defined(PCBSKY9X)
  if (telemetryProtocol == PROTOCOL_FRSKY_D_SECONDARY) {
    uint8_t data;
    while (telemetrySecondPortReceive(data)) {
      processFrskyTelemetryData(data);
    }
  }
  else {
    // Receive serial data here
    rxPdcUsart(processTelemetryData);
  }
#endif

#if !defined(CPUARM)
  if (IS_FRSKY_D_PROTOCOL()) {
    // Attempt to transmit any waiting Fr-Sky alarm set packets every 50ms (subject to packet buffer availability)
    static uint8_t frskyTxDelay = 5;
    if (frskyAlarmsSendState && (--frskyTxDelay == 0)) {
      frskyTxDelay = 5; // 50ms
#if !defined(SIMU)
      frskyDSendNextAlarm();
#endif
    }
  }
#endif

#if defined(CPUARM)
  for (int i=0; i<MAX_SENSORS; i++) {
    const TelemetrySensor & sensor = g_model.telemetrySensors[i];
    if (sensor.type == TELEM_TYPE_CALCULATED) {
      telemetryItems[i].eval(sensor);
    }
  }
#endif

#if defined(VARIO)
  if (TELEMETRY_STREAMING() && !IS_FAI_ENABLED()) {
    varioWakeup();
  }
#endif

#if defined(PCBTARANIS) && defined(REVPLUS)
  #define FRSKY_BAD_ANTENNA() (IS_VALID_XJT_VERSION() && telemetryData.swr.value > 0x33)
#else
  #define FRSKY_BAD_ANTENNA() (telemetryData.swr.value > 0x33)
#endif

#if defined(CPUARM)
  static tmr10ms_t alarmsCheckTime = 0;
  #define SCHEDULE_NEXT_ALARMS_CHECK(seconds) alarmsCheckTime = get_tmr10ms() + (100*(seconds))
  if (int32_t(get_tmr10ms() - alarmsCheckTime) > 0) {

    SCHEDULE_NEXT_ALARMS_CHECK(1/*second*/);

    uint8_t now = TelemetryItem::now();
    bool sensor_lost = false;
    for (int i=0; i<MAX_SENSORS; i++) {
      if (isTelemetryFieldAvailable(i)) {
        uint8_t lastReceived = telemetryItems[i].lastReceived;
        if (lastReceived < TELEMETRY_VALUE_TIMER_CYCLE && uint8_t(now - lastReceived) > TELEMETRY_VALUE_OLD_THRESHOLD) {
          sensor_lost = true;
          telemetryItems[i].lastReceived = TELEMETRY_VALUE_OLD;
          TelemetrySensor * sensor = & g_model.telemetrySensors[i];
          if (sensor->unit == UNIT_DATETIME) {
            telemetryItems[i].datetime.datestate = 0;
            telemetryItems[i].datetime.timestate = 0;
          }
        }
      }
    }
    if (sensor_lost && TELEMETRY_STREAMING()) {
      audioEvent(AU_SENSOR_LOST);
    }

#if defined(PCBTARANIS) || defined(PCBHORUS)
    if ((g_model.moduleData[INTERNAL_MODULE].rfProtocol != RF_PROTO_OFF || g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_XJT) && FRSKY_BAD_ANTENNA()) {
      AUDIO_SWR_RED();
      POPUP_WARNING(STR_WARNING);
      const char * w = STR_ANTENNAPROBLEM;
      SET_WARNING_INFO(w, strlen(w), 0);
      SCHEDULE_NEXT_ALARMS_CHECK(10/*seconds*/);
    }
#endif

    if (TELEMETRY_STREAMING()) {
      if (getRssiAlarmValue(1) && TELEMETRY_RSSI() < getRssiAlarmValue(1)) {
        AUDIO_RSSI_RED();
        SCHEDULE_NEXT_ALARMS_CHECK(10/*seconds*/);
      }
      else if (getRssiAlarmValue(0) && TELEMETRY_RSSI() < getRssiAlarmValue(0)) {
        AUDIO_RSSI_ORANGE();
        SCHEDULE_NEXT_ALARMS_CHECK(10/*seconds*/);
      }
    }
  }

  if (TELEMETRY_STREAMING()) {
    if (telemetryState == TELEMETRY_KO) {
      AUDIO_TELEMETRY_BACK();
    }
    telemetryState = TELEMETRY_OK;
  }
  else if (telemetryState == TELEMETRY_OK) {
    telemetryState = TELEMETRY_KO;
    AUDIO_TELEMETRY_LOST();
  }
#endif
}

void telemetryInterrupt10ms()
{
#if defined(FRSKY_HUB) && !defined(CPUARM)
  uint16_t voltage = 0; /* unit: 1/10 volts */
  for (uint8_t i=0; i<telemetryData.hub.cellsCount; i++)
    voltage += telemetryData.hub.cellVolts[i];
  voltage /= (10 / TELEMETRY_CELL_VOLTAGE_MUTLIPLIER);
  telemetryData.hub.cellsSum = voltage;
  if (telemetryData.hub.cellsSum < telemetryData.hub.minCells) {
    telemetryData.hub.minCells = telemetryData.hub.cellsSum;
  }
#endif

  if (TELEMETRY_STREAMING()) {
    if (!TELEMETRY_OPENXSENSOR()) {
#if defined(CPUARM)
      for (int i=0; i<MAX_SENSORS; i++) {
        const TelemetrySensor & sensor = g_model.telemetrySensors[i];
        if (sensor.type == TELEM_TYPE_CALCULATED) {
          telemetryItems[i].per10ms(sensor);
        }
      }
#else
      // power calculation
      uint8_t channel = g_model.frsky.voltsSource;
      if (channel <= FRSKY_VOLTS_SOURCE_A2) {
        voltage = applyChannelRatio(channel, telemetryData.analog[channel].value) / 10;
      }

#if defined(FRSKY_HUB)
      else if (channel == FRSKY_VOLTS_SOURCE_FAS) {
        voltage = telemetryData.hub.vfas;
      }
#endif

#if defined(FRSKY_HUB)
      uint16_t current = telemetryData.hub.current; /* unit: 1/10 amps */
#else
      uint16_t current = 0;
#endif

      channel = g_model.frsky.currentSource - FRSKY_CURRENT_SOURCE_A1;
      if (channel < MAX_FRSKY_A_CHANNELS) {
        current = applyChannelRatio(channel, telemetryData.analog[channel].value) / 10;
      }

      telemetryData.hub.power = ((current>>1) * (voltage>>1)) / 25;

      telemetryData.hub.currentPrescale += current;
      if (telemetryData.hub.currentPrescale >= 3600) {
        telemetryData.hub.currentConsumption += 1;
        telemetryData.hub.currentPrescale -= 3600;
      }
#endif
    }

#if !defined(CPUARM)
    if (telemetryData.hub.power > telemetryData.hub.maxPower) {
      telemetryData.hub.maxPower = telemetryData.hub.power;
    }
#endif
  }

#if defined(WS_HOW_HIGH)
  if (wshhStreaming > 0) {
    wshhStreaming--;
  }
#endif

  if (telemetryStreaming > 0) {
    telemetryStreaming--;
  }
  else {
#if !defined(SIMU)
#if defined(CPUARM)
    telemetryData.rssi.reset();
#else
    telemetryData.rssi[0].set(0);
    telemetryData.rssi[1].set(0);
#endif
#endif
  }
}

void telemetryReset()
{
  memclear(&telemetryData, sizeof(telemetryData));

#if defined(CPUARM)
  for (int index=0; index<MAX_SENSORS; index++) {
    telemetryItems[index].clear();
  }
#endif

  telemetryStreaming = 0; // reset counter only if valid frsky packets are being detected
  link_counter = 0;

#if defined(CPUARM)
  telemetryState = TELEMETRY_INIT;
#endif

#if defined(FRSKY_HUB) && !defined(CPUARM)
  telemetryData.hub.gpsLatitude_bp = 2;
  telemetryData.hub.gpsLongitude_bp = 2;
  telemetryData.hub.gpsFix = -1;
#endif

#if defined(SIMU)

#if defined(CPUARM)
  telemetryData.swr.value = 30;
  telemetryData.rssi.value = 75;
#else
  telemetryData.rssi[0].value = 75;
  telemetryData.rssi[1].value = 75;
  telemetryData.analog[TELEM_ANA_A1].set(120, UNIT_VOLTS);
  telemetryData.analog[TELEM_ANA_A2].set(240, UNIT_VOLTS);
#endif

#if !defined(CPUARM)
  telemetryData.hub.fuelLevel = 75;
  telemetryData.hub.rpm = 12000;
  telemetryData.hub.vfas = 100;
  telemetryData.hub.minVfas = 90;

#if defined(GPS)
  telemetryData.hub.gpsFix = 1;
  telemetryData.hub.gpsLatitude_bp = 4401;
  telemetryData.hub.gpsLatitude_ap = 7710;
  telemetryData.hub.gpsLongitude_bp = 1006;
  telemetryData.hub.gpsLongitude_ap = 8872;
  telemetryData.hub.gpsSpeed_bp = 200;  //in knots
  telemetryData.hub.gpsSpeed_ap = 0;
  getGpsPilotPosition();

  telemetryData.hub.gpsLatitude_bp = 4401;
  telemetryData.hub.gpsLatitude_ap = 7455;
  telemetryData.hub.gpsLongitude_bp = 1006;
  telemetryData.hub.gpsLongitude_ap = 9533;
  getGpsDistance();
#endif

  telemetryData.hub.airSpeed = 1000; // 185.1 km/h

  telemetryData.hub.cellsCount = 6;
  telemetryData.hub.cellVolts[0] = 410/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.cellVolts[1] = 420/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.cellVolts[2] = 430/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.cellVolts[3] = 440/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.cellVolts[4] = 450/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.cellVolts[5] = 460/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.minCellVolts = 250/TELEMETRY_CELL_VOLTAGE_MUTLIPLIER;
  telemetryData.hub.minCell = 300;    //unit 10mV
  telemetryData.hub.minCells = 220;  //unit 100mV
  //telemetryData.hub.cellsSum = 261;    //calculated from cellVolts[]

  telemetryData.hub.gpsAltitude_bp = 50;
  telemetryData.hub.baroAltitude_bp = 50;
  telemetryData.hub.minAltitude = 10;
  telemetryData.hub.maxAltitude = 500;

  telemetryData.hub.accelY = 100;
  telemetryData.hub.temperature1 = -30;
  telemetryData.hub.maxTemperature1 = 100;

  telemetryData.hub.current = 55;
  telemetryData.hub.maxCurrent = 65;
#endif
#endif

/*Add some default sensor values to the simulator*/
#if defined(CPUARM) && defined(SIMU)
  for (int i=0; i<MAX_SENSORS; i++) {
    const TelemetrySensor & sensor = g_model.telemetrySensors[i];
    switch (sensor.id)
    {
      case RSSI_ID:
        setTelemetryValue(TELEM_PROTO_FRSKY_SPORT, RSSI_ID, 0, sensor.instance , 75, UNIT_RAW, 0);
        break;
      case ADC1_ID:
        setTelemetryValue(TELEM_PROTO_FRSKY_SPORT, ADC1_ID, 0, sensor.instance, 100, UNIT_RAW, 0);
        break;
      case ADC2_ID:
        setTelemetryValue(TELEM_PROTO_FRSKY_SPORT, ADC2_ID, 0, sensor.instance, 245, UNIT_RAW, 0);
        break;
      case SWR_ID:
        setTelemetryValue(TELEM_PROTO_FRSKY_SPORT, SWR_ID, 0, sensor.instance, 30, UNIT_RAW, 0);
        break;
      case BATT_ID:
        setTelemetryValue(TELEM_PROTO_FRSKY_SPORT, BATT_ID, 0, sensor.instance, 100, UNIT_RAW, 0);
        break;
    }
  }
#endif
}

#if defined(CPUARM)
// we don't reset the telemetry here as we would also reset the consumption after model load
void telemetryInit(uint8_t protocol) {
#if defined(MULTIMODULE)
  // TODO: Is there a better way to communicate this to this function?
  if (!IS_TELEMETRY_INTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_MULTIMODULE) {
    // The DIY Multi module always speaks 100000 baud regardless of the telemetry protocol in use
    telemetryPortInit(MULTIMODULE_BAUDRATE, TELEMETRY_SERIAL_8E2);
  } else
#endif
  if (protocol == PROTOCOL_FRSKY_D) {
    telemetryPortInit(FRSKY_D_BAUDRATE, TELEMETRY_SERIAL_8N1);
  }
#if defined(CROSSFIRE)
  else if (protocol == PROTOCOL_PULSES_CROSSFIRE) {
    telemetryPortInit(CROSSFIRE_BAUDRATE, TELEMETRY_SERIAL_8N1);
#if defined(LUA)
    outputTelemetryBufferSize = 0;
    outputTelemetryBufferTrigger = 0;
#endif
    telemetryPortSetDirectionOutput();
  }
#endif
  else if (protocol == PROTOCOL_FRSKY_D_SECONDARY) {
    telemetryPortInit(0, TELEMETRY_SERIAL_8N1);
    serial2TelemetryInit(PROTOCOL_FRSKY_D_SECONDARY);
  }
  else {
    telemetryPortInit(FRSKY_SPORT_BAUDRATE, TELEMETRY_SERIAL_8N1);
#if defined(LUA)
    outputTelemetryBufferSize = 0;
    outputTelemetryBufferTrigger = 0x7E;
#endif
  }

#if defined(REVX) && !defined(SIMU)
  if (serialInversion) {
    setMFP();
  }
  else {
    clearMFP();
  }
#endif
}
#else
void telemetryInit()
{
  telemetryPortInit();
}
#endif

NOINLINE uint8_t getRssiAlarmValue(uint8_t alarm)
{
  return (45 - 3*alarm + g_model.frsky.rssiAlarms[alarm].value);
}

#if defined(LOG_TELEMETRY) && !defined(SIMU)
extern FIL g_telemetryFile;
void logTelemetryWriteStart()
{
  static tmr10ms_t lastTime = 0;
  tmr10ms_t newTime = get_tmr10ms();
  if (lastTime != newTime) {
    struct gtm utm;
    gettime(&utm);
    f_printf(&g_telemetryFile, "\r\n%4d-%02d-%02d,%02d:%02d:%02d.%02d0:", utm.tm_year+1900, utm.tm_mon+1, utm.tm_mday, utm.tm_hour, utm.tm_min, utm.tm_sec, g_ms100);
    lastTime = newTime;
  }
}

void logTelemetryWriteByte(uint8_t data)
{
  f_printf(&g_telemetryFile, " %02X", data);
}
#endif

uint8_t outputTelemetryBuffer[TELEMETRY_OUTPUT_FIFO_SIZE] __DMA;
uint8_t outputTelemetryBufferSize = 0;
uint8_t outputTelemetryBufferTrigger = 0;

#if defined(LUA)
Fifo<uint8_t, LUA_TELEMETRY_INPUT_FIFO_SIZE> * luaInputTelemetryFifo = NULL;
#endif
