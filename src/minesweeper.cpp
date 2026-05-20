#include "shared.h"

static constexpr int MSC=16,MSR=14,MSCELL=17,MSOX=8,MSOY=26;
static uint8_t msGrid[MSR][MSC]; // 0=hidden,1=revealed,2=flagged
static uint8_t msMine[MSR][MSC];
static uint8_t msAdj[MSR][MSC];
static int     msCols,msRows,msMines,msDiff;
static int     msCurR,msCurC,msFlagCnt,msRevCnt,msSafeCnt;
static bool    msFirst,msOver,msWon;
static uint32_t msStart,msElapsed;
static uint32_t msRng;

static uint32_t msRand(){msRng^=msRng<<13;msRng^=msRng>>17;msRng^=msRng<<5;return msRng;}

static void msPlace(int sr,int sc){
  memset(msMine,0,sizeof(msMine));
  int placed=0,lim=0;
  while(placed<msMines&&lim++<10000){
    int r=(int)(msRand()%msRows),c=(int)(msRand()%msCols);
    if(r>=sr-1&&r<=sr+1&&c>=sc-1&&c<=sc+1) continue;
    if(msMine[r][c]) continue;
    msMine[r][c]=1; placed++;
  }
  memset(msAdj,0,sizeof(msAdj));
  for(int r=0;r<msRows;r++) for(int c=0;c<msCols;c++){
    if(msMine[r][c]) continue;
    int cnt=0;
    for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++){
      int nr=r+dr,nc=c+dc;
      if(nr>=0&&nr<msRows&&nc>=0&&nc<msCols&&msMine[nr][nc]) cnt++;
    } msAdj[r][c]=(uint8_t)cnt;
  }
}

static void msReveal(int sr,int sc){
  struct P{int r,c;};
  P stk[MSC*MSR]; int top=0;
  stk[top++]={sr,sc};
  while(top>0){
    P p=stk[--top];
    if(p.r<0||p.r>=msRows||p.c<0||p.c>=msCols) continue;
    if(msGrid[p.r][p.c]) continue;
    msGrid[p.r][p.c]=1; msRevCnt++;
    if(msAdj[p.r][p.c]==0&&!msMine[p.r][p.c])
      for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++)
        if((dr||dc)&&top<MSC*MSR-1) stk[top++]={p.r+dr,p.c+dc};
  }
}

static const EADK::Color msNumCol[]={Col::Black,Col::Blue,Col::Green,Col::Red,Col::DarkBlue,Col::Brown,Col::Cyan,Col::Black,Col::MidGray};

static void msRender(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,24,Col::DarkBlue); Draw::drawRect(0,0,SW,24,Col::Blue);
  Draw::text("MINESWEEPER",4,4,Col::Gold,Col::DarkBlue);
  char buf[24];
  fmt(buf,24,"Mines:%d",msMines-msFlagCnt); Draw::text(buf,160,7,Col::Red,Col::DarkBlue);
  fmt(buf,24,"Time:%us",msElapsed);         Draw::text(buf,250,7,Col::Cyan,Col::DarkBlue);

  int cw=msCols>12?18:MSCELL;
  int ch=msRows>10?16:MSCELL;

  for(int r=0;r<msRows;r++) for(int c=0;c<msCols;c++){
    int px=MSOX+c*cw, py=MSOY+r*ch;
    bool cur=(r==msCurR&&c==msCurC);
    if(msGrid[r][c]==1){
      EADK::Color bg=msMine[r][c]?Col::Red:EADK::Color(0xC0C0B0);
      Draw::fillRect(px,py,cw-1,ch-1,bg);
      if(msMine[r][c]) Draw::text("*",px+2,py+2,Col::White,bg);
      else if(msAdj[r][c]>0){char s[2]={'0'+(char)msAdj[r][c],0};Draw::text(s,px+4,py+2,msNumCol[msAdj[r][c]],bg);}
    } else {
      EADK::Color bg=cur?Col::Blue:EADK::Color(0x808080);
      Draw::fillRect(px,py,cw-1,ch-1,bg);
      Draw::drawRect(px,py,cw-1,ch-1,Col::LightGray);
      if(msGrid[r][c]==2) Draw::text("F",px+3,py+2,Col::Red,bg);
      if(cur) Draw::drawRect(px+1,py+1,cw-3,ch-3,Col::White);
    }
  }
  Draw::text("OK:Reveal 0:Flag Back:Quit",4,SH-10,Col::MidGray,Col::Black);
}

void runMinesweeper(){
  msRng=(uint32_t)EADK::Timing::millis()^0xB00B5BABU;
  const char* opts[]={"Easy  (9x9, 10 mines)","Medium(13x9,20 mines)","Hard  (16x14,40mines)"};
  int sel=runMenu(opts,3,0,"MINESWEEPER");
  if(sel<0) return;
  msDiff=sel;
  struct{int c,r,m;}cfgs[]={{9,9,10},{13,9,20},{16,14,40}};
  msCols=cfgs[sel].c; msRows=cfgs[sel].r; msMines=cfgs[sel].m;
  msSafeCnt=msCols*msRows-msMines;
  memset(msGrid,0,sizeof(msGrid)); memset(msMine,0,sizeof(msMine));
  msCurR=msRows/2; msCurC=msCols/2;
  msFlagCnt=0; msRevCnt=0; msFirst=true; msOver=false; msWon=false;
  msStart=EADK::Timing::millis(); msElapsed=0;

  Input::Edge inp; inp.reset(); int frame=0;
  bool prevZero=false;

  while(!msOver&&!msWon){
    msElapsed=(EADK::Timing::millis()-msStart)/1000;
    Input::Keys k=inp.updateRepeat(frame++);
    if(k.back) return;
    if(k.left  &&msCurC>0)       msCurC--;
    if(k.right &&msCurC<msCols-1)msCurC++;
    if(k.up    &&msCurR>0)       msCurR--;
    if(k.down  &&msCurR<msRows-1)msCurR++;
    if(k.ok&&msGrid[msCurR][msCurC]==0){
      if(msFirst){msPlace(msCurR,msCurC);msFirst=false;msStart=EADK::Timing::millis();}
      if(msMine[msCurR][msCurC]){
        for(int r=0;r<msRows;r++) for(int c=0;c<msCols;c++) if(msMine[r][c]) msGrid[r][c]=1;
        msOver=true;
      } else { msReveal(msCurR,msCurC); if(msRevCnt>=msSafeCnt) msWon=true; }
    }
    // 0 key = flag
    bool zeroNow=k.num[0];
    if(zeroNow&&!prevZero&&msGrid[msCurR][msCurC]!=1){
      if(msGrid[msCurR][msCurC]==2){msGrid[msCurR][msCurC]=0;msFlagCnt--;}
      else{msGrid[msCurR][msCurC]=2;msFlagCnt++;}
    }
    prevZero=zeroNow;
    msRender();
    EADK::Timing::msleep(50);
  }

  if(msWon){g_save.msWins[msDiff]++;if(msElapsed<g_save.msBest[msDiff])g_save.msBest[msDiff]=msElapsed;}
  else g_save.msLosses[msDiff]++;
  savePersist();

  msRender();
  Draw::fillRect(60,90,200,60,Col::DarkBlue);
  Draw::drawRect(60,90,200,60,msWon?Col::Gold:Col::Red);
  Draw::textCentered(msWon?"CLEARED!":"BOOM!",100,msWon?Col::Gold:Col::Red,Col::DarkBlue,true);
  char buf[32]; fmt(buf,32,"Time: %us",msElapsed); Draw::textCentered(buf,122,Col::White,Col::DarkBlue);
  Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
  Input::waitRelease(); inp.reset();
  while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
}
