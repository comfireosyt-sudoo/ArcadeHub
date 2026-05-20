#include "shared.h"

static constexpr int BBC=8,BBR=8,BBCELL=24,BBOX=10,BBOY=24;
static uint8_t bbGrid[BBR][BBC];
static uint32_t bbScore,bbHigh;

struct BBPiece{int8_t cells[5][2];int count;uint8_t color;};
static BBPiece bbPieces[3];
static int bbSel,bbPR,bbPC;
static uint32_t bbRng;

static const EADK::Color bbCol[]={Col::Black,Col::Red,Col::Blue,Col::Green,Col::Yellow,Col::Purple};

static const int8_t bbShapes[10][5][2]={
  {{0,0},{0,1},{0,2},{0,3},{0,4}}, // I5
  {{0,0},{0,1},{0,2},{0,3},{0,0}}, // I4
  {{0,0},{0,1},{0,2},{0,0},{0,0}}, // I3
  {{0,0},{1,0},{0,1},{1,1},{0,0}}, // 2x2
  {{0,0},{0,1},{0,2},{1,0},{0,0}}, // L
  {{0,0},{0,1},{0,2},{1,2},{0,0}}, // J
  {{0,0},{0,1},{1,1},{1,2},{0,0}}, // S
  {{0,1},{0,2},{1,0},{1,1},{0,0}}, // Z
  {{0,0},{0,1},{0,2},{1,1},{0,0}}, // T
  {{0,0},{1,0},{2,0},{2,1},{0,0}}  // corner
};
static const int bbSizes[10]={5,4,3,4,4,4,4,4,4,4};

static uint32_t bbRand(){bbRng^=bbRng<<13;bbRng^=bbRng>>17;bbRng^=bbRng<<5;return bbRng;}

static void bbGenPiece(BBPiece& p){
  int t=(int)(bbRand()%10);
  p.count=bbSizes[t]; p.color=(uint8_t)(1+bbRand()%5);
  for(int i=0;i<5;i++){p.cells[i][0]=bbShapes[t][i][0];p.cells[i][1]=bbShapes[t][i][1];}
}

static void bbRefill(){for(int i=0;i<3;i++) bbGenPiece(bbPieces[i]);}

static bool bbCan(const BBPiece& p,int r,int c){
  for(int i=0;i<p.count;i++){int rr=r+p.cells[i][0],cc=c+p.cells[i][1];if(rr<0||rr>=BBR||cc<0||cc>=BBC||bbGrid[rr][cc]) return false;}return true;
}

static void bbPlace(const BBPiece& p,int r,int c){
  for(int i=0;i<p.count;i++) bbGrid[r+p.cells[i][0]][c+p.cells[i][1]]=p.color;
}

static int bbClear(){
  int n=0;
  for(int r=0;r<BBR;r++){bool f=true;for(int c=0;c<BBC;c++) if(!bbGrid[r][c]){f=false;break;}if(f){memset(bbGrid[r],0,BBC);n++;}}
  for(int c=0;c<BBC;c++){bool f=true;for(int r=0;r<BBR;r++) if(!bbGrid[r][c]){f=false;break;}if(f){for(int r=0;r<BBR;r++) bbGrid[r][c]=0;n++;}}
  return n;
}

static bool bbAny(){
  for(int pi=0;pi<3;pi++) if(bbPieces[pi].count>0) for(int r=0;r<BBR;r++) for(int c=0;c<BBC;c++) if(bbCan(bbPieces[pi],r,c)) return true;
  return false;
}

static void bbRender(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,22,Col::DarkBlue); Draw::drawRect(0,0,SW,22,Col::Blue);
  Draw::text("BLOCK BLAST",4,4,Col::Gold,Col::DarkBlue);
  char buf[24]; fmt(buf,24,"Score:%u",bbScore); Draw::text(buf,180,6,Col::White,Col::DarkBlue);
  // Grid
  Draw::fillRect(BBOX-1,BBOY-1,BBC*BBCELL+2,BBR*BBCELL+2,Col::DarkGray);
  for(int r=0;r<BBR;r++) for(int c=0;c<BBC;c++){
    int px=BBOX+c*BBCELL,py=BBOY+r*BBCELL;
    Draw::fillRect(px+1,py+1,BBCELL-1,BBCELL-1,bbGrid[r][c]?bbCol[bbGrid[r][c]]:Col::Black);
    Draw::drawRect(px,py,BBCELL,BBCELL,Col::DarkGray);
  }
  // Preview placement
  if(bbSel>=0&&bbPieces[bbSel].count>0){
    bool valid=bbCan(bbPieces[bbSel],bbPR,bbPC);
    for(int i=0;i<bbPieces[bbSel].count;i++){
      int r=bbPR+bbPieces[bbSel].cells[i][0],c=bbPC+bbPieces[bbSel].cells[i][1];
      if(r>=0&&r<BBR&&c>=0&&c<BBC)
        Draw::fillRect(BBOX+c*BBCELL+1,BBOY+r*BBCELL+1,BBCELL-1,BBCELL-1,valid?bbCol[bbPieces[bbSel].color]:Col::Red);
    }
    Draw::drawRect(BBOX+bbPC*BBCELL,BBOY+bbPR*BBCELL,BBCELL+1,BBCELL+1,Col::White);
  }
  // Piece slots
  int sx=BBOX+BBC*BBCELL+6;
  Draw::text("Pieces:",sx,BBOY,Col::LightGray,Col::Black);
  for(int pi=0;pi<3;pi++){
    int py=BBOY+16+pi*52;
    bool s=(pi==bbSel);
    Draw::fillRect(sx,py,52,46,s?Col::DarkBlue:Col::DarkGray);
    Draw::drawRect(sx,py,52,46,s?Col::Gold:Col::MidGray);
    if(bbPieces[pi].count>0) for(int i=0;i<bbPieces[pi].count;i++){
      int cx=sx+4+bbPieces[pi].cells[i][1]*10,cy=py+4+bbPieces[pi].cells[i][0]*10;
      Draw::fillRect(cx,cy,9,9,bbCol[bbPieces[pi].color]);
    }
  }
  char buf2[20]; fmt(buf2,20,"Hi:%u",bbHigh); Draw::text(buf2,sx,BBOY+180,Col::Silver,Col::Black);
  Draw::text("1/2/3:Piece",sx,SH-22,Col::MidGray,Col::Black);
  Draw::text("OK:Place",sx,SH-11,Col::MidGray,Col::Black);
}

void runBlockBlast(){
  bbRng=(uint32_t)EADK::Timing::millis()^0xCACAFEEDU;
  bbHigh=g_save.bbHigh; bbScore=0; bbSel=0; bbPR=0; bbPC=0;
  memset(bbGrid,0,sizeof(bbGrid)); bbRefill();

  Input::Edge inp; inp.reset(); int frame=0;

  while(true){
    Input::Keys k=inp.updateRepeat(frame++);
    if(k.back) break;
    if(k.up    &&bbPR>0)       bbPR--;
    if(k.down  &&bbPR<BBR-1)   bbPR++;
    if(k.left  &&bbPC>0)       bbPC--;
    if(k.right &&bbPC<BBC-1)   bbPC++;
    if(k.num[1]) bbSel=0;
    if(k.num[2]) bbSel=1;
    if(k.num[3]) bbSel=2;
    if(k.ok&&bbSel>=0&&bbPieces[bbSel].count>0){
      if(bbCan(bbPieces[bbSel],bbPR,bbPC)){
        bbPlace(bbPieces[bbSel],bbPR,bbPC);
        bbScore+=bbPieces[bbSel].count*10;
        int cl=bbClear();
        static const int bonus[]={0,100,300,600,1000};
        if(cl>0) bbScore+=(cl<5?bonus[cl]:1000);
        bbPieces[bbSel].count=0;
        bool allUsed=true; for(int i=0;i<3;i++) if(bbPieces[i].count>0){allUsed=false;break;}
        if(allUsed) bbRefill();
        bbSel=-1; for(int i=0;i<3;i++) if(bbPieces[i].count>0){bbSel=i;break;}
        if(!bbAny()) break;
      }
    }
    if(bbScore>bbHigh) bbHigh=bbScore;
    bbRender();
    EADK::Timing::msleep(50);
  }
  if(bbScore>g_save.bbHigh) g_save.bbHigh=bbScore;
  g_save.bbGames++; savePersist();
  bbRender();
  Draw::fillRect(70,90,180,55,Col::DarkBlue); Draw::drawRect(70,90,180,55,Col::Red);
  Draw::textCentered("GAME OVER",100,Col::White,Col::DarkBlue,true);
  char buf[24]; fmt(buf,24,"Score:%u",bbScore); Draw::textCentered(buf,124,Col::Gold,Col::DarkBlue);
  Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
  Input::waitRelease(); inp.reset();
  while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
}
