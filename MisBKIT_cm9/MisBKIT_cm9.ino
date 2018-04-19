
#include "libpandora_types.h"
#include "Arduino-compatibles.h"

#include "XSerial.h"
#include "esp8266.h"
#include "DxlMotor.h"
//#include "osc.h"

const int   cm9Num = 9;                //TOTHINK changeable --> EEPROM

const int version[2] = {5,0};

char cm9Name[8] = "CM9_00";            //modifié par cm9Num
char cm9Pswd[16] = "ensad-mbk00";      //modifié par cm9Num
const char* routerSSID ="MisBKit00";   //Router ssid  //TOTHINK changeable --> EEPROM
const char* routerPswd ="ensadmbk00";  //password

const int localPort  = 41234;
const int remotePort = 41235; 

boolean usbEnabled = false;
boolean useOSC = false;

HardwareTimer ledTimer(1);

#define BUTTON_PIN 23 //25
//--------------------------------
//#include "EEPROM.h"
//EEPROM eeprom;
//--------------------------------
//PING : PARALLAX utrasound distance sensor  TODO test attach/detach interrupts
//--------------------------------
//IR : Sharp GP2Y0A21YK0F
//--------------------------------

unsigned long loopTime = 0;
unsigned long cmdTime = 0;
int blinkCount = 0;
int blinkMax   = 2;
boolean fatalError = false;

//====================================================
void setup() 
{
  //fast blink while setup
  pinMode(BOARD_LED_PIN, OUTPUT);
  ledTimer.pause();
  ledTimer.setPeriod(50000); // in microseconds
  ledTimer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
  ledTimer.setCompare(TIMER_CH1, 1);
  ledTimer.attachInterrupt(TIMER_CH1, toggleLed);
  ledTimer.refresh();
  ledTimer.resume();
  
  pinMode(0, INPUT_ANALOG);
  pinMode(1, INPUT_ANALOG);
  pinMode(2, INPUT_ANALOG);
  pinMode(3, INPUT_ANALOG);
  pinMode(6, INPUT_ANALOG);
  pinMode(7, INPUT_ANALOG);
  pinMode(8, INPUT_ANALOG);
  pinMode(9, INPUT_ANALOG);

  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  digitalWrite(BOARD_LED_PIN, LOW);    //setup: led on
  
  char* p0 = strchr(cm9Pswd,'0');
  if(cm9Num>10){
    itoa(cm9Num,&cm9Name[4],10);
    itoa(cm9Num,p0,10);
  }
  else{
    itoa(cm9Num,&cm9Name[5],10);
    itoa(cm9Num,p0+1,10);
  }

  //xSerialUSB.begin(115200); //INUTILE !!! ?      
  xSerialESP.begin(115200);
  pSerial = &xSerialESP;

  //TODO flush ?

  if( !xSerialESP.connectTo(routerSSID,routerPswd,cm9Num) ) //try router connection , static ip
    xSerialESP.startSAP(cm9Name,cm9Pswd);                   //create acces point 

  ledTimer.pause();
  digitalWrite(BOARD_LED_PIN, LOW);//led on
    
  dxlInitialize();

  xSerialESP.startTCPserver(localPort);
  loopTime = millis();
  
  //eeprom.begin();
  //saveRouter();
  cmdTime  = millis();
  loopTime = millis();
}

void toggleLed(void){
  toggleLED();
}

void loop()
{
  int rcvCount = 0;
  char* rcv = xSerialESP.readStr();
  while(rcv!=NULL){
    //LOGUSB("rcv:",rcv);
    mbkParseStr(rcv);
    rcvCount++;
    rcv = xSerialESP.readStr();
  }
    
  //if(xSerialESP.lastError !=0 )
  //  fatalError = true;

  unsigned long t = millis();
  if( (t-loopTime)>=50 ){
    loopTime=t;
    mbkUpdate();
    xSerialESP.flushBuffer();

    if(++blinkCount>blinkMax){  //slow blink
      blinkCount=0;
      digitalWrite(BOARD_LED_PIN, LOW);      
    }
    else if(blinkCount==1)
      digitalWrite(BOARD_LED_PIN, HIGH);          
  }

  usbComnandLine();

}


void usbComnandLine(){
  while(xSerialUSB.available()){
    char* str = xSerialUSB.readStr();
    if(str!=NULL){
      usbEnabled = true; //enable LOGUSB
      if(strBegin(str,"AT")){
        LOGUSB("esp>>",str);
        Serial2.print(str);  //direct print, no buffering
      }
      else{
        SerialUSB.print("echo:");SerialUSB.println(str);
        switch( *str ){
          case 'h':
          case '?':
              LOGUSB("cm9:",cm9Name);
              LOGUSB("psw:",cm9Pswd);
              xSerialESP.usbReport();
            break;
          case 's': xSerialESP.startSAP(cm9Name,cm9Pswd);
            break;
          case 'r': 
              xSerialESP.connectTo(routerSSID,routerPswd,cm9Num);
              xSerialESP.startTCPserver(localPort);
            break;
          case 'a': xSerial2.println("AT"); break;
          case 'c': xSerialESP.getConnectedIPs(); break;
          case 'u':
            xSerialESP.startUDP("192.168.4.7,41235");
            break;
          case 't':
            //oscSend(&xSerialESP,"/mbk/pos",",i",engines[0].currPos);
            //oscSend(&xSerialESP,"/mbk/pos",",i",123);
            //engines[0].update(0);
            //SerialUSB.print("pos:");SerialUSB.println(engines[0].currPos);
            xSerialESP.startTCPserver(1337);
            break;
          case 'o':
            if( strBegin(str,"off")){
              usbEnabled=false;
            }
           break; 
          case 'w':
            xSerialESP.scanWifi();
            break;
          //case 'E': saveRouter((char*)routerSSID);break;  
          //case 'e': readRouter();break;
          //case 'R': setRouter(++str);break;
        }
      }
    }//str
  }//usb available  
}



/*
void setRouter(char* pstr){
  if(pstr==NULL)
    return;
  //remove crlf
  char* crlf = strchr(pstr,(char)10);
  if(crlf!=NULL)*crlf=0;
  crlf = strchr(pstr,(char)13);
  if(crlf!=NULL)*crlf=0;

  LOGUSB("setRouter:",pstr);
  char* ssid = strchr(pstr,':');
  if(ssid!=NULL){
    ssid++;
    char* psw  = strchr(ssid,':');
    if(psw!=NULL){
      *psw = 0;
      psw++;
      LOGUSB(" ssid:",ssid);
      LOGUSB("  psw:",psw);      
    }
  }
  
  
  
}


void saveRouter(char* pstr){
  //char* pstr = (char*)routerSSID;
  int addr = 0;
  while(pstr[addr]!=0){
    eeprom.write(addr,pstr[addr]);
    addr++;
  }    
  eeprom.write(addr,0);
  LOGUSB("saved:",pstr);
}

void readRouter(){
  char str[20];
  for(int i=0;i<16;i++){
    str[i]='a'+i;
  }
  str[16]=0;  
  int addr = 0;
  while(addr<16){
    char c = eeprom.read(addr);
    str[addr++]=c;
    if(c==0)
      break;
  }
  LOGUSB("eeprom:",str);
  
}
*/


 
