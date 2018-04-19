#ifdef USE_OSC

#ifndef OSC_H
#define OSC_H

#include "utils.h"

void oscStart(char* buff,int len);
void oscEnd();
char oscNextType();
float oscGetFloat();
int   oscGetInt();
void oscSend(XSerial* pS ,const char* addr,const char* format,...);

#endif

#endif
