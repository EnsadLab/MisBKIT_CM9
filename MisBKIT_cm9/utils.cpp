
#include "libpandora_types.h"
#include "Arduino-compatibles.h"
#include "stdarg.h"

extern boolean usbEnabled;

char tmpstr[64]; //unsafe


int strlen(const char* str){
  int l = 0;
  while(str[l]!=0)l++;
  return l;
}

char* strchr(char* sce,char c){
  while(*sce!=0){
    if(*sce==c)
      return sce;
    sce++;
  }
  return NULL;
}

char* skipLine(char* str){
  while(*str>=' ')str++;               //skip string
  while( (*str<' ')&&(*str!=0) )str++; //skip crlf
  return str;
}


char* skipWord(char* str){ // ' ' ','
  while( (*str>' ')&&(*str!=',')  )str++;
  while( (*str<' ')&&(*str!=0) )str++; //skip crlf
  return str;
}


//strcpy //return end of dest
char* str2str(char*dest,const char* sce){
  while(*sce!=0){*dest=*sce;dest++;sce++;}
  return dest;
}

//itoa //return end of dest
char* int2str(char* dest,char separator,int n){
  if(separator!=0){*dest=separator;dest++;}
  itoa(n,dest,10);
  while(*dest!=0)dest++;
  return dest;  
}

//grrrrrr X.xxx
char* float2str(char* dest,float n){
  if(n<0){
    *dest='-';dest++;
    n = -n;
  }
  int x  = (int)n;
  n = (n-x)*1000;
  dest = int2str(dest,0,x);
  *dest = '.';dest++;
  if(n<100){*dest = '0';dest++;}
  if(n<10){*dest = '0';dest++;}  
  return int2str(dest,0,(int)n);
}


//format = "%i,%f %s" int float string
char* strPrint(char* dest,const char* format, ... ){
  va_list argList;
  va_start(argList, format);
  char* pf = (char*)format;
  while(*pf!=0){
    if(*pf!='%'){*dest = *pf;dest++;}
    else{
      pf++;
      switch(*pf){
        case 'i':dest = int2str(dest,0,va_arg(argList, int));break;
        case 'f':dest = float2str(dest,va_arg(argList, double));break;
        case 's':dest = str2str(dest,va_arg(argList, char*));break;
        default: pf--;break; //type not handled
      }
    }
    pf++;
  }
  va_end(argList);
  *dest = 0;
  return dest;
}


//format = "%i,%f %s" int float string
void LOGF(const char* format, ... ){
  if(!usbEnabled)
    return;
  char* ptmp = tmpstr;
  va_list argList;
  va_start(argList, format);
  char* pf = (char*)format;
  while(*pf!=0){
    if(*pf!='%'){*ptmp = *pf;ptmp++;}
    else{
      if(ptmp>tmpstr){
        *ptmp=0;
        ptmp = tmpstr;
        SerialUSB.print(tmpstr);
      }      
      pf++;
      switch(*pf){
        case 'i':SerialUSB.print(va_arg(argList, int));break;
        case 'f':SerialUSB.print(va_arg(argList, double));break;
        case 's':SerialUSB.print(va_arg(argList, char*));break;
        default: pf--;break; //type not handled
      }
    }
    pf++;
  }
  va_end(argList);
  if(ptmp>tmpstr){
    *ptmp=0;
    SerialUSB.print(tmpstr);
  }
}

//find cs in str , returns str* after string found , NULL if not found !!! END of str: <' ' !!!
char* strFind(char* str,const char* cs){
  char* pt;
  char* ps;
  while(*str>=' '){ // 0 or CRLF = end of string
    pt = (char*)cs;
    ps = str;
    while((*ps == *pt)&&(*ps!=0)){
      ps++;pt++;
      if(*pt==0)
        return ps;
    }
    str++;
  }
  return NULL; //not found
} 

char* getQuotedString(char* sce,char* dest){
  *dest = 0;
  sce = strchr(sce,'"');
  if(sce==NULL)
    return NULL;
  sce++;  
  while( (*sce!='"')&&(*sce!=0) ){
    *dest = *sce;
    sce++;
    dest++; 
  }
  *dest = 0;
  if(*sce!=0)
    sce++;
  return sce;  
}


//beginWith
boolean strBegin(char* p1,const char* p2){
  int i=0;
  while(p2[i]>=' '){
    if(p1[i] != p2[i] )
      return false;
    i++;
  }
  return true;
}

char* firstNum(char* str){
  char c = *str;
  while( (c<'0')||(c>'9') ){
    if(c=='-') return str;
    if(c==0)   return NULL;
    str++;
    c=*str; 
  }
  return str;
}


char* parseInt(char* str,int* result){
  while( ((*str<'-')||(*str>'9'))&&(*str>=' ') )str++;  
  if( *str>' ' ){
    //SerialUSB.print(" atoi:");SerialUSB.print(str);
    *result = atoi(str);
    do{ str++; }while( (*str>='0')&&(*str<='9') ); //skip num
    //SerialUSB.print(" atoi:");SerialUSB.print(str);
  }
  return str;
}

int parseInt(char** ppstr,int prev){
  char* str = *ppstr;
  int r = prev;
  while( ((*str<'-')||(*str>'9'))&&(*str>=' ') )str++;  //skip to num
  if( *str>' ' ){
    r = atoi(str);
    do{ str++; }while( (*str>='0')&&(*str<='9') ); //skip - num
  }
  *ppstr = str;
  return r;
}

