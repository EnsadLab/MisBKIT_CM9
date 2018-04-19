#if 0
#include "libpandora_types.h"
#include "Arduino-compatibles.h"
#include "Dynamixel.h"
#include "dxl_constants.h"

#include "DxlEngine.h"
#include "DxlMotor.h"
#include "XSerial.h"
#include "esp8266.h"
#include "utils.h"
#include "osc.h"

extern Dynamixel Dxl;

//#define NB_ENGINES 6
DxlEngine engines[NB_ENGINES];
//DxlMotor  motors[NB_ENGINES];

//extern EEPROM CM9_EEPROM;
extern XSerial* pSerial;
extern const char* cm9Name;

extern void mbkExecCmd(char* pcmd,float* pParams,int nbParams);
extern int dxlWrite(int imot,int ireg,int val);
extern int dxlWrite4bytes(int imot,int addr,int8 b0,int8 b1,int8 b2,int8 b3);
extern int dxlRead(int imot,int ireg);

#define RELAX_MODE 0
#define JOINT_MODE 1
#define WHEEL_MODE 2

#define TASK_JOINT 1
#define TASK_WHEEL 2


extern byte dxlAdrr[];


// ------- statics --------
char dxlTxtBuffer[256];
//char dxlStrBuffer[256];

//sync : id,pos,speed
int  syncStatus = 0;
//word syncGoalSpeed[]={1,0,0, 2,0,0, 3,0,0, 4,0,0, 5,0,0, 6,0,0 };

unsigned long DxlEngine::updateTime = 0;
int DxlEngine::currentTask = 0;
//int DxlEngine::nbParams = 0;
float DxlEngine::params[32];
//int DxlEngine::cmdParams[32];
int DxlEngine::scanBauds = 0;
int DxlEngine::scanId = 0;
int DxlEngine::scanIndex = 0;

int  tmpCount = 0;
int updateCount = 0;
extern int ipdOverflow;

void DxlEngine::initialize(){ //TODO baud rate , nb engines
  Dxl.begin(3);
  //0: 9615  bauds
  //1; 57143 bauds
  //2: 115200 (117647)
  //3: 1000000
  delay(1000);
  for(int i=0;i<NB_ENGINES;i++)
  {
    engines[i].index = i;
    engines[i].init();
    delay(50);
  }
  updateTime = millis();
}




int scanDelay = 0;

/*
void DxlEngine::update(){
  //listenSerial();
  
  unsigned long t = millis();
  if( (t-updateTime)>50 ){
    updateTime = t;
    DxlEngine::task(t);
    //DxlEngine::sendAllPos();
    syncGoals();
    syncSpeeds();
  }
  
}
*/

/*
void DxlEngine::listenSerial(){
  char* prcv = pSerial->readStr();
  while(prcv != NULL){
    parseCmd(prcv);
    prcv = pSerial->readStr();
  } 
  //syncGoals();
  //syncSpeeds();
}
*/

boolean DxlEngine::task(unsigned long t){
  updateCount++;
  switch(currentTask){
    case 0:
       for(int i=0;i<NB_ENGINES;i++)
         engines[i].update(t);
      break;
    case 1: //slow scan
      LOGUSB("scan:",scanId);
      scanDelay = 0; //(++scanDelay)&1;
      if( scanDelay==0){
        if(++scanId>=BROADCAST_ID){ //BROADCAST_ID) //(!!! id>128 reserved for future use)
            currentTask=0;
            scanId = 0;
            scanIndex = 0;
            //Dxl.writeByte(0xFE,P_LED,0);
            sendAllIds();
        }
        else{
          LOGUSB("scan:",scanId);
          xSerialESP.sendf("ping %i\n",scanId);
          int p = Dxl.ping(scanId);      
          int m = (int)Dxl.readWord(scanId,P_MODEL);
          if(p<0xFF)
            //serialSend("pong",scanIndex,scanId,m); //,dxlTxtBuffer);
            xSerialESP.sendf("pong %i %i %i\n",scanIndex,scanId,m);

          if(m!=0xFFFF){
            //engines[scanIndex].setId(scanId);
            //engines[scanIndex].model = m;
            //serialSend("cm9Id",scanIndex,scanId,m); //,dxlTxtBuffer);
            xSerialESP.sendf("cm9Id %i %i %i\n",scanIndex,scanId,m);
            
            if(++scanIndex>=6) //6 motors
              scanId=256;//>> scan end
          }
        }
      }
      break;
      case 3:
      
      break;
  }
  return true;
}

//TODO >>> A_MisBKit
#ifdef USE_OSC
boolean DxlEngine::parseOsc(char* pBuf){
   LOGUSB("parseOsc:",pBuf);
  // "/mbk/cmd/0" ",fifi" params
  if(!strBegin(pBuf,"/mbk/"))
    return false;
  
  int parseStep = 0;
  char* pcmd = pBuf+5;
  char* pnum = pcmd;
  oscStart(pBuf,2048);
  while( (*pnum!='/')&&(*pnum!=0) ) //skip cmd;
    pnum++;
    
  while( *pnum == '/'){ //params in address
    *pnum = 0;
    pnum++;
    params[parseStep++]=atof(pnum);
    while( (*pnum!='/')&&(*pnum!=0) ) //skip num;
      pnum++;
  }    
  
  char c=oscNextType();
  while(c!=0){
    switch(c){
      case 'f':params[parseStep++]=oscGetFloat(); break;
      case 'i':params[parseStep++]=(float)oscGetInt(); break;
    }
    c=oscNextType();
  }  
  oscEnd();    
  
  /*
  SerialUSB.print("mbk:");
  SerialUSB.print(pcmd);
  SerialUSB.print(" ");SerialUSB.print((int)params[0]);
  SerialUSB.print(",");SerialUSB.println(params[1]);
  */
  
  //execCmd(pcmd,params,parseStep);
   mbkExecCmd(pcmd,params,parseStep); 
  return true; 
}
#endif


void DxlEngine::parseCmd(char* pBuf){  
  //SerialUSB.print(pBuf);
  
  int len = strlen(pBuf);
  char* pCmd;
  char* p1 = pBuf;
  
  while(*p1>0){
    pCmd = p1;  
    while(*p1>' '){p1++;} //skip cmd
    if(*p1!=0){*p1=0;p1++;}
    
    int nbParams = 0;
    while(*p1>=' '){ // until 0 cr lf
      while( (*p1==' ')||(*p1==',') ){p1++;} //skip spaces
      if( (*p1>='-')&&(*p1<='9') ){ //is num - . / 0 1 2 3 4 5 6 7 8 9
        params[nbParams++] = (float)atoi(p1);          
      }
      //while((*p1>='-')&&(*p1<='9')){p1++;} //skip num - . / 0 1 2 3 4 5 6 7 8 9
      while( (*p1>' ')&&(*p1!=',') ){p1++;} //skip param
    }
    while( (*p1>0)&&(*p1<=' ') ){*p1=0;p1++;} //skip cr lf space
    
    DxlEngine::execCmd(pCmd,params,nbParams);
    //LOGUSB("parsed:",pCmd);
    delay(0);
  }//next cmd
  delay(0);
}

void DxlEngine::execCmd(char* pcmd,float* pParams,int nbParams){
  int param0 = (int)pParams[0];
  switch( *pcmd ){ //first char
  
    case 'E': //cmd index params
      if( (param0>=0)&&(param0<NB_ENGINES) )
        engines[param0].onCmd(pcmd,pParams+1,nbParams-1);
        //LOGUSB("execCmd:EI:", (int)pParams[1]);     
    break;
    
    case 'D'://cmd dxlId params
      {
        int param1 = (int)pParams[1];
        if( (pcmd[1]=='W') && (nbParams==3)){
          dxlWrite(param0,param1,(int)pParams[2]);
          LOGUSB("DxlW:",(int)pParams[2]);
        }
        else if((pcmd[1]=='R') && (nbParams==2)){
          int val = dxlRead(param0,param1);
          if(val>-1)
            //serialSend("dxlReg",pParams[0],pParams[1],val);
            xSerialESP.sendf("dxlReg %i %i %i\n",param0,param1,val);
            //SerialUSB.print("read: ");SerialUSB.print(param1);SerialUSB.print(" ");SerialUSB.println(val);
        }
      }
      break;
      
    case 'G':
      if( (param0>=0)&&(param0<NB_ENGINES) ){
         if(pcmd[1]=='N')
           engines[param0].setNGoal(pParams[1]);
         else
           engines[param0].cmdGoal=(int)pParams[1];
         //SerialUSB.print("Ngoal:");SerialUSB.println(engines[param0].cmdGoal);
      }
      break;
      
    case 'S':
      if( (param0>=0)&&(param0<NB_ENGINES) ){
         if(pcmd[1]=='N')
           engines[param0].setNSpeed(pParams[1]);
         else
           engines[param0].cmdSpeed=(int)pParams[1];
      }
      break;
      
    case 'N':
      if( (param0>=0)&&(param0<NB_ENGINES) )
         if(pcmd[1]=='G')
           engines[param0].setNGoal(pParams[1]);
         else if(pcmd[1]=='S')
           engines[param0].setNSpeed(pParams[1]);
      break;
      
    case 'g':
      if(strBegin(pcmd,"goals")){
        //if(nbParams!=6) pSerial->print(pcmd);
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
      
    case 'j':
      if(strBegin(pcmd,"joint")){
        if( (nbParams>0)&&(param0>=0)&&(param0<NB_ENGINES) )        
          engines[param0].setJointMode();
      }
      break;
    case 'w':
      if(strBegin(pcmd,"wheel")){
        if( (nbParams>0)&&(param0>=0)&&(param0<NB_ENGINES) )        
          engines[param0].setWheelMode();
      }
      break;
      
    case 'r':
      if(strBegin(pcmd,"relax")){
        if( (nbParams>0)&&(param0>=0)&&(param0<NB_ENGINES) )        
          engines[param0].setRelaxMode();
      }
      break;
      
    case 'm':
      if(strBegin(pcmd,"model")){
        if( (nbParams>0)&&(param0>=0)&&(param0<NB_ENGINES) ){
           int m = engines[param0].getModel();
           //serialSend("model",pParams[0],m);
          pSerial->sendf("model %i %i\n",param0,m);           
        }
      }
      if(strBegin(pcmd,"motorIDs")){
        int n = nbParams;if(n>NB_ENGINES)n=NB_ENGINES;
        for(int i=0;i<n;i++){
          engines[i].dxlId = (int)pParams[i];
          LOGUSB("motorIDs:",(int)pParams[i]);
        }
      }
      break;
    case 'M': //"Motor index,dxlID,mode
      if(strBegin(pcmd,"Motors")){
        int im = (int)pParams[0];
        if( (im<NB_ENGINES) && (nbParams>1) ){
          engines[im].dxlId   = (int)pParams[1];
          if(nbParams>2)
            engines[im].cmdMode = (int)pParams[2];
        }
      }    
      break;   
    case 'v':
      if(strBegin(pcmd,"version")){
        LOGUSB("VERSION:","3.7");
        pSerial->sendf("version %i %i %s\n",3,7,cm9Name);
      }
      break;        
    case 'n':
      if(strBegin(pcmd,"name")){
        LOGUSB("NAME:",cm9Name);
        pSerial->sendf("name %s\n",cm9Name);
      }
    default:
      break;
   }

}

void DxlEngine::startScan(){
  LOGUSB("startScan:",scanIndex);
    //Dxl.writeByte(0xFE,P_LED,1);
    for(int i=0;i<NB_ENGINES;i++){
      engines[i].dxlId=-1;
      engines[i].model=-1;
    }
    scanIndex = 0;
    scanId = 0;
    currentTask=1;
  //SerialUSB.print("STARTSCAN:");SerialUSB.print(updateCount);SerialUSB.print(" ovf:");SerialUSB.print(ipdOverflow);
}
void DxlEngine::stopScan(){
    scanIndex = 0;
    scanId = 0;
    currentTask=0;
}

void DxlEngine::stopAll(){
  stopScan();
  for(int i=0;i<NB_ENGINES;i++)
    engines[i].stop();
}

void DxlEngine::syncSpeeds(){
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
      //LOGUSB("sync",s);
     len+=3; 
    }
  }
  if(len>2)
    Dxl.txPacket(BROADCAST_ID,INST_SYNC_WRITE,len);
  else
    Dxl.setTxPacketLength(0);
}

int DxlEngine::pushGoal(){
  if((dxlId<=0)||(cmdGoal<0))
    return 0;
    
  Dxl.pushByte(dxlId);
  Dxl.pushByte(cmdGoal & 0xFF);
  Dxl.pushByte(cmdGoal >> 8);
  cmdGoal = -1;
  return 3;
}


int DxlEngine::pushGoalSpeed(){
  if(dxlId<=0)
    return 0;
    
  Dxl.pushByte(dxlId);
  Dxl.pushByte(cmdGoal & 0xFF);
  Dxl.pushByte(cmdGoal >> 8);
  Dxl.pushByte(cmdSpeed & 0xFF);
  Dxl.pushByte(cmdSpeed >> 8);       
  return 5;
}


void DxlEngine::syncGoalSpeeds(){
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(P_GOAL_POSITION_L);
  Dxl.pushByte(4); //4 words
  int len = 2;
  for(int i=0;i<NB_ENGINES;i++){
    int g = engines[i].cmdGoal;   
    if( (g>=0)&&(engines[i].dxlId > 0) ){
      engines[i].cmdGoal = -1;
      Dxl.pushByte(engines[i].dxlId);
      Dxl.pushByte(g & 0xFF);
      Dxl.pushByte(g >> 8);
      int s=engines[i].cmdSpeed;
      Dxl.pushByte(s & 0xFF);
      Dxl.pushByte(s >> 8);      
      len+=5; 
    }
  }
  if(len>2)
    Dxl.txPacket(BROADCAST_ID,INST_SYNC_WRITE,len);
  else
    Dxl.setTxPacketLength(0);
}


// ------------------------
/*
int DxlEngine::getIdFromFlash()
{
  return CM9_EEPROM.read(index);// read data from virtual address 0~9  
}
*/

//int tmpCount = 0;
DxlEngine::DxlEngine()
{
  index = 0;
  dxlId = -1;
  model = -1;
  minPos = 0;
  maxPos = 1023; //AX12
  currPos = -1;
  //script = null;
  cmdMode  = -1;  //0:joint 1:wheel
  cmdGoal  = -1;
  cmdSpeed = -1;
}

int DxlEngine::getModel()
{
  model = -1;  
  if(dxlId>0)
  {
    int m = (int)Dxl.readWord(dxlId,0);
    if(m!=0xFFFF){
      model = m;
      if(m == 12){
        dxlWrite4bytes(dxlId,P_CW_COMPLIANCE_GAP,0,0,128,128);
      }
      else{ //MX28 ...
        dxlWrite4bytes(dxlId,P_CW_COMPLIANCE_GAP,0,0,4,0);
      } 
      //oscSend(&xSerialESP,"/mbk/info",",sii","Model:",dxlId,m);
      LOGUSB("model:",m);
    }  
    else{
      model = 12;//!!! par default , certains AX12 ne repondent pas   
      //oscSend(&xSerialESP,"/mbk/err",",sii","Model?",dxlId); 
      LOGUSB("model:","?");
    }
  }
  
  minPos = 0;
  if(model==12)maxPos=1023;
  else maxPos=4095;
   
  //model = 29; //!!!FORCE_MX28
  //model = 12; //!!!FORCE_AX12
  //debug();
  return model;
}

void DxlEngine::setId(int id)
{
  dxlId = id;  
  init();
  //LOGUSB("setId:",dxlId);
}



void DxlEngine::init()
{
  model    = -1;
  taskId   = 0;
  taskStep = 0;
  status = RELAX_MODE;
  minPos = 0;
  maxPos = 1023; //4095;  //MX28 64 106 ...
  torqueLimit = 1000;
  //cmdModel = -1;
  currentMode = -1;
  cmdMode  = -1;
  cmdCW    = -1;
  cmdCCW   = -1;
  cmdGoal  = -1;
  cmdSpeed = -1;
  cmdTorque= -1;

  if( dxlId<=0 )
    return;
/*  
  int m = getModel();
  minPos = 0;
  if(m==12)  //AX12
    maxPos = 1023;
  else
    maxPos = 4095;
*/  
  //anim.init(this);
  
  /*
  Dxl.writeByte(dxlId,P_ALARM_SHUTDOWN,4);
  Dxl.writeByte(dxlId,P_ALARM_LED,4);
  Dxl.writeWord(dxlId,P_CW_ANGLE_LIMIT_L,0);
  Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,maxPos);
  Dxl.writeByte(dxlId,P_CW_COMPLIANCE_SLOPE,128);
  Dxl.writeByte(dxlId,P_CCW_COMPLIANCE_SLOPE,128);
  Dxl.writeWord(dxlId,P_GOAL_POSITION_L,512);
  Dxl.writeWord(dxlId,P_GOAL_SPEED_L,200);
  Dxl.writeWord(dxlId,P_TORQUE_LIMIT_L,1000);
  jointMode = true;
  */
  
  // V2
  /* V3
  Dxl.writeByte(dxlId,P_DELAY,100);
  Dxl.writeByte(dxlId,P_RETURN_LEVEL,1); 
  //Dxl.writeWord(dxlId,P_TORQUE_MAX,1023);
  Dxl.writeByte(dxlId,P_ALARM_SHUTDOWN,4);
  Dxl.writeByte(dxlId,P_ALARM_LED,4);
  Dxl.writeWord(dxlId,P_CW_ANGLE_LIMIT_L,0);
  Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,0); //mode speed = relax (AX12)

  Dxl.writeByte(dxlId,P_CW_COMPLIANCE_SLOPE,128); //AX12
  Dxl.writeByte(dxlId,P_CCW_COMPLIANCE_SLOPE,128); //AX12
  
  //Dxl.writeByte(dxlId,P_MX_D,0);
  //Dxl.writeByte(dxlId,P_MX_I,0);
  //Dxl.writeByte(dxlId,P_MX_P,8);
  
  //Dxl.writeWord(dxlId,P_GOAL_POSITION_L,512);
  Dxl.writeWord(dxlId,P_GOAL_SPEED_L,0);
  Dxl.writeWord(dxlId,P_TORQUE_LIMIT_L,1020); //FORCE_TORQUE
  //jointMode = false;  
  */
  //pSerial->sendf("model %i %i\n",index,model);

  //relax(false);  
}

void DxlEngine::stop()
{
  if(dxlId>0){
    Dxl.writeWord(dxlId,P_TORQUE_LIMIT_L,0);
    //Dxl.writeWord(dxlId,P_CW_ANGLE_LIMIT_L,0);
    //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,0);
    Dxl.writeWord(dxlId,P_GOAL_SPEED_L,0);
  }
}

void DxlEngine::donothing()
{
}

// true = finished / false runningTask
bool DxlEngine::update(unsigned int t)
{
  if(dxlId <= 0)
    return true; //finished  
  switch(taskId){
    case 0:
      currPos = Dxl.readWord(dxlId,P_PRESENT_POSITION_L);
      if(currPos>=0xFFFF)currPos = -1; //send -1, so we are awake
    break;
    case 1:
      
    break;
  }
  //TOTHINK: relaxMode
  //in case we miss mode 
  if( (cmdGoal>=0)&&(currentMode!=JOINT_MODE)){
    cmdMode = JOINT_MODE;
  }
  else if( (cmdSpeed !=-1)&&(cmdSpeed !=0)&&(currentMode!=WHEEL_MODE)){
    cmdMode = WHEEL_MODE;    
  }
  
  if(cmdMode == JOINT_MODE){ //TODO try sync
    dxlWriteWord(dxlId,P_CCW_ANGLE_LIMIT_L,maxPos); //(GOAL_RANGE-1));
    dxlWriteWord(dxlId,P_GOAL_SPEED_L,0); //(GOAL_RANGE-1));
    LOGUSB("cmd JOINT_MODE:",cmdCCW);
    cmdCW  = -1;
    cmdCCW = -1;
    cmdMode = -1;
    currentMode =JOINT_MODE;
  }
  else if(cmdMode == WHEEL_MODE){ //TODO try sync
    dxlWriteWord(dxlId,P_CCW_ANGLE_LIMIT_L,0);
    dxlWriteWord(dxlId,P_GOAL_SPEED_L,0);
    LOGUSB("cmd WHEEL_MODE",cmdCCW);
    cmdCW  = -1;
    cmdCCW = -1;
    cmdMode = -1;
    currentMode = WHEEL_MODE;
  }
  
  if(cmdCW>=0){
    dxlWriteWord(dxlId,P_CW_ANGLE_LIMIT_L,cmdCW);
    cmdCW = -1;
  }
  if(cmdCCW>=0){
    dxlWriteWord(dxlId,P_CCW_ANGLE_LIMIT_L,cmdCCW);
    cmdCCW = -1;
  }
  if(cmdTorque>=0){
    dxlWriteWord(dxlId,P_TORQUE_LIMIT_L,cmdTorque);
    cmdTorque = -1;
  }     
  return true;
}



void  DxlEngine::onCmd(const char* cmd,float* pParam,int nbp )
{
  int param0 = (int)pParam[0];
  if( cmd[1]=='I')
  {
    if(nbp==1)
      setId(param0);
      //LOGUSB("onCmd:EI:",dxlId);
    return;
  }

  if(dxlId<=0)
  {
    //pSerial->sendf("!???: %i\n",index);
    return;
  }
    
  if( (cmd[1]=='W')&&(nbp==2) )
  {
    if(param0>=P_CW_ANGLE_LIMIT_L) //PROTECTED
      setDxlValue(param0,pParam[1]);
    return;
  }
  
  if( cmd[1]=='M')
  {
    int m = model; //getModel();
    //serialSend("model",index,m);
    pSerial->sendf("model %i %i\n",index,m); //TODO OSC
    return;
  }
  if( cmd[1]=='R') //ER
  {
    if(dxlAdrr[param0]>0){    
      int val = dxlRead(dxlId,param0);
      //serialSend("dxlReg",index,pParam[0],val);
      pSerial->sendf("dxlReg %i %i %i\n",index,param0,val);
    }   
    return;
  }

  if( (cmd[1]=='X')&&(nbp==2) )
  {
    //PROTECTED
    setDxlValue(param0,pParam[1]);
    return;
  }
  
  
  //....  
}

void DxlEngine::setDxlValue(int addr,int val)
{  
  if(dxlId<=0)
  {
    //serialSend("!Bad Motor ID:",index);
    pSerial->sendf("!Bad Motor ID: %i\n",index);
    return;
  }
  //serialSend("!write",dxlId,addr,val,myBuffer);

  switch(addr)
  {
    case P_ID:
        //serialSend("!ChangeID",index,val);
        //SerialUSB->sendf("!ChangeID %i %i\n",index,val);
        changeId(val);
    break;
    
    case P_CW_ANGLE_LIMIT_L:
      //Dxl.writeWord(dxlId,P_CW_ANGLE_LIMIT_L,val);
      //dxlWriteWord(dxlId,P_CW_ANGLE_LIMIT_L,val);
      cmdCW = val;
      break;

    case P_CCW_ANGLE_LIMIT_L:  
      //if(val<=0) setWheelMode();  //!!! assume cw limit == 0 !!!
      //else {setJointMode();} 
      //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,val);
      // dxlWriteWord(dxlId,P_CCW_ANGLE_LIMIT_L,val);
      cmdCCW = val;
      break;

    case P_CW_COMPLIANCE_SLOPE:
        Dxl.writeByte(dxlId,P_CW_COMPLIANCE_SLOPE,val);
        break;
    
    case P_CCW_COMPLIANCE_SLOPE:
        Dxl.writeByte(dxlId,P_CCW_COMPLIANCE_SLOPE,val);
        break;

    case P_GOAL_POSITION_L:
        //Dxl.writeWord(dxlId,P_GOAL_POSITION_L,val);
        cmdGoal = val;
        break;

    case P_GOAL_SPEED_L:
        //if(val<0)val=1024-val;
        //Dxl.writeWord(dxlId,P_GOAL_SPEED_L,val);
        if(val!=-1){ //-1 = disable ??? anyway speed -1 ne bouge pas !
          if(val<0)val=1024-val;
          cmdSpeed = val;
          //LOGUSB("cmdSpeed:",cmdSpeed);
        }
        break;
        
    case P_TORQUE_LIMIT_L :
        //setTorque(val);
        //Dxl.writeWord(dxlId,P_TORQUE_LIMIT_L,val);
        cmdTorque = val;
        break;
    
    default:
      //serialSend("!default",dxlId,addr,val,myBuffer);
      dxlWrite(dxlId,addr,val);
        
        
  }
}

void DxlEngine::setSpeed(int s)
{
  if(s>=0)
  {
    if(s>2047) s=2047;     //TODO
    Dxl.writeWord(dxlId,32,s);
  }
  else
  {
    if(s<-1023)s=-1023;
    Dxl.writeWord(dxlId,32,1024-s);
  }
  //serialSend(" %idxl speed ",dxlId,s,myBuffer);
  //lastSpeed = s;
}

void DxlEngine::setNSpeed(float s)
{
    int is = (int)(s*1024);
    if( is > 1023 )
      is = 1023;
    else if(is < 0){      
      if( is <-1023 )
        is = -1023;
      is = 1024-is;
    }    
    cmdSpeed = is;
}

/*
void DxlEngine::setGoalSpeed(int g,int s){
  setGoal(g);
  setSpeed(s);
}
void DxlEngine::setGoal(int g)
{
    Dxl.writeWord(dxlId,P_GOAL_POSITION_L,g);
}
*/

void DxlEngine::setNGoal(float g)
{
    if(model<=0)
      getModel();
      
    if(model==12){ //AX12
      cmdGoal = (int)(g*512)+512;
      if(cmdGoal<0)cmdGoal=0;
      else if(cmdGoal>1023)cmdGoal=1023;
    }
    else{ //   if(model==  //MX28 MX64 MX106 ...
      cmdGoal = (int)(g*2048)+2048;
      if(cmdGoal<0)cmdGoal=0;
      else if(cmdGoal>4095)cmdGoal=4095;
    }    
}

void DxlEngine::setDGoal(float g) //degrees
{
    if(model<=0)
      getModel(); //

    if(model==12){ //AX12
      cmdGoal = (int)(g*512/150)+512; //300°
      if(cmdGoal<0)cmdGoal=0;
      else if(cmdGoal>1023)cmdGoal=1023;
    }
    else
    { //   if(model==  //MX28 MX64 MX106 ... (!no multitour) 
      cmdGoal = (int)(g*2048/160)+2048; //360°
      if(cmdGoal<0)cmdGoal=0;
      else if(cmdGoal>4095)cmdGoal=4095;
    }    
}

/*
int DxlEngine::getGoal()
{
  return Dxl.readWord(dxlId,P_GOAL_POSITION_L);
}
*/
int DxlEngine::getPos()
{
  return Dxl.readWord(dxlId,P_PRESENT_POSITION_L);
}
int DxlEngine::getTemperature()
{
  return Dxl.readByte(dxlId,P_TEMPERATURE);
}


int DxlEngine::getSpeed()
{
  int s = Dxl.readWord(dxlId,32);
  if(s>=1024)
    s= 1024-s;
  return s;
}
int DxlEngine::getCurrSpeed()
{
  int s = Dxl.readWord(dxlId,32);
  if(s>=1024)
    s= 1024-s;
  return s;
}

/*
void DxlEngine::relax(bool dorelax)
{
   //serialSend(" %relax%",dxlId);  
  //maxLoad = 0;
  if(dorelax)
  {
    Dxl.writeWord(dxlId,P_TORQUE_LIMIT_L,0);
    status = RELAXED;
  }
  else
  {
    int p = Dxl.readWord(dxlId,P_PRESENT_POSITION_L);  //currPos
    Dxl.writeWord(dxlId,P_GOAL_POSITION_L,p);          //goal = currPos
    Dxl.writeWord(dxlId,P_GOAL_SPEED_L,0);             //speed 0 !!!!!! max ?    
    Dxl.writeWord(dxlId,P_TORQUE_LIMIT_L,torqueLimit);              //
    status &= ~RELAXED;
  }
}
*/

void DxlEngine::setTorque(int tl)
{
  if(tl<=0)
    relax(true);
  else
  {
    torqueLimit = tl;
    relax(false);
  }  
}


void DxlEngine::setMode(int mode){
  switch(mode){
    case JOINT_MODE: setJointMode();break;
    case WHEEL_MODE: setWheelMode();break;
    case RELAX_MODE:
      //TODO REDO ...
      break;
    default:
      //setRelaxMode();
      break;    
  }
}

void DxlEngine::setRelaxMode()
{
  if(dxlId>=0){
    dxlWrite(dxlId,P_TORQUE_ENABLE,1);
    dxlWrite(dxlId,P_TORQUE_LIMIT_L,0);
    dxlWrite(dxlId,P_CW_ANGLE_LIMIT_L,0);
    dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,0);
    dxlWrite(dxlId,P_GOAL_SPEED_L,0);
  }
}

void DxlEngine::setWheelMode()
{
  if(dxlId>=0){
    //dxlWrite(dxlId,P_TORQUE_ENABLE,1);
    //dxlWrite(dxlId,P_GOAL_SPEED_L,1);      // vitesse "nulle" meme en mode joint
    //dxlWrite(dxlId,P_TORQUE_LIMIT_L,0);
    //dxlWrite(dxlId,P_CW_ANGLE_LIMIT_L,0);
    //dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,0);
    //delay(1);
    //LOGUSB("WHEEL1:",maxPos);
    //dxlWriteWord(dxlId,P_CCW_ANGLE_LIMIT_L,0);

    //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,0);
    //delay(10);
    //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,0);
    //delay(10);
    
    //dxlWriteWord(dxlId,P_TORQUE_LIMIT_L,1023);
    //dxlWrite(dxlId,P_GOAL_SPEED_L,0);

    //delay(1); 
    //dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,0); //GRRR marche pas à tous les coups
    //getModel();
    cmdMode = WHEEL_MODE;
    cmdCCW   = 0;
    cmdSpeed = 0;    
    cmdTorque = 1020;
  }
}

void DxlEngine::setJointMode()
{
  //LOGUSB("setJointMode:",dxlId);
  if(dxlId>=0){
     if(model<=0)
       getModel();
    //Dxl.writeWord(dxlId,P_GOAL_SPEED_L,0);
    //delay(10);
    //dxlWrite(dxlId,P_TORQUE_ENABLE,1);  // ??? alow write
    //dxlWrite(dxlId,P_GOAL_SPEED_L,1);   // vitesse "nulle" meme en mode joint
    //dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,4095); //(GOAL_RANGE-1));
    //int m = getModel(); //TODO AX12 MX28 ... ...
    //int p = dxlRead(dxlId,P_PRESENT_POSITION_L);
    //dxlWrite(dxlId,P_TORQUE_LIMIT_L,0);         //relax
    //delay(1);
    /*
    delay(10);
    dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,4095); //(GOAL_RANGE-1));
    delay(10);
    dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,4095); //(GOAL_RANGE-1));
    delay(10);
    */
      //LOGUSB("JOINT1:",maxPos);
      if(currPos>=0)
        cmdGoal = currPos;
      cmdMode = JOINT_MODE;
      cmdSpeed = 0;
      cmdCCW  = maxPos;
      cmdTorque = 1020;
//        dxlWriteWord(dxlId,P_GOAL_POSITION_L,currPos);      
//      dxlWriteWord(dxlId,P_CCW_ANGLE_LIMIT_L,(word)maxPos);
//      dxlWriteWord(dxlId,P_TORQUE_LIMIT_L,1020);
      //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,1023); //(GOAL_RANGE-1));
      //delay(10);
      //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,1023); //(GOAL_RANGE-1));
      //delay(10);
    //cmdMode = JOINT_MODE;
    //Dxl.writeWord(dxlId,P_CCW_ANGLE_LIMIT_L,4090);
    //dxlWrite(dxlId,P_GOAL_SPEED_L,0); //vmax
    //delay(10);
    //if(p>=0)
    //  dxlWrite(dxlId,P_GOAL_POSITION_L,p);    
    //dxlWrite(dxlId,P_TORQUE_LIMIT_L,1023);
    //SerialUSB.print("pos:");SerialUSB.println(p);//DBG

    //delay(10); //GRRRR marche pas à tous les coups
    //dxlWrite(dxlId,P_CCW_ANGLE_LIMIT_L,4095); //(GOAL_RANGE-1));
    //delay(1);
    //SerialUSB.println("JOINT");
    //getModel();  

  }
}

void DxlEngine::setCompliance(int c0,int c1,int c2,int c3)
{
  if(dxlId>0)
    dxlWrite4bytes(dxlId,P_CW_COMPLIANCE_GAP,c0,c1,c2,c3);
}

void DxlEngine::changeId(int newId){
  if( (dxlId>=0)&&(newId>=0)&&(newId<240) ){
    Dxl.writeByte(dxlId,P_ID,newId);
    dxlId = newId;    
  }
}

void DxlEngine::debug(){
  if(dxlId<=0)
    return;
    
  int p = Dxl.ping(dxlId);          
      //serialSend("pong",scanIndex,scanId,m); //,dxlTxtBuffer);
      //pSerial->sendf("pong %i %i %i\n",scanIndex,scanId,m);
  SerialUSB.print("ping     ");SerialUSB.println(p);

  int m = dxlRead( dxlId,P_TORQUE_ENABLE);
  SerialUSB.print("model    ");SerialUSB.println(m);
  
  int f = dxlRead( dxlId,P_MODEL);
  SerialUSB.print("firmware ");SerialUSB.println(f);
  
  
}


// =========================================
void DxlEngine::sendAllPos(){ //!!! not safe 
  char* pstr = str2str(dxlTxtBuffer,"dxlpos");
  for(int i=0;i<NB_ENGINES;i++){
    int p=(engines[i].currPos);
      pstr = int2str(pstr,',',p);
  }
  pstr[0]=13;
  pstr[1]=10;
  pstr[2]=0;
  //serialSend(dxlTxtBuffer);
  xSerialESP.print(dxlTxtBuffer);
}

void DxlEngine::sendAllIds(){ //!!! not safe 
  char* pstr = str2str(dxlTxtBuffer,"motorIds");
  for(int i=0;i<NB_ENGINES;i++){
      pstr = int2str(pstr,' ',engines[i].dxlId);
  }
  pstr[0]=10;
  pstr[1]=0;
  //serialSend(dxlTxtBuffer);
  pSerial->print(dxlTxtBuffer);
}
#endif



