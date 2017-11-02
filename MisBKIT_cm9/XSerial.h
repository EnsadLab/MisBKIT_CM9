#ifndef XSERIAL_H
#define XSERIAL_H

#include "Print.h"
#include "usb_callbacks.h" // [ROBOTIS]2012-12-14 add to support making user interrupt function
#include "libpandora_types.h"
#include "Arduino-compatibles.h"
#include "usart.h"
#include "stdarg.h"

#define USB 1
#define SERIAL2 2
#define ESP8266 3
#define OSC     4

#define SERIAL2INTERUPT false

extern int s2count;

class XSerial
{
  public:
  virtual void begin(uint32 bauds){};
  virtual void close(){};
  virtual void attachInterrupt(){};
  virtual void detachInterrupt(){};

  virtual uint32 available(){return 0;};
  virtual uint8  read(){return 0;};
  virtual void   write(uint8 c){};
  virtual void   write(uint8* pu,int len){};
  virtual char*  readStr(){return NULL;};
  virtual void   print(const char* str){};
  virtual void   println(const char* str){};
  
  virtual void sendf(const char* format, ... );

  int count;

  int  strIndex;
  char strBuffer[1024];
  //static char bufferStr[2048]; 
  static void device(int d);// USB SERIAL2 ESP8266 OSC
  
};

class XSerialUSB : public XSerial {
  public:
  void begin(uint32 bauds);
  void close();
  uint32 available();
  uint8  read();
  void   write(uint8 c);
  void   println(const char* str);
  void   print(const char* str);  
  char*  readStr();
};

class XSerial2 : public XSerial {
  public:
  void begin(uint32 bauds);
  void close();
  uint32 available();
  uint8  read();
  boolean gotLF();
  void   write(uint8 c);
  void   println(const char* str);
  void   print(const char* str);  
  char*  readStr();
 
  #if SERIAL2INTERUPT
  int  tail;
  int  head;
  int  overflow;
  char fifoBuffer[2048];
  #endif
};

extern XSerial* pSerial;
extern XSerialUSB xSerialUSB;
extern XSerial2   xSerial2;
extern void serial2Interrupt(byte b);

#endif
