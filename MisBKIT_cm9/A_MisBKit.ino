
#include "DxlMotor.h"
#include "utils.h"

#define NB_MOTORS 6


extern Dynamixel Dxl;

extern boolean useOSC;
extern int dxlWrite(int imot,int ireg);
extern int dxlRead(int imot,int ireg);


DxlMotor motors[NB_MOTORS];

unsigned long mbkFrameTime = 0;

//int mbkNbParams = 0;
//int mbkParams[32];

//int mbkTask = 0;
//int mbkScanStep = -1;
//unsigned long mbkSensorTime = 0;
//int mbkAnalogs[10]={0,0,0,0,0,0,0,0,0,0};
//unsigned long mbkTemperatureTime = 0;
//int mbkTemperatureIndex = 0;
int debugCount = 0;
int debugMax = 0; //65535;

void mbkUpdate(){  
  unsigned long t = millis();
  mbkFrameTime = t;
     
  for(int i=0;i<NB_MOTORS;i++){
    motors[i].update();
  }
  
  dxlSyncGoalSpeeds();  
  dxlSendAllPos();
  dxlSendTemperature(t);
  mbkSendAnalogs();
  
  //if(mbkScanStep>=0) mbkSlowScan(); //works badly ... see ping        
}


void mbkParseStr(char* str){
  char* cmd = str;
  while( *cmd != 0 ){
    while( (*cmd<=' ')&&(*cmd!=0) )cmd++; //skip crlf and spaces    
    switch(*cmd){
      case 'd':
        if(strBegin(cmd,"dxl")){
          str = dxlParseStr(cmd+3);
        }
      case 'v':
        if(strBegin(cmd,"version")){
           xSerialESP.sendf("version %i %i %s\n",version[0],version[1],cm9Name);
        }
        cmd=skipLine(cmd);
        break;      
      case '#': //message num
        { int i = atoi(++cmd);
          //LOGUSB('#',i);
          cmd=skipLine(cmd);
        }break;        
      default:
        { char* p=cmd;
          while(*p>=' ')p++;
          char c=*p;
          *p=0;
          LOGUSB("/???:",cmd);
          *p=c;
          cmd=skipLine(cmd);
        }break;        
    }
  }  
}

/*
char* mbkCmdNums(char* str){
  char* p1 = str;  
  mbkNbParams = 0;
  while(*p1>=' '){ // until 0 crlf
    while( (*p1==' ')||(*p1==',') )p1++; //skip spaces or ','
    if( (*p1>='-')&&(*p1<='9') ){ //is num - . / 0 1 2 3 4 5 6 7 8 9
      mbkParams[mbkNbParams++] = atoi(p1);
    }
    while( (*p1>' ')&&(*p1!=',') )p1++; //skip param
  }
  while( (*p1>0)&&(*p1<=' ') )p1++; //skip cr lf space

  SerialUSB.print("params:");
  for(int i=0;i<mbkNbParams;i++){
    SerialUSB.print(mbkParams[i]);SerialUSB.print(",");
  }
  SerialUSB.print("\n");
    
  //DxlEngine::execCmd(pCmd,params,nbParams);
  return p1;
}
*/



/*
void mbkOnMessage(char* msg){
    #ifdef USE_OSC  
    if(*msg == '/'){
      useOSC = true;
      DxlEngine::parseOsc(msg);
    }
    else
    #endif
    {
        useOSC = false;
        //mbkDebugMsg(msg);
        //LOGUSB("parseCmd:",msg);
        //DxlEngine::parseCmd(msg);
    }
}
*/
/*
void mbkDebugMsg(char* msg,int len){
  int num = -1;
  char* cmd = msg;
  int imsg = 0;
  while(imsg<len){
    while((msg[imsg]>=' ')&&(imsg<len)){imsg++;}
    if(msg[imsg]==10){
      msg[imsg]=0;
      imsg++;
    }
    LOGUSB("CMD:",cmd);
    if(strBegin(cmd,"version")){
      LOGUSB("VERSION:","3.7");
      pSerial->sendf("version %i %i %s\n",4,0,cm9Name);
    }
    else if(strBegin(cmd,"name")){
      LOGUSB("NAME:",cm9Name);
      pSerial->sendf("name %s\n",cm9Name);
    }
    else if(cmd[0]=='#'){
      xSerialESP.println(cmd);
    }
    cmd = &msg[imsg];
    if((*cmd<' ')&&(imsg<len)){
      cmd++;
      LOGUSB("???",cmd);
      break;
    }
  }
  //LOGUSB("===",imsg);
}
*/

/*
void mbkExecCmd(char* pcmd,float* pParams,int nbParams){
  //SerialUSB.println(pcmd);
    
  int iservo = (int)pParams[0];
  switch( *pcmd ){ //first char
  
    case 'E': //cmd index params   //EI EM EW ER EX
      if( (iservo>=0)&&(iservo<NB_ENGINES) )
        engines[iservo].onCmd(pcmd,pParams+1,nbParams-1);        
    break;
    
    case 'D'://DW DR
      if( (pcmd[1]=='W') && (nbParams==3)){       //DW dxlId addr value
        dxlWrite(iservo,pParams[1],pParams[2]);
      }
      else if((pcmd[1]=='R') && (nbParams==2)){  //DR dxlId addr
        int val = dxlRead(iservo,pParams[1]);
        if(val>-1)
          //serialSend("dxlReg",pParams[0],pParams[1],val);
          pSerial->sendf("dxlReg %i %i %i\n",iservo,pParams[1],val);
      }
      break;

    case 'N': //goal
      if( (iservo>=0)&&(iservo<NB_ENGINES) ){
         if(pcmd[1]=='G'){
           engines[iservo].setNGoal(pParams[1]);
         }
         else if(pcmd[1]=='S'){
           engines[iservo].setNSpeed(pParams[1]);
         //SerialUSB.print("Ngoal:");SerialUSB.println(engines[iservo].cmdGoal);
         }
      }
      break;

      
    case 'G': //goal
      if( (iservo>=0)&&(iservo<NB_ENGINES) ){
         if(pcmd[1]=='N')
           engines[iservo].setNGoal(pParams[1]);
         else
           engines[iservo].cmdGoal=(int)pParams[1];
         //SerialUSB.print("Ngoal:");SerialUSB.println(engines[iservo].cmdGoal);
      }
      break;
      
    case 'S': //speed
      if( (iservo>=0)&&(iservo<NB_ENGINES) ){
         if(pcmd[1]=='N')
           engines[iservo].setNSpeed(pParams[1]);
         else
           engines[iservo].setSpeed((int)pParams[1]);
      }
      break;
            
    case 'g':
      if(strBegin(pcmd,"goals")){
        int n = nbParams;
        if(n > NB_ENGINES)
          n = NB_ENGINES;
        for(int i=0;i<n;i++){
          engines[i].cmdGoal = (int)pParams[i];
        }
      }
      break;
    case 's':
      if(strBegin(pcmd,"speeds")){
        int n = nbParams;
        if(n > NB_ENGINES)
          n = NB_ENGINES;
        for(int i=0;i<n;i++){
          engines[i].cmdSpeed = (int)pParams[i];
        }
      }
      else if( strBegin(pcmd,"stop") ){
        DxlEngine::stopAll();
        //serialSend("!AllStopped\n");
        //pSerial->print("!AllStopped\n");
      }
      else if( strBegin(pcmd,"scan") ){
        DxlEngine::startScan();
      }
      break;
    
    case 'I':
      if( strBegin(pcmd,"ID") ){
        if( (nbParams>0)&&(iservo>=0)&&(iservo<NB_ENGINES) )        
          engines[iservo].setId((int)pParams[1]);
        //SerialUSB.print("ID:");SerialUSB.println(engines[iservo].dxlId);
      }    
      break;  
    case 'j':
      if(strBegin(pcmd,"joint")){
        if( (nbParams>0)&&(iservo>=0)&&(iservo<NB_ENGINES) )        
          engines[iservo].setJointMode();
          //SerialUSB.print("model:");SerialUSB.println(engines[iservo].model);
      }
      break;
    case 'w':
      if(strBegin(pcmd,"wheel")){
        if( (nbParams>0)&&(iservo>=0)&&(iservo<NB_ENGINES) )        
          engines[iservo].setWheelMode();
          //SerialUSB.print("model:");SerialUSB.println(engines[iservo].model);
      }
      break;
      
    case 'r':
      if(strBegin(pcmd,"relax")){
        if( (nbParams>0)&&(iservo>=0)&&(iservo<NB_ENGINES) )        
          engines[iservo].setRelaxMode();
      }
      break;
      
    case 'm':
      if(strBegin(pcmd,"model")){
        if( (nbParams>0)&&(iservo>=0)&&(iservo<NB_ENGINES) ){
           int m = engines[iservo].getModel();
           //serialSend("model",pParams[0],m);
          pSerial->sendf("model %i %i\n",iservo,m);           
        }
      }
      break;
      
    case 'v':
      if(strBegin(pcmd,"version")){
        LOGUSB("VERSION:","2.2");
        //serialSend("version",2,1);
        pSerial->sendf("version %i %i\n",2,2);
      }
      break;        
    default:
      break;
   }

}
*/
/*
void mbkTemperature(unsigned long t){
  if( (t-mbkTemperatureTime)>2333 ){
    mbkTemperatureTime = t;
    if(++mbkTemperatureIndex>=NB_MOTORS)
      mbkTemperatureIndex=0;
    if(motors[mbkTemperatureIndex].dxlID > 0){
      int tp = dxlRead(motors[mbkTemperatureIndex].dxlID,P_TEMPERATURE);
      xSerialESP.sendf("dxlTemp %i %i\n",mbkTemperatureIndex,tp);           
      //LOGUSB("temperaturev:",tp);
    }    
  }
}
*/
void mbkSendAnalogs(){
    xSerialESP.sendf("analogs %i %i %i %i %i %i %i %i %i %i\n",
      (int)analogRead(0),
      (int)analogRead(1),
      (int)analogRead(2),
      (int)analogRead(3),
      0, //serial2
      0, //serial2
      (int)analogRead(6),
      (int)analogRead(7),
      (int)analogRead(8),
      (int)analogRead(9)      
      );
}
/*
void mbkSendAnalogs(){
    
    mbkAnalogs[0]=analogRead(0);
    mbkAnalogs[1]=analogRead(1);
    mbkAnalogs[2]=analogRead(2);
    mbkAnalogs[3]=analogRead(3);
    mbkAnalogs[6]=analogRead(6);
    mbkAnalogs[7]=analogRead(7);
    mbkAnalogs[8]=analogRead(8);
    mbkAnalogs[9]=analogRead(9);
    
    xSerialESP.sendf("analogs %i %i %i %i %i %i %i %i %i %i\n",
      mbkAnalogs[0],
      mbkAnalogs[1],
      mbkAnalogs[2],
      mbkAnalogs[3],
      0, //serial2
      0, //serial2
      mbkAnalogs[6],
      mbkAnalogs[7],
      mbkAnalogs[8],
      mbkAnalogs[9]      
      );
}
*/

/*
void mbkSlowScan(){
  if(mbkScanStep<254){
     //int m = (int)Dxl.readWord(mbkScanStep,P_MODEL);
     int m = dxlRead(mbkScanStep,P_MODEL);
     if( m==0xFFFF ) m=-1;
     xSerialESP.sendf("dxlScan %i %i\n",mbkScanStep,m);
     mbkScanStep++;
  }
  else
    mbkScanStep=-1; //stop
}
*/


