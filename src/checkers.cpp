#include "shared.h"
// 1=Red(player) 2=RedKing 3=Black(AI) 4=BlackKing
static uint8_t ckB[8][8];
static int ckTurn,ckDiff,ckSR,ckSC,ckFR,ckFC;
struct CkMove{int fr,fc,tr,tc,cr,cc;bool isCap()const{return cr>=0;}};
static uint32_t ckRng;
static uint32_t ckRand(){ckRng^=ckRng<<13;ckRng^=ckRng>>17;ckRng^=ckRng<<5;return ckRng;}

static inline bool ckRed(uint8_t p){return p==1||p==2;}
static inline bool ckBlk(uint8_t p){return p==3||p==4;}
static inline bool ckKing(uint8_t p){return p==2||p==4;}

static int ckMovesFrom(int r,int c,CkMove* out,int max){
  uint8_t p=ckB[r][c]; if(!p) return 0;
  bool red=ckRed(p),king=ckKing(p);
  int dirs[4][2]={{-1,-1},{-1,1},{1,-1},{1,1}};
  int nd=king?4:2, sd=red?0:2;
  if(king) sd=0;
  int cnt=0,cc2=0;
  for(int d=sd;d<sd+nd&&d<4;d++){
    int dr=dirs[d][0],dc=dirs[d][1];
    int mr=r+dr,mc=c+dc,lr=r+2*dr,lc=c+2*dc;
    if(lr<0||lr>=8||lc<0||lc>=8) continue;
    uint8_t mid=ckB[mr][mc]; if(!mid) continue;
    bool enemy=red?ckBlk(mid):ckRed(mid);
    if(enemy&&!ckB[lr][lc]&&cnt<max) {out[cnt++]={r,c,lr,lc,mr,mc};cc2++;}
  }
  if(cc2) return cnt;
  for(int d=sd;d<sd+nd&&d<4;d++){
    int dr=dirs[d][0],dc=dirs[d][1],nr=r+dr,nc=c+dc;
    if(nr<0||nr>=8||nc<0||nc>=8) continue;
    if(!ckB[nr][nc]&&cnt<max) out[cnt++]={r,c,nr,nc,-1,-1};
  }
  return cnt;
}

static int ckAllMoves(int player,CkMove* out,int max){
  int cnt=0;bool hasCap=false;
  for(int r=0;r<8&&cnt<max;r++) for(int c=0;c<8&&cnt<max;c++){
    uint8_t p=ckB[r][c];if(!p) continue;
    bool mine=(player==1)?ckRed(p):ckBlk(p);if(!mine) continue;
    CkMove tmp[16];int tc=ckMovesFrom(r,c,tmp,16);
    for(int i=0;i<tc&&cnt<max;i++) if(tmp[i].isCap()){out[cnt++]=tmp[i];hasCap=true;}
  }
  if(hasCap) return cnt;
  for(int r=0;r<8&&cnt<max;r++) for(int c=0;c<8&&cnt<max;c++){
    uint8_t p=ckB[r][c];if(!p) continue;
    bool mine=(player==1)?ckRed(p):ckBlk(p);if(!mine) continue;
    CkMove tmp[8];int tc=ckMovesFrom(r,c,tmp,8);
    for(int i=0;i<tc&&cnt<max;i++) if(!tmp[i].isCap()){out[cnt++]=tmp[i];}
  }
  return cnt;
}

static uint8_t ckApply(CkMove& m){
  uint8_t piece=ckB[m.fr][m.fc],cap=0;
  ckB[m.tr][m.tc]=piece;ckB[m.fr][m.fc]=0;
  if(m.isCap()){cap=ckB[m.cr][m.cc];ckB[m.cr][m.cc]=0;}
  if(piece==1&&m.tr==0) ckB[m.tr][m.tc]=2;
  if(piece==3&&m.tr==7) ckB[m.tr][m.tc]=4;
  return cap;
}

static void ckUndo(CkMove& m,uint8_t cap){
  ckB[m.fr][m.fc]=ckB[m.tr][m.tc];ckB[m.tr][m.tc]=0;
  if(m.isCap()) ckB[m.cr][m.cc]=cap;
}

static int ckEval(){
  int s=0;
  for(int r=0;r<8;r++) for(int c=0;c<8;c++){
    uint8_t p=ckB[r][c];
    if(ckBlk(p)) s+=ckKing(p)?3:2;
    if(ckRed(p)) s-=ckKing(p)?3:2;
  } return s;
}

static int ckMinimax(int depth,int alpha,int beta,int player){
  CkMove moves[64];int cnt=ckAllMoves(player,moves,64);
  if(!depth||!cnt) return ckEval();
  int next=player==1?3:1;
  bool maxi=(player==3);int best=maxi?-1000:1000;
  for(int i=0;i<cnt;i++){
    uint8_t cap=ckApply(moves[i]);
    int v=ckMinimax(depth-1,alpha,beta,next);
    ckUndo(moves[i],cap);
    if(maxi){if(v>best)best=v;if(v>alpha)alpha=v;}
    else{if(v<best)best=v;if(v<beta)beta=v;}
    if(beta<=alpha) break;
  } return best;
}

static CkMove ckBestMove(){
  CkMove moves[64];int cnt=ckAllMoves(3,moves,64);
  if(!cnt) return {-1,-1,-1,-1,-1,-1};
  int depth=(ckDiff==0)?2:(ckDiff==1)?4:6;
  int best=-1000,bi=0;
  for(int i=0;i<cnt;i++){
    int si=(ckDiff==0)?(int)(ckRand()%cnt):i;
    uint8_t cap=ckApply(moves[si]);
    int v=ckMinimax(depth-1,-1000,1000,1);
    ckUndo(moves[si],cap);
    if(v>best||i==0){best=v;bi=si;}
  } return moves[bi];
}

static int ckCount(int pl){int n=0;for(int r=0;r<8;r++) for(int c=0;c<8;c++){uint8_t p=ckB[r][c];if(pl==1&&ckRed(p))n++;if(pl==3&&ckBlk(p))n++;}return n;}

static void ckRender(){
  Draw::fillScreen(Col::Black);
  for(int r=0;r<8;r++) for(int c=0;c<8;c++){
    int px=8+c*26,py=20+r*26;
    bool dark=(r+c)%2==1;
    Draw::fillRect(px,py,26,26,dark?EADK::Color(0x604020):EADK::Color(0xE8D098));
    uint8_t p=ckB[r][c];
    if(p){
      EADK::Color col=ckRed(p)?Col::Red:Col::DarkGray;
      EADK::Color brd=ckRed(p)?Col::Orange:Col::LightGray;
      Draw::fillCircle(px+13,py+13,10,col);
      Draw::drawRect(px+3,py+3,21,21,brd);
      if(ckKing(p)){char s[2]={'K',0};Draw::text(s,px+9,py+8,Col::Gold,col);}
    }
  }
  // Highlight valid moves
  if(ckFR>=0){
    CkMove tmp[16];int tc=ckMovesFrom(ckFR,ckFC,tmp,16);
    for(int i=0;i<tc;i++){
      int px=8+tmp[i].tc*26,py=20+tmp[i].tr*26;
      Draw::drawRect(px+1,py+1,24,24,tmp[i].isCap()?Col::Yellow:Col::Green);
    }
    Draw::drawRect(8+ckFC*26,20+ckFR*26,26,26,Col::Gold);
  }
  // Cursor
  Draw::drawRect(8+ckSC*26,20+ckSR*26,26,26,Col::White);
  Draw::drawRect(9+ckSC*26,21+ckSR*26,24,24,Col::White);

  int sx=8+8*26+6;
  char buf[24];
  fmt(buf,24,"Red: %d",ckCount(1)); Draw::text(buf,sx,22,Col::Red,Col::Black);
  fmt(buf,24,"Blk: %d",ckCount(3)); Draw::text(buf,sx,36,Col::LightGray,Col::Black);
  Draw::text(ckTurn==1?"YOUR TURN":"AI...",sx,52,ckTurn==1?Col::Green:Col::Orange,Col::Black);
  static const char* dn[]={"Easy","Med","Hard"};
  fmt(buf,24,"AI:%s",dn[ckDiff]); Draw::text(buf,sx,68,Col::MidGray,Col::Black);
  Draw::text("Arrow:Move",sx,90,Col::MidGray,Col::Black);
  Draw::text("OK:Select",sx,102,Col::MidGray,Col::Black);
  Draw::text("Back:Quit",sx,114,Col::MidGray,Col::Black);
}

void runCheckers(){
  ckRng=(uint32_t)EADK::Timing::millis()^0xC4EC4E5U;
  const char* opts[]={"Easy AI","Medium AI","Hard AI (Minimax)"};
  int sel=runMenu(opts,3,1,"CHECKERS");
  if(sel<0) return;
  ckDiff=sel;
  memset(ckB,0,sizeof(ckB));
  for(int r=0;r<8;r++) for(int c=0;c<8;c++) if((r+c)%2==1){if(r<=2)ckB[r][c]=3;if(r>=5)ckB[r][c]=1;}
  ckTurn=1;ckSR=5;ckSC=1;ckFR=-1;ckFC=-1;

  Input::Edge inp; inp.reset(); int frame=0;

  while(true){
    CkMove allM[64];int cnt=ckAllMoves(ckTurn,allM,64);
    if(!cnt) break;

    if(ckTurn==3){
      ckRender(); EADK::Timing::msleep(300);
      CkMove best=ckBestMove(); if(best.fr<0) break;
      ckApply(best);
      if(best.isCap()){CkMove caps[16];int nc=ckMovesFrom(best.tr,best.tc,caps,16);
        for(int i=0;i<nc;i++) if(caps[i].isCap()){EADK::Timing::msleep(200);ckApply(caps[i]);break;}}
      ckTurn=1; ckRender(); continue;
    }

    Input::Keys k=inp.updateRepeat(frame++);
    if(k.back) break;
    if(k.up    &&ckSR>0) ckSR--;
    if(k.down  &&ckSR<7) ckSR++;
    if(k.left  &&ckSC>0) ckSC--;
    if(k.right &&ckSC<7) ckSC++;
    if(k.ok){
      if(ckFR<0){if(ckRed(ckB[ckSR][ckSC])){ckFR=ckSR;ckFC=ckSC;}}
      else{
        CkMove tmp[16];int tc2=ckMovesFrom(ckFR,ckFC,tmp,16);
        bool moved=false;
        for(int i=0;i<tc2;i++) if(tmp[i].tr==ckSR&&tmp[i].tc==ckSC){
          ckApply(tmp[i]);
          if(tmp[i].isCap()){CkMove caps[16];int nc=ckMovesFrom(ckSR,ckSC,caps,16);
            bool more=false;for(int j=0;j<nc;j++) if(caps[j].isCap())more=true;
            if(more){ckFR=ckSR;ckFC=ckSC;moved=true;break;}}
          ckFR=-1;ckFC=-1;ckTurn=3;moved=true;break;
        }
        if(!moved){if(ckRed(ckB[ckSR][ckSC])){ckFR=ckSR;ckFC=ckSC;}else{ckFR=-1;ckFC=-1;}}
      }
    }
    ckRender(); EADK::Timing::msleep(50);
  }

  bool pWon=!ckCount(3)||(ckTurn==3&&!ckAllMoves(3,nullptr,0)>0);
  bool aWon=!ckCount(1)||(ckTurn==1&&!cnt>0);
  (void)pWon;(void)aWon;

  bool playerWon=(ckCount(3)==0);
  bool aiWon=(ckCount(1)==0);
  if(playerWon) g_save.chkWins[ckDiff]++;
  else if(aiWon) g_save.chkLosses++;
  else g_save.chkDraws++;
  savePersist();

  ckRender();
  Draw::fillRect(60,90,200,60,Col::DarkBlue);
  Draw::drawRect(60,90,200,60,playerWon?Col::Gold:Col::Red);
  Draw::textCentered(playerWon?"YOU WIN!":aiWon?"AI WINS":"DRAW",100,playerWon?Col::Gold:Col::White,Col::DarkBlue,true);
  Draw::textCentered("OK to continue",138,Col::LightGray,Col::DarkBlue);
  Input::waitRelease(); inp.reset();
  while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
}
