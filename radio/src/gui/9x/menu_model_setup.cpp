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

#if defined(CPUARM)
uint8_t g_moduleIdx;
void menuModelFailsafe(uint8_t event);
#endif

enum menuModelSetupItems {
  ITEM_MODEL_NAME,
  ITEM_MODEL_TIMER1,
  CASE_CPUARM(ITEM_MODEL_TIMER1_NAME)
  CASE_PERSISTENT_TIMERS(ITEM_MODEL_TIMER1_PERSISTENT)
  ITEM_MODEL_TIMER1_MINUTE_BEEP,
  ITEM_MODEL_TIMER1_COUNTDOWN_BEEP,
  ITEM_MODEL_TIMER2,
  CASE_CPUARM(ITEM_MODEL_TIMER2_NAME)
  CASE_PERSISTENT_TIMERS(ITEM_MODEL_TIMER2_PERSISTENT)
  ITEM_MODEL_TIMER2_MINUTE_BEEP,
  ITEM_MODEL_TIMER2_COUNTDOWN_BEEP,
  CASE_CPUARM(ITEM_MODEL_TIMER3)
  CASE_CPUARM(ITEM_MODEL_TIMER3_NAME)
  CASE_CPUARM(ITEM_MODEL_TIMER3_PERSISTENT)
  CASE_CPUARM(ITEM_MODEL_TIMER3_MINUTE_BEEP)
  CASE_CPUARM(ITEM_MODEL_TIMER3_COUNTDOWN_BEEP)
  ITEM_MODEL_EXTENDED_LIMITS,
  ITEM_MODEL_EXTENDED_TRIMS,
  CASE_CPUARM(ITEM_MODEL_DISPLAY_TRIMS)
  ITEM_MODEL_TRIM_INC,
  ITEM_MODEL_THROTTLE_REVERSED,
  ITEM_MODEL_THROTTLE_TRACE,
  ITEM_MODEL_THROTTLE_TRIM,
  CASE_CPUARM(ITEM_MODEL_PREFLIGHT_LABEL)
  CASE_CPUARM(ITEM_MODEL_CHECKLIST_DISPLAY)
  ITEM_MODEL_THROTTLE_WARNING,
  ITEM_MODEL_SWITCHES_WARNING,
  ITEM_MODEL_BEEP_CENTER,
  CASE_CPUARM(ITEM_MODEL_USE_GLOBAL_FUNCTIONS)
#if defined(CPUARM)
  ITEM_MODEL_EXTERNAL_MODULE_LABEL,
  ITEM_MODEL_EXTERNAL_MODULE_MODE,
#ifdef MULTIMODULE
  ITEM_MODEL_EXTERNAL_MODULE_SUBTYPE,
#endif
  ITEM_MODEL_EXTERNAL_MODULE_CHANNELS,
  ITEM_MODEL_EXTERNAL_MODULE_BIND,
#if defined(PCBSKY9X) && defined(REVX)
  ITEM_MODEL_EXTERNAL_MODULE_OUTPUT_TYPE,
#endif
  ITEM_MODEL_EXTERNAL_MODULE_FAILSAFE,
#ifdef MULTIMODULE
  ITEM_MODEL_EXTERNAL_MODULE_AUTOBIND,
  ITEM_MODEL_EXTERNAL_MODULE_LOWPOWER,
#endif
#if defined(PCBSKY9X) && !defined(REVA)
  ITEM_MODEL_EXTRA_MODULE_LABEL,
  ITEM_MODEL_EXTRA_MODULE_CHANNELS,
  ITEM_MODEL_EXTRA_MODULE_BIND,
#endif
#else
  ITEM_MODEL_PPM1_PROTOCOL,
  ITEM_MODEL_PPM1_PARAMS,
#endif
  ITEM_MODEL_SETUP_MAX
};

#if defined(PCBSKY9X)
  #define FIELD_PROTOCOL_MAX 2
#else
  #define FIELD_PROTOCOL_MAX 1
#endif

#define MODEL_SETUP_2ND_COLUMN        (LCD_W-11*FW-MENUS_SCROLLBAR_WIDTH)
#define MODEL_SETUP_BIND_OFS          2*FW+1
#define MODEL_SETUP_RANGE_OFS         4*FW+3
#define MODEL_SETUP_SET_FAILSAFE_OFS  7*FW-2

#if defined(PCBSKY9X) && !defined(REVA)
  #define CURRENT_MODULE_EDITED(k)         (k>=ITEM_MODEL_EXTRA_MODULE_LABEL ? EXTRA_MODULE : EXTERNAL_MODULE)
#else
  #define CURRENT_MODULE_EDITED(k)         (EXTERNAL_MODULE)
#endif

void menuModelSetup(uint8_t event)
{
#if defined(CPUARM)
  #define IF_EXTERNAL_MODULE_XJT(x)         (IS_MODULE_XJT(EXTERNAL_MODULE) ? (uint8_t)(x) : HIDDEN_ROW)
  #define IF_EXTERNAL_MODULE_ON(x)          (g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_NONE ? HIDDEN_ROW : (uint8_t)(x))
  #define IS_D8_RX(x)                       (g_model.moduleData[x].rfProtocol == RF_PROTO_D8)
  #define EXTERNAL_MODULE_CHANNELS_ROWS     IF_EXTERNAL_MODULE_ON((IS_MODULE_DSM2(EXTERNAL_MODULE) || IS_MODULE_MULTIMODULE(EXTERNAL_MODULE)) ? (uint8_t)0 : (uint8_t)1)
  #define EXTERNAL_MODULE_BIND_ROWS()   (IS_MODULE_XJT(EXTERNAL_MODULE) && IS_D8_RX(EXTERNAL_MODULE)) ? (uint8_t)1 : (IS_MODULE_PPM(EXTERNAL_MODULE) || IS_MODULE_XJT(EXTERNAL_MODULE) || IS_MODULE_DSM2(EXTERNAL_MODULE) || IS_MODULE_MULTIMODULE(EXTERNAL_MODULE)) ? (uint8_t)2 : HIDDEN_ROW

#if defined(PCBSKY9X) && defined(REVX)
  #define OUTPUT_TYPE_ROWS()                (IS_MODULE_PPM(EXTERNAL_MODULE) ? (uint8_t)0 : HIDDEN_ROW) ,
#else
  #define OUTPUT_TYPE_ROWS() 
#endif
  #define TRAINER_CHANNELS_ROWS()           (HIDDEN_ROW)
  #define PORT_CHANNELS_ROWS(x)             (x==EXTERNAL_MODULE ? EXTERNAL_MODULE_CHANNELS_ROWS : 0)

#if defined MULTIMODULE
  #define MULTIMODULE_HASOPTIONS(x)       (x == MM_RF_PROTO_HUBSAN || x == MM_RF_PROTO_FRSKY || x == MM_RF_PROTO_DSM2 || x == MM_RF_PROTO_SFHSS || x >= MM_RF_PROTO_CUSTOM)
  #define MULTIMODULE_FAILSAFEROWS(x)     (IS_MODULE_MULTIMODULE(x) && (MULTIMODULE_HASOPTIONS(g_model.moduleData[x].multi.rfProtocol))) ? (uint8_t) 0: HIDDEN_ROW
#else
  #define MULTIMODULE_FAILSAFEROWS(x)     HIDDEN_ROW
#endif
  #define FAILSAFE_ROWS(x)                  (IS_MODULE_XJT(x) && HAS_RF_PROTOCOL_FAILSAFE(g_model.moduleData[x].rfProtocol) ? (g_model.moduleData[x].failsafeMode==FAILSAFE_CUSTOM ? (uint8_t)1 : (uint8_t)0) : MULTIMODULE_FAILSAFEROWS(x))

#if defined(MULTIMODULE)
  #define MULTIMODULE_HAS_SUBTYPE(x)      (x == MM_RF_PROTO_FLYSKY || x == MM_RF_PROTO_FRSKY || x == MM_RF_PROTO_HISKY || x == MM_RF_PROTO_DSM2 || x == MM_RF_PROTO_YD717 || x == MM_RF_PROTO_KN || x == MM_RF_PROTO_SYMAX || x == MM_RF_PROTO_CX10 || x == MM_RF_PROTO_CG023 || x == MM_RF_PROTO_MT99XX || x == MM_RF_PROTO_MJXQ)
  #define MULTIMODULE_MODE_ROWS(x)        (g_model.moduleData[x].multi.rfProtocol >= MM_RF_PROTO_CUSTOM ) ? (uint8_t) 3 :MULTIMODULE_HAS_SUBTYPE(g_model.moduleData[x].multi.rfProtocol) ? (uint8_t)2 : (uint8_t)1
  #define MULTIMODULE_MODULE_ROWS         IS_MODULE_MULTIMODULE(EXTERNAL_MODULE) ? (uint8_t) 0 : HIDDEN_ROW, IS_MODULE_MULTIMODULE(EXTERNAL_MODULE) ? (uint8_t) 0 : HIDDEN_ROW,
  #define MULTIMODULE_RFPROTO_ROWS(x)     (g_model.moduleData[x].multi.rfProtocol >= MM_RF_PROTO_CUSTOM) ? (uint8_t) 1 :MULTIMODULE_HAS_SUBTYPE(g_model.moduleData[x].multi.rfProtocol) ? (uint8_t) 0 : HIDDEN_ROW
  #define MULTIMODULE_SUBTYPE_ROWS(x)     IS_MODULE_MULTIMODULE(x) ? MULTIMODULE_RFPROTO_ROWS(x) : HIDDEN_ROW,

#else
  #define MULTIMODULE_MODE_ROWS(x)        (uint8_t)0
  #define MULTIMODULE_MODULE_ROWS
  #define MULTIMODULE_SUBTYPE_ROWS(x)
#endif

  #define EXTERNAL_MODULE_MODE_ROWS         (IS_MODULE_XJT(EXTERNAL_MODULE) || IS_MODULE_DSM2(EXTERNAL_MODULE) || IS_MODULE_MULTIMODULE(EXTERNAL_MODULE)) ? (uint8_t)1 : (uint8_t)0

#define CURSOR_ON_CELL                    (true)
  #define MODEL_SETUP_MAX_LINES             (1+ITEM_MODEL_SETUP_MAX)
  #define POT_WARN_ITEMS()                  ((g_model.nPotsToWarn >> 6) ? (uint8_t)NUM_POTS : (uint8_t)0)
  #define TIMER_ROWS                        2, 0, CASE_PERSISTENT_TIMERS(0) 0, 0
#if defined(PCBSKY9X) && !defined(REVA)
  #define EXTRA_MODULE_ROWS                 LABEL(ExtraModule), 1, 2,
#else
  #define EXTRA_MODULE_ROWS
#endif
  #define TRAINER_MODULE_ROWS
  MENU_TAB({ 0, 0, TIMER_ROWS, TIMER_ROWS, TIMER_ROWS, 0, 1, 0, 0, 0, 0, 0, CASE_CPUARM(LABEL(PreflightCheck)) CASE_CPUARM(0) 0, 6, NUM_STICKS+NUM_POTS+NUM_ROTARY_ENCODERS-1, 0,
  LABEL(ExternalModule),
  EXTERNAL_MODULE_MODE_ROWS,
  MULTIMODULE_SUBTYPE_ROWS(EXTERNAL_MODULE)
  EXTERNAL_MODULE_CHANNELS_ROWS,
  EXTERNAL_MODULE_BIND_ROWS(),
  OUTPUT_TYPE_ROWS()
  FAILSAFE_ROWS(EXTERNAL_MODULE),
  MULTIMODULE_MODULE_ROWS
  EXTRA_MODULE_ROWS
  TRAINER_MODULE_ROWS });
#elif defined(CPUM64)
  #define CURSOR_ON_CELL                    (true)
  #define MODEL_SETUP_MAX_LINES             ((IS_PPM_PROTOCOL(protocol)||IS_DSM2_PROTOCOL(protocol)||IS_PXX_PROTOCOL(protocol)) ? 1+ITEM_MODEL_SETUP_MAX : ITEM_MODEL_SETUP_MAX)
  uint8_t protocol = g_model.protocol;
  MENU_TAB({ 0, 0, 2, CASE_PERSISTENT_TIMERS(0) 0, 0, 2, CASE_PERSISTENT_TIMERS(0) 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, NUM_STICKS+NUM_POTS+NUM_ROTARY_ENCODERS-1, FIELD_PROTOCOL_MAX, 2 });
#else
  #define CURSOR_ON_CELL                    (true)
  #define MODEL_SETUP_MAX_LINES             ((IS_PPM_PROTOCOL(protocol)||IS_DSM2_PROTOCOL(protocol)||IS_PXX_PROTOCOL(protocol)) ? 1+ITEM_MODEL_SETUP_MAX : ITEM_MODEL_SETUP_MAX)
  uint8_t protocol = g_model.protocol;
  MENU_TAB({ 0, 0, 2, CASE_PERSISTENT_TIMERS(0) 0, 0, 2, CASE_PERSISTENT_TIMERS(0) 0, 0, 0, 1, 0, 0, 0, 0, 0, NUM_SWITCHES, NUM_STICKS+NUM_POTS+NUM_ROTARY_ENCODERS-1, FIELD_PROTOCOL_MAX, 2, CASE_PCBSKY9X(1) CASE_PCBSKY9X(2) });
#endif

  MENU_CHECK(menuTabModel, e_ModelSetup, MODEL_SETUP_MAX_LINES);

#if defined(CPUARM) && (defined(DSM2) || defined(PXX))
  if (menuEvent) {
    moduleFlag[0] = 0;
#if NUM_MODULES > 1
    moduleFlag[1] = 0;
#endif
  }
#endif

  TITLE(STR_MENUSETUP);

  uint8_t sub = menuVerticalPosition - 1;
  int8_t editMode = s_editMode;

  for (uint8_t i=0; i<NUM_BODY_LINES; ++i) {
    coord_t y = MENU_HEADER_HEIGHT + 1 + i*FH;
    uint8_t k = i+menuVerticalOffset;
#if defined(CPUARM)
    for (int j=0; j<=k; j++) {
      if (mstate_tab[j+1] == HIDDEN_ROW) {
        if (++k >= (int)DIM(mstate_tab)) {
    	  return;
    	}
      }
    }
#endif

    LcdFlags blink = ((editMode>0) ? BLINK|INVERS : INVERS);
    LcdFlags attr = (sub == k ? blink : 0);

    switch(k) {
      case ITEM_MODEL_NAME:
        editSingleName(MODEL_SETUP_2ND_COLUMN, y, STR_MODELNAME, g_model.header.name, sizeof(g_model.header.name), event, attr);
#if defined(CPUARM)
        memcpy(modelHeaders[g_eeGeneral.currModel].name, g_model.header.name, sizeof(g_model.header.name));
#endif
        break;

#if defined(CPUARM)
      case ITEM_MODEL_TIMER1:
      case ITEM_MODEL_TIMER2:
      case ITEM_MODEL_TIMER3:
      {
        unsigned int timerIdx = (k>=ITEM_MODEL_TIMER3 ? 2 : (k>=ITEM_MODEL_TIMER2 ? 1 : 0));
        TimerData * timer = &g_model.timers[timerIdx];
        drawStringWithIndex(0*FW, y, STR_TIMER, timerIdx+1);
        putsTimerMode(MODEL_SETUP_2ND_COLUMN, y, timer->mode, menuHorizontalPosition==0 ? attr : 0);
        putsTimer(MODEL_SETUP_2ND_COLUMN+5*FW-2+5*FWNUM+1, y, timer->start, menuHorizontalPosition==1 ? attr : 0, menuHorizontalPosition==2 ? attr : 0);
        if (attr && (editMode>0 || p1valdiff)) {
          div_t qr = div(timer->start, 60);
          switch (menuHorizontalPosition) {
            case 0:
            {
              int8_t timerMode = timer->mode;
              if (timerMode < 0) timerMode -= TMRMODE_COUNT-1;
              CHECK_INCDEC_MODELVAR_CHECK(event, timerMode, -TMRMODE_COUNT-SWSRC_LAST+1, TMRMODE_COUNT+SWSRC_LAST-1, isSwitchAvailableInTimers);
              if (timerMode < 0) timerMode += TMRMODE_COUNT-1;
              timer->mode = timerMode;
#if defined(AUTOSWITCH)
              if (s_editMode>0) {
                int8_t val = timer->mode - (TMRMODE_COUNT-1);
                int8_t switchVal = checkIncDecMovedSwitch(val);
                if (val != switchVal) {
                  timer->mode = switchVal + (TMRMODE_COUNT-1);
                  storageDirty(EE_MODEL);
                }
              }
#endif
              break;
            }
            case 1:
              CHECK_INCDEC_MODELVAR_ZERO(event, qr.quot, 539); // 8:59
              timer->start = qr.rem + qr.quot*60;
              break;
            case 2:
              qr.rem -= checkIncDecModel(event, qr.rem+2, 1, 62)-2;
              timer->start -= qr.rem ;
              if ((int16_t)timer->start < 0) timer->start=0;
              if ((int16_t)timer->start > 5999) timer->start=32399; // 8:59:59
              break;
          }
        }
        break;
      }

      case ITEM_MODEL_TIMER1_NAME:
      case ITEM_MODEL_TIMER2_NAME:
      case ITEM_MODEL_TIMER3_NAME:
      {
        TimerData * timer = &g_model.timers[k>=ITEM_MODEL_TIMER3 ? 2 : (k>=ITEM_MODEL_TIMER2 ? 1 : 0)];
        editSingleName(MODEL_SETUP_2ND_COLUMN, y, STR_TIMER_NAME, timer->name, sizeof(timer->name), event, attr);
        break;
      }

      case ITEM_MODEL_TIMER1_MINUTE_BEEP:
      case ITEM_MODEL_TIMER2_MINUTE_BEEP:
      case ITEM_MODEL_TIMER3_MINUTE_BEEP:
      {
        TimerData * timer = &g_model.timers[k>=ITEM_MODEL_TIMER3 ? 2 : (k>=ITEM_MODEL_TIMER2 ? 1 : 0)];
        timer->minuteBeep = editCheckBox(timer->minuteBeep, MODEL_SETUP_2ND_COLUMN, y, STR_MINUTEBEEP, attr, event);
        break;
      }

      case ITEM_MODEL_TIMER1_COUNTDOWN_BEEP:
      case ITEM_MODEL_TIMER2_COUNTDOWN_BEEP:
      case ITEM_MODEL_TIMER3_COUNTDOWN_BEEP:
      {
        TimerData * timer = &g_model.timers[k>=ITEM_MODEL_TIMER3 ? 2 : (k>=ITEM_MODEL_TIMER2 ? 1 : 0)];
        timer->countdownBeep = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_BEEPCOUNTDOWN, STR_VBEEPCOUNTDOWN, timer->countdownBeep, COUNTDOWN_SILENT, COUNTDOWN_COUNT-1, attr, event);
        break;
      }

      case ITEM_MODEL_TIMER1_PERSISTENT:
      case ITEM_MODEL_TIMER2_PERSISTENT:
      case ITEM_MODEL_TIMER3_PERSISTENT:
      {
        TimerData * timer = &g_model.timers[k>=ITEM_MODEL_TIMER3 ? 2 : (k>=ITEM_MODEL_TIMER2 ? 1 : 0)];
        timer->persistent = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_PERSISTENT, STR_VPERSISTENT, timer->persistent, 0, 2, attr, event);
        break;
      }
#else
      case ITEM_MODEL_TIMER1:
      case ITEM_MODEL_TIMER2:
      case ITEM_MODEL_TIMER1_MINUTE_BEEP:
      case ITEM_MODEL_TIMER2_MINUTE_BEEP:
      case ITEM_MODEL_TIMER1_COUNTDOWN_BEEP:
      case ITEM_MODEL_TIMER2_COUNTDOWN_BEEP:
      {
        TimerData *timer = &g_model.timers[k>=ITEM_MODEL_TIMER2 ? 1 : 0];
        if (k==ITEM_MODEL_TIMER1_MINUTE_BEEP || k==ITEM_MODEL_TIMER2_MINUTE_BEEP) {
          timer->minuteBeep = editCheckBox(timer->minuteBeep, MODEL_SETUP_2ND_COLUMN, y, STR_MINUTEBEEP, attr, event);
        }
        else if (k==ITEM_MODEL_TIMER1_COUNTDOWN_BEEP || k==ITEM_MODEL_TIMER2_COUNTDOWN_BEEP) {
          timer->countdownBeep = editCheckBox(timer->countdownBeep, MODEL_SETUP_2ND_COLUMN, y, STR_BEEPCOUNTDOWN, attr, event);
        }
        else {
          drawStringWithIndex(0*FW, y, STR_TIMER, k>=ITEM_MODEL_TIMER2 ? 2 : 1);
          putsTimerMode(MODEL_SETUP_2ND_COLUMN, y, timer->mode, menuHorizontalPosition==0 ? attr : 0);
          putsTimer(MODEL_SETUP_2ND_COLUMN+5*FW-2+5*FWNUM+1, y, timer->start, menuHorizontalPosition==1 ? attr : 0, menuHorizontalPosition==2 ? attr : 0);
          if (attr && (editMode>0 || p1valdiff)) {
            div_t qr = div(timer->start, 60);
            switch (menuHorizontalPosition) {
              case 0:
                CHECK_INCDEC_MODELVAR_CHECK(event, timer->mode, SWSRC_FIRST, TMRMODE_COUNT+SWSRC_LAST-1/*SWSRC_None removed*/, isSwitchAvailableInTimers);
                break;
              case 1:
                CHECK_INCDEC_MODELVAR_ZERO(event, qr.quot, 59);
                timer->start = qr.rem + qr.quot*60;
                break;
              case 2:
                qr.rem -= checkIncDecModel(event, qr.rem+2, 1, 62) - 2;
                if ((int16_t)timer->start >= qr.rem) {
                  timer->start -= qr.rem ;
                }
                if ((int16_t)timer->start > 3599) {
                  timer->start = 3599; // 59:59
                }
                break;
            }
          }
        }
        break;
      }

#if defined(CPUM2560)
      case ITEM_MODEL_TIMER1_PERSISTENT:
      case ITEM_MODEL_TIMER2_PERSISTENT:
      {
        TimerData &timer = g_model.timers[k==ITEM_MODEL_TIMER2_PERSISTENT];
        timer.persistent = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_PERSISTENT, STR_VPERSISTENT, timer.persistent, 0, 2, attr, event);
        break;
      }
#endif
#endif

      case ITEM_MODEL_EXTENDED_LIMITS:
        ON_OFF_MENU_ITEM(g_model.extendedLimits, MODEL_SETUP_2ND_COLUMN, y, STR_ELIMITS, attr, event);
        break;

      case ITEM_MODEL_EXTENDED_TRIMS:
#if defined(CPUM64)
        ON_OFF_MENU_ITEM(g_model.extendedTrims, MODEL_SETUP_2ND_COLUMN, y, STR_ETRIMS, attr, event);
#else
        ON_OFF_MENU_ITEM(g_model.extendedTrims, MODEL_SETUP_2ND_COLUMN, y, STR_ETRIMS, menuHorizontalPosition<=0 ? attr : 0, event==EVT_KEY_BREAK(KEY_ENTER) ? event : 0);
        lcdDrawText(MODEL_SETUP_2ND_COLUMN+3*FW, y, STR_RESET_BTN, (menuHorizontalPosition>0  && !NO_HIGHLIGHT()) ? attr : 0);
        if (attr && menuHorizontalPosition>0) {
          s_editMode = 0;
          if (event==EVT_KEY_LONG(KEY_ENTER)) {
            START_NO_HIGHLIGHT();
            for (uint8_t i=0; i<MAX_FLIGHT_MODES; i++) {
              memclear(&g_model.flightModeData[i], TRIMS_ARRAY_SIZE);
            }
            storageDirty(EE_MODEL);
            AUDIO_WARNING1();
          }
        }
#endif
        break;

#if defined(CPUARM)
      case ITEM_MODEL_DISPLAY_TRIMS:
        g_model.displayTrims = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_DISPLAY_TRIMS, STR_VDISPLAYTRIMS, g_model.displayTrims, 0, 2, attr, event);
        break;
#endif

      case ITEM_MODEL_TRIM_INC:
        g_model.trimInc = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_TRIMINC, STR_VTRIMINC, g_model.trimInc, -2, 2, attr, event);
        break;

      case ITEM_MODEL_THROTTLE_REVERSED:
        ON_OFF_MENU_ITEM(g_model.throttleReversed, MODEL_SETUP_2ND_COLUMN, y, STR_THROTTLEREVERSE, attr, event ) ;
        break;

      case ITEM_MODEL_THROTTLE_TRACE:
      {
        lcd_putsLeft(y, STR_TTRACE);
        if (attr) CHECK_INCDEC_MODELVAR_ZERO(event, g_model.thrTraceSrc, NUM_POTS+NUM_CHNOUT);
        uint8_t idx = g_model.thrTraceSrc + MIXSRC_Thr;
        if (idx > MIXSRC_Thr)
          idx += 1;
        if (idx >= MIXSRC_FIRST_POT+NUM_POTS)
          idx += MIXSRC_CH1 - MIXSRC_FIRST_POT - NUM_POTS;
        putsMixerSource(MODEL_SETUP_2ND_COLUMN, y, idx, attr);
        break;
      }

      case ITEM_MODEL_THROTTLE_TRIM:
        ON_OFF_MENU_ITEM(g_model.thrTrim, MODEL_SETUP_2ND_COLUMN, y, STR_TTRIM, attr, event);
        break;

#if defined(CPUARM)
      case ITEM_MODEL_PREFLIGHT_LABEL:
        lcd_putsLeft(y, STR_PREFLIGHT);
        break;

      case ITEM_MODEL_CHECKLIST_DISPLAY:
        ON_OFF_MENU_ITEM(g_model.displayChecklist, MODEL_SETUP_2ND_COLUMN, y, STR_CHECKLIST, attr, event);
        break;
#endif

      case ITEM_MODEL_THROTTLE_WARNING:
        g_model.disableThrottleWarning = !editCheckBox(!g_model.disableThrottleWarning, MODEL_SETUP_2ND_COLUMN, y, STR_THROTTLEWARNING, attr, event);
        break;

      case ITEM_MODEL_SWITCHES_WARNING:
      {
        lcd_putsLeft(y, STR_SWITCHWARNING);
        swarnstate_t states = g_model.switchWarningState;
        char c;
        if (attr) {
          s_editMode = 0;
          if (!READ_ONLY()) {
            switch (event) {
              CASE_EVT_ROTARY_BREAK
              case EVT_KEY_BREAK(KEY_ENTER):
#if defined(CPUM64)
                g_model.switchWarningEnable ^= (1 << menuHorizontalPosition);
                storageDirty(EE_MODEL);
#else
                if (menuHorizontalPosition < NUM_SWITCHES-1) {
                  g_model.switchWarningEnable ^= (1 << menuHorizontalPosition);
                  storageDirty(EE_MODEL);
                }
#endif
                break;

              case EVT_KEY_LONG(KEY_ENTER):
#if defined(CPUM64)
                getMovedSwitch();
                g_model.switchWarningState = switches_states;
                AUDIO_WARNING1();
                storageDirty(EE_MODEL);
#else
                if (menuHorizontalPosition == NUM_SWITCHES-1) {
                  START_NO_HIGHLIGHT();
                  getMovedSwitch();
                  g_model.switchWarningState = switches_states;
                  AUDIO_WARNING1();
                  storageDirty(EE_MODEL);
                }
#endif
                killEvents(event);
                break;
            }
          }
        }

        LcdFlags line = attr;

        for (uint8_t i=0; i<NUM_SWITCHES-1/*not on TRN switch*/; i++) {
          uint8_t swactive = !(g_model.switchWarningEnable & 1 << i);
          attr = 0;

          if (IS_3POS(i)) {
            c = '0'+(states & 0x03);
            states >>= 2;
          }
          else {
            if ((states & 0x01) && swactive)
              attr = INVERS;
            c = pgm_read_byte(STR_VSWITCHES - 2 + 9 + (3*(i+1)));
            states >>= 1;
          }
          if (line && (menuHorizontalPosition == i)) {
            attr = BLINK;
            if (swactive)
              attr |= INVERS;
          }
          lcdDrawChar(MODEL_SETUP_2ND_COLUMN+i*FW, y, (swactive || (attr & BLINK)) ? c : '-', attr);
#if !defined(CPUM64)
          lcdDrawText(MODEL_SETUP_2ND_COLUMN+(NUM_SWITCHES*FW), y, PSTR("<]"), (menuHorizontalPosition == NUM_SWITCHES-1 && !NO_HIGHLIGHT()) ? line : 0);
#endif
        }
        break;
      }

      case ITEM_MODEL_BEEP_CENTER:
        lcd_putsLeft(y, STR_BEEPCTR);
        for (uint8_t i=0; i<NUM_STICKS+NUM_POTS+NUM_ROTARY_ENCODERS; i++) {
          // TODO flash saving, \001 not needed in STR_RETA123
          coord_t x = MODEL_SETUP_2ND_COLUMN+i*FW;
          lcdDrawTextAtIndex(x, y, STR_RETA123, i, ((menuHorizontalPosition==i) && attr) ? BLINK|INVERS : (((g_model.beepANACenter & ((BeepANACenter)1<<i)) || (attr && CURSOR_ON_LINE())) ? INVERS : 0 ) );
        }
        if (attr && CURSOR_ON_CELL) {
          if (event==EVT_KEY_BREAK(KEY_ENTER) || p1valdiff) {
            if (READ_ONLY_UNLOCKED()) {
              s_editMode = 0;
              g_model.beepANACenter ^= ((BeepANACenter)1<<menuHorizontalPosition);
              storageDirty(EE_MODEL);
            }
          }
        }
        break;

#if defined(CPUARM)
      case ITEM_MODEL_USE_GLOBAL_FUNCTIONS:
        lcd_putsLeft(y, STR_USE_GLOBAL_FUNCS);
        drawCheckBox(MODEL_SETUP_2ND_COLUMN, y, !g_model.noGlobalFunctions, attr);
        if (attr) g_model.noGlobalFunctions = !checkIncDecModel(event, !g_model.noGlobalFunctions, 0, 1);
        break;
#endif

#if defined(PCBSKY9X)
      case ITEM_MODEL_EXTRA_MODULE_LABEL:
        lcd_putsLeft(y, "RF Port 2 (PPM)");
        break;
#endif

#if defined(CPUARM)
      case ITEM_MODEL_EXTERNAL_MODULE_LABEL:
        lcd_putsLeft(y, TR_EXTERNALRF);
        break;

      case ITEM_MODEL_EXTERNAL_MODULE_MODE:
        lcd_putsLeft(y, STR_MODE);
        lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_TARANIS_PROTOCOLS, g_model.moduleData[EXTERNAL_MODULE].type, menuHorizontalPosition==0 ? attr : 0);
        if (IS_MODULE_XJT(EXTERNAL_MODULE))
          lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN+5*FW, y, STR_XJT_PROTOCOLS, 1+g_model.moduleData[EXTERNAL_MODULE].rfProtocol, menuHorizontalPosition==1 ? attr : 0);
        else if (IS_MODULE_DSM2(EXTERNAL_MODULE))
          lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN+5*FW, y, STR_DSM_PROTOCOLS, g_model.moduleData[EXTERNAL_MODULE].rfProtocol, menuHorizontalPosition==1 ? attr : 0);
#if defined(MULTIMODULE)
        else if (IS_MODULE_MULTIMODULE(EXTERNAL_MODULE)) {
          int8_t multi_rfProto = min(g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol, (uint8_t) MM_RF_PROTO_CUSTOM);
          lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN+5*FW, y, STR_MULTI_PROTOCOLS, multi_rfProto, menuHorizontalPosition==1 ? attr : 0);
        }
#endif
        if (attr && (editMode>0 || p1valdiff)) {
          switch (menuHorizontalPosition) {
            case 0:
              g_model.moduleData[EXTERNAL_MODULE].type = checkIncDec(event, g_model.moduleData[EXTERNAL_MODULE].type, MODULE_TYPE_NONE, MODULE_TYPE_COUNT-1, EE_MODEL, isModuleAvailable);
              if (checkIncDec_Ret) {
                g_model.moduleData[EXTERNAL_MODULE].rfProtocol = 0;
                g_model.moduleData[EXTERNAL_MODULE].channelsStart = 0;
                if (g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_PPM)
                  g_model.moduleData[EXTERNAL_MODULE].channelsCount = 0;
                else
                  g_model.moduleData[EXTERNAL_MODULE].channelsCount = MAX_EXTERNAL_MODULE_CHANNELS();
              }
              break;
            case 1:
              if (IS_MODULE_DSM2(EXTERNAL_MODULE))
                CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].rfProtocol, DSM2_PROTO_LP45, DSM2_PROTO_DSMX);
#if defined(MULTIMODULE)
              else if (IS_MODULE_MULTIMODULE(EXTERNAL_MODULE)) {
                CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol, MM_RF_PROTO_FIRST, MM_RF_PROTO_LAST);
                if (checkIncDec_Ret) {
                  // When in custom protocol mode the highest bit (0x20) is set to indicate the protocl we might be way above MM_RF_PROTO_LAST.
                  // Reset to MM_RF_PROTO_LAST-1 in that case
                  if (checkIncDec_Ret == -1 && g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol >= MM_RF_PROTO_LAST)
                  {
                    g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol = MM_RF_PROTO_LAST-1;
                  }
                  g_model.moduleData[EXTERNAL_MODULE].subType = 0;
                  // Sensible default for DSM2 (same as for ppm): 6ch@11ms
                  if (g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol == MM_RF_PROTO_DSM2)
                    g_model.moduleData[EXTERNAL_MODULE].multi.optionValue = 6;
                  else
                    g_model.moduleData[EXTERNAL_MODULE].multi.optionValue = 0;
                }
              }
#endif
              else
                CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].rfProtocol, RF_PROTO_X16, RF_PROTO_LAST);
              if (checkIncDec_Ret) {
                g_model.moduleData[EXTERNAL_MODULE].channelsStart = 0;
                g_model.moduleData[EXTERNAL_MODULE].channelsCount = MAX_EXTERNAL_MODULE_CHANNELS();
              }
          }
        }
        break;
#endif
#if defined (MULTIMODULE)
      case ITEM_MODEL_EXTERNAL_MODULE_SUBTYPE: {
        lcd_putsLeft(y, STR_SUBTYPE);
        int8_t multi_rfProto = min(g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol, (uint8_t) MM_RF_PROTO_CUSTOM);
        switch (multi_rfProto) {
          case MM_RF_PROTO_FLYSKY:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_FLYSKY, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_FRSKY:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_FRSKY, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_HISKY:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_HISKY, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_DSM2:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_DSM_PROTOCOLS, g_model.moduleData[EXTERNAL_MODULE].subType + 1, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_YD717:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_YD717, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_KN:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_KN, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_SYMAX:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_SYMAX, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_CX10:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_CX10, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_CG023:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_CG023, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_MT99XX:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_MT99, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_MJXQ:
            lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_SUBTYPE_MJXQ, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 0 ? attr : 0);
            break;
          case MM_RF_PROTO_CUSTOM:
            lcdDrawNumber(MODEL_SETUP_2ND_COLUMN + 3 * FW, y, g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol & 0x1f, menuHorizontalPosition == 0 ? attr : 0, 2);
            lcdDrawNumber(MODEL_SETUP_2ND_COLUMN + 5 * FW, y, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition == 1 ? attr : 0, 2);
            break;
        }
        if (attr && (editMode > 0 || p1valdiff)) {
          switch (menuHorizontalPosition) {
            case 0:
              switch (min(g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol, (uint8_t) MM_RF_PROTO_CUSTOM)) {
                case MM_RF_PROTO_HISKY:
                case MM_RF_PROTO_DSM2:
                case MM_RF_PROTO_SYMAX:
                case MM_RF_PROTO_KN:
                  CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 1);
                  break;
                case MM_RF_PROTO_CG023:
                case MM_RF_PROTO_MT99XX:
                case MM_RF_PROTO_FRSKY:
                  CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 2);
                  break;
                case MM_RF_PROTO_FLYSKY:
                case MM_RF_PROTO_MJXQ:
                  CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 3);
                  break;
                case MM_RF_PROTO_YD717:
                  CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 4);
                  break;
                case MM_RF_PROTO_CX10:
                  CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 7);
                  break;
                case MM_RF_PROTO_CUSTOM:
                  //custom protocol using the highest bit 0x20 to indicate that the protocol and the lower bits as the rfProtocol
                  g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol = 0x20 | checkIncDec(event, g_model.moduleData[EXTERNAL_MODULE].multi.rfProtocol & 0x1f, 1, 31, EE_MODEL);
                  break;
              }
              break;
            case 1:
              // Custom protocol, third column is subtype
              CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 7);
              break;
          }
        }
      }
        break;
#endif
#if defined(PCBSKY9X)
      case ITEM_MODEL_EXTRA_MODULE_CHANNELS:
#endif
#if defined(CPUARM)
      case ITEM_MODEL_EXTERNAL_MODULE_CHANNELS:
      {
        uint8_t moduleIdx = CURRENT_MODULE_EDITED(k);
        ModuleData & moduleData = g_model.moduleData[moduleIdx];
        lcd_putsLeft(y, STR_CHANNELRANGE);
        if ((int8_t)PORT_CHANNELS_ROWS(moduleIdx) >= 0) {
          lcdDrawText(MODEL_SETUP_2ND_COLUMN, y, STR_CH, menuHorizontalPosition==0 ? attr : 0);
          lcdDrawNumber(lcdLastPos, y, moduleData.channelsStart+1, LEFT | (menuHorizontalPosition==0 ? attr : 0));
          lcdDrawChar(lcdLastPos, y, '-');
          lcdDrawNumber(lcdLastPos + FW+1, y, moduleData.channelsStart+NUM_CHANNELS(moduleIdx), LEFT | (menuHorizontalPosition==1 ? attr : 0));
          if (attr && (editMode>0 || p1valdiff)) {
            switch (menuHorizontalPosition) {
              case 0:
                CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.channelsStart, 32-8-moduleData.channelsCount);
                break;
              case 1:
                CHECK_INCDEC_MODELVAR(event, moduleData.channelsCount, -4, min<int8_t>(MAX_CHANNELS(moduleIdx), 32-8-moduleData.channelsStart));
                if ((k == ITEM_MODEL_EXTERNAL_MODULE_CHANNELS && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_PPM)) {
                  SET_DEFAULT_PPM_FRAME_LENGTH(moduleIdx);
                }
                break;
            }
          }
        }
        break;
      }
#endif

#if defined(PCBSKY9X)
      case ITEM_MODEL_EXTRA_MODULE_BIND:
#endif
#if defined(CPUARM)
      case ITEM_MODEL_EXTERNAL_MODULE_BIND:
      {
        uint8_t moduleIdx = CURRENT_MODULE_EDITED(k);
        ModuleData & moduleData = g_model.moduleData[moduleIdx];
        if (IS_MODULE_PPM(moduleIdx)) {
          lcd_putsLeft(y, STR_PPMFRAME);
          lcdDrawText(MODEL_SETUP_2ND_COLUMN+3*FW, y, STR_MS);
          lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, (int16_t)moduleData.ppm.frameLength*5 + 225, (menuHorizontalPosition<=0 ? attr : 0) | PREC1|LEFT);
          lcdDrawChar(MODEL_SETUP_2ND_COLUMN+8*FW+2, y, 'u');
          lcdDrawNumber(MODEL_SETUP_2ND_COLUMN+8*FW+2, y, (moduleData.ppm.delay*50)+300, (CURSOR_ON_LINE() || menuHorizontalPosition==1) ? attr : 0);
          lcdDrawChar(MODEL_SETUP_2ND_COLUMN+10*FW, y, moduleData.ppm.pulsePol ? '+' : '-', (CURSOR_ON_LINE() || menuHorizontalPosition==2) ? attr : 0);

          if (attr && (editMode>0 || p1valdiff)) {
            switch (menuHorizontalPosition) {
              case 0:
                CHECK_INCDEC_MODELVAR(event, moduleData.ppm.frameLength, -20, 35);
                break;
              case 1:
                CHECK_INCDEC_MODELVAR(event, moduleData.ppm.delay, -4, 10);
                break;
              case 2:
                CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.ppm.pulsePol, 1);
                break;
            }
          }
        }
        else {
          horzpos_t l_posHorz = menuHorizontalPosition;
          coord_t xOffsetBind = MODEL_SETUP_BIND_OFS;
          if (IS_MODULE_XJT(moduleIdx) && IS_D8_RX(moduleIdx)) {
            xOffsetBind = 0;
            lcd_putsLeft(y, STR_RECEIVER);
            if (attr) l_posHorz += 1;
          }
          else {
            lcd_putsLeft(y, STR_RECEIVER_NUM);
          }
          if (IS_MODULE_XJT(moduleIdx) || IS_MODULE_DSM2(moduleIdx) || IS_MODULE_MULTIMODULE(moduleIdx)) {
            if (xOffsetBind) lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, g_model.header.modelId[moduleIdx], (l_posHorz==0 ? attr : 0) | LEADING0|LEFT, 2);
            if (attr && l_posHorz==0) {
              if (editMode>0 || p1valdiff) {
                CHECK_INCDEC_MODELVAR_ZERO(event, g_model.header.modelId[moduleIdx], IS_MODULE_DSM2(moduleIdx) ? 20 : IS_MODULE_MULTIMODULE(moduleIdx) ? 15 : 63);
                if (checkIncDec_Ret) {
                  modelHeaders[g_eeGeneral.currModel].modelId[moduleIdx] = g_model.header.modelId[moduleIdx];
                }
              }
              if (editMode==0 && event==EVT_KEY_BREAK(KEY_ENTER)) {
                checkModelIdUnique(g_eeGeneral.currModel, moduleIdx);
              }
            }
            lcdDrawText(MODEL_SETUP_2ND_COLUMN+xOffsetBind, y, STR_MODULE_BIND, l_posHorz==1 ? attr : 0);
            lcdDrawText(MODEL_SETUP_2ND_COLUMN+MODEL_SETUP_RANGE_OFS+xOffsetBind, y, STR_MODULE_RANGE, l_posHorz==2 ? attr : 0);
            uint8_t newFlag = 0;
            if (attr && l_posHorz>0 && s_editMode>0) {
              if (l_posHorz == 1)
                newFlag = MODULE_BIND;
              else if (l_posHorz == 2) {
                newFlag = MODULE_RANGECHECK;
              }
            }
            moduleFlag[moduleIdx] = newFlag;
          }
        }
        break;
      }
#endif

#if defined(PCBSKY9X) && defined(REVX)
      case ITEM_MODEL_EXTERNAL_MODULE_OUTPUT_TYPE:
      {
        uint8_t moduleIdx = CURRENT_MODULE_EDITED(k);
        ModuleData & moduleData = g_model.moduleData[moduleIdx];
        moduleData.ppm.outputType = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_OUTPUT_TYPE, STR_VOUTPUT_TYPE, moduleData.ppm.outputType, 0, 1, attr, event);
        break;
      }
#endif

#if defined(CPUARM)
      case ITEM_MODEL_EXTERNAL_MODULE_FAILSAFE: {
        uint8_t moduleIdx = CURRENT_MODULE_EDITED(k);
        ModuleData &moduleData = g_model.moduleData[moduleIdx];
        if (IS_MODULE_XJT(moduleIdx)) {
          lcd_putsLeft(y, TR_FAILSAFE);
          lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_VFAILSAFE, moduleData.failsafeMode, menuHorizontalPosition == 0
                                                                                                ? attr : 0);
          if (moduleData.failsafeMode == FAILSAFE_CUSTOM)
            lcdDrawText(MODEL_SETUP_2ND_COLUMN + MODEL_SETUP_SET_FAILSAFE_OFS, y, STR_SET, menuHorizontalPosition == 1
                                                                                           ? attr : 0);
          if (attr) {
            if (moduleData.failsafeMode != FAILSAFE_CUSTOM)
              menuHorizontalPosition = 0;
            if (menuHorizontalPosition == 0) {
              if (editMode > 0 || p1valdiff) {
                CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.failsafeMode, FAILSAFE_LAST);
                if (checkIncDec_Ret) SEND_FAILSAFE_NOW(moduleIdx);
              }
            }
            else if (menuHorizontalPosition == 1) {
              s_editMode = 0;
              if (moduleData.failsafeMode == FAILSAFE_CUSTOM && event == EVT_KEY_FIRST(KEY_ENTER)) {
                g_moduleIdx = moduleIdx;
                pushMenu(menuModelFailsafe);
              }
            }
            else {
              lcdDrawFilledRect(MODEL_SETUP_2ND_COLUMN, y, LCD_W - MODEL_SETUP_2ND_COLUMN - MENUS_SCROLLBAR_WIDTH, 8);
            }
          }
        }
#if defined(MULTIMODULE)
        else if (IS_MODULE_MULTIMODULE(moduleIdx)) {
          switch (g_model.moduleData[moduleIdx].multi.rfProtocol) {
            case MM_RF_PROTO_FRSKY:
            case MM_RF_PROTO_SFHSS:
              lcd_putsLeft(y, STR_MULTI_RFTUNE);
              break;
            case MM_RF_PROTO_HUBSAN:
              lcd_putsLeft(y, STR_MULTI_VIDFREQ);
              break;
            case MM_RF_PROTO_DSM2:
              g_model.moduleData[moduleIdx].multi.optionValue = selectMenuItem(MODEL_SETUP_2ND_COLUMN, y, STR_MULTI_DSMFRAME, STR_OPTIONS_DSM, g_model.moduleData[moduleIdx].multi.optionValue, 0, 12, attr, event);
              break;
            default:
              lcd_putsLeft(y, STR_MULTI_OPTION);
              break;
          }
          if (g_model.moduleData[moduleIdx].multi.rfProtocol != MM_RF_PROTO_DSM2) {
            lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, g_model.moduleData[moduleIdx].multi.optionValue, LEFT | attr);
            if (attr) {
              CHECK_INCDEC_MODELVAR(event, g_model.moduleData[moduleIdx].multi.optionValue, -128, 127);
            }
          }
        }
#endif
      }
      break;

#if defined(MULTIMODULE)
    case ITEM_MODEL_EXTERNAL_MODULE_AUTOBIND:
      g_model.moduleData[EXTERNAL_MODULE].multi.autoBindMode = editCheckBox(g_model.moduleData[EXTERNAL_MODULE].multi.autoBindMode, MODEL_SETUP_2ND_COLUMN, y, STR_MULTI_AUTOBIND, attr, event);
      break;
    case  ITEM_MODEL_EXTERNAL_MODULE_LOWPOWER:
      g_model.moduleData[EXTERNAL_MODULE].multi.lowPowerMode = editCheckBox(g_model.moduleData[EXTERNAL_MODULE].multi.lowPowerMode, MODEL_SETUP_2ND_COLUMN, y, STR_MULTI_LOWPOWER, attr, event);
      break;
#endif
#endif

#if !defined(CPUARM)
      case ITEM_MODEL_PPM1_PROTOCOL:
        lcd_putsLeft(y, NO_INDENT(STR_PROTO));
        lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_VPROTOS, protocol, menuHorizontalPosition<=0 ? attr : 0);
        if (IS_PPM_PROTOCOL(protocol)) {
          lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN+7*FW, y, STR_NCHANNELS, g_model.ppmNCH+2, menuHorizontalPosition!=0 ? attr : 0);
        }
        else if (menuHorizontalPosition>0 && attr) {
          MOVE_CURSOR_FROM_HERE();
        }
        if (attr && (editMode>0 || p1valdiff || (!IS_PPM_PROTOCOL(protocol) && !IS_DSM2_PROTOCOL(protocol)))) {
          switch (menuHorizontalPosition) {
            case 0:
              CHECK_INCDEC_MODELVAR_ZERO(event, g_model.protocol, PROTO_MAX-1);
              break;
            case 1:
              CHECK_INCDEC_MODELVAR(event, g_model.ppmNCH, -2, 4);
              g_model.ppmFrameLength = g_model.ppmNCH * 8;
              break;
          }
        }
        break;
#endif

#if 0
      case ITEM_MODEL_PPM2_PROTOCOL:
        lcd_putsLeft(y, PSTR("Port2"));
        lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_VPROTOS, 0, 0);
        lcdDrawText(MODEL_SETUP_2ND_COLUMN+4*FW+3, y, STR_CH, menuHorizontalPosition<=0 ? attr : 0);
        lcdDrawNumber(lcdLastPos, y, g_model.moduleData[1].channelsStart+1, LEFT | (menuHorizontalPosition<=0 ? attr : 0));
        lcdDrawChar(lcdLastPos, y, '-');
        lcdDrawNumber(lcdLastPos + FW+1, y, g_model.moduleData[1].channelsStart+8+g_model.moduleData[1].channelsCount, LEFT | (menuHorizontalPosition!=0 ? attr : 0));
        if (attr && (editMode>0 || p1valdiff)) {
          switch (menuHorizontalPosition) {
            case 0:
              CHECK_INCDEC_MODELVAR_ZERO(event, g_model.moduleData[1].channelsStart, 32-8-g_model.moduleData[1].channelsCount);
              SET_DEFAULT_PPM_FRAME_LENGTH(1);
              break;
            case 1:
              CHECK_INCDEC_MODELVAR(event, g_model.moduleData[1].channelsCount, -4, min<int8_t>(8, 32-8-g_model.moduleData[1].channelsStart));
              SET_DEFAULT_PPM_FRAME_LENGTH(1);
              break;
          }
        }
        break;

      case ITEM_MODEL_PPM2_PARAMS:
        lcd_putsLeft(y, STR_PPMFRAME);
        lcdDrawText(MODEL_SETUP_2ND_COLUMN+3*FW, y, STR_MS);
        lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, (int16_t)g_model.moduleData[1].ppmFrameLength*5 + 225, (menuHorizontalPosition<=0 ? attr : 0) | PREC1|LEFT);
        lcdDrawChar(MODEL_SETUP_2ND_COLUMN+8*FW+2, y, 'u');
        lcdDrawNumber(MODEL_SETUP_2ND_COLUMN+8*FW+2, y, (g_model.moduleData[1].ppmDelay*50)+300, (menuHorizontalPosition < 0 || menuHorizontalPosition==1) ? attr : 0);
        lcdDrawChar(MODEL_SETUP_2ND_COLUMN+10*FW, y, g_model.moduleData[1].ppmPulsePol ? '+' : '-', (menuHorizontalPosition < 0 || menuHorizontalPosition==2) ? attr : 0);
        if (attr && (editMode>0 || p1valdiff)) {
          switch (menuHorizontalPosition) {
            case 0:
              CHECK_INCDEC_MODELVAR(event, g_model.moduleData[1].ppmFrameLength, -20, 35);
              break;
            case 1:
              CHECK_INCDEC_MODELVAR(event, g_model.moduleData[1].ppmDelay, -4, 10);
              break;
            case 2:
              CHECK_INCDEC_MODELVAR_ZERO(event, g_model.moduleData[1].ppmPulsePol, 1);
              break;
          }
        }
        break;
#endif

#if !defined(CPUARM)
      case ITEM_MODEL_PPM1_PARAMS:
        if (IS_PPM_PROTOCOL(protocol)) {
          lcd_putsLeft(y, STR_PPMFRAME);
          lcdDrawText(MODEL_SETUP_2ND_COLUMN+3*FW, y, STR_MS);
          lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, (int16_t)g_model.ppmFrameLength*5 + 225, (menuHorizontalPosition<=0 ? attr : 0) | PREC1|LEFT);
          lcdDrawChar(MODEL_SETUP_2ND_COLUMN+8*FW+2, y, 'u');
          lcdDrawNumber(MODEL_SETUP_2ND_COLUMN+8*FW+2, y, (g_model.ppmDelay*50)+300, (CURSOR_ON_LINE() || menuHorizontalPosition==1) ? attr : 0);
          lcdDrawChar(MODEL_SETUP_2ND_COLUMN+10*FW, y, g_model.pulsePol ? '+' : '-', (CURSOR_ON_LINE() || menuHorizontalPosition==2) ? attr : 0);
          if (attr && (editMode>0 || p1valdiff)) {
            switch (menuHorizontalPosition) {
              case 0:
                CHECK_INCDEC_MODELVAR(event, g_model.ppmFrameLength, -20, 35);
                break;
              case 1:
                CHECK_INCDEC_MODELVAR(event, g_model.ppmDelay, -4, 10);
                break;
              case 2:
                CHECK_INCDEC_MODELVAR_ZERO(event, g_model.pulsePol, 1);
                break;
            }
          }
        }
#if defined(DSM2) || defined(PXX)
        else if (IS_DSM2_PROTOCOL(protocol) || IS_PXX_PROTOCOL(protocol)) {
          if (attr && menuHorizontalPosition > 1) {
            REPEAT_LAST_CURSOR_MOVE(); // limit 3 column row to 2 colums (Rx_Num and RANGE fields)
          }
          lcd_putsLeft(y, STR_RECEIVER_NUM);
          lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, g_model.header.modelId[0], (menuHorizontalPosition<=0 ? attr : 0) | LEADING0|LEFT, 2);
          if (attr && (menuHorizontalPosition==0 && (editMode>0 || p1valdiff))) {
            CHECK_INCDEC_MODELVAR_ZERO(event, g_model.header.modelId[0], 99);
          }
#if defined(PXX)
          if (protocol == PROTO_PXX) {
            lcdDrawText(MODEL_SETUP_2ND_COLUMN+4*FW, y, STR_SYNCMENU, menuHorizontalPosition!=0 ? attr : 0);
            uint8_t newFlag = 0;
            if (attr && menuHorizontalPosition>0 && editMode>0) {
              // send reset code
              newFlag = MODULE_BIND;
            }
            moduleFlag[0] = newFlag;
          }
#endif
#if defined(DSM2)
          if (IS_DSM2_PROTOCOL(protocol)) {
            lcdDrawText(MODEL_SETUP_2ND_COLUMN+4*FW, y, STR_MODULE_RANGE, menuHorizontalPosition!=0 ? attr : 0);
            moduleFlag[0] = (attr && menuHorizontalPosition>0 && editMode>0) ? MODULE_RANGECHECK : 0; // [MENU] key toggles range check mode
          }
#endif
        }
#endif
        break;
#endif
    }
  }

#if defined(CPUARM) && defined(PXX)
  if (IS_RANGECHECK_ENABLE()) {
    displayPopup("RSSI: ");
    lcdDrawNumber(16+4*FW, 5*FH, TELEMETRY_RSSI(), BOLD);
  }
#endif
}

#if defined(CPUARM)
void menuModelFailsafe(uint8_t event)
{
  static bool longNames = false;
  bool newLongNames = false;
  uint8_t ch = 0;

  if (event == EVT_KEY_LONG(KEY_ENTER) && s_editMode) {
    START_NO_HIGHLIGHT();
    g_model.moduleData[g_moduleIdx].failsafeChannels[menuVerticalPosition] = channelOutputs[menuVerticalPosition];
    storageDirty(EE_MODEL);
    AUDIO_WARNING1();
    SEND_FAILSAFE_NOW(g_moduleIdx);
  }

  SIMPLE_SUBMENU_NOTITLE(NUM_CHNOUT);

  SET_SCROLLBAR_X(0);

  #define COL_W   (LCD_W)
  const uint8_t SLIDER_W = 90;
  ch = 8 * (menuVerticalPosition / 8);

  lcd_putsCenter(0*FH, FAILSAFESET);
  lcdInvertLine(0);

  uint8_t col = 0;

  {
    coord_t x = col*COL_W+1;

    // Channels
    for (uint8_t line=0; line<8; line++) {
      coord_t y = 9+line*7;
      int32_t val;
      uint8_t ofs = (col ? 0 : 1);

      if (ch < g_model.moduleData[g_moduleIdx].channelsStart || ch >= NUM_CHANNELS(g_moduleIdx) + g_model.moduleData[g_moduleIdx].channelsStart)
        val = 0;
      else if (s_editMode && menuVerticalPosition == ch)
        val = channelOutputs[ch];
      else
        val = g_model.moduleData[g_moduleIdx].failsafeChannels[8*col+line];

      putsChn(x+1-ofs, y, ch+1, SMLSIZE);

      // Value
      LcdFlags flags = TINSIZE;
      if (menuVerticalPosition == ch && !NO_HIGHLIGHT()) {
        flags |= INVERS;
        if (s_editMode)
          flags |= BLINK;
      }
#if defined(PPM_UNIT_US)
      uint8_t wbar = (longNames ? SLIDER_W-10 : SLIDER_W);
      lcdDrawNumber(x+COL_W-4-wbar-ofs, y, PPM_CH_CENTER(ch)+val/2, flags);
#elif defined(PPM_UNIT_PERCENT_PREC1)
      uint8_t wbar = (longNames ? SLIDER_W-16 : SLIDER_W-6);
      lcdDrawNumber(x+COL_W-4-wbar-ofs, y, calcRESXto1000(val), PREC1|flags);
#else
      uint8_t wbar = (longNames ? SLIDER_W-10 : SLIDER_W);
      lcdDrawNumber(x+COL_W-4-wbar-ofs, y, calcRESXto1000(val)/10, flags);
#endif

      // Gauge
      lcdDrawRect(x+COL_W-3-wbar-ofs, y, wbar+1, 6);
      uint16_t lim = g_model.extendedLimits ? 640*2 : 512*2;
      uint8_t len = limit((uint8_t)1, uint8_t((abs(val) * wbar/2 + lim/2) / lim), uint8_t(wbar/2));
      coord_t x0 = (val>0) ? x+COL_W-ofs-3-wbar/2 : x+COL_W-ofs-2-wbar/2-len;
      lcdDrawSolidHorizontalLine(x0, y+1, len);
      lcdDrawSolidHorizontalLine(x0, y+2, len);
      lcdDrawSolidHorizontalLine(x0, y+3, len);
      lcdDrawSolidHorizontalLine(x0, y+4, len);

      ch++;
    }
  }

  longNames = newLongNames;
}
#endif
