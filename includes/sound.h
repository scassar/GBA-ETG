//Sound definitions
#define REG_SOUNDCNT_X  *(volatile u16*)0x4000084 //Extended sound control
#define REG_SOUNDCNT_L  *(volatile u16*)0x4000080 //DMG sound control
#define REG_SOUNDCNT_H  *(volatile u16*)0x4000082 //Direct sound control
#define REG_SOUND3CNT_L *(volatile u16*)0x4000070
#define SOUND3BANK32    0x0000	                  //Use two banks of 32 steps each
#define SOUND3SETBANK1  0x0040
#define REG_WAVE_RAM0   *(volatile u32*)0x4000090 
#define REG_WAVE_RAM1   *(volatile u32*)0x4000094 
#define REG_WAVE_RAM2   *(volatile u32*)0x4000098 
#define REG_WAVE_RAM3   *(volatile u32*)0x400009C 
#define SOUND3SETBANK0  0x0000	                  //Bank to play 0 or 1 (non set bank is written to)
#define SOUND3PLAY	    0x0080	                  //Output sound
#define REG_SOUND3CNT_H *(volatile u16*)0x4000072 
#define SOUND3OUTPUT1   0x2000	                  //Output unmodified
#define REG_SOUND3CNT_X *(volatile u16*)0x4000074 
#define SOUND3INIT	    0x8000	                  //Makes the sound restart
#define SOUND3PLAYONCE  0x4000	                  //Play sound once
int lastFr=0,FPS=0;    
#define REG_TM2D       *(volatile u16*)0x4000108

//Music struct
typedef struct
{
 u16* song;
 int tic;
 int spd;
 int size;
 int onOff;
}Music; Music M[5];

//Music notes for the song
u16 notes[] = 
{
   44, 157,  263, 363,  458,  547, 631,  711, 786,  856, 923,  986,//C2,C2#, D2,D2#, E2, F2,F2#, G2,G2#, A2,A2#, B2 
 1046,1102, 1155,1205, 1253, 1297,1340, 1379,1417, 1452,1486, 1517,//C3,C3#, D3,D3#, E3, F3,F3#, G3,G3#, A3,A3#, B3 	
 1547,1575, 1602,1627, 1650, 1673,1694, 1714,1732, 1750,1767, 1783,//C4,C4#, D4,D4#, E4, F4,F4#, G4,G4#, A4,A4#, B4
 1798,1812, 1825,1837, 1849, 1860,1871, 1881,1890, 1899,1907, 1915,//C5,C5#, D5,D5#, E5, F5,F5#, G5,G5#, A5,A5#, B5
 1923,1930, 1936,1943, 1949, 1954,1959, 1964,1969, 1974,1978, 1982,//C6,C6#, D6,D6#, E6, F6,F6#, G6,G6#, A6,A6#, B6
 1985,1989, 1992,1995, 1998, 2001,2004, 2006,2009, 2011,2013 ,2015,//C7,C7#, D7,D7#, E7, F7,F7#, G7,G7#, A7,A7#, B7
};


u16 song_1[]={ 10,0,10, 0, 3,2,3,2, 7,22,0,8, 9,24,0,12};          //Unused
u16 song_2[]={ 2,2,0,0, 3,7,5,0, 4,0,4,0, 4,4,6,10};               //game song
u16 song_3[]={ 60,58,56,54, 52,50,48,46, 44,42,40,38, 36,34,32,30};  //lose condition
u16 song_4[]={ 30,34,36,35,32,36,32,35,32,27,30,25,0,0,0,0};      //attempted godfather home theme
u16 song_5[]={ 40,0,50,51,51,51,51,0,0,0,0,0,0,0,0,0};      //winning sound


void PlayNote( u16 frequency, unsigned char length ){

	unsigned char adjustedLength = 0xFF - length;

	REG_SOUNDCNT_X= 0x80;
	REG_SOUNDCNT_L=0xFF77;

	REG_SOUNDCNT_H = 2;

	REG_SOUND3CNT_L = SOUND3BANK32 | SOUND3SETBANK1;
	REG_WAVE_RAM0=0x10325476;
	REG_WAVE_RAM1=0x98badcfe;
	REG_WAVE_RAM2=0x10325476;
	REG_WAVE_RAM3=0x98badcfe;
	REG_SOUND3CNT_L = SOUND3BANK32 | SOUND3SETBANK0;

	REG_SOUND3CNT_L |= SOUND3PLAY;

	REG_SOUND3CNT_H = SOUND3OUTPUT1 | adjustedLength;
	REG_SOUND3CNT_X=SOUND3INIT|SOUND3PLAYONCE| frequency ;
}

void playSong(int s, int loop)
{
 if(FPS%M[s].spd==0 && M[s].onOff==1) 
 {
  int note=M[s].song[M[s].tic];
  
  if(note>0) { 
    PlayNote(notes[note],64);
    }
  
  M[s].tic+=1; 

  if(M[s].tic>M[s].size) { 
    
    M[s].tic=0; 
    if(loop==0){
         M[s].onOff=0;}
    }
 }
}//-----------------------------------------------------------------------------
