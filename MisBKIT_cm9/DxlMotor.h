#ifndef DLXMOTOR_H
#define DXLMOTOR_H

#pragma once

#include "libpandora_types.h"
#include "Arduino-compatibles.h"

class DxlMotor
{
  public:
  int index;
  int dxlID;
  int model;
  int status;
  int minPos,maxPos;
  int currPos;
  int currMode; // J:Joint W:wheel R:relax else undef
  int cmdMode;
  int cmdGoal;  //synchronised
  int cmdSpeed; //synchronised
  int cmdTorque;
  //int temperature;
  
  DxlMotor();
  DxlMotor(int id);
  void init();
  void setId(int id);
  
  char* parseIMSGT(char* cmd); //"id,mode,goal,speed[,torque]"
  //void cmdIMGST(int* params,int nbParams); //"id,mode,goal,speed[,torque]"
  
  int getModel();

  void update();
  void setModel( int m );
  void chgMode(int m);
  void setJointMode();
  void setWheelMode();
  void setRelaxMode();
  
  void setGoal(int g);
  void setSpeed(int s);
  void setNGoal(float g);
  void setDGoal(float g);
  void setNSpeed(float s);
  
  int readPosition();
  int readTemperature();


  
  
  void setMode(int mode); //RELAX JOINT WHEEL   
  void stop();

  
};

#endif
