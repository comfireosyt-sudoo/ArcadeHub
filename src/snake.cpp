// ============================================================
// snake.cpp
// ============================================================
#include "shared.h"

static constexpr int SNC=24,SNR=17,SNCELL=12,SNOX=16,SNOY=24;
struct SnPos{int x,y;};
static SnPos snBody[SNC*SNR];
static int   snLen, snDir, snNextDir;
static SnPos snFood;
static uint32_t snScore, snHigh;
static uint32_t snRng;

static uint32_t snRand(){snRng^=snRng<<13;snRng^=snRng>>17;snRng^=snRng<<5;return snRng;}

static bool snBodyHit(int x,int y,int skip){
  for(int i=0;i<snLen-skip;i++) if(snBody[i].x==x&&snBody[i].y==y) return true;
  return false;
}
static void snPlaceFood(){
  do{snFood.x=(int)(snRand()%SNC);snFood.y=(int)(snRand()%SNR);}
  while(snBodyHit(snFood.x,snFood.y,0));
}

static void snRender(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,22,Col::DarkBlue);
  Draw::drawRect(0,0,SW,22,Col::Blue);
  Draw::text("SNAKE",4,3,Col::Gold,Col::DarkBlue);
  char buf[24];
  fmt(buf,24,"Score:%u",snScore); Draw::text(buf,120,6,Col::White,Col::DarkBlue);
  fmt(buf,24,"Best:%u", snHigh);  Draw::text(buf,230,6,Col::Silver,Col::DarkBlue);
  Draw::fillRect(SNOX-1,SNOY-1,SNC*SNCELL+2,SNR*SNCELL+2,Col::DarkGray);
  // Food
  Draw::fillRect(SNOX+snFood.x*SNCELL+1,SNOY+snFood.y*SNCELL+1,SNCELL-1,SNCELL-1,Col::Red);
  // Body
  for(int i=snLen-1;i>=1;i--){
    uint32_t g=0x008000+(uint32_t)(i*4%80)*0x100;
    Draw::fillRect(SNOX+snBody[i].x*SNCELL+1,SNOY+snBody[i].y*SNCELL+1,SNCELL-1,SNCELL-1,EADK::Color(g));
  }
  // Head
  Draw::fillRect(SNOX+snBody[0].x*SNCELL+1,SNOY+snBody[0].y*SNCELL+1,SNCELL-1,SNCELL-1,Col::Green);
  Draw::text("Arrow:Dir  Back:Quit",4,SH-10,Col::MidGray,Col::Black);
}

void runSnake(){
  snRng=(uint32_t)EADK::Timing::millis()^0x5AA55AA5U;
  snHigh=g_save.snakeHigh; snScore=0; snLen=3; snDir=1; snNextDir=1;
  for(int i=0;i<snLen;i++){snBody[i].x=SNC/2-i;snBody[i].y=SNR/2;}
  snPlaceFood();

  Input::Edge inp; inp.reset();
  int speed=180;
  uint32_t lastMove=EADK::Timing::millis();

  while(true){
    Input::Keys k=inp.update();
    if(k.back) break;
    if(k.up    &&snNextDir!=2) snNextDir=0;
    if(k.right &&snNextDir!=3) snNextDir=1;
    if(k.down  &&snNextDir!=0) snNextDir=2;
    if(k.left  &&snNextDir!=1) snNextDir=3;

    uint32_t now=EADK::Timing::millis();
    if((int)(now-lastMove)>=speed){
      lastMove=now; snDir=snNextDir;
      static const int dx[]={0,1,0,-1}, dy[]={-1,0,1,0};
      int nx=snBody[0].x+dx[snDir], ny=snBody[0].y+dy[snDir];
      if(nx<0||nx>=SNC||ny<0||ny>=SNR||snBodyHit(nx,ny,1)) break;
      bool ate=(nx==snFood.x&&ny==snFood.y);
      if(!ate) for(int i=snLen-1;i>0;i--) snBody[i]=snBody[i-1];
      else{ if(snLen<SNC*SNR) snLen++; for(int i=snLen-1;i>0;i--) snBody[i]=snBody[i-1];
        snScore+=10; if(snScore%50==0&&speed>60) speed-=10; snPlaceFood(); }
      snBody[0]={nx,ny};
      if(snScore>snHigh) snHigh=snScore;
    }
    snRender();
    EADK::Timing::msleep(16);
  }
  if(snScore>g_save.snakeHigh) g_save.snakeHigh=snScore;
  g_save.snakeGames++; savePersist();

  snRender();
  Draw::fillRect(70,90,180,55,Col::DarkBlue); Draw::drawRect(70,90,180,55,Col::Red);
  Draw::textCentered("GAME OVER",100,Col::White,Col::DarkBlue,true);
  char buf[24]; fmt(buf,24,"Score:%u",snScore); Draw::textCentered(buf,124,Col::Gold,Col::DarkBlue);
  Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
  Input::waitRelease(); inp.reset();
  while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
}
