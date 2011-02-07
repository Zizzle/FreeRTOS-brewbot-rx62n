///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
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

#define LEFT_CRANE_END_SW  LED14
#define RIGHT_CRANE_END_SW LED12
#define SSR                LED8
#define MOTOR_DIR          LED13
#define MOTOR_CRANE_X      LED7
#define MOTOR_CRANE_Y      LED11
#define STIRRER            LED6
#define SOLENOID           LED9

#define BUTTON_UP    PORTE.PORT.BIT.B4
#define BUTTON_DOWN  PORTE.PORT.BIT.B5
#define BUTTON_LEFT  PORTE.PORT.BIT.B6
#define BUTTON_RIGHT PORTE.PORT.BIT.B7

#define HOP_DROPPER_1 PORTA.PORT.BIT.B2
#define HOP_DROPPER_2 PORTD.PORT.BIT.B0
#define HOP_DROPPER_3 PORTD.PORT.BIT.B0
#define HOP_DROPPER_1_DDR PORT1.DDR.BIT.B2
#define HOP_DROPPER_2_DDR PORTD.DDR.BIT.B0
#define HOP_DROPPER_3_DDR PORTD.DDR.BIT.B1


#endif
