
#include "libpandora_types.h"
#include "Arduino-compatibles.h"
#include "stdarg.h"

int strlen(const char* str){
  int l = 0;
  while(str[l]!=0)
    l++;
  return l;
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

char* strchr(char* sce,char c){
  while(*sce!=0){
    if(*sce==c)
      return sce;
    sce++;
  }
  return NULL;
}

//find cs in str , returns str* after string found  
char* strFind(char* str,const char* cs){
  char* pt = (char*)cs;
  char* ps = str;
  while(*str != 0 ){
    if(*str == *pt){
      ps=str;
      do{
        if(*pt==0)
          return ps;
        ps++;
        pt++;
      }while((*ps == *pt)&&(*ps!=0));
      pt = (char*)cs;
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
  char* next = firstNum(str);
  if(next!=NULL){
    *result = atoi(next);
    do{ next++; }while( (*next>='0')&&(*next<='9') );
    return next;
  }
  return str;
}

