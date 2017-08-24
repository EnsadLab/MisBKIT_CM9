#include "esp8266.h"
#include "HardwareSerial.h"
#include "osc.h"

extern const char* name;
extern const char* routerSSID;
extern const char* routerPswd;
extern const int localPort;  //41234;
extern const int remotePort; //41235; 
extern int blinkMax;

ESP xSerialESP;
char espReadBuffer[256];

int rcvCount = 0;
int ipdOverflow = 0;
unsigned long helloTime = 0;

//Station state
#define STATION_NONE -1
#define STATION_DISCONNECTED 0
#define STATION_WAIT 1
#define STATION_GOT_IP 2
#define STATION_BROADCAST 3
#define STATION_HASCLIENT 4

#if SERIAL2INTERUPT
void espInterrupt(byte b){
  xSerial2.head = (++xSerial2.head)&2047;
  if(xSerial2.head==xSerial2.tail){
    xSerial2.tail = (xSerial2.head+1)&2047;
    xSerial2.overflow++;
  } 
}
#endif


void ESP::begin(uint32 bauds){
  ready = false;
  stationState = STATION_NONE;
  whenOK = ONOK_NOTHING;
  lastError = 0;
  portIn = 41234; //default
  portOut= 41235; //default
  str2str("0.0.0.0",ipSAP);
  str2str("0.0.0.0",ipSTA);  
  xSerial2.strIndex = 0;
  Serial2.begin(bauds);
}

void ESP::close(){
}

uint32 ESP::available(){
  return xSerial2.available();
}

uint8 ESP::read(){
  return xSerial2.read();
}

void ESP::write(uint8 c){
  Serial2.write(c);
}

void ESP::write(uint8* pu,int len){
  sendUDP(pu,len);  
}

void ESP::print(const char* s){
  sendUDP((uint8*)s,strlen(s));
}

void ESP::println(const char* s){
  int i=0;
  for(i=0;s[i]!=0;i++)
    strBuffer[i]=s[i];
    
  strBuffer[i++]=13;
  strBuffer[i++]=10;
  strBuffer[i++]=0;
  sendUDP((uint8*)strBuffer,i);
}

char* ESP::readStr(){
  unsigned int t = millis();
  char* str = readLine();
  if( ipdLength>0 ){
    rcvTime = t;
    ipdLength = 0;
    LOGUSB("!readIPD:",ipdBuffer);
    rcvCount++;
    return ipdBuffer;
  }
  else if(str!=NULL){ 
   LOGUSB("readStr:",str);
   return NULL;
  }
  
  if( (t-rcvTime)>20000 ){ //timeout 30s
    if( (t-helloTime)>5000 ){ //helloIntervale: 3s
      helloTime=t;
      //LOGUSB("readStr:timeout:",rcvCount);
      //sendUDP("hello world\n");
      /*
      if(stationState<=STATION_DISCONNECTED){
        connectTo(routerSSID,routerPswd);
      }        
      else if(stationState==STATION_BROADCAST){
        SerialUSB.print(">>broadCast:");SerialUSB.println(cmdSend);
        sendUDP("hello world\n");
      }
      */
    } 
  }
  
  return NULL;
}




//=============================================================

void ESP::restart(){
  ready = false;
  stationState = STATION_DISCONNECTED;
  str2str("0.0.0.0",ipSTA);  
  Serial2.println("ATE0");waitOK();
  
  Serial2.println("AT+CIPCLOSE=5");waitOK(); //close all connections
  //Serial2.println("AT+RST");waitOK();      // ->>>> ????
  Serial2.println("AT+CWQAP");   waitOK();   //disconnect from routeur    
  Serial2.println("AT+CWMODE=3");waitOK();   //AP+station

  connectTo(routerSSID,routerPswd);
  helloTime = 0; //
  rcvTime = 0;
  LOGUSB("restarted"," !");
};


/*
boolean ESP::startSAP(const char* ssid,const char* psw){
  strIndex = 0;
  Serial2.println("ATE0");
  waitOK();
  Serial2.println("AT+CIPCLOSE");
  waitOK();
  strPrint(strBuffer,"AT+CWSAP=\"%s\",\"%s\",5,3",ssid,psw);
  Serial2.println(strBuffer);
  delay(500);
  return waitOK();
}
*/

//SAP soft Access Point
boolean ESP::startSAP(const char* ssid,const char* psw){
  ready = false;
  LOGUSB("soft Access Point:",ssid);
  strIndex = 0;
  Serial2.println("ATE0");waitOK();
  
  //Serial2.println("AT+CIPCLOSE");waitOK();
  Serial2.println("AT+CWMODE=2");waitOK(); //AP+station
    
  strPrint(strBuffer,"AT+CWSAP=\"%s\",\"%s\",5,3,4",ssid,psw); //5:channel //3:WPA2_PSK //maxconnexions
  Serial2.println(strBuffer);
  delay(500);
  LOGUSB("startSAP:",strBuffer);
  boolean isok = waitOK();
  LOGUSB("-----SAP:",espReadBuffer);
  if(isok){
    getSapIP();
    startUDP("192.168.4.255,41235"); //pas de broadcast?
    blinkMax = 20;
  }
  else
  LOGUSB("-----notOK:",espReadBuffer);
  
  rcvTime = 0;
  helloTime = 0;
  return isok;
}


//STATION
void ESP::connectTo(const char* ssid,const char* psw){
  ready = false;
  blinkMax = 1;
  LOGUSB("[station:...",ssid);
  strIndex = 0;
  Serial2.println("AT+CIPCLOSE");waitOK(); //close conections

  Serial2.println("AT+CWQAP");waitOK(); //disconnect from routeur
  
  //Serial2.println("AT+CWMODE=3");waitOK(); //AP+station
  Serial2.println("AT+CWMODE=1");waitOK(); //station
    
  strPrint(strBuffer,"AT+CWJAP=\"%s\",\"%s\"",ssid,psw);
  Serial2.println(strBuffer);
  stationState = STATION_WAIT;
  LOGUSB(strBuffer,"WAIT...");  

  while( waitString()>0 ){ //ATTENTION ----->looooong timeout -----> onWIFI
    //SerialUSB.print("connectTo:");SerialUSB.println(espReadBuffer); //... WIFI CONNECTED ... WIFI GOT IP ... OK ... FAIL ...
    LOGUSB(" CWJAP:",espReadBuffer); //... WIFI CONNECTED ... WIFI GOT IP ... OK ... FAIL ...
    //if(strBegin(espReadBuffer,"WIFI")){ onWIFI(); }
    if(strBegin(espReadBuffer,"OK"))break;
    if(strBegin(espReadBuffer,"ERROR"))break;
    if(strBegin(espReadBuffer,"FAIL"))break;
  }
  /*  
  if( stationState = STATION_GOT_IP ){
      getStaIP();
      //TODO check ip      
      stationState=STATION_BROADCAST;
      SerialUSB.println("STATION_BROADCAST");      
  }
  else{
      stationState = STATION_DISCONNECTED;
      SerialUSB.println("STATION_DISCONNECTED");
  }
  */
  LOGUSB("station ]","...");
  rcvTime = millis(); //reset timeout    
  //esp send "WIFI DISCONNECT" if router turns off !!!
  // ... et se reconnecte seul lorsque le routeur reviens "WIFI GOT IP"
}


boolean ESP::waitOK(){ //OK:true ERROR:false
  whenOK = ONOK_NOTHING; //only wait
  boolean ok=false;
  while( waitString()>0 ){
    //SerialUSB.print(espReadBuffer);
    if(strBegin(espReadBuffer,"OK")){ok=true;break;}
    if(strBegin(espReadBuffer,"ERROR")){
      LOGUSB("waitOk:",espReadBuffer);
      break;
    }
  }
  return ok;
}

int ESP::waitString(){
  strIndex = 0;
  espReadBuffer[0]=0;
  int i = 0;
  unsigned long t = millis();
  do{
    if(available()){
      uint8 c = read();
      espReadBuffer[i++]=c;
      if( (c==10)||(i>2040) ){
        
        if(*espReadBuffer>=' '){
          espReadBuffer[i]=0;
          return i;
        }
        i=0; //ligne vide, still waiting
      }
    }
  }while( (millis()-t)<2000 ); //wow ça arrive !!!
  LOGUSB("waitString:","timeout");
  return 0;
}

void ESP::buildBroadcastAT(){ //TODO with mask ?
  char* sce  = ipSTA;
  char* dest = clientIP;
  int i=0;
  char c;
  do{ c=sce[i];dest[i]=c;i++;}while( (c!='.')&&(c!=0) ); //skip
  do{ c=sce[i];dest[i]=c;i++;}while( (c!='.')&&(c!=0) ); //skip
  do{ c=sce[i];dest[i]=c;i++;}while( (c!='.')&&(c!=0) ); //skip
  dest[i++]='2';dest[i++]='5';dest[i++]='5'; //255
  dest[i++]=0;
  LOGUSB("startBroadcast:",clientIP);
  startUDP(clientIP);
}


void ESP::startUDP(const char* ip){ //ipport = "xxx.xxx.xxx.xxx,nnnn" //TODO port

  
  Serial2.println("AT+CIPCLOSE");waitOK(); //close all connexions

  //strPrint((char*)strBuffer,"AT+CIPSTART=\"UDP\",\"%s\",%i,%i,2",ip,portOut,portIn);//0:destNotChange,1:changeOnce,2:changeAllowed
  char* s  = (char*)ip;
  char* d1 = str2str(cmdSend,"AT+CIPSEND=0000,\"");
  char* d2 = str2str(strBuffer,"AT+CIPSTART=\"UDP\",\"");
  while( (*s>='.')&&(*s<='9') ){
    *d1=*s;d1++;
    *d2=*s;d2++;
    s++;
  }
  *d1='\"';d1++;
  *d2='\"';d2++;
  LOGUSB("startUDP:",ip);
  d1=int2str(d1,',',portOut);
  d2=int2str(d2,',',portOut);
  d2=int2str(d2,',',portIn);
  d2=int2str(d2,',',2);
  LOGUSB("cipstart:",strBuffer);
  LOGUSB("cmdSend :",cmdSend);
    
  Serial2.println((char*)strBuffer);waitOK(); //start UDP connexion TODO ERROR
  Serial2.println("AT+CIPDINFO=1");waitOK(); //get ip & port on messages //last message = client
  ready = true;
}

//( 8,10.0.0.6,41235: )
void ESP::getClientIP(char* str){ //!!! not safe !!!  
  while(*str!=','){str++;} //skiplength
  str++; //skip ','
  //compare//
  char* ip = &cmdSend[17];
  boolean changed = false;
  for(int i=0;ip[i]!='\"';i++){
    if( str[i] != ip[i] ){
       changed = true;
       break;
    }
  }
  if( changed ){
    str2str(clientIP,str);
    LOGUSB("Client:",clientIP);
    startUDP(clientIP);
   }
}


void ESP::sendUDP(const char* str){
    //SerialUSB.print("send");SerialUSB.print(str);
    if(ready){
      int len=0;
      while(str[len]!=0)len++;
      sendUDP((uint8*)str,len);
    }  
}

boolean ESP::sendUDP(uint8* buf,int len){
  //TODO test status
  if(!ready)
    return false;
  
  //set len in cmdSend
  char* str = &cmdSend[11];
  if(len<1000){*str='0';str++;}
  if(len<100){*str='0';str++;}
  if(len<10){*str='0';str++;}
  itoa(len,str,10);
  cmdSend[15]=','; //itoa sets 0
  
  //SerialUSB.print("!sendUDP:");SerialUSB.println(cmdSend);
  Serial2.println(cmdSend);
  if(waitOK()){
    Serial2.write(buf,len);
    while( waitString()>0 ){ //> //Recv len bytes //SEND OK
      //SerialUSB.print("dbg:");SerialUSB.println(espReadBuffer);
      if(strBegin(espReadBuffer,"SEND")){ //SEND OK  , SEND FAIL ... 2 fois ???
        //SerialUSB.println(strBuffer);
        return true;
      }
    }     
  }
  LOGUSB("sendUDP:fail:",espReadBuffer);
  //lastError = 100;
  return false;
}


/* old
boolean ESP::sendUDP(uint8* buf,int len){
  //TODO test status
  
  //set len in cmdSend
  char* str = &cmdSend[11];
  if(len<1000){*str='0';str++;}
  if(len<100){*str='0';str++;}
  if(len<10){*str='0';str++;}
  itoa(len,str,10);
  cmdSend[15]=','; //itoa sets 0
  
  //SerialUSB.print("!sendUDP:");SerialUSB.println(cmdSend);
  Serial2.println(cmdSend);
  if(waitOK()){
    Serial2.write(buf,len);
    while( waitString()>0 ){ //> //Recv len bytes //SEND OK
      //SerialUSB.print("dbg:");SerialUSB.println(strBuffer);
      if(strBegin(espReadBuffer,"SEND")){ //SEND OK  , SEND FAIL ... 2 fois ???
        //SerialUSB.println(strBuffer);
        return true;
      }
    }     
  }
  SerialUSB.print("!UDP SEND FAILED!:");SerialUSB.println(espReadBuffer);
  //lastError = 100;
  return false;
}
*/



//---------------------------------------------

void ESP::getSapIP(){ //!!!! ne faire si SAP off ???
  Serial2.println("AT+CIPAP?");
  while( waitString()>0 ){
    //SerialUSB.println(espReadBuffer);
    if(strBegin(espReadBuffer,"OK")){
      break;
    }
    else if(strBegin(espReadBuffer,"ERROR")){
      SerialUSB.println("CIPAP ERROR");
      break;
    }
    else if(strBegin(espReadBuffer,"+CIPAP:ip:")){
      getQuotedString(espReadBuffer,ipSAP);
    }
  }
  LOGUSB("softAP IP:",ipSAP);
}

void ESP::getStaIP(){ //!!! ne faire si station off
  Serial2.println("AT+CIPSTA?");
  while( waitString()>0 ){
    //SerialUSB.println(espReadBuffer);
    if(strBegin(espReadBuffer,"OK"))break;
    else if(strBegin(espReadBuffer,"ERROR")){
      SerialUSB.println("CIPSTA ERROR");
      break;  
    }
    else if(strBegin(espReadBuffer,"+CIPSTA:ip")){
      getQuotedString(espReadBuffer,ipSTA);
    }
  }
  LOGUSB("station IP:",ipSTA);
}

/*
char* ESP::getIP(){ //SAP
  Serial2.println("AT+CIFSR");
  while( waitString()>0 ){
     SerialUSB.println(espReadBuffer);
    if(strBegin(espReadBuffer,"OK")){
      break;
    }
    else if(strBegin(espReadBuffer,"ERROR")){
      SerialUSB.println("CIFSR ERROR");
      break;
    }  
    else if(strBegin(espReadBuffer,"+CIFSR:APIP,")){
      getQuotedString(espReadBuffer,ipSAP);
    }
  }
  return ipSAP;
}
*/

/*
AT+CIPSTA?
+CIPSTA:ip:"10.0.0.5"
STA:gateway:"10.0.0.1"
+CIPSTA:netmask:"255.255.255.0"
OK
*/
/*
char* ESP::getMyIP(){ //station
  Serial2.println("AT+CIPSTA?");
  while( waitString()>0 ){
    if(strBegin(espReadBuffer,"OK"))break;
    else if(strBegin(espReadBuffer,"ERROR"))break;  
    else if(strBegin(espReadBuffer,"+CIPSTA:ip")){ //0.0.0.0 if disconnected
      getQuotedString(espReadBuffer,ipSTA);
    }
  }
  SerialUSB.print("localIP:");SerialUSB.println(ipSTA);
  
  //buildBroadcastIP(localIP);
  //SerialUSB.println(remoteIP);
  
  return ipSTA;
}
*/

char* ESP::readLine(){
  while(available()){
    uint8 c = read();
    strBuffer[strIndex++]=c;
    if( (strIndex==5)&&(strBegin(strBuffer,"+IPD,")) ){
      readIPD();
      strBuffer[0]=0;
      strIndex=0;
      //SerialUSB.println("!readline IPD");
      return NULL;
    }
    if(c==10){
      strBuffer[strIndex]=0; //crlf compris      
      if(*strBuffer>=' '){ //skip emptyline
        SerialUSB.print("readLine:");SerialUSB.print(strBuffer);
        if( (strBuffer[0]=='O')&&(strBuffer[1]=='K') ){ //OK
          onOK();
        }        
        else if(strBegin(strBuffer,"WIFI")){ //WIFI ...
            onWIFI();
            *strBuffer = 0;  //continue
        }
        else if(strBegin(strBuffer,"FAIL")){
          SerialUSB.println("readLine:FAIL");
          if(stationState==STATION_WAIT){
            stationState=STATION_DISCONNECTED;
            ready = false;
            *strBuffer = 0;  //continue
          }
        }
        strIndex=0;
        if(*strBuffer>=' ')
          return strBuffer;
      }
      strIndex=0;
    }
    if(strIndex>2047)
      strIndex=0;
  }
  return NULL;
}

//+IPD, <len>[, <remote IP>, <remote port>]:<data>
boolean ESP::readIPD(){
  int i=0;
  uint8 c = 0;  
  while(c!=':'){ //read length ( 8,10.0.0.6,41235: )
    if(available()){
      c = read();
      strBuffer[i++]=c;
    }
  }
  strBuffer[i]=0;

  ipdLength=atoi(strBuffer);
  if(ipdLength>254){
    ipdOverflow++;
    ipdLength=0;
    return false;
  }
  //read buffer
  i=0;
  while(i<ipdLength){
    if(available())
      ipdBuffer[i++]=read();
  }
  ipdBuffer[i]=0; //en cas de string

  //TODO TOTHINK
  if( stationState < STATION_HASCLIENT ){
    getClientIP(strBuffer);
    //Serial2.println("AT+CIPDINFO=0");waitOK(); //dont ip & port on messages
  }
  
  return i>0;
}



void ESP::onOK(){
  int w = whenOK;
  whenOK = ONOK_NOTHING;
  switch(w){
    case ONOK_GETIP: //Station ip
      LOGUSB("onOK:","STATION_GOT_IP");      
      stationState = STATION_GOT_IP;
      
      getStaIP();
      //TODO check ip
      buildBroadcastAT(); 
      stationState=STATION_BROADCAST;
      LOGUSB("onOK:","STATION_BROADCAST");      
      blinkMax = 20; //normal blink
      /*
      buildBroadcastIP(ipSTA);
      startUDP(remoteIP,portOut,portIn);
      status = STATUS_BROADCAST;
      helloTime = millis();
      */
    break;
    case ONOK_SETBROADCAST: break;
    case ONOK_SETCLIENT: break;
  }
  whenOK = ONOK_NOTHING;
}

void ESP::onWIFI(){
  LOGUSB("onWIFI:",strBuffer);
  if(strBegin(strBuffer,"WIFI GOT IP")){
    whenOK = ONOK_GETIP; //peut mettre un certain temp
  }
  else if(strBegin(strBuffer,"WIFI DISCONNECT")){
    stationState = STATION_DISCONNECTED;  //disconnected
    LOGUSB("onWIFI:","STATION_DISCONNECTED");
  }
  *strBuffer = 0; //forget this line
}

// -------------------------------------------------------
void ESP::usbReport(){
  SerialUSB.println("------usbReport------");
  Serial2.println("AT+CWSAP?");
  while(waitString()>0){
    if(strBegin(espReadBuffer,"OK"))
      break;
    LOGUSB("cwsap?:",espReadBuffer);
  }
  getSapIP();
  LOGUSB("ipSAP:",ipSAP);
  LOGUSB("ipSTA:",ipSTA);
  
  LOGUSB("cmdSend :",cmdSend);
  
  //cipState(); //udp status
  //getConnectedIPs();
  LOGUSB("nb receptions:",rcvCount);  
  LOGUSB("time recept  :",(int)rcvTime);
  LOGUSB("ready:",(int)ready);
  SerialUSB.println("--------------------");
}

void ESP::cipState(){ //UDP status
  //STATUS:5   //+CIPSTATUS:0,"UDP","192.168.4.2",41235,41234,0
  Serial2.println("AT+CIPSTATUS");
  while( waitString()>0 ){
    LOGUSB("cipstatus:",espReadBuffer);
    /*    
    if(strBegin(espReadBuffer,"OK")){
      SerialUSB.print(cipType);SerialUSB.print(":");
      SerialUSB.print(cipStatus);SerialUSB.print(" ");
      SerialUSB.print(remoteIP);SerialUSB.print(" out:");
      SerialUSB.print(portOut);
      SerialUSB.print(" in:");
      SerialUSB.println(portIn);           
      break;
    }
    else if(strBegin(espReadBuffer,"STATUS:")){
      //station: 2:gotIP 3:tcp/udp 4:udp/tcp disconnected 5:notStation
      cipStatus = atoi(&espReadBuffer[7]);
    }  
    else if(strBegin(espReadBuffer,"+CIPSTATUS:")){
      SerialUSB.println(espReadBuffer);
      //id of connection
      char* next = getQuotedString(espReadBuffer,cipType); //"UDP"
      next = getQuotedString(next,remoteIP);
      next = parseInt(next,&portOut);
      next = parseInt(next,&portIn);
      //0: client 1:server 
    }
    */
  }
}

int ESP::getConnectedIPs(){
  Serial2.println("AT+CWLIF"); //response: ip,mac
  nbConnected = 0;
  while( waitString()>0 ){
    if( (*espReadBuffer>='0')&&(*espReadBuffer<='9') ){
      char* comma = strchr(espReadBuffer,',');
      if(comma != NULL )
        *comma = 0;
       str2str(remoteIP,espReadBuffer);
       SerialUSB.print("connected:");
       SerialUSB.println(remoteIP);    
       nbConnected++;
    }
    else if(strBegin(espReadBuffer,"OK") )
      break;
    else if(strBegin(espReadBuffer,"ERROR") )
      break;
  }
  SerialUSB.print("nb connecions:");SerialUSB.println(nbConnected);
  return nbConnected;
}



