#ifndef ESP8266_H
#define ESP8266_H

#include "libpandora_types.h"
#include "utils.h"
#include "XSerial.h"

#define ONOK_NOTHING 0
#define ONOK_GETIP   1
#define ONOK_SETBROADCAST 2
#define ONOK_SETCLIENT 3

class ESP : public XSerial {
  public:
  //ESP();
  boolean ready;
  unsigned int timeRef;
  int stationState;
  int whenOK;
  int lastError;
  void begin(uint32 bauds);
  void close();
  uint32 available();
  uint8  read();
  void   write(uint8 c);
  void   write(uint8* pu ,int len);
  void   println(const char* str);
  void   print(const char* str);  
  char*  readStr();  

  //-------------------------------------------------
  boolean startSAP(const char* ssid,const char* psw);
  boolean connectTo(const char* ssid,const char* psw,int cm9num); //const char* stip);
  
  void update( unsigned long t );
 
  char* readLine();
  //char* getIP();
  //char* getMyIP();
  int   getConnectedIPs();
  void buildBroadcastAT(const char* ip,int port);
  //boolean startUDP(const char* ip,int in,int out);
  void startUDP(const char* ipport);
  void sendUDP(const char* str);
  boolean sendUDP(uint8* buf,int len);
  //boolean    send(int len,char* buf);
  void cipState();
  
  void onOK();
  void onWIFI();
  
  boolean waitOK();
  boolean waitOKorERROR();
  boolean waitString(const char* str);
  int waitString(); 
  char* readIPD();
  
  int  getLocalIPs();
  void getSapIP();
  void getStaIP();
  void getClientIP(char* str);
  void usbReport();
  void restart();
  void scanWifi();

  unsigned long rcvTime;  
  int portIn;
  int portOut;
  
  int  ipdIndex;
  int  ipdLength;
  char ipdBuffer[2048];
  
  int   bufSendIndex;
  uint8 bufSend[1024];
  
  int nbConnected;
  char ipSAP[24];  //ip Soft Access Point
  char ipSTA[24];  //ip Station
  char clientIP[24];//123.123.123.123,12345
  char remoteIP[24];//123.123.123.123 16char max
  char cmdSend[64];
  char cmdSendTCP[64];
  //char cmdBroadcast[64];
  
  void writeToBuffer( char* str );
  void writeToBuffer(uint8* pu,int len);
  boolean flushBuffer();
  
    
  int cipStatus;
  char cipType[8];

  int cipMux;
  int muxRcvID;
  int msgGood;
  int msgCount;
  void startTCPserver(int port);
  boolean sendTCP(uint8* buf,int len);


 
};

extern ESP xSerialESP;
#endif

