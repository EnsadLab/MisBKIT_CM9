#ifndef DXLENGINE_H
#define DXLENGINE_H

#define NB_ENGINES 6

#define P_MODEL    0
#define P_FIRMWARE 2
#define P_ID  3
#define P_BAUDS 4
#define P_DELAY 5
#define P_CW_ANGLE_LIMIT_L  6
#define P_CCW_ANGLE_LIMIT_L 8  
#define P_TORQUE_MAX        14
#define P_RETURN_LEVEL      16
#define P_ALARM_LED           17
#define P_ALARM_SHUTDOWN      18
#define P_TORQUE_ENABLE       24
#define P_LED                 25
//MX
#define P_MX_D 26
#define P_MX_I 27
#define P_MX_P 28
//AX
#define P_CW_COMPLIANCE_GAP     26
#define P_CCW_COMPLIANCE_GAP    27
#define P_CW_COMPLIANCE_SLOPE   28
#define P_CCW_COMPLIANCE_SLOPE  29

#define P_GOAL_POSITION_L       30
#define P_GOAL_SPEED_L          32
#define P_TORQUE_LIMIT_L        34
#define P_PRESENT_POSITION_L    36
#define P_VOLTAGE     42
#define P_TEMPERATURE 43

/*
#define RELAX 0
#define JOINT 1
#define WHEEL 2
//#define MULTITOUR 3
*/



#include "libpandora_types.h"
#include "Arduino-compatibles.h"
//#include Script.h

extern void dxlWriteWord(uint8 id,uint8 addr,word val);


class DxlEngine
{
  public:
  int index;
  int model;
  int dxlId;
  int taskId;
  int taskStep;
  int status;
  int minPos,maxPos;
  int currPos;
  int torqueLimit;
  int currentMode;
  int cmdMode;
  int cmdCW;
  int cmdCCW;
  int cmdGoal;  //synchronised
  int cmdSpeed; //synchronised
  int cmdTorque;
    
  //Script anim;
  
  DxlEngine();
  DxlEngine(int id);
  void init();
  void setId(int id);
  int getModel();
  void setMode(int mode); //RELAX JOINT WHEEL   
  void stop();
  
  void setGoal(int goal);   // AX12: [0,1023]
  void setDGoal(float goal); //goal en degrés
  void setNGoal(float goal); //goal normalisé(-1,1)
  void setSpeed(int speed); // [-1023,1023]
  void setNSpeed(float goal); //speed normalisée(-1,1)
  void setGoalSpeed(int goal,int speed);
  
  void  onCmd(const char* cmd,float* pParams,int nbp );

  //void startScript(const char* name,const char* label=NULL);
  void donothing();
  //true=JOINT , false=WHEEL
  bool checkMode(); //fast: check only CCW
  bool update(unsigned int t);
  void execTokenDbg(int tok,int value);
  void execToken(int tok,int value);
  bool waitGoal();
  bool taskJoint(unsigned int dt);  //wait goal or bezier
  bool taskWheel(unsigned int dt);  //lineaire

  //void parse(char* pScript = NULL); //TOREMOVE
  void execCmd(int* pIntCmd);
  void setWheelSpeed(int s);
  int getGoal();
  int getPos();
  int getTemperature();
  int getSpeed();
  int getCurrSpeed();
  void relax(bool dorelax);
  void setTorque(int tl);
  void setWheelMode();
  void setJointMode();  
  void setRelaxMode();  
  void setCompliance(int c0,int c1,int c2,int c3);
  void setDxlValue(int addr,int val);
  void changeId(int newId);
  int  pushGoal();
  int  pushSpeed();  
  int  pushGoalSpeed();
  
  void debug();

static void initialize();
static void update();
static boolean task(unsigned long t);
static void listenSerial();
static void parseCmd(char* pcmd);
static boolean parseOsc(char* pBuf);

static void execCmd(char* pcmd,float* pParams,int nbParams);

static void sendAllPos();
static void sendAllIds();
static void syncGoals();
static void syncSpeeds();
static void syncGoalSpeeds(); 
static void stopAll();
static void startScan();
static void stopScan();

static unsigned long updateTime;
static int currentTask;
//static int nbParams;
static float params[32];
//static int cmdParams[32];
static int scanBauds;
static int scanId;
static int scanIndex;
static char* txtBuffer;


  
};

extern DxlEngine engines[];


#endif
