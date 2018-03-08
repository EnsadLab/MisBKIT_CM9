
#include "libpandora_types.h"
#include "Arduino-compatibles.h"

#include "DxlEngine.h"
#include "XSerial.h"
#include "esp8266.h"
#include "osc.h"

#define USE_ANALOGS

const char* cm9Name="CM9_05";   //Soft Access Point ssid //TOTHINK changeable
const char* cm9Pswd  ="ensad-mbk05";  //password

const char* routerSSID ="MisBKit00";  //Router ssid
const char* routerPswd ="ensadmbk00"; //password
const char* staticIP   ="10.0.0.205"; // 200 + cm9Num 

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
unsigned long teaTime = 0;
int blinkCount = 0;
int blinkMax   = 2;
boolean fatalError = false;

//====================================================
void setup() 
{
  ledTimer.pause();
  pinMode(BOARD_LED_PIN, OUTPUT);
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
  if( !xSerialESP.connectTo(routerSSID,routerPswd,staticIP) )
    xSerialESP.startSAP(cm9Name,cm9Pswd);
    
  delay(500);
  DxlEngine::initialize();
  delay(500);

  loopTime = millis();
  
  ledTimer.pause();

  digitalWrite(BOARD_LED_PIN, HIGH);//led off

  //eeprom.begin();
  //saveRouter();

}

void toggleLed(void){
  toggleLED();
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
          case 'r': xSerialESP.connectTo(routerSSID,routerPswd,staticIP);
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
            #ifdef USE_OSC
            oscSend(&xSerialESP,"/mbk/pos",",ii",engines[0].currPos,engines[1].currPos);
            #endif
            break;
          case 'o':
            if( strBegin(str,"off")){
              usbEnabled=false;
            }
           break; 
          case 'w':
            xSerialESP.scanWifi();
            break;
          /*
          case 'E': saveRouter((char*)routerSSID);break;  
          case 'e': readRouter();break;
          case 'R': setRouter(++str);break;
          */
        }
      }
    }//str
  }//usb available
  
  char* rcv = xSerialESP.readStr();
  while(rcv!=NULL){
    /*    
    if(*rcv == '/'){
      useOSC = true;
      DxlEngine::parseOsc(rcv);
    }
    else{
        useOSC = false;
        //LOGUSB("parseCmd:",rcv);
        DxlEngine::parseCmd(rcv);
      }
    */
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



 
