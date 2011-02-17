///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date:  6 Jan 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BREWBOT_H
#define BREWBOT_H

#include "iodefine.h"
#include "yrdkrx62ndef.h"

#define CRANE_END_SW_LEFT  PORTE.PORT.BIT.B3 //LED5
#define CRANE_END_SW_RIGHT PORTE.PORT.BIT.B0 //LED7
#define CRANE_END_SW_UP    PORTE.PORT.BIT.B1 //LED13
#define CRANE_END_SW_DOWN  PORTE.PORT.BIT.B2 //LED9

#define MOTOR_DIR          LED6
#define MOTOR_DIR_DDR      LED6_DDR
#define MOTOR_CRANE_X      LED8
#define MOTOR_CRANE_X_DDR  LED8_DDR
#define MOTOR_CRANE_Y      LED12
#define MOTOR_CRANE_Y_DDR  LED12_DDR
#define STIRRER            LED4
#define STIRRER_DDR        LED4_DDR
#define SOLENOID           LED15
#define SOLENOID_DDR       LED15_DDR
#define SSR                LED11
#define SSR_DDR            LED11_DDR

#define outputOff(x) x = 1
#define outputOn(x)  x = 0

#define BUTTON_UP          PORTE.PORT.BIT.B7
#define BUTTON_DOWN        PORTE.PORT.BIT.B6
#define BUTTON_LEFT        PORTE.PORT.BIT.B5
#define BUTTON_RIGHT       PORTE.PORT.BIT.B4

#define BUTTON_PORT        PORTE.PORT.BYTE

#define HOP_DROPPER_1      PORTA.DR.BIT.B2
#define HOP_DROPPER_2      PORTD.DR.BIT.B0
#define HOP_DROPPER_3      PORTD.DR.BIT.B0
#define HOP_DROPPER_1_DDR  PORTA.DDR.BIT.B2
#define HOP_DROPPER_2_DDR  PORTD.DDR.BIT.B0
#define HOP_DROPPER_3_DDR  PORTD.DDR.BIT.B1

#define LEVEL_PROBE_HEAT_DDR PORT4.DDR.BIT.B2
#define LEVEL_PROBE_FULL_DDR PORT4.DDR.BIT.B3

#endif
