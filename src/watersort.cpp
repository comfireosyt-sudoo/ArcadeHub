#include "shared.h"

static constexpr int WS_TUBES=8,WS_CAP=4;
static uint8_t wsTubes[WS_TUBES][WS_CAP];
static int     wsFill[WS_TUBES];
static int     wsNumTubes,wsNumColors;
static uint32_t wsLevel,wsRng;
static int     wsSel,wsCur;

struct WsMove{int from,to;};
static WsMove wsHistory[32]; int wsHistSz=0;

static const EADK::Color wsCol[]={Col::Black,Col::Red,Col::Blue,Col::Green,Col::Yellow,Col::Purple,Col::Orange,Col::Cyan};

static uint32_t wsRand(){wsRng^=wsRng<<13;wsRng^=wsRng>>17;wsRng^=wsRng<<5;return wsRng;}

static bool wsCanPour(int from,int to){
  if(from==to||!wsFill[from]||wsFill[to]==WS_CAP) return false;
  uint8_t tf=wsTubes[from][wsFill[from]-1];
  if(!wsFill[to]) return true;
  return tf==wsTubes[to][wsFill[to]-1];
}

static void wsPour(int from,int to){
  if(!wsCanPour(from,to)) return;
  uint8_t col=wsTubes[from][wsFill[from]-1];
  int mv=0; for(int i=wsFill[from]-1;i>=0&&wsTubes[from][i]==col;i--) mv++;
  int sp=WS_CAP-wsFill[to]; if(mv>sp) mv=sp;
  for(int i=0;i<mv;i++){wsTubes[to][wsFill[to]++]=col;wsTubes[from][--wsFill[from]]=0;}
  if(wsHistSz<32) wsHistory[wsHistSz++]={from,to};
}

static void wsUndo(){
  if(!wsHistSz) return;
  WsMove mv=wsHistory[--wsHistSz];
  if(!wsFill[mv.to]) return;
  uint8_t col=wsTubes[mv.to][wsFill[mv.to]-1];
  int mv2=0; for(int i=wsFill[mv.to]-1;i>=0&&wsTubes[mv.to][i]==col;i--) mv2++;
  for(int i=0;i<mv2;i++){wsTubes[mv.from][wsFill[mv.from]++]=col;wsTubes[mv.to][--wsFill[mv.to]]=0;}
}

static bool wsWon(){
  for(int t=0;t<wsNumTubes;t++){
    if(!wsFill[t]) continue;
    if(wsFill[t]!=WS_CAP) return false;
    uint8_t c=wsTubes[t][0];
    for(int s=1;s<WS_CAP;s++) if(wsTubes[t][s]!=c) return false;
  } return true;
}

static void wsGenLevel(uint32_t seed){
  wsRng=seed^0xDEADC0DEU;
  wsNumColors=(int)(3+wsLevel%5); if(wsNumColors>7) wsNumColors=7;
  wsNumTubes=wsNumColors+2; if(wsNumTubes>WS_TUBES) wsNumTubes=WS_TUBES;
  memset(wsTubes,0,sizeof(wsTubes)); memset(wsFill,0,sizeof(wsFill));
  for(int c=0;c<wsNumColors;c++){for(int s=0;s<WS_CAP;s++) wsTubes[c][s]=(uint8_t)(c+1);wsFill[c]=WS_CAP;}
  for(int it=0;it<200;it++){int f=(int)(wsRand()%wsNumTubes),t=(int)(wsRand()%wsNumTubes);if(wsCanPour(f,t))wsPour(f,t);}
  wsHistSz=0; wsSel=-1; wsCur=0;
}

static void wsRender(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,18,Col::DarkBlue); Draw::drawRect(0,0,SW,18,Col::Blue);
  char buf[24]; fmt(buf,24,"WATER SORT - Lv %u",wsLevel+1);
  Draw::textCentered(buf,2,Col::Gold,Col::DarkBlue);

  int spacing=28, tw=wsNumTubes*spacing-4;
  bool two=(tw>SW-4); int r1=two?(wsNumTubes+1)/2:wsNumTubes;
  int TH=72, TW=22;

  for(int i=0;i<wsNumTubes;i++){
    int row=two?(i/r1):0, col=two?(i%r1):i;
    int rc=two?(row==0?r1:(wsNumTubes-r1)):wsNumTubes;
    int tw2=rc*spacing-4;
    int tx=(SW-tw2)/2+col*spacing;
    int ty=24+row*(TH+12);
    bool sel=(i==wsCur);
    EADK::Color border=(i==wsSel)?Col::Gold:sel?Col::White:Col::LightGray;
    Draw::fillRect(tx,ty,TW,TH,Col::DarkGray); Draw::drawRect(tx,ty,TW,TH,border);
    int segH=(TH-4)/WS_CAP;
    for(int s=0;s<wsFill[i];s++){
      int sy=ty+TH-2-(s+1)*segH;
      Draw::fillRect(tx+2,sy,TW-4,segH-1,wsCol[wsTubes[i][s]]);
    }
    if(sel) Draw::fillRect(tx+7,ty-5,8,4,Col::White);
  }
  Draw::text("L/R:Sel OK:Pick/Pour Up:Undo Back:Quit",2,SH-10,Col::MidGray,Col::Black);
}

void runWaterSort(){
  wsLevel=g_save.wsLevel;
  wsRng=(uint32_t)EADK::Timing::millis()^0xABCD1234U;
  wsGenLevel(wsLevel*137+42);

  Input::Edge inp; inp.reset();

  while(true){
    Input::Keys k=inp.update();
    if(k.back) break;
    if(k.left  &&wsCur>0) {wsCur--;wsSel=-1;}
    if(k.right &&wsCur<wsNumTubes-1){wsCur++;wsSel=-1;}
    if(k.up) {wsUndo();wsSel=-1;}
    if(k.ok){
      if(wsSel<0){if(wsFill[wsCur]>0) wsSel=wsCur;}
      else{
        if(wsSel==wsCur) wsSel=-1;
        else if(wsCanPour(wsSel,wsCur)){wsPour(wsSel,wsCur);wsSel=-1;}
        else wsSel=-1;
      }
    }
    wsRender();
    if(wsWon()){
      Draw::fillRect(60,88,200,55,Col::DarkBlue); Draw::drawRect(60,88,200,55,Col::Gold);
      Draw::textCentered("SORTED!",98,Col::Gold,Col::DarkBlue,true);
      char buf[32]; fmt(buf,32,"Level %u complete!",wsLevel+1);
      Draw::textCentered(buf,120,Col::White,Col::DarkBlue);
      Draw::textCentered("OK: Next Level",136,Col::LightGray,Col::DarkBlue);
      g_save.wsCompleted|=(1U<<(wsLevel%32)); wsLevel++;
      g_save.wsLevel=wsLevel; savePersist();
      Input::waitRelease(); inp.reset();
      while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
      wsGenLevel(wsLevel*137+42);
    }
    EADK::Timing::msleep(50);
  }
  g_save.wsLevel=wsLevel; savePersist();
}
