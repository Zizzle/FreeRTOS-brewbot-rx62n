///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 13 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BREW_H
#define BREW_H

void brew_start_task();

void brew_start(int init);
int  brew_key_handler(unsigned char key);
void brew_resume(int init);
int  brew_resume_key(unsigned char key);

unsigned short httpd_get_status(void *arg);

#endif
