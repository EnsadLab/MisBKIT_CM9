#ifndef UTILS_H
#define UTILS_H

#include "libpandora_types.h"
#include "Arduino-compatibles.h"
#include "stdarg.h"

extern boolean usbEnabled;
#define LOGUSB(x,y) {if(usbEnabled){SerialUSB.print(x);SerialUSB.println(y);}}

extern void LOGF(const char* format,...);

extern int strlen(const char* str);
extern char* skipLine(char* str);
extern char* skipWord(char* str);
extern char* strPrint(char* dest,const char* format, ... );//format "%i %f %s"

extern char* str2str(char*dest,const char* sce);
extern char* int2str(char* dest,char separator,int n);
extern char* float2str(char* dest,float n);
extern boolean strBegin(char* p1,const char* p2);
extern char* strchr(char* sce,char c);
extern char* strFind(char* str,const char* cs); //AREVOIR
extern char* getQuotedString(char* sce,char* dest);
extern char* parseInt(char* str,int* result);

#endif
