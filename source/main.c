#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_timers.h>
#include <stdint.h>
#include <stdbool.h>
#include "../includes/graphics.h"
#include "../includes/sound.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

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


// Definitions to be declared
#define SCREEN_W 240
#define SCREEN_H 160
#define REG_DISPTCNT *((unsigned int*)(0x04000000))
#define VIDEOMODE_3 0x0003
#define BG_ENABLE2 0x0400
#define SCREENBUFFER ((unsigned short*)VRAM)
#define NUM_CHARS_LINE 10
#define PAUSE_LENGTH 180
#define LOGO_POSITION_X 22
#define LOGO_POSITION_Y 20
#define left_line_x SCREEN_W/4      //Keep a constant divider between the left and right side road of the screen
#define right_line_x SCREEN_W/4 * 3


//This function will be called at the start of the game to setup the player
// It sets the params of the game object for control and is the master of a single game state
void setupGame(struct rect *player, struct gameInstance *game) { 
      
       clearScreen();
       player->x = SCREEN_W/2; 
       player->y = (SCREEN_H/8) * 7;
       player->prevX = player->x;
       player->prevY = player->y; 
       player->width = 8;
       player->height = 8;
       player->colour = setColour(31,31,31);

       game->difficulty = 1;
       game->maxMenuPositions = 2;
       game->game_running = 0;
       game->menuPosition = 1;
       game->settingMenu = 0;
       game->menuOpen = 0;
       game->menuStartX = 80;
       game->menuStartY = 60;
       game->gameScore = 0;
       game->pauseCounter = 0;
       game->musicRestart = 0;
       game->winScore=200;
     
}

// Purpose of this code is to draw the menu cursor location for the selection
void setMenuCursor(int menuPosition) {

  u16 colour = setColour(31,31,31);

  if (menuPosition == 1) { 
    printChar(selector[0],72,75,colour);
    printChar(selector[1],72,60,colour);
  }
    if (menuPosition == 2) { 
    printChar(selector[0],72,60,colour);
    printChar(selector[1],72,75,colour);
  }
  
}

//Code for drawing the colours on the menu
void drawMenuBackground() { 

    u16 backgroundColour = setColour(5,5,5);
    u16 backgroundColour2 = setColour(10,10,10);
    u16 LogoBGColour = setColour(0,0,0);

    //Code will draw on the right and left vertical grass equivilients 
    /* 
      |   logo     |
      |   text     | 
      |            |
    
    */

    
    for (int i = 0; i < SCREEN_W; i= i+SCREEN_W/4*3) { 
      for(int x = i; x < i+SCREEN_W/4; x++) {
        for (int j = 0; j<SCREEN_H; j++) { 
          plotPixel(x,j, backgroundColour);
        }
      }
    }
    
    //Now we want to draw the type and bottom colours towards the logo

    //Code will draw the top part of the menu
    for(int i = 0; i < 50; i++) { 
      for (int j = 0; j < right_line_x - left_line_x; j++ )  {
        plotPixel(j+left_line_x,i, backgroundColour);
      }
    }

    //Code will draw the bottom part of the menu
    for(int i = SCREEN_H-50; i < SCREEN_H; i++) { 
      for (int j = 0; j < right_line_x - left_line_x; j++ )  {
        plotPixel(j+left_line_x,i, backgroundColour);
      }
    }
     
     //Draw the logo box area

    for(int i = 0; i < 210; i++) { 
      for (int j = 0; j < 25; j++ )  {
        plotPixel(15+i,10+j, LogoBGColour);
      }
    }
}

//Responsible for the logo code or the banner of the game on the title screen
void drawMenuLogo() { 

  u16 logoColour = setColour(31,0,0); 
  
  printChar(images[1],LOGO_POSITION_X, LOGO_POSITION_Y, logoColour);
  displayText("ESCAPE THE", LOGO_POSITION_X+15,LOGO_POSITION_Y,logoColour);
  displayText("GODFATHER", LOGO_POSITION_X+105,LOGO_POSITION_Y,logoColour);
  printChar(images[1],LOGO_POSITION_X+185, LOGO_POSITION_Y,logoColour);

}

// Basic code that will render the home screen message for the title of the game
// Basic plan is as follows: 
// 1) use a string variable to hold the text you want to write
// 2) loop through the string, and match each letter to a position in the characters array. 
// Draw the letter based on the values of the character display map

void displayHomeMenu(struct gameInstance *game) { 

  u16 colour = setColour(31,31,31);

  if (game->menuOpen == 0) { 
    game->menuOpen = 1;
    drawMenuBackground();
  }

  drawMenuLogo();
  setMenuCursor(game->menuPosition);
  displayText("START GAME", game->menuStartX,game->menuStartY,colour);
  displayText("DIFFICULTY", game->menuStartX,game->menuStartY+15,colour);
  displayText("HSCORE: ", game->menuStartX,game->menuStartY+37,colour);
  displayScore(game->highScore, game->menuStartX + 60, game->menuStartY+37,colour);

  int keys_released;
  keys_released = keysUp();

  if (keys_released & KEY_DOWN) { 
    if (game->menuPosition < game->maxMenuPositions) { 
    game->menuPosition = game->menuPosition + 1;
    }
  }

  if (keys_released & KEY_UP) { 
    
    if(game->menuPosition > 1) { 
    game->menuPosition = game->menuPosition - 1;
    }
  }

  if (keys_released & KEY_A) { 
    if (game->menuPosition == 1) { 
      clearScreen();
      game->game_running = 1;
      startGame(game);
      game->musicRestart = 1;
    }
    else if (game->menuPosition == 2) { 
      game->menuPosition = 1;
      game->settingMenu = 1;
      }
    }



}

//Random number function
int random_number(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

//Function will return a new enemy object
//X location will be random between the middle of the screen
struct rect createEnemy() { 
 
  struct rect baseEnemy;

  baseEnemy.colour = setColour(31,0,0);
  baseEnemy.x = random_number(left_line_x+1, right_line_x-20);
  baseEnemy.y = 10;
  baseEnemy.prevX = baseEnemy.x;
  baseEnemy.prevY = baseEnemy.y;
  baseEnemy.height = 8; 
  baseEnemy.width = 8;
  baseEnemy.speed = random_number(1,3);

  return baseEnemy;


}

//Update the y position of each enemy. 
// If the position is > 160, we need to destroy the object and create a new enemy
// For now just focus on moving from top to bottom 

void updateEnemies(struct gameInstance *game) { 

  for(int i = 0; i < NUM_ENEMIES; i++) { 

    int current_y;
    int speed; 
    speed = game->enemies[i].speed;
    current_y = game->enemies[i].y;
    
    game->enemies[i].prevY = current_y;
    game->enemies[i].y = current_y + speed;
    
       
    //Now check to see if we need to deallocate the enemy and add to the score

    if(game->enemies[i].y > 160) { 
      game->enemies[i] = createEnemy();
      game->gameScore +=1;
    }

  }
}

/* Bounding Box Collision Detection */
bool collision(
    int x1, int y1, int width1, int height1,
    int x2, int y2, int width2, int height2) {
    if (x1 + width1 > x2 &&
        x1 < x2 + width2 &&
        y1 + height1 > y2 &&
        y1 < y2 + height2) return true;
    return false;
}

//Function to check if the play colided with any objects 
//SPeed this up by only checking colissions if enemy is > 100 pixel height

bool checkCollisions(struct rect *player, struct gameInstance *game) { 
  
  bool isCollision = false;

  for (int i = 0; i < NUM_ENEMIES; i++) { 
    isCollision = collision(player->x, player->y, player->width, player->height, game->enemies[i].x, game->enemies[i].y, game->enemies[i].width, game->enemies[i].height);

    if (isCollision) { 
      break;
    }
  }
  
  return isCollision;
}

void pauseMessage(char message[], struct gameInstance *game) { 

  u16 colour = setColour(31,31,31);
  displayText(message, 90,60,colour );

} 

//Return boundary side if the player as hit the road boundary. 
//Main gameloop will handle logic to prevent further movement
bool checkBoundaryRight(struct rect *player, struct gameInstance *game) { 

 
  if(player->x + player->width >= right_line_x) { 
    return true;
  }
  return false;

}

bool checkBoundaryLeft(struct rect *player, struct gameInstance *game) { 

  
  if(player->x <= left_line_x+1) { 
    return true;
  }
  return false;

}

void updateHighScore(struct gameInstance *game) { 

  if (game->gameScore > game->highScore) { 
  game->highScore = game->gameScore;
  }

}

int gameRunning (struct rect *player, struct gameInstance *game) { 
  
  if(!game->paused) {
  
      playSong(1,1);
      // Get the key inputs
      // Save  position for fast screen clear
      //USER INPUT

      bool boundaryLeft = false;
      bool boundaryRight = false;

      boundaryLeft = checkBoundaryLeft(player,game);
      boundaryRight = checkBoundaryRight(player,game);


      //UPDATE PHASE

      int keys_held;
          
      keys_held = keysHeld();

      if ((keys_held & KEY_RIGHT) && !boundaryRight) { 
        player->prevX = player->x;
        player->x = player->x+1;

      }
      
      if ((keys_held & KEY_LEFT)  && !boundaryLeft) { 
        player->prevX = player->x;
        player->x = player->x-1;
        }     
      
      
      updateEnemies(game);        
      
      //DRAW PHASE

      drawPlayer(player); //Initial draw and continue this tomorrow
      drawLane();
      drawScore(game,10,10); 
      drawEnemies(game); 

      //Collisions
      bool collision = false;

      collision = checkCollisions(player, game); 

      if (collision) { 
        collision = false;
        pauseMessage("YOU LOSE!", game); 
        updateHighScore(game);
        game->paused = true;
        FPS=0;
        playSong(2,1);
        game->musicRestart = 1;
        
      }

      //WIN

      if(game->gameScore >= game->winScore) { 
        pauseMessage("YOU WIN!", game); 
        updateHighScore(game);
        game->paused = true;
        FPS=0;
        playSong(4,0);
        game->musicRestart = 1;

      } 

  }
  else { 
    game->pauseCounter++;

    if(game->pauseCounter > 80) { 
      game->paused = false;
      game->pauseCounter = 0;
      setupGame(player,game);
    }
  }
  return 0;
}

//Function checks to make sure no x coordinate will cause and overlaps in the current enemies
int randomNumberEnemyNoCollide(struct gameInstance *game) { 

    bool nocollide = false; 
    int tempX; 

    while(!nocollide) { 

      tempX = random_number(left_line_x+1, right_line_x-20);

      for(int i = 0; i < NUM_ENEMIES; i++) { 
          if ((game->enemies[i].x > tempX && game->enemies[i].x < tempX+8) || (tempX < game->enemies[i].x+8 && tempX > game->enemies[i].x )) { 
            nocollide = false; //Loop again

          }
          else { 
            nocollide = true;
            return tempX;
          }
      }
    }
    
}

void startGame(struct gameInstance *game){ 

  //Initial video setup
  drawRoad();
  
  // Set up game attributes for running 

  //Create the number of enemies on screen for start of game
  
  for(int i = 0; i < NUM_ENEMIES; i++) { 
    
    struct rect baseEnemy;

    baseEnemy.colour = setColour(31,0,0);
    baseEnemy.x = left_line_x + 10 + i * 30;    //Set fixed starting positions for the cars
    baseEnemy.y = 10;
    baseEnemy.prevX = baseEnemy.x;
    baseEnemy.prevY = baseEnemy.y;
    baseEnemy.height = 8;
    baseEnemy.width = 8; 

    if(game->difficulty == 1) { 
      baseEnemy.speed = random_number(1,2);
    }
    else if (game->difficulty == 2) { 
      baseEnemy.speed = random_number(2,5);
    }
    
    game->enemies[i] = baseEnemy;
    
  }
}

void displayDifficultyMenu(struct gameInstance *game) { 
  
  u16 logoColour = setColour(31,31,31);

  drawMenuLogo();
  setMenuCursor(game->menuPosition);
  displayText("EASY          ", game->menuStartX,game->menuStartY,logoColour);
  displayText("HARD", game->menuStartX,game->menuStartY+15,logoColour);
  displayText("       ", game->menuStartX,game->menuStartY+37,logoColour);

  int keys_released; 

  keys_released = keysUp();

  if (keys_released & KEY_DOWN) { 
  if (game->menuPosition < game->maxMenuPositions) { 
      game->menuPosition = game->menuPosition + 1;
  }
}

if (keys_released & KEY_UP) { 
  if(game->menuPosition > 1) { 
      game->menuPosition = game->menuPosition - 1;
  }
}

if (keys_released & KEY_A) { 
  
  
  if(game->menuPosition == 1) { 
      game->difficulty = 1;
      game->settingMenu = 0;

  }
  else { 
      game->difficulty = 2;
      game->settingMenu = 0;
     }

  game->menuPosition = 1;

  }

}

int main(void) {

    REG_DISPTCNT = VIDEOMODE_3 | BG_ENABLE2;            //This sets the value of memory location 0x04000000 to value 0100000000011 

    // Interrupt handlers
    irqInit();

    // Enable Vblank Interrupt, Allow VblankIntrWait
    irqEnable(IRQ_VBLANK);

    srand(time(NULL));

    //Setup values of the song structs. The purpose of these is to control the pace and speed of a song
    M[0].song=song_1; M[0].spd=4; M[0].tic=0; M[0].size=15; M[0].onOff=1;
    M[1].song=song_2; M[1].spd=16; M[1].tic=0; M[1].size=15; M[1].onOff=1;
    M[2].song=song_3; M[2].spd=2; M[2].tic=0; M[2].size=15; M[2].onOff=1;
    M[3].song=song_4; M[3].spd=13; M[3].tic=0; M[3].size=15; M[3].onOff=1;
    M[4].song=song_5; M[4].spd=4; M[4].tic=0; M[4].size=15; M[4].onOff=1;

    struct gameInstance game;
    struct rect player;

    game.highScore = 0;
    
    //Initialise the game state. We use the game object to hold the state and settings of the game
    setupGame(&player, &game);

    int keys_released;

    /* Main Game Loop */
    while (1) {
        VBlankIntrWait();    //Code will only run once we are in the vblank section of processing.
        scanKeys();

       // Restart the music clock if action has been taken. 

       if (game.musicRestart == 1) { 
        game.musicRestart = 0;
        M[0].tic=0;
        M[1].tic=0;
        M[2].tic=0;
        M[3].tic=0;
       }

       //Actually playable game is not running
       //Clean up this menu logic

       if (game.game_running == 0) { 
         
         //Menu music
         playSong(3,1);

         if (game.settingMenu == 1) { 
          
          displayDifficultyMenu(&game);
        }
         
         else if(game.settingMenu ==0) { 
         
         displayHomeMenu(&game);

        }
      }
       
       //game->is_running = 0
       //Game codde will execute here
       else { 

        gameRunning(&player, &game);
        
      }
    //Need to understand how this is important to the music generator
      
      FPS+=1; /*if(lastFr>REG_TM2D>>12){ FPS=0;}                          //Need to get rid of this code as I dont think timers are needed now with this implementation
      lastFr=REG_TM2D>>12;     */                                         //Keep track of FPS as this is relevant to the song. Happens +1 each Vblank loop
  
    }

    return 0;
}