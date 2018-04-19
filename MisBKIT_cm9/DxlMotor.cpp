#include "DxlMotor.h"
#include "DxlEngine.h"
#include "utils.h"

extern Dynamixel Dxl;
extern int dxlRead(int id,int addr);
extern void dxlWriteWord(uint8 id,uint8 addr,word val);
extern void dxlWrite2words(int id,int addr,word w0,word w1);


#define CMD_OFF   -1
#define CMD_JOINT  0
#define CMD_WHEEL  1

DxlMotor::DxlMotor(){
  index = 0;
  dxlID = -1;
  model = -1;
  minPos = 0;
  maxPos = 1023; //AX12
  currPos = -1;
  currMode = CMD_OFF;
  cmdMode  = CMD_OFF;
  cmdGoal   = -1;
  cmdSpeed  =  0;
  cmdTorque = -1;
}

void DxlMotor::setId(int id){
  dxlID = id;  
  init();
}

void DxlMotor::init(){ //doesnt change id
  model = -1;
  minPos = 0;
  maxPos = 1023; //default AX12
  currPos = -1;
  currMode = CMD_OFF; // J:Joint W:wheel R:relax else undef
  cmdMode  = CMD_OFF;
  cmdGoal   = -1; //synchronised
  cmdSpeed  = -1; //synchronised
  cmdTorque = -1;
}

void DxlMotor::update(){
  if(currMode!=cmdMode){
    LOGUSB("change mode:",cmdMode);
    chgMode(cmdMode);    
  }
}

// dxlId , mode , goal , speed ,[torque]
//char* parseIMGST(char* str);// dxlId , mode , goal , speed ,[torque]
//void  cmdIMGST(int* params,int nbParams);

//ID Mode Speed Goal Torque
char* DxlMotor::parseIMSGT(char* str){
    int id=dxlID, m=cmdMode, g=cmdGoal, s=cmdSpeed, t=cmdTorque;
    str = parseInt(str,&dxlID);
    if(id!=dxlID)
      init();    
    if(*str>=' ')str=parseInt(str,&cmdMode);
    if(*str>=' ')str=parseInt(str,&cmdSpeed);
    if(*str>=' ')str=parseInt(str,&cmdGoal);
    if(*str>=' ')str=parseInt(str,&cmdTorque);
    /*
    if( (id!=dxlID)||(m!=cmdMode)||(g!=cmdGoal)||(s!=cmdSpeed) ){
      LOGF("Motor%i{i:%i,m:%i,s:%i,g:%i}\n",index,dxlID,cmdMode,cmdSpeed,cmdGoal);
    } 
   */ 
   return str;    
}

void DxlMotor::setModel(int m){
  if( (m>0)&&(m<0xFF00) ){
    model = m;
    if(model==12)
      maxPos = 1023;
    else //MX28 ...
      maxPos = 4095;
    if( currMode == CMD_JOINT ){
      dxlWrite2words(dxlID,P_CW_ANGLE_LIMIT_L,(word)minPos,(word)maxPos);
    }
  }
}

void DxlMotor::chgMode(int m){
  switch(m){
    case CMD_OFF:   setRelaxMode();break;
    case CMD_JOINT: setJointMode();break;
    case CMD_WHEEL: setWheelMode();break;
    default:
      cmdMode = currMode;
      break;    
  }
}

void DxlMotor::setJointMode(){
  LOGUSB("---JOINT:",dxlID);
  if(dxlID>=0){
    dxlWriteWord(dxlID,P_GOAL_SPEED_L,1); //dont move now
    if( currPos>=0 ){
      dxlWriteWord(dxlID,P_GOAL_POSITION_L,currPos);
      cmdGoal = currPos;
    }
    //dxlWriteWord(dxlID,P_CCW_ANGLE_LIMIT_L,(word)maxPos);
    dxlWrite2words(dxlID,P_CW_ANGLE_LIMIT_L,(word)minPos,(word)maxPos);
    currMode = CMD_JOINT;
  }
}
void DxlMotor::setWheelMode(){
  LOGUSB("---WHEEL:",dxlID);
  if(dxlID>=0){
    //dxlWriteWord(dxlID,P_CCW_ANGLE_LIMIT_L,0);
    dxlWrite2words(dxlID,P_CW_ANGLE_LIMIT_L,0,0);
    dxlWriteWord(dxlID,P_GOAL_SPEED_L,0);
    cmdSpeed = 0;
    currMode = CMD_WHEEL;
  }
}

void DxlMotor::setRelaxMode(){
  if(dxlID>=0){
    //AX12: mode wheel //à tester avec Torque sur MX28
    dxlWriteWord(dxlID,P_CCW_ANGLE_LIMIT_L,0);
    dxlWriteWord(dxlID,P_GOAL_SPEED_L,0);
    cmdSpeed = 0;
    currMode = CMD_OFF;
  }
}

void DxlMotor::setGoal(int g){
  if(g>maxPos) g=maxPos;
  else if(g<minPos) g = minPos;
  cmdGoal = g;
}

void DxlMotor::setSpeed(int s){
  if(s>=0){
    if(s>2047) s=2047; //TOTHINK ... may it be a dxl negative speed ?
    cmdSpeed = s;
  }
  else{
    if(s<-1023)s=-1023;
    cmdSpeed = 1024-s;
  }
}

void DxlMotor::setNGoal(float g){
    int center = (maxPos+1)>>1; //!!! 1023 !!!
    setGoal( (int)(g*center)+center );
}

void DxlMotor::setNSpeed(float s){ //[-1,1]
    setSpeed( (int)(s*1023) );
}

void DxlMotor::setDGoal(float g){ //degrees
    //TODO: AX12:300° //MX28:360° ... multitour ...
    int center = (maxPos+1)>>1; //!!! 1023>>1 !!!
    setGoal( (int)(g*center/150.0)+center ); //!!! AX12:300° , MX28:360° 
}

int DxlMotor::readPosition()
{
  int p = -1;
  if(dxlID>0){
    //p = dxlRead(dxlID,P_PRESENT_POSITION_L);
    p = Dxl.readWord(dxlID,P_PRESENT_POSITION_L);
    if(p>=65535)p=-1;
  }
  currPos = p; //may be -1
  return p;
}

void DxlMotor::stop(){
  setRelaxMode();       //TOTHINK
  if(currPos>=0)
    cmdGoal = currPos;
}

int DxlMotor::readTemperature()
{
  if(dxlID>0)
    return Dxl.readByte(dxlID,P_TEMPERATURE);
  return -1;
}


