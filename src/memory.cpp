#include "shared.h"

static uint8_t memCards[36];
static bool    memFlipped[36],memMatched[36];
static int     memCols,memRows,memTotal,memPairs,memMatched2,memMoves,memFirst,memDiff;
static uint32_t memStart,memElapsed;
static uint32_t memRng;
static const char memSyms[18]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R'};

static uint32_t memRand(){memRng^=memRng<<13;memRng^=memRng>>17;memRng^=memRng<<5;return memRng;}

static void memRender(int cur,int pauseFlip){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,22,Col::DarkBlue); Draw::drawRect(0,0,SW,22,Col::Blue);
  Draw::text("MEMORY MATCH",4,4,Col::Gold,Col::DarkBlue);
  char buf[24];
  fmt(buf,24,"Moves:%d",memMoves); Draw::text(buf,160,6,Col::White,Col::DarkBlue);
  fmt(buf,24,"Time:%us",memElapsed); Draw::text(buf,250,6,Col::Cyan,Col::DarkBlue);

  int cw=(memCols==6)?48:(memCols==4?68:52);
  int ch=(memRows==6)?34:(memRows==5?38:44);
  int gx=(SW-memCols*(cw+3))/2, gy=26;

  for(int i=0;i<memTotal;i++){
    int r=i/memCols,c=i%memCols;
    int px=gx+c*(cw+3),py=gy+r*(ch+3);
    bool isCur=(i==cur), face=memFlipped[i]||memMatched[i];
    EADK::Color bg=memMatched[i]?Col::DarkGreen:face?Col::Blue:isCur?Col::DarkGray:Col::MidGray;
    EADK::Color brd=isCur?Col::Gold:memMatched[i]?Col::Green:Col::LightGray;
    Draw::fillRect(px,py,cw,ch,bg); Draw::drawRect(px,py,cw,ch,brd);
    if(face){char s[2]={memSyms[memCards[i]],0};Draw::textLarge(s,px+cw/2-5,py+ch/2-7,Col::White,bg);}
    else{Draw::text("?",px+cw/2-3,py+ch/2-5,Col::LightGray,bg);}
  }
  Draw::text("Arrow:Move OK:Flip Back:Quit",4,SH-10,Col::MidGray,Col::Black);
}

void runMemory(){
  memRng=(uint32_t)EADK::Timing::millis()^0xF00DBEEFU;
  const char* opts[]={"Easy  (4x4, 8 pairs)","Medium(4x5,10 pairs)","Hard  (6x6,18 pairs)"};
  int sel=runMenu(opts,3,0,"MEMORY MATCH");
  if(sel<0) return;
  memDiff=sel;
  int dims[][2]={{4,4},{4,5},{6,6}};
  memCols=dims[sel][0]; memRows=dims[sel][1];
  memTotal=memCols*memRows; memPairs=memTotal/2;
  memMatched2=0; memMoves=0; memFirst=-1;
  memset(memFlipped,0,sizeof(memFlipped)); memset(memMatched,0,sizeof(memMatched));
  for(int i=0;i<memPairs;i++){memCards[i*2]=(uint8_t)i;memCards[i*2+1]=(uint8_t)i;}
  for(int i=memTotal-1;i>0;i--){int j=memRand()%(i+1);uint8_t t=memCards[i];memCards[i]=memCards[j];memCards[j]=t;}
  memStart=EADK::Timing::millis(); memElapsed=0;

  int cur=0; int pauseTimer=0;
  Input::Edge inp; inp.reset(); int frame=0;

  while(true){
    memElapsed=(EADK::Timing::millis()-memStart)/1000;
    if(pauseTimer>0){
      pauseTimer--;
      if(pauseTimer==0){
        if(memCards[memFirst]==memCards[cur]){memMatched[memFirst]=memMatched[cur]=true;memMatched2++;}
        else{memFlipped[memFirst]=memFlipped[cur]=false;}
        memFirst=-1;
      }
      memRender(cur,pauseTimer); EADK::Timing::msleep(50); frame++; continue;
    }
    Input::Keys k=inp.updateRepeat(frame++);
    if(k.back) return;
    if(k.left  &&cur%memCols>0)           cur--;
    if(k.right &&cur%memCols<memCols-1)   cur++;
    if(k.up    &&cur>=memCols)             cur-=memCols;
    if(k.down  &&cur+memCols<memTotal)     cur+=memCols;
    if(k.ok&&!memMatched[cur]&&!memFlipped[cur]){
      memFlipped[cur]=true;
      if(memFirst<0) memFirst=cur;
      else if(memFirst!=cur){memMoves++;pauseTimer=20;}
    }
    if(memMatched2==memPairs){
      uint32_t el=memElapsed;
      if(el<g_save.memBest[memDiff]) g_save.memBest[memDiff]=el;
      g_save.memWins++; savePersist();
      memRender(cur,0);
      Draw::fillRect(50,80,220,70,Col::DarkBlue); Draw::drawRect(50,80,220,70,Col::Gold);
      Draw::textCentered("YOU WIN!",90,Col::Gold,Col::DarkBlue,true);
      char buf[32]; fmt(buf,32,"Time: %us",el); Draw::textCentered(buf,114,Col::White,Col::DarkBlue);
      fmt(buf,32,"Moves: %d",memMoves); Draw::textCentered(buf,130,Col::Cyan,Col::DarkBlue);
      Draw::textCentered("OK to exit",148,Col::LightGray,Col::DarkBlue);
      Input::waitRelease(); inp.reset();
      while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
      return;
    }
    memRender(cur,0);
    EADK::Timing::msleep(50);
  }
}
