
#include "libpandora_types.h"
#include "Arduino-compatibles.h"

#include "DxlEngine.h"
#include "XSerial.h"
#include "esp8266.h"
#include "osc.h"
//#include "EEPROM.h"

const char* cm9Name="CM9_Didier";     //Soft Access Point ssid
const char* cm9Pswd  ="ensad-mbk";  //password

const char* routerSSID ="MisBKit00";  //Router ssid
const char* routerPswd ="ensadmbk00"; //password

//const char* remoteIP = "192.168.4.255";
const int localPort  = 41234;
const int remotePort = 41235; 

boolean usbEnabled = false;
boolean useOSC = true;

#define BUTTON_PIN 23 //25
//--------------------------------
//EEPROM CM9_EEPROM;
//--------------------------------
//PING : PARALLAX utrasound distance sensor  TODO test attach/detach interrupts
//--------------------------------
//IR : Sharp GP2Y0A21YK0F
//--------------------------------

unsigned long loopTime = 0;
unsigned long teaTime = 0;
int blinkCount = 0;
int blinkMax   = 2;
boolean fatalError = false;

//====================================================
void setup() 
{
  pinMode(6, INPUT_ANALOG);
  pinMode(7, INPUT_ANALOG);
  pinMode(8, INPUT_ANALOG);
  pinMode(9, INPUT_ANALOG);

  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  digitalWrite(BOARD_LED_PIN, LOW);    //setup: led on

  //CM9_EEPROM.begin();
  //xSerialUSB.begin(115200); //INUTILE !!! ?
      
  delay(1000);
  xSerialESP.begin(115200);
  pSerial = &xSerialESP;

  //flush
  for(int i=0;i<10;i++){
    delay(100);
    while( xSerialESP.readStr() != NULL ){}
  }

  //xSerialESP.restart();
  xSerialESP.startSAP(cm9Name,cm9Pswd);
    
  delay(500);
  DxlEngine::initialize();
  delay(500);

  loopTime = millis();
  digitalWrite(BOARD_LED_PIN, HIGH);//led off

}

void blinkFatal(){
  //delay(40);
  //toggleLED();
}

void loop()
{
  if(fatalError){
    //blinkFatal();
    //return;
    fatalError = false;
  }
  
  while(xSerialUSB.available()){
    char* str = xSerialUSB.readStr();
    if(str!=NULL){
      usbEnabled = true; //enable usb logs
      //SerialUSB.print(str);
      if(strBegin(str,"AT")){
        Serial2.print(str);
      }
      else{
        SerialUSB.print("echo:");SerialUSB.println(str);
        switch( *str ){
          case 'h':
          case '?': xSerialESP.usbReport();
            break;
          case 's': xSerialESP.startSAP(cm9Name,cm9Pswd);
            break;
          case 'r': xSerialESP.connectTo(routerSSID,routerPswd);
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
            SerialUSB.print("pos:");SerialUSB.println(engines[0].currPos);
            oscSend(&xSerialESP,"/mbk/pos",",ii",engines[0].currPos,engines[1].currPos);
            break;
          case 'o':
            if( strBegin(str,"off")){
              usbEnabled=false;
            }   
        }
      }
    }//str
  }//usb available
  
  char* rcv = xSerialESP.readStr();
  while(rcv!=NULL){    
    if(*rcv == '/'){
      useOSC = true;
      DxlEngine::parseOsc(rcv);
    }
    else{
        useOSC = false;
        //LOGUSB("parseCmd:",rcv);
        DxlEngine::parseCmd(rcv);
      }
    rcv = xSerialESP.readStr();
  }
  
  //mbkUpdate();
  if(xSerialESP.lastError !=0 )
    fatalError = true;

  unsigned long t = millis();
  if( (t-loopTime)>=50 ){
    loopTime=t;
    if(++blinkCount>blinkMax){
      blinkCount=0;
      digitalWrite(BOARD_LED_PIN, LOW);      
    }
    else if(blinkCount==1)
      digitalWrite(BOARD_LED_PIN, HIGH);          
  }
  
  mbkUpdate();  
}
/*
float angle = 0;
void testSinus(){
  float s = sin( angle );
  angle += 0.05;
  int pos = (s*511)+512; 
  engines[0].setGoal(pos);  
}

float sinOSC(){
  float s = sin( angle );
  angle += 0.05;
  //oscSend(&xSerialESP,"/dxl/1/S",s);
  oscSend(&xSerialESP,"/dxl/1/S",",f",s);
  return s;
}
*/



 
