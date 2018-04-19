#include "XSerial.h"
#include "usb_serial.h"
#include "HardwareSerial.h"
#include "utils.h"
#include "stdarg.h"

extern XSerial* pSerial;

XSerialUSB xSerialUSB;
XSerial2   xSerial2;
//XSerial*   pCurrent = &xSerialUSB;
XSerial* pSerial = &xSerialUSB;

//char XSerial::bufferStr[2048];


/*
void usbInterrupt(byte* p,uint8 b){
  for(uint8 i=0;i<b;i++){
    xSerialUSB.fifoBuffer[xSerialUSB.head]=p[i];
    xSerialUSB.head = (++xSerialUSB.head)&2047;
    if(xSerialUSB.head==xSerialUSB.tail){
      xSerialUSB.tail = (xSerialUSB.head+1)&2047;
      xSerialUSB.overflow++;
    }
  }
}
*/

#if SERIAL2INTERUPT
void serial2Interrupt(byte b){
  xSerial2.fifoBuffer[xSerial2.head]=b;
  xSerial2.head = (++xSerial2.head)&2047;
  if(xSerial2.head==xSerial2.tail){
    xSerial2.tail = (xSerial2.head+1)&2047;
    xSerial2.overflow++;
  } 
}
#endif

void XSerial::device(int d){
  switch(d){
    case 1: pSerial=&xSerialUSB;break;
    case 2: pSerial=&xSerial2;break;
    default:
      pSerial=&xSerialUSB;
      break;    
  }
}


//format = "%i,%f %s" int float string
void XSerial::sendf(const char* format, ... ){
  strIndex = 0;
  char* dest = strBuffer;
  char* pf = (char*)format;
  va_list argList;
  va_start(argList, format);
  while(*pf!=0){
    if(*pf!='%'){*dest = *pf;dest++;}
    else{
      pf++;
      switch(*pf){
        case 'i':dest = int2str(dest,0,va_arg(argList, int));break;
        case 'f':dest = float2str(dest,va_arg(argList, double));break;
        case 's':dest = str2str(dest,va_arg(argList, char*));break;
        default: pf--;break; //type not handled
      }
    }
    pf++;
  }
  va_end(argList);
  *dest = 0;
  //SerialUSB.print("sendf:");SerialUSB.print(strBuffer);
  print(strBuffer);
}


//===================================
void XSerialUSB::begin(uint32 bauds){
  count = 0;
  //close();
  strIndex = 0; 
  SerialUSB.begin();
  //SerialUSB.attachInterrupt(usbInterrupt);
}
void XSerial2::begin(uint32 bauds){
  //close();
  strIndex = 0;
  Serial2.begin(bauds);
  #if SERIAL2INTERUPT
    head = 0;
    tail = 0;
    Serial2.attachInterrupt(serial2Interrupt);
  #endif
}

uint32 XSerialUSB::available(){
    //return ( tail!=head );
    return SerialUSB.available();
}
uint32 XSerial2::available(){
  #if SERIAL2INTERUPT
    return ( tail!=head );
  #else
    return Serial2.available();
  #endif
}

uint8 XSerialUSB::read(){
  return SerialUSB.read();
}

uint8 XSerial2::read(){
  #if SERIAL2INTERUPT
    if(tail==head )
      return 0;
    uint8 c = fifoBuffer[tail];
    tail = (++tail)&2047;
    return c;
  #else
    return Serial2.read();
  #endif
}

void XSerialUSB::write(uint8 c){
  SerialUSB.write(c);
}
void XSerial2::write(uint8 c){
  Serial2.write(c);
}

void XSerialUSB::print(const char* s){
  SerialUSB.print(s);
}
void XSerial2::print(const char* s){
  Serial2.print(s);
}

void XSerialUSB::println(const char* s){
  SerialUSB.println(s);
}
void XSerial2::println(const char* s){
  Serial2.println(s);
}

char* XSerialUSB::readStr(){
  while(available()){
    uint8 c=read();
    strBuffer[strIndex++]=c;
    if(c==10){
      strBuffer[strIndex]=0;
      strIndex = 0;
      return strBuffer;
    }    
  }
  return NULL;
}

char* XSerial2::readStr(){
  while(available()){
    uint8 c=read();
    strBuffer[strIndex++]=c;
    if(c==10){
      strBuffer[strIndex]=0;
      strIndex = 0;
      return strBuffer;
    }    
  }
  return NULL;
}

void XSerialUSB::close()
{
  SerialUSB.detachInterrupt();
  SerialUSB.end();
}
void XSerial2::close()
{
  Serial2.detachInterrupt();
  Serial2.end();
}


