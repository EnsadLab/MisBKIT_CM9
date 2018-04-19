
#include "DxlDefinitions.h"

extern Dynamixel Dxl;
Dynamixel Dxl(1); //Dynamixel on Serial1(USART1)
#include "DxlMotor.h"

//sync :
//FF FF FE len 0x83(syncwrite) startaddr len
// + id datas , id datas ... chksum

byte dxlAdrr[50]= //1:byte 2:word
{
  2,0, //0 R  ModelNumber
  1,  //2 R  FirmWare
  1,  //3 RW ID
  1,  //4 RW Baudrate
  1,  //5 RW Return delay time
  2,0, //6 RW CW  angle limit
  2,0, //8 RW CCW
  0,   //10 ?
  1,   //11 RW temp max
  1,   //12 volt min
  1,   //13 volt max
  2,0, //14 Max Torque
  1,   //16 return level
  1,   //17 alarm led
  1,   //18 alarm shutdown
  0,   //19 ?
  0,0,0,0,   //20,21,22,23 Multitour
  1,   //24 torque enable
  1,   //25 led
  1,   //26 margin CW   /D
  1,   //27 margin CCW  /I
  1,   //28 slope  CW   /P
  1,   //29 slope  CCW  /?
  2,0, //30 Goal Position
  2,0, //32 Moving Speed
  2,0, //34 Torque Limit
  2,0, //36 Present Position
  2,0, //38 Present Speed
  2,0, //40 Present Load
  1,   //42 Present Voltage
  1,   //43 Present temp
  1,   //44 Registered
  0,   //45 ??
  1,   //46 Moving
  1,   //47 Lock EEPROM
  2,0  //48 Punch 
};

unsigned long dxlTemperatureTime = 0;
int dxlTemperatureIndex = 0;

//uint8 msgWriteByte[256];
//char msgReadByte[256];

void dxlInitialize(){
  Dxl.begin(3);
  //0: 9615  bauds
  //1; 57143 bauds
  //2: 115200 (117647)
  //3: 1000000
  for(int i=0;i<NB_MOTORS;i++)
  {
    motors[i].index = i;
    motors[i].init();
  }
}

DxlMotor* dxlFindMotorByID( int dxlid ){
  for(int i=0;i<NB_MOTORS;i++){
    if(motors[i].dxlID == dxlid )
      return &motors[i];
  }
  return NULL;
}

char* dxlParseStr(char* str){
  if(strBegin(str,"Motor")){ //"dxlM index dxlID mode speed goal [torque]
    int index = 999;
    str = parseInt(str,&index);
    if( index<NB_MOTORS ){
      str = motors[index].parseIMSGT(str);
    }
  }  
  else if(strBegin(str,"W")){ //dxlWrite id,addr,value //dxlWb dxlWw
    int i=-1,a=-1,v=-1;
    str = parseInt(str,&i);
    str = parseInt(str,&a);          
    str = parseInt(str,&v);
    if( (i>=0)&&(a>=0)&&(v>=0) ){
      dxlWrite(i,a,v);
    }      
    LOGF("dxlW i:%i a:%i v:%i\n",i,a,v);
  }
  else if(strBegin(str,"R")){ //dxlRead id,addr //dxlRb dxlRw
    int i=-1,a=-1,r=-1;
    str = parseInt(str,&i);
    str = parseInt(str,&a);
    if( (i>=0)&&(a>=0) ){
      r = dxlRead(i,a);
    }
    xSerialESP.sendf("dxlR %i %i %i\n",i,a,r);
    LOGF("dxlR i:%i a:%i v:%i\n",i,a,r);
  }
  else if(strBegin(str,"Stop")){ //"dxlStop [dxlID]"
    int id=-1;
    str = parseInt(str,&id);
    if( id>0 ){
      DxlMotor* pm = dxlFindMotorByID(id);
      if(pm)pm->stop();      
    }
    else{
      for(int i=0;i<NB_MOTORS;i++){
        motors[i].stop();
      }
    }  
  }
  else if(strBegin(str,"Model")){ //"dxlModel dxlID"
    int id=-1;
    str = parseInt(str,&id);
    int model = dxlReadModel(id);
    xSerialESP.sendf("dxlModel %i %i\n",id,model);
    LOGF("dxlModel i:%i v:%i\n",id,model);    
  }
  else if(strBegin(str,"Ping")){ //slower but getmodel best than real ping
    int id=-1;
    str = parseInt(str,&id);
    int model = dxlReadModel(id);
    xSerialESP.sendf("dxlPing %i %i\n",id,model);
    LOGF("dxlPing i:%i v:%i\n",id,model);
  }
  else{
    char* p = str;
    while(*p>' ')p++;
    char c=*p;*p=0;
    LOGUSB("dxl???:",str);
    *p=c;    
  }
  str=skipLine(str);
  return str;
}

int dxlReadModel(int id){
    int m = -1;  
    if( (id>0)&&(id<253) ){
      m = dxlRead(id,P_MODEL);
      if( m>0xFF00 )m=-1; //got strange values sometimes
      DxlMotor* pm = dxlFindMotorByID(id);
      if(pm) pm->setModel(m);
    }
    return m;
}

void dxlWriteByte(uint8 id,uint8 addr,uint8 val)
{
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(addr);
  Dxl.pushByte(val);
  Dxl.txPacket(id,INST_WRITE,2);
}

void dxlWriteWord(uint8 id,uint8 addr,word val)
{
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(addr);
  Dxl.pushByte(val & 0xFF);
  Dxl.pushByte(val >> 8);  
  Dxl.txPacket(id,INST_WRITE,3);
}

void dxlWrite(int id,int addr,int value)
{
  if(addr>48)
    return;
  Dxl.setTxPacketLength(0);  
  if( dxlAdrr[addr]==1 ){
    //Dxl.writeByte(id,ireg,value);
    Dxl.pushByte(addr);
    Dxl.pushByte(value);
    Dxl.txPacket(id,INST_WRITE,2);
  }
  else if(dxlAdrr[addr]==2){
    //Dxl.writeWord(id,ireg,value);
    Dxl.pushByte(addr);
    Dxl.pushByte(value & 0xFF);
    Dxl.pushByte(value >> 8);  
    Dxl.txPacket(id,INST_WRITE,3);
  }
}


void dxlWrite2words(int id,int addr,word w0,word w1)
{
  if( (addr<=4)||addr>48)
    return;
    
  Dxl.setTxPacketLength(0);  
  Dxl.pushByte(addr);
  Dxl.pushByte(w0 & 0xFF);
  Dxl.pushByte(w0 >> 8);  
  Dxl.pushByte(w1 & 0xFF);  
  Dxl.pushByte(w1 >> 8);  
  Dxl.txPacket(id,INST_WRITE,5);
}

void dxlWrite4bytes(int id,int addr,int8 b0,int8 b1,int8 b2,int8 b3)
{
  if( (addr<=4)||addr>48)
    return;
    
  Dxl.setTxPacketLength(0);  
  Dxl.pushByte(addr);
  Dxl.pushByte(b0);
  Dxl.pushByte(b1);  
  Dxl.pushByte(b2);  
  Dxl.pushByte(b3);  
  Dxl.txPacket(id,INST_WRITE,5);
}

//sync :
//FF FF FE len 0x83(syncwrite) startaddr len
// + id datas , id datas ... chksum

//note: Dxl.readWord() return parfois -1 alors que la reponse est bonne !!!
int dxlRead(int dxlid,int ireg){
  
  uint8* pr = Dxl.getRxPacketPtr();    
  //pr[4]=0xFF; //return status par defaut
  pr[3]=0; //return len par defaut
  pr[5]=0xFF;
  pr[6]=0xFF;

  if( dxlAdrr[ireg]==1 )
  {
    Dxl.readByte((uint8)dxlid,(uint8)ireg);
    if(pr[3]!=0)
      return (int)pr[5];
  } 
  else if(dxlAdrr[ireg]==2)
  {
    word r = Dxl.readWord((uint8)dxlid,(uint8)ireg);
    if(pr[3]!=0){
      int v=(int)pr[5]+((int)pr[6]<<8);
      if(v<65535)
        return v;
    }
    //serialSend("!err",(int)pr[2],(int)pr[3],(int)pr[4]);
  }
  return -1;
}


void dxlSyncGoalSpeeds(){
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(P_GOAL_POSITION_L);
  Dxl.pushByte(4); //2 word
  int len = 2;
  for(int i=0;i<NB_MOTORS;i++){
    if( motors[i].dxlID > 0 ){
      int g = motors[i].cmdGoal;
      int s = motors[i].cmdSpeed;
      if(s<0)s=1024-s;
      Dxl.pushByte(motors[i].dxlID);
      Dxl.pushByte(g & 0xFF);
      Dxl.pushByte(g >> 8);
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


void dxlSyncGoals(){  
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(P_GOAL_POSITION_L);
  Dxl.pushByte(2); //1 word
  int len = 2;
  for(int i=0;i<NB_MOTORS;i++){
    if( motors[i].dxlID > 0 ){
      int g = motors[i].cmdGoal;
      Dxl.pushByte(motors[i].dxlID);
      Dxl.pushByte(g & 0xFF);
      Dxl.pushByte(g >> 8);
      len+=3; 
    }
  }
  if(len>2)
    Dxl.txPacket(BROADCAST_ID,INST_SYNC_WRITE,len);
  else
    Dxl.setTxPacketLength(0);
}

void dxlSyncSpeeds(){
  Dxl.setTxPacketLength(0);
  Dxl.pushByte(P_GOAL_SPEED_L);
  Dxl.pushByte(2); //1 word
  int len = 2;
  for(int i=0;i<NB_MOTORS;i++){
    if( motors[i].dxlID > 0 ){
      int s = motors[i].cmdSpeed;
      if(s<0)s=1024-s;
      Dxl.pushByte(motors[i].dxlID);
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

void dxlSendAllPos(){
  xSerialESP.print("dxlpos");
  for(int i=0;i<NB_MOTORS;i++){
    xSerialESP.sendf(",%i",motors[i].readPosition());
  }
  xSerialESP.print("\n");
}


void dxlSendTemperature(unsigned long t){
  if( (t-dxlTemperatureTime)>2333 ){ 
    dxlTemperatureTime = t;
    if(++dxlTemperatureIndex>=NB_MOTORS)
      dxlTemperatureIndex=0;
    int tp = motors[dxlTemperatureIndex].readTemperature();
    if(tp>0){
      xSerialESP.sendf("dxlTemp %i %i\n",dxlTemperatureIndex,tp);           
      //LOGUSB("temperaturev:",tp);
    }
  }
}


