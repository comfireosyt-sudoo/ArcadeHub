// ============================================================
// fruitmerge.cpp  (integer-physics fruit merge)
// ============================================================
#include "shared.h"

static constexpr int FM_MAX=48,FM_BX=44,FM_BY=20,FM_BW=196,FM_BH=185;
static constexpr int FM_TYPES=8;
static const int    fmR[FM_TYPES]={8,11,14,17,20,23,26,30};
static const EADK::Color fmC[FM_TYPES]={Col::Red,Col::Orange,Col::Yellow,Col::Green,Col::Cyan,Col::Blue,Col::Purple,Col::Pink};

struct FmFruit{int x,y,vx,vy,type;bool active;};
static FmFruit fmF[FM_MAX]; static int fmN;
static int fmDropX,fmNext; static uint32_t fmScore,fmHigh;
static uint32_t fmRng;

static uint32_t fmRand(){fmRng^=fmRng<<13;fmRng^=fmRng>>17;fmRng^=fmRng<<5;return fmRng;}
// Fixed-point *16
static int FP(int px){return px*16;}
static int FI(int fp){return fp/16;}

static void fmDrop(){
  if(fmN>=FM_MAX) return;
  FmFruit& f=fmF[fmN++];
  f.x=FP(fmDropX);f.y=FP(FM_BY+6);f.vx=0;f.vy=0;f.type=fmNext;f.active=true;
  fmNext=(int)(fmRand()%4);
}

static void fmUpdate(){
  int bot=FP(FM_BY+FM_BH),lft=FP(FM_BX),rgt=FP(FM_BX+FM_BW);
  for(int i=0;i<fmN;i++){
    if(!fmF[i].active) continue;
    fmF[i].vy+=FP(2)/8; fmF[i].x+=fmF[i].vx; fmF[i].y+=fmF[i].vy;
    int ri=FP(fmR[fmF[i].type]);
    if(fmF[i].y+ri>bot){fmF[i].y=bot-ri;fmF[i].vy=-fmF[i].vy*3/8;fmF[i].vx=fmF[i].vx*7/8;}
    if(fmF[i].x-ri<lft){fmF[i].x=lft+ri;fmF[i].vx=-fmF[i].vx/2;}
    if(fmF[i].x+ri>rgt){fmF[i].x=rgt-ri;fmF[i].vx=-fmF[i].vx/2;}
  }
  // Merges
  for(int i=0;i<fmN;i++){
    if(!fmF[i].active) continue;
    for(int j=i+1;j<fmN;j++){
      if(!fmF[j].active||fmF[i].type!=fmF[j].type) continue;
      int dx=FI(fmF[i].x)-FI(fmF[j].x),dy=FI(fmF[i].y)-FI(fmF[j].y);
      int r=fmR[fmF[i].type];
      if(dx*dx+dy*dy<(2*r+2)*(2*r+2)){
        int nt=fmF[i].type+1<FM_TYPES?fmF[i].type+1:FM_TYPES-1;
        int mx=(fmF[i].x+fmF[j].x)/2,my=(fmF[i].y+fmF[j].y)/2;
        fmF[i].active=fmF[j].active=false;
        if(fmN<FM_MAX){FmFruit& nf=fmF[fmN++];nf.x=mx;nf.y=my;nf.vx=0;nf.vy=-FP(2);nf.type=nt;nf.active=true;}
        static const uint32_t ms[]={1,2,4,8,16,32,64,128};
        fmScore+=ms[fmF[i].type<8?fmF[i].type:7];
        g_save.fmMerges++;
      }
    }
  }
}

static void fmRender(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,18,Col::DarkBlue);
  Draw::text("FRUIT MERGE",4,2,Col::Gold,Col::DarkBlue);
  char buf[24]; fmt(buf,24,"Score:%u",fmScore); Draw::text(buf,180,4,Col::White,Col::DarkBlue);
  Draw::drawRect(FM_BX-1,FM_BY-1,FM_BW+2,FM_BH+2,Col::MidGray);
  // Next fruit
  Draw::text("NEXT:",4,FM_BY+10,Col::LightGray,Col::Black);
  Draw::fillCircle(14,FM_BY+36,fmR[fmNext],fmC[fmNext]);
  // Drop cursor
  Draw::vLine(fmDropX,FM_BY,FM_BH,Col::DarkGray);
  Draw::fillCircle(fmDropX,FM_BY+fmR[fmNext]+2,fmR[fmNext],fmC[fmNext]);
  // Fruits
  for(int i=0;i<fmN;i++) if(fmF[i].active){
    Draw::fillCircle(FI(fmF[i].x),FI(fmF[i].y),fmR[fmF[i].type],fmC[fmF[i].type]);
    Draw::drawRect(FI(fmF[i].x)-fmR[fmF[i].type],FI(fmF[i].y)-fmR[fmF[i].type],fmR[fmF[i].type]*2+1,fmR[fmF[i].type]*2+1,Col::White);
  }
  char buf2[20]; fmt(buf2,20,"Hi:%u",fmHigh); Draw::text(buf2,4,FM_BY+60,Col::Silver,Col::Black);
  Draw::text("L/R:Move OK:Drop",4,SH-10,Col::MidGray,Col::Black);
}

void runFruitMerge(){
  fmRng=(uint32_t)EADK::Timing::millis()^0xFE1E1E1EU;
  fmHigh=g_save.fmHigh; fmScore=0; fmN=0;
  fmDropX=FM_BX+FM_BW/2; fmNext=(int)(fmRand()%4);
  memset(fmF,0,sizeof(fmF));

  Input::Edge inp; inp.reset(); int cooldown=0;

  while(true){
    Input::Keys k=inp.update();
    if(k.back) break;
    if(k.left  &&fmDropX>FM_BX+20)         fmDropX-=4;
    if(k.right &&fmDropX<FM_BX+FM_BW-20)   fmDropX+=4;
    if(k.ok&&cooldown==0){fmDrop();cooldown=20;}
    if(cooldown>0) cooldown--;
    fmUpdate();
    bool over=false;
    for(int i=0;i<fmN;i++) if(fmF[i].active&&FI(fmF[i].y)-fmR[fmF[i].type]<FM_BY+6) over=true;
    if(fmScore>fmHigh) fmHigh=fmScore;
    fmRender();
    if(over){
      Draw::fillRect(70,90,180,55,Col::DarkBlue); Draw::drawRect(70,90,180,55,Col::Red);
      Draw::textCentered("GAME OVER",100,Col::White,Col::DarkBlue,true);
      char buf[24]; fmt(buf,24,"Score:%u",fmScore); Draw::textCentered(buf,124,Col::Gold,Col::DarkBlue);
      Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
      if(fmScore>g_save.fmHigh) g_save.fmHigh=fmScore; savePersist();
      Input::waitRelease(); inp.reset();
      while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
      return;
    }
    EADK::Timing::msleep(33);
  }
  if(fmScore>g_save.fmHigh) g_save.fmHigh=fmScore; savePersist();
}
