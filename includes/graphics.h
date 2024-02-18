#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdint.h>
#include <stdbool.h>
#include "../includes/characters.h"
#include <stdlib.h>
#include "../includes/images.h"

#define SCREENBUFFER ((unsigned short*)VRAM)
#define NUM_CHARS_LINE 10
#define SCREEN_W 240
#define SCREEN_H 160
#define REG_DISPTCNT *((unsigned int*)(0x04000000))
#define VIDEOMODE_3 0x0003
#define BG_ENABLE2 0x0400
#define NUM_ENEMIES 4

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t s16; 
typedef int32_t s32; 

typedef volatile uint8_t vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile int8_t   vs8;
typedef volatile int16_t vs16; 
typedef volatile int32_t vs32; 

struct rect { 
    s32 x;
    s32 y;
    s32 prevX; 
    s32 prevY;
    s32 width; 
    s32 height; 
    u16 colour;
    int speed; 
};


struct gameInstance { 

    int game_running;
    int difficulty;
    int menuPosition; 
    int menuStartX; 
    int menuStartY;
    int maxMenuPositions; 
    int menuOpen;
    int gameScore; 
    int scoreX; 
    int scoreY;
    int pauseCounter;
    int settingMenu;
    int musicRestart;
    int winScore;
    int highScore;
    bool paused;
    struct rect enemies[2];

};

//Function: Return back an unsigned 16 byte value for the colour. On the GBA, this is 5 bits per colour and 1 alpha bit 
// Passing in a int means that the values can be something of 16 bits. However, the max value would be 31 in int because this would be 00001111
// We & this with 0x1F because this is the same as anding it with 00011111 to return the bottom 5 bots
u16 setColour(u8 a_red, u8 a_green, u8 a_blue) 
{ 
    return (a_red & 0x1F) | (a_green & 0x1F) << 5 | (a_blue & 0x1F) << 10;

}

void plotPixel(u16 a_x, u16 a_y, u16 a_colour)
{
    SCREENBUFFER[a_x+a_y*SCREEN_W] = a_colour;
}

void drawLine(s32 a_x, s32 a_y, s32 a_x2, s32 a_y2, u16 a_colour) { 

  //printf("(%2d, %2d) to (%2d, %2d): ", x0, y0, x1, y1);
  int dx = abs(a_x2 - a_x), sx = a_x < a_x2 ? 1 : -1;
  int dy = abs(a_y2 - a_y), sy = a_y < a_y2 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;

  for (;;) {
    // setPixel(x0,y0,Matrix);
   //printf("(%d,%d), ", x0, y0);
    plotPixel(a_x, a_y, a_colour);
    if (a_x == a_x2 && a_y == a_y2)
      break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      a_x += sx;
    }
    if (e2 < dy) {
      err += dx;
      a_y += sy;
    }
  }
}

//Understand how the background renders as this clobbers drawings that follow this. 
void clearScreen() { 
    s32 x,y; 
    for (x = 0; x< SCREEN_W; ++x) {      //Loop through the screen memory in bitmap mode pixel by pixel
       for(y=0; y< SCREEN_H; ++y) {
            plotPixel(x,y, setColour(0,0,0));
        }
    }
}

// print the passed in characters

void printChar(bool values[64], int x, int y, u16 colour) { 

  for (int i = 0; i < 8; i++) { 
    for (int j = 0; j < 8; j++)  // Loop each value in a row
    { 
      u16 DispColour = setColour(0,0,0); 
      // Now check if the value is one

      if (values[i*8+j]) { 
       DispColour = colour;   //Draw the pixel
      }
       
       plotPixel(x+j, y+i, DispColour);
     }
  }
}

void displayScore(int value, int x, int y, u16 colour) { 

      
    if (value < 10) { 
      printChar(score[0], x, y,colour);
      printChar(score[value], x+8, y,colour);
    }
    else if(value >= 10 && value < 100) { 
      // Get the tens place
      int tens = value / 10;
      int ones = value % 10;
      printChar(score[tens], x, y,colour);
      printChar(score[ones], x+8, y,colour);
    }
     else if(value >= 100) { 
      // Get the tens place
      int hundreds = value / 100;
      int tens = value / 10;
      int ones = value % 10;
      printChar(score[hundreds], x, y,colour);
      printChar(score[tens], x+8, y,colour);
      printChar(score[ones], x+16, y,colour);
    }
    
    
    
}

//Clear where the previous object was 
void clearPrevious(struct rect *player) { 

    for (int i = player->prevX; i < player->prevX+ player->width; i++) { 
        for (int j = player->prevY;j < player->prevY + player->height; j++) { 
            plotPixel(i, j, setColour(0,0,0));
        }
    }
}

//Clear behind the enemy

void clearPreviousEnemy(struct rect *enemy) { 

    int speed = enemy->speed;

    for (int i = 0; i<enemy->width; i++) { 
      for(int j = 0; j < speed; j++) { 
        plotPixel(enemy->prevX+i, enemy->prevY+j, setColour(0,0,0));
      }
    }

}


//Will be used to create the initial enemies coming towards you
//Accepts a memory locations of a struct to be passed in

void drawRect(struct rect *rectangle)
{
    
    s32 rect_y,rect_x,width,height; 
    u16 colour;

    rect_y=rectangle->y;
    rect_x=rectangle->x; 
    width=rectangle->width;
    height=rectangle->height; 
    colour= rectangle->colour;
    
    
    for (s32 y = 0; y<height; ++y) { 
        for (s32 x = 0; x<width; ++x) { 
            plotPixel(rect_x+x, rect_y+y, colour);
        }
    }

}

//This function will draw the boundary lines for the road
//It will also draw the actual road
void drawRoad() { 

  int left_line_x,right_line_x;
  u16 colour;


  left_line_x = SCREEN_W/4;
  right_line_x = SCREEN_W/4 * 3;
  colour = setColour(31,31,31);
  
  drawLine(left_line_x, 0, left_line_x, 160, colour);
  drawLine(right_line_x, 0, right_line_x, 160, setColour(31,31,31));
  
  //Grass is the issue

  int right_grass_size = SCREEN_W - right_line_x;    //Difference between edge of screen and line of the road. 240 - 160 = 40 pixels
  int left_grass_size = left_line_x;                //Difference between edge of screen and line of the road. 240 - 160 = 40 pixels

  u16 grass_colour =  setColour(0,31,10);

  for (int i = 1; i < right_grass_size; i++) {        //40 pixels of X across 
    for (int j = 0; j < SCREEN_H; j++) { 
      plotPixel(right_line_x+i,j,grass_colour);
    }
  }

   for (int i = 0; i < left_grass_size; i++) {        //40 pixels of X across 
    for (int j = 0; j < SCREEN_H; j++) { 
      plotPixel(i,j,grass_colour);
    }
  }

 

}

void drawLane() { 

  u16 colour = setColour(31,31,31);
  int center_line_x = SCREEN_W/2;
    //draw 4 lines per screen
  for (int i = 0; i < 5; i++) { 
     drawLine(center_line_x, i*30+20, center_line_x, i * 30 + 30, colour);
  }

}

//Function to draw the scoreboard on the screen. Passing in the current game score
void drawScore(struct gameInstance *game, int scoreX, int scoreY) {  

  u16 colour = setColour(31,31,31); 

  displayScore(game->gameScore, scoreX, scoreY,colour);
  
}

//This function will draw the car model
//We can make use of the print char command for now 
void drawPlayerModel(struct rect *player) { 

  printChar(images[0],player->x, player->y, player->colour);

}

void drawEnemyModel(struct rect *player) { 

  printChar(images[1],player->x, player->y,player->colour);

}


//Keep track of player location and draw
void drawPlayer(struct rect *player) { 

  //Clear player region for the previous coordinates
  clearPrevious(player);
  drawRect(player);
  drawPlayerModel(player);
  


}

//Code to draw through the loop of enemies.
//Can confirm this is working
void drawEnemies(struct gameInstance *game) { 

   for (int i = 0; i< NUM_ENEMIES; i++) { 
         
         
         clearPreviousEnemy(&game->enemies[i]);
         drawRect(&game->enemies[i]);        //To confirm, basically this passed the memory address of the rect object sitting within this struct to drawRect (as this takes a rect * as arguqment)
         drawEnemyModel(&game->enemies[i]);

    }
    

}

// Pass in string and starting location to write text 
void displayText(char textBuffer[], int x, int y, u16 colour) { 

for (int i = 0; i < NUM_CHARS_LINE; ++i) { 
  
  if (textBuffer[i] == 0x20) { 
    printChar(selector[0],x+i*8,y, colour);     //Push the chracter acrosss 8 characters spots as you loop through
  }
  else if (textBuffer[i] == 0x2E) { 
    printChar(punctuation[0],x+i*8,y,colour);  //Period
  }
  else if (textBuffer[i] == 0x21) {   //0010 0001
    printChar(punctuation[1],x+i*8,y,colour);  //Exclam
  }
   else if (textBuffer[i] == 0x3A) {   //0010 0001
    printChar(punctuation[2],x+i*8,y,colour);  //colom
  }
  else if (textBuffer[i] >= 0x30 && textBuffer[i] <= 0x39) { 		
            printChar(score[textBuffer[i] - 0x30], x+i*8, y,colour);
        // Letters
        } else {
            printChar(alphabet[textBuffer[i] - 0x41], x+i*8, y,colour); 			
        }
  }
}




