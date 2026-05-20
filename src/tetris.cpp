#include "shared.h"

// ---- Tetromino data ----
// 7 pieces x 4 rotations x 4 cells x (dx,dy)
static const int8_t k_pieces[7][4][4][2] = {
  // I
  {{{0,0},{1,0},{2,0},{3,0}},{{1,-1},{1,0},{1,1},{1,2}},
   {{0,1},{1,1},{2,1},{3,1}},{{2,-1},{2,0},{2,1},{2,2}}},
  // O
  {{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}},
   {{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}}},
  // T
  {{{0,0},{1,0},{2,0},{1,1}},{{1,-1},{1,0},{2,0},{1,1}},
   {{0,0},{1,0},{2,0},{1,-1}},{{1,-1},{0,0},{1,0},{1,1}}},
  // S
  {{{1,0},{2,0},{0,1},{1,1}},{{1,-1},{1,0},{2,0},{2,1}},
   {{1,0},{2,0},{0,1},{1,1}},{{1,-1},{1,0},{2,0},{2,1}}},
  // Z
  {{{0,0},{1,0},{1,1},{2,1}},{{2,-1},{1,0},{2,0},{1,1}},
   {{0,0},{1,0},{1,1},{2,1}},{{2,-1},{1,0},{2,0},{1,1}}},
  // J
  {{{0,0},{0,1},{1,1},{2,1}},{{1,-1},{2,-1},{1,0},{1,1}},
   {{0,0},{1,0},{2,0},{2,1}},{{1,-1},{1,0},{0,1},{1,1}}},
  // L
  {{{2,0},{0,1},{1,1},{2,1}},{{1,-1},{1,0},{1,1},{2,1}},
   {{0,0},{1,0},{2,0},{0,1}},{{0,-1},{1,-1},{1,0},{1,1}}}
};

static const EADK::Color k_tColors[8] = {
  Col::Black, Col::Cyan, Col::Yellow, Col::Purple,
  Col::Green, Col::Red,  Col::Blue,   Col::Orange
};

static constexpr int TC = 10, TR = 20, CS = 11;
static constexpr int BX = 55, BY = 18;

static uint8_t  board[TR][TC];
static int      pType, pRot, pX, pY;
static int      holdType, nextQ[3];
static bool     holdUsed;
static uint32_t tScore, tHigh;
static int      tLevel, tLines;
static uint32_t tRng;

static uint32_t tRand() {
  tRng ^= tRng<<13; tRng ^= tRng>>17; tRng ^= tRng<<5;
  return tRng;
}

static bool tCollide(int t, int r, int x, int y) {
  for (int i=0;i<4;i++){
    int cx=x+k_pieces[t][r][i][0], cy=y+k_pieces[t][r][i][1];
    if(cx<0||cx>=TC||cy>=TR) return true;
    if(cy>=0 && board[cy][cx]) return true;
  }
  return false;
}

static int tGhostY(){
  int gy=pY;
  while(!tCollide(pType,pRot,pX,gy+1)) gy++;
  return gy;
}

static void tSpawn(){
  pType=nextQ[0]; nextQ[0]=nextQ[1]; nextQ[1]=nextQ[2];
  nextQ[2]=tRand()%7;
  pRot=0; pX=3; pY=0; holdUsed=false;
}

static void tPlace(){
  for(int i=0;i<4;i++){
    int cx=pX+k_pieces[pType][pRot][i][0];
    int cy=pY+k_pieces[pType][pRot][i][1];
    if(cy>=0&&cy<TR&&cx>=0&&cx<TC) board[cy][cx]=(uint8_t)(pType+1);
  }
}

static int tClear(){
  int n=0;
  for(int r=TR-1;r>=0;r--){
    bool full=true;
    for(int c=0;c<TC;c++) if(!board[r][c]){full=false;break;}
    if(full){
      for(int rr=r;rr>0;rr--) memcpy(board[rr],board[rr-1],TC);
      memset(board[0],0,TC); r++; n++;
    }
  }
  return n;
}

static void tRotate(int d){
  int nr=(pRot+4+d)%4;
  if(!tCollide(pType,nr,pX,pY)){pRot=nr;return;}
  int kicks[]={1,-1,2,-2};
  for(int k:kicks) if(!tCollide(pType,nr,pX+k,pY)){pX+=k;pRot=nr;return;}
}

static void drawTCell(int c,int r,uint8_t ci,bool ghost=false){
  int px=BX+c*CS, py=BY+r*CS;
  EADK::Color col = ci==0?Col::DarkGray:k_tColors[ci];
  if(ghost){
    Draw::drawRect(px+1,py+1,CS-1,CS-1,col);
  } else {
    Draw::fillRect(px+1,py+1,CS-1,CS-1,col);
    if(ci) Draw::fillRect(px+1,py+1,CS-2,2,Col::White);
  }
}

static void drawMiniPiece(int t, int px, int py){
  if(t<0) return;
  for(int i=0;i<4;i++){
    int cx=px+k_pieces[t][0][i][0]*8;
    int cy=py+k_pieces[t][0][i][1]*8;
    Draw::fillRect(cx,cy,7,7,k_tColors[t+1]);
  }
}

static void tRender(bool paused){
  Draw::fillScreen(Col::Black);
  // Board bg
  Draw::fillRect(BX-1,BY-1,TC*CS+2,TR*CS+2,Col::DarkGray);
  Draw::drawRect(BX-1,BY-1,TC*CS+2,TR*CS+2,Col::MidGray);
  // Board cells
  for(int r=0;r<TR;r++) for(int c=0;c<TC;c++) drawTCell(c,r,board[r][c]);
  // Ghost
  int gy=tGhostY();
  if(gy!=pY) for(int i=0;i<4;i++){
    int gc=pX+k_pieces[pType][pRot][i][0];
    int gr=gy+k_pieces[pType][pRot][i][1];
    if(gr>=0) drawTCell(gc,gr,(uint8_t)(pType+1),true);
  }
  // Active piece
  for(int i=0;i<4;i++){
    int ac=pX+k_pieces[pType][pRot][i][0];
    int ar=pY+k_pieces[pType][pRot][i][1];
    if(ar>=0) drawTCell(ac,ar,(uint8_t)(pType+1));
  }
  // Sidebar
  int sx=BX+TC*CS+6;
  char buf[20];
  fmt(buf,20,"SC:%u",tScore);  Draw::text(buf,sx,20,Col::Gold,Col::Black);
  fmt(buf,20,"HI:%u",tHigh);   Draw::text(buf,sx,32,Col::Silver,Col::Black);
  fmt(buf,20,"LV:%d",tLevel);  Draw::text(buf,sx,44,Col::Cyan,Col::Black);
  fmt(buf,20,"LN:%d",tLines);  Draw::text(buf,sx,56,Col::Green,Col::Black);
  Draw::text("NEXT",sx,72,Col::LightGray,Col::Black);
  drawMiniPiece(nextQ[0],sx,84);
  drawMiniPiece(nextQ[1],sx,120);
  Draw::text("HOLD",4,20,Col::LightGray,Col::Black);
  drawMiniPiece(holdType,4,32);
  // Controls
  Draw::text("Up:Rot Dn:Drop OK:Hard",2,SH-11,Col::MidGray,Col::Black);
  if(paused){
    Draw::fillRect(80,90,160,50,Col::DarkBlue);
    Draw::drawRect(80,90,160,50,Col::Cyan);
    Draw::textCentered("PAUSED",102,Col::White,Col::DarkBlue,true);
    Draw::textCentered("OK:Resume",124,Col::LightGray,Col::DarkBlue);
  }
}

void runTetris() {
  memset(board,0,sizeof(board));
  tRng=(uint32_t)EADK::Timing::millis()^0xCAFEBABEU;
  holdType=-1; holdUsed=false;
  tScore=0; tLevel=1; tLines=0;
  tHigh=g_save.tetrisHigh;
  for(int i=0;i<3;i++) nextQ[i]=tRand()%7;
  tSpawn();

  Input::Edge inp; inp.reset();
  bool paused=false, over=false;
  int dropTimer=0, dropInterval=40, lockDelay=0;
  int frame=0;

  while(!over){
    Input::Keys k=inp.updateRepeat(frame++);

    if(paused){
      tRender(true);
      if(k.ok){paused=false; inp.reset();}
      if(k.back) return;
      EADK::Timing::msleep(16); continue;
    }

    if(k.back){paused=true; inp.reset(); continue;}
    if(k.left  && !tCollide(pType,pRot,pX-1,pY)) pX--;
    if(k.right && !tCollide(pType,pRot,pX+1,pY)) pX++;
    if(k.up)   tRotate(1);
    if(k.down && !tCollide(pType,pRot,pX,pY+1)){pY++;tScore++;dropTimer=0;}

    // Hard drop
    if(k.ok){
      int gy=tGhostY(); tScore+=(gy-pY)*2; pY=gy;
      tPlace();
      int cl=tClear();
      static const uint32_t ls[]={0,100,300,500,800};
      if(cl>0){tScore+=ls[cl<5?cl:4]*(uint32_t)tLevel;tLines+=cl;
        tLevel=1+tLines/10; dropInterval=(40-tLevel*3)<5?5:(40-tLevel*3);}
      tSpawn();
      if(tCollide(pType,pRot,pX,pY)) over=true;
    }

    // Auto drop
    if(++dropTimer>=dropInterval){
      dropTimer=0;
      if(!tCollide(pType,pRot,pX,pY+1)) pY++;
      else if(++lockDelay>=30){
        tPlace();
        int cl=tClear();
        static const uint32_t ls2[]={0,100,300,500,800};
        if(cl>0){tScore+=ls2[cl<5?cl:4]*(uint32_t)tLevel;tLines+=cl;
          tLevel=1+tLines/10; dropInterval=(40-tLevel*3)<5?5:(40-tLevel*3);}
        tSpawn(); lockDelay=0;
        if(tCollide(pType,pRot,pX,pY)) over=true;
      }
    }

    if(tScore>tHigh) tHigh=tScore;
    tRender(false);
    EADK::Timing::msleep(16);
  }

  // Save
  if(tScore>g_save.tetrisHigh) g_save.tetrisHigh=tScore;
  g_save.tetrisGames++; g_save.tetrisLines+=tLines;
  if(tLevel>(int)g_save.tetrisMaxLevel) g_save.tetrisMaxLevel=(uint8_t)tLevel;
  savePersist();

  // Game over screen
  Draw::fillRect(70,90,180,55,Col::DarkBlue);
  Draw::drawRect(70,90,180,55,Col::Red);
  Draw::textCentered("GAME OVER",100,Col::White,Col::DarkBlue,true);
  char buf[24]; fmt(buf,24,"Score:%u",tScore);
  Draw::textCentered(buf,124,Col::Gold,Col::DarkBlue);
  Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
  Input::waitRelease(); inp.reset();
  while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
}
