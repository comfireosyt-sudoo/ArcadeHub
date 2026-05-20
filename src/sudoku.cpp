#include "shared.h"

static uint8_t sdPuzzle[81], sdSolution[81], sdPlayer[81], sdNotes[81];
static bool    sdGiven[81], sdErrors[81];
static int     sdRow, sdCol, sdDiff;
static bool    sdNoteMode;
static uint32_t sdRng;

static uint32_t sdRand(){sdRng^=sdRng<<13;sdRng^=sdRng>>17;sdRng^=sdRng<<5;return sdRng;}

static bool sdValid(uint8_t g[81],int idx,int v){
  int r=idx/9,c=idx%9,br=(r/3)*3,bc=(c/3)*3;
  for(int i=0;i<9;i++){
    if(g[r*9+i]==v||g[i*9+c]==v||g[(br+i/3)*9+bc+i%3]==v) return false;
  } return true;
}
static bool sdSolve(uint8_t g[81],int idx){
  if(idx==81) return true;
  if(g[idx]) return sdSolve(g,idx+1);
  for(int v=1;v<=9;v++) if(sdValid(g,idx,v)){g[idx]=v;if(sdSolve(g,idx+1))return true;g[idx]=0;}
  return false;
}
static int sdCountSol(uint8_t g[81],int idx,int lim){
  if(idx==81) return 1;
  if(g[idx]) return sdCountSol(g,idx+1,lim);
  int cnt=0;
  for(int v=1;v<=9&&cnt<lim;v++) if(sdValid(g,idx,v)){g[idx]=v;cnt+=sdCountSol(g,idx+1,lim-cnt);g[idx]=0;}
  return cnt;
}

static void sdGenerate(int clues){
  memset(sdSolution,0,81);
  // Fill diagonal boxes
  for(int box=0;box<3;box++){
    uint8_t nums[9]={1,2,3,4,5,6,7,8,9};
    for(int i=8;i>0;i--){int j=sdRand()%(i+1);uint8_t t=nums[i];nums[i]=nums[j];nums[j]=t;}
    int br=box*3,bc=box*3;
    for(int i=0;i<9;i++) sdSolution[(br+i/3)*9+bc+i%3]=nums[i];
  }
  sdSolve(sdSolution,0);
  // Dig holes
  memcpy(sdPuzzle,sdSolution,81);
  int order[81]; for(int i=0;i<81;i++) order[i]=i;
  for(int i=80;i>0;i--){int j=sdRand()%(i+1);int t=order[i];order[i]=order[j];order[j]=t;}
  int toRemove=81-clues;
  for(int k=0;k<81&&toRemove>0;k++){
    int idx=order[k]; uint8_t bak=sdPuzzle[idx]; sdPuzzle[idx]=0;
    uint8_t tmp[81]; memcpy(tmp,sdPuzzle,81);
    if(sdCountSol(tmp,0,2)!=1) sdPuzzle[idx]=bak; else toRemove--;
  }
}

static void sdCheckErrors(){
  for(int i=0;i<81;i++) sdErrors[i]=(sdPlayer[i]!=0&&sdPlayer[i]!=sdSolution[i]);
}
static bool sdIsSolved(){
  for(int i=0;i<81;i++) if(sdPlayer[i]!=sdSolution[i]) return false;
  return true;
}

static constexpr int CS=24,GX=4,GY=22;

static void sdDrawCell(int r,int c){
  int px=GX+c*CS, py=GY+r*CS, idx=r*9+c;
  bool sel=(r==sdRow&&c==sdCol);
  EADK::Color bg = sel?Col::Blue : sdErrors[idx]?EADK::Color(0x400000) : sdGiven[idx]?Col::DarkGray:Col::Black;
  Draw::fillRect(px+1,py+1,CS-1,CS-1,bg);
  if(sdPlayer[idx]){
    EADK::Color fg=sdGiven[idx]?Col::White:sdErrors[idx]?Col::Red:Col::Cyan;
    char s[2]={'0'+(char)sdPlayer[idx],0};
    Draw::textLarge(s,px+6,py+4,fg,bg);
  } else if(sdNotes[idx]){
    for(int n=0;n<9;n++) if(sdNotes[idx]&(1<<n)){
      char s[2]={'1'+(char)n,0};
      Draw::text(s,px+2+(n%3)*7,py+2+(n/3)*7,Col::LightGray,bg);
    }
  }
}

static void sdRender(){
  Draw::fillScreen(Col::Black);
  // Draw cells
  for(int r=0;r<9;r++) for(int c=0;c<9;c++) sdDrawCell(r,c);
  // Grid lines
  for(int i=0;i<=9;i++){
    bool thick=(i%3==0); int lw=thick?2:1;
    EADK::Color lc=thick?Col::White:Col::MidGray;
    Draw::fillRect(GX+i*CS-(thick?1:0),GY,lw,9*CS,lc);
    Draw::fillRect(GX,GY+i*CS-(thick?1:0),9*CS,lw,lc);
  }
  // Sidebar
  int sx=GX+9*CS+6;
  Draw::text(sdNoteMode?"MODE:NOTE":"MODE:FILL",sx,GY,sdNoteMode?Col::Yellow:Col::Green,Col::Black);
  Draw::text("OK:Mode",sx,GY+24,Col::MidGray,Col::Black);
  Draw::text("0:Clear",sx,GY+36,Col::MidGray,Col::Black);
  Draw::text("1-9:Fill",sx,GY+48,Col::MidGray,Col::Black);
  Draw::text("Back:Quit",sx,GY+60,Col::MidGray,Col::Black);
  // Header
  Draw::fillRect(0,0,SW,20,Col::DarkBlue);
  static const char* dn[]={"Easy","Medium","Hard"};
  char buf[24]; fmt(buf,24,"SUDOKU - %s",sdDiff>=0&&sdDiff<3?dn[sdDiff]:"?");
  Draw::textCentered(buf,2,Col::Gold,Col::DarkBlue);
}

void runSudoku(){
  sdRng=(uint32_t)EADK::Timing::millis()^0xBEEFCAFEU;
  sdRow=0; sdCol=0; sdNoteMode=false;

  // Difficulty select
  const char* opts[]={"Easy (36 clues)","Medium (28 clues)","Hard (22 clues)","Resume Saved"};
  bool hasSave=g_save.sudokuHasSave;
  int nopts=hasSave?4:3;
  int sel=runMenu(opts,nopts,0,"SUDOKU");
  if(sel<0) return;

  if(sel==3&&hasSave){
    memcpy(sdPuzzle,  g_save.sudokuPuzzle,   81);
    memcpy(sdSolution,g_save.sudokuSolution,  81);
    memcpy(sdPlayer,  g_save.sudokuPuzzle,    81);
    memset(sdNotes,0,81);
    for(int i=0;i<81;i++) sdGiven[i]=(sdPuzzle[i]!=0);
    sdDiff=g_save.sudokuDiff;
  } else {
    sdDiff=sel;
    int clues[]={36,28,22};
    sdGenerate(clues[sdDiff]);
    memcpy(sdPlayer,sdPuzzle,81);
    memset(sdNotes,0,81);
    for(int i=0;i<81;i++) sdGiven[i]=(sdPuzzle[i]!=0);
  }
  memset(sdErrors,0,81);

  Input::Edge inp; inp.reset();
  int prevNum=-1;

  while(true){
    Input::Keys k=inp.update();
    if(k.up    && sdRow>0) sdRow--;
    if(k.down  && sdRow<8) sdRow++;
    if(k.left  && sdCol>0) sdCol--;
    if(k.right && sdCol<8) sdCol++;
    if(k.ok) sdNoteMode=!sdNoteMode;
    if(k.back){
      memcpy(g_save.sudokuPuzzle,  sdPuzzle,   81);
      memcpy(g_save.sudokuSolution,sdSolution,  81);
      g_save.sudokuHasSave=true; g_save.sudokuDiff=(uint8_t)sdDiff;
      savePersist(); return;
    }
    // Number keys
    int num=-1;
    for(int i=0;i<=9;i++) if(k.num[i]){num=i;break;}
    if(num>=0&&num!=prevNum&&!sdGiven[sdRow*9+sdCol]){
      int idx=sdRow*9+sdCol;
      if(sdNoteMode&&num>0) sdNotes[idx]^=(1<<(num-1));
      else{ sdPlayer[idx]=(uint8_t)num; sdNotes[idx]=0; sdCheckErrors(); }
      prevNum=num;
    } else if(num<0) prevNum=-1;

    sdRender();

    if(sdIsSolved()){
      Draw::fillRect(50,80,220,70,Col::DarkBlue);
      Draw::drawRect(50,80,220,70,Col::Gold);
      Draw::textCentered("SOLVED!",90,Col::Gold,Col::DarkBlue,true);
      static const char* dn2[]={"Easy","Medium","Hard"};
      char buf[32]; fmt(buf,32,"Difficulty: %s",sdDiff>=0&&sdDiff<3?dn2[sdDiff]:"?");
      Draw::textCentered(buf,116,Col::White,Col::DarkBlue);
      Draw::textCentered("OK to exit",134,Col::LightGray,Col::DarkBlue);
      if(sdDiff>=0&&sdDiff<3) g_save.sudokuDone[sdDiff]++;
      g_save.sudokuHasSave=false; savePersist();
      Input::waitRelease(); inp.reset();
      while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
      return;
    }
    EADK::Timing::msleep(50);
  }
}
