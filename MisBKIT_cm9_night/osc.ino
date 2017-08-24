#include "Xserial.h"
#include "utils.h"

char  oscNull[4] = {0,0,0,0};
char* oscAddr    = oscNull;
char* oscFormat  = oscNull;
char* oscData    = oscNull;
uint8 oscBuffer[2048];


void oscStart(char* buff,int len){
  if(len<=0)
    len = 2048;
  int i=0;
  int f=0;
  oscAddr = buff;
  
  while((i<len)&&(buff[i]!=0)){i++;}
  i=(i+4) & 0xFFFC;
  oscFormat = &buff[i];
  
  while((i<len)&&(buff[i]!=0)){i++;}
  i=(i+4) & 0xFFFC;
  oscData = &buff[i];

/*  
  SerialUSB.print("OSC:");
  SerialUSB.print(oscAddr);
  SerialUSB.print(" ");
  SerialUSB.print(oscFormat);
  SerialUSB.print(" ");
  
  if(oscFormat[1]=='f')
   SerialUSB.print(oscGetFloat());
  
  SerialUSB.print("]");
  SerialUSB.println(len-i);
*/ 
}

void oscEnd(){
  oscAddr=oscFormat=oscData = oscNull;
}

void oscSwap( uint8* dest, uint8* sce ){
  dest[0] = sce[3];
  dest[1] = sce[2];
  dest[2] = sce[1];
  dest[3] = sce[0];  
}

char oscNextType(){
  if(*oscFormat!=0)
    oscFormat++;
  return *oscFormat;  
}

float oscGetFloat(){
  float val = 0;
  oscSwap((uint8*)&val , (uint8*)oscData);
  oscData+=4;
  return val;
}

int oscGetInt(){
  int val = 0;
  oscSwap((uint8*)&val , (uint8*)oscData);
  oscData+=4;
  return val;
}


void oscSendFloat(XSerial* pS ,const char* addr,float val){
  uint8* pBuf = (uint8*)pS->strBuffer;
  int i = 0;
  while( addr[i]!=0 ){
    pBuf[i]=addr[i++];
  }
  do{
    pBuf[i++]=0;
  }while( (i&3)!=0 );
  
  pBuf[i++]=',';
  pBuf[i++]='f';  
  pBuf[i++]=0;
  pBuf[i++]=0;
  
  oscSwap( &pBuf[i] , (uint8*)&val);
  pS->write( pBuf , i+4 ); 
}

void oscSend(XSerial* pS ,const char* addr,const char* format,...){
  uint8* pBuf = oscBuffer;
  int i = 0;
  while( addr[i]!=0 ) pBuf[i]=addr[i++];
  do{ pBuf[i++]=0; }while( (i&3)!=0 );
  
  char* pf = (char*)format;
  while( *pf!=0 ){ pBuf[i++]=*pf;pf++; }
  do{ pBuf[i++]=0; }while( (i&3)!=0 );

  va_list argList;
  va_start(argList, format);
  int ival;
  float fval;

  pf = (char*)format+1; //skip ','
  while( *pf!=0 ){
    switch( *pf ){
      case 'i':
        ival = va_arg(argList, int);
        oscSwap( &pBuf[i] , (uint8*)&ival);
        i+=4;
        break;
      case 'f':
        fval = (float)va_arg(argList, double);
        oscSwap( &pBuf[i] , (uint8*)&fval);
        i+=4;
        break;
      case 's':
        char* str = va_arg(argList, char*);
        while(*str !=0){          
          pBuf[i++]=*str;
          str++;
        }
        do{
          pBuf[i++]=0;
        }while( (i & 3)!=0 );       
        break;
    }
    pf++;
  }
  va_end(argList);  
  pS->write( pBuf , i ); 

  //SerialUSB.print("send:");for(int z=0;z<i;z++){SerialUSB.print(" ");SerialUSB.print((int)pBuf[z]);}
  //SerialUSB.println("]");
}



