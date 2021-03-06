#include "DxlEngine.h"
extern Dynamixel Dxl;

extern boolean useOSC;



unsigned long mbkFrameTime = 0;
int mbkToggleSensors = 0;
int mbkTask = 0;
unsigned long mbkSensorTime = 0;
int mbkAnalogIndex = 0;
int mbkAnalogs[10]={0,0,0,0,0,0,0,0,0,0};
unsigned long mbkTemperatureTime = 0;
int mbkTemperatureIndex = 0;
int debugCount = 0;
int debugMax = 0; //65535;

void mbkOnMessage(char* msg){
  
    if(*msg == '/'){
      useOSC = true;
      DxlEngine::parseOsc(msg);
    }
    else{
        useOSC = false;
        LOGUSB("parseCmd:",msg);
        DxlEngine::parseCmd(msg);
    }
}


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

//-------------------------
void mbkUpdate(){
  unsigned long t = millis(); 
  //mbkTemperature(t);

  if( (t-mbkFrameTime)<50 ){
    if(mbkAnalogIndex<10){
      mbkAnalogs[mbkAnalogIndex]=analogRead(mbkAnalogIndex);
      if(++mbkAnalogIndex==4)  //skip serial2
        mbkAnalogIndex=6;
    }
    //#ifdef USE_ANALOGS
    //mbkSensors(t);
    //#endif
    mbkTemperature(t);
    return;
  }
  
  if(mbkAnalogIndex<10)
    debugCount++;
  mbkAnalogIndex = 0;
  
  
  mbkFrameTime = t;
  DxlEngine::task(t);
  mbkSyncGoals();
  mbkSyncSpeeds();
 
  //get All positions
  for(int i=0;i<NB_ENGINES;i++){
    engines[i].update(t);
  }

  #ifdef USE_ANALOGS
    mbkToggleSensors ^= 1;
  #endif
  if(mbkToggleSensors==0){
    if(useOSC)
      oscSend(&xSerialESP,"/mbk/pos",",iiii",engines[0].currPos,engines[1].currPos,engines[2].currPos,engines[3].currPos);
    else
      DxlEngine::sendAllPos();
  }
  else{
    mbkSensors(t);
  }
    
  //mbkSensors(t);
    
}

void mbkTemperature(unsigned long t){
  if( (t-mbkTemperatureTime)>2333 ){
    mbkTemperatureTime = t;
    if(++mbkTemperatureIndex>=NB_ENGINES)
      mbkTemperatureIndex=0;
    if(engines[mbkTemperatureIndex].dxlId > 0){
      //int tmp = Dxl.readByte((uint8)imot,(uint8)ireg);
      int tp = dxlRead(engines[mbkTemperatureIndex].dxlId,P_TEMPERATURE);
      pSerial->sendf("temperature %i %i\n",mbkTemperatureIndex,tp);           
      //LOGUSB("temperaturev:",tp);
    }    
  }
}

void mbkSensors(unsigned long t){
  //if( (t-mbkSensorTime)>100 )
  {
    mbkSensorTime = t;
    /*
    mbkAnalogs[0]=analogRead(0);
    mbkAnalogs[1]=analogRead(1);
    mbkAnalogs[2]=analogRead(2);
    mbkAnalogs[3]=analogRead(3);
    mbkAnalogs[6]=analogRead(6);
    mbkAnalogs[7]=analogRead(7);
    mbkAnalogs[8]=analogRead(8);
    mbkAnalogs[9]=analogRead(9);
    */
    
    pSerial->sendf("analogs %i %i %i %i %i %i %i %i %i %i\n",
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
  
}


void mbkSyncGoals(){
   
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(P_GOAL_POSITION_L);
  Dxl.pushByte(2); //1 word
  int len = 2;
  for(int i=0;i<NB_ENGINES;i++){
    //len+=engines[i].pushGoal();
    int g = engines[i].cmdGoal;
    engines[i].cmdGoal = -1;
    if( (g>=0)&&(engines[i].dxlId > 0) ){
      Dxl.pushByte(engines[i].dxlId);
      Dxl.pushByte(g & 0xFF);
      Dxl.pushByte(g >> 8);
      
      //if(i==0){SerialUSB.print("!g0=");SerialUSB.println(g);}
      
     len+=3; 
    }
  }
  if(len>2)
    Dxl.txPacket(BROADCAST_ID,INST_SYNC_WRITE,len);
  else
    Dxl.setTxPacketLength(0);
}

void mbkSyncSpeeds(){
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(P_GOAL_SPEED_L);
  Dxl.pushByte(2); //1 word
  int len = 2;
  for(int i=0;i<NB_ENGINES;i++){
    int s = engines[i].cmdSpeed;
    engines[i].cmdSpeed = -1;   
    if( (engines[i].dxlId > 0)&&(s>=0) ){
      Dxl.pushByte(engines[i].dxlId);
      Dxl.pushByte(s & 0xFF);
      Dxl.pushByte(s >> 8);
     len+=3; 
    }
  }
  if(len>2){
    Dxl.txPacket(BROADCAST_ID,INST_SYNC_WRITE,len);
  }
  else
    Dxl.setTxPacketLength(0);  
}


