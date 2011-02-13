///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date:  9 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BUTTONS_H
#define BUTTONS_H

void startButtonsTask();

#define KEY_UP    0x8
#define KEY_DOWN  0x4
#define KEY_LEFT  0x2
#define KEY_RIGHT 0x1

#define KEY_PRESSED 0x10

#define isKeyPressed(key, state) ((key && state) && (KEY_PRESSED & state)) 

#define upKeyPressed(state)    ((KEY_UP    & state) && (KEY_PRESSED & state)) 
#define downKeyPressed(state)  ((KEY_DOWN  & state) && (KEY_PRESSED & state)) 
#define leftKeyPressed(state)  ((KEY_LEFT  & state) && (KEY_PRESSED & state)) 
#define rightKeyPressed(state) ((KEY_RIGHT & state) && (KEY_PRESSED & state)) 

#endif
