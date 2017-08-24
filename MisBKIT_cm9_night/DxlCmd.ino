#define P_ID 3

Dynamixel Dxl(1); //Dynamixel on Serial1(USART1)

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

//uint8 msgWriteByte[256];
//char msgReadByte[256];

/*
void dxlInitialize(){
  Dxl.begin(3);//0:9615bauds //1; 57143 bauds //2: 115200 (117647) //3: 1000000
  delay(1000);
  for(int i=0;i<NB_ENGINES;i++)
  {
    engines[i].init();
    delay(50);
  }
}
*/

//CF byte Dynamixel::txPacket(byte bID, byte bInstruction, int bParameterLength)
/*
void dxlSend(id,uint8* msg,len)
{
  Dxl.initPacket(id,msg[0]);
  const uint8* pRx = Dxl.getRxPacketPtr();
  for(int i=1;i<len;i++)
    Dxl.pushByte(msg[i]);
  //return Dxl.txrxPacket();  
}
*/

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
  //!!!MX28 FORCE_MX28
  //if( (value>0)&&(addr==P_CCW_ANGLE_LIMIT_L) )
  //  value = 4095; //(GOAL_RANGE-1); 
  
  if(addr == P_TORQUE_ENABLE)
    value = 1;
    
  if(addr == P_TORQUE_LIMIT_L )
    return;  
  
  //if(ireg<=4)
  //  return;
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

//sync :
//FF FF FE len 0x83(syncwrite) startaddr len
// + id datas , id datas ... chksum

//note: Dxl.readWord() return parfois -1 alors que la reponse est bonne !!!
int dxlRead(int imot,int ireg)
{
  uint8* pr = Dxl.getRxPacketPtr();    
  //pr[4]=0xFF; //return status par defaut
  pr[3]=0; //return len par defaut

  if( dxlAdrr[ireg]==1 )
  {
    Dxl.readByte((uint8)imot,(uint8)ireg);
    if(pr[3]!=0)
      return (int)pr[5];
  } 
  else if(dxlAdrr[ireg]==2)
  {
    Dxl.readWord((uint8)imot,(uint8)ireg);
    if(pr[3]!=0)
      return (int)pr[5]+((int)pr[6]<<8);
    //serialSend("!err",(int)pr[2],(int)pr[3],(int)pr[4]);
  }
  return -1;
}

int dxlFindEngine(int from)
{
  for(int i=from;i<254;i++)
  {
    int id=Dxl.readByte(i,3); //P_ID
    if( id==i)
      return i;
    delay(20);
  }
  return -1;
}


