///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 11 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HOP_DROPPERS_H
#define HOP_DROPPERS_H

#define HOP_DROPPER_NUM 3

void servo_set_pos(short servo, short degrees);
void hops_start_task();
void hops_drop(short dropper, void (*taskErrorHandler)(brew_task_t *));


#endif
