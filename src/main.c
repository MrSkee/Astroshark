/*Sean Kee*/
/*Astroshark v0.6.8*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "asteroids.h"
#include "lasers.h"
#include "backend.h"

#define PI 3.14159265

#define WINDOW_HEIGHT 720
#define WINDOW_WIDTH 1280
/*Title of the window*/
char windowTitle[18] = {"Astroshark  v0.6.8"};

enum direction {NORTH = 5, EAST, SOUTH, WEST};
enum location {TOP = 0, RIGHT, BOTTOM, LEFT};
enum shipAnimation {AT_REST, ENGINE_START, ENGINE_1, ENGINE_2, ENGINE_3, ENGINE_4};
/*Strut for a ship*/
typedef struct shipCharacter {
	SDL_Rect dstrect;
	SDL_Rect srcrect;
	SDL_Texture *texture;
	Mix_Chunk *shootSFX;
	/*Constants*/
	int defaultOrientation;
	/*Variables for event tests*/
	int moveForward;
	int moveBackward;
	int strafeLeft;
	int strafeRight;
	int rotateLeft;
	int rotateRight;
	int actionShoot;
	/*Movement variables*/
	int speed;
	int rotate;
	int deltaX;
	int deltaY;
	/*Ship variables*/
	int lives;
	int animationFrame;
} shipInstance;

/*Function to calculate movement using my algorithm*/
/*This Algorithm calculates the direction that the player should move if the W(forwards) key is pressed by using the angle and speed*/
/*This algorithm involves trigonometry to calculate (using unit circle)*/
void calculateMovement(int *deltaX, int *deltaY, int angle, int speed) {
	int quadrant = 0;
	float sinT;
	float cosT;
/*Checks to see if rotation is greater or less than 0/360*/
	if (angle >= 360)
		angle -= 360;
	if (angle < 0)
		angle +=360;

/*Sets correct quadrant*/
	if (angle < 90 && angle > 0) {
		quadrant = 1;
	}
	if (angle < 180 && angle > 90) {
		quadrant = 2;
		angle -= 90;
	}
	if (angle < 270 && angle > 180) {
		quadrant = 3;
		angle -= 180;
	}
	if (angle <= 359 && angle > 270) {
		quadrant = 4;
		angle -= 270;
	}
/*Sets direction if player is orientated towards an axis*/
	if (angle == 0)
		quadrant = EAST;
	if (angle == 90)
		quadrant = NORTH;
	if (angle == 180)
		quadrant = WEST;
	if (angle == 270)
		quadrant = SOUTH;
/*Calculates the change in x and y using trigonometric functions*/
	sinT = sin(angle*PI/180) * speed;
	cosT= cos(angle*PI/180) * speed;
//	printf("deltaX = %f\ndeltaY = %f\nangle = %d\n", deltaX, deltaY, angle);

/*Updates the changes based on correct orientation and deltaX and deltaY*/
	if (deltaX != NULL && deltaY != NULL) {
		switch(quadrant) {
			case 1:
				*deltaX = cosT;
				*deltaY = -1 * sinT;
				break;
			case 2:
				*deltaX = -1 * sinT;
				*deltaY = -1 * cosT;	
				break;
			case 3:
				*deltaX = -1 * cosT;
				*deltaY = sinT;
				break;
			case 4:
				*deltaX = sinT;
				*deltaY = cosT;
				break;
			case NORTH:
				*deltaX = 0;
				*deltaY = -1 * sinT + 1;
				break;
			case EAST:
				*deltaX = cosT - 1;
				*deltaY = 0;
				break;
			case SOUTH:
				*deltaX = 0;
				*deltaY = -1 * sinT - 1;
				break;
			case WEST:
				*deltaX = cosT + 1;
				*deltaY = 0;
				break;
		}
	}
}

/*Function for initalizing Astroshark*/
int initializeAstroshark(int *debug) {

	printf("Initialized Astroshark\n\n%s", windowTitle);

	int i; //Standard Counter
	int j; //Secondary Counter
	int playerScore = 0;
	srand(time(NULL));
/*Initalizes SDL while testing if it was successful*/
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO) != 0) {
		printf("\n\n***ERROR: Unable to initalize SDL: %s\nEND ERROR***\n", SDL_GetError());
		*debug = 1;
		return 1;
	}

	IMG_Init(IMG_INIT_PNG);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

	/*Declares the gameWindow(pointer) with datatype SDL_Window*/
	SDL_Window *gameWindow;
	gameWindow = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0); 
	/*Tests to see if window creation was successful or not */
	if (gameWindow == NULL) {
		printf("\n\n***ERROR: Could not create window: %s\nEND ERROR***\n", SDL_GetError());
		*debug = 1;
		return 1;
	}
	/*Creates Renderer flags*/
	/*Creates Renderer*/
	Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	SDL_Renderer *renderer = SDL_CreateRenderer(gameWindow, -1, render_flags);
	if(renderer == NULL) {
		printf("\n\n***ERROR: Failed to create renderer: %s\nEND ERROR***\n", SDL_GetError());
		*debug = 1;
		return 1;
	}

	/*Creates Loading Screen*/
	SDL_Texture *splash_screenTexture;
	SDL_Rect splash_screenRect;
	createSprite(renderer, &splash_screenRect.w, &splash_screenRect.h, &splash_screenTexture, "resources/gfx/splash_screen.png");
	splash_screenRect.x = 0;
	splash_screenRect.y = 0;
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, splash_screenTexture, NULL, &splash_screenRect);
	SDL_RenderPresent(renderer);

	/*Creates End texture*/
	SDL_Texture *endTexture;

	SDL_Rect endRect;
	SDL_Surface *tempSurface2 = IMG_Load("resources/gfx/gameOver.png");
	endTexture = SDL_CreateTextureFromSurface(renderer, tempSurface2);
	SDL_FreeSurface(tempSurface2);
	SDL_QueryTexture(endTexture, NULL, NULL, &endRect.w, &endRect.h);
	endRect.x = 0;
	endRect.y = 0;
	endRect.w = WINDOW_WIDTH;
	endRect.h = WINDOW_HEIGHT;

	SDL_Texture *backgroundTexture;
	SDL_Rect backgroundRect;
	createSprite(renderer, &backgroundRect.w, &backgroundRect.h, &backgroundTexture, "resources/gfx/background_1920x1920.png");
	backgroundRect.x = 0 - ((backgroundRect.w / 2) - (WINDOW_WIDTH / 2));
	backgroundRect.y = 0 - ((backgroundRect.h / 2) - (WINDOW_HEIGHT / 2));

	SDL_Texture *hudTexture;
	SDL_Rect hudRect;
	createSprite(renderer, &hudRect.w, &hudRect.h, &hudTexture, "resources/gfx/hud.png");
	hudRect.x = 0;
	hudRect.y = 0;

	
	/*Creates ship's destination rectangle, a.k.a. the ship "object"*/
/*Creates Source rectangle, to highlight the area in which the correct sprite on the spritesheet is located*/
/*Creates the Ship's texture*/
/*Sends the addresses the necessary structs and data to createShip()*/
	shipInstance playerShip;
	createSprite(renderer, &playerShip.dstrect.w, &playerShip.dstrect.h, &playerShip.texture, "resources/gfx/playerShip_spritesheet_320x480.png");
/*Resizes the width of the rectangle to the size of a single sprite*/
	/*Scales down the ship*/
	playerShip.dstrect.w -= 1600;
	playerShip.dstrect.w /= 10;
	playerShip.dstrect.h /= 10;

	playerShip.dstrect.x = WINDOW_WIDTH / 2;	
	playerShip.dstrect.y = WINDOW_HEIGHT / 2;

	/*Sets the proper texture location*/
	playerShip.srcrect.x = 0;
	playerShip.srcrect.y = 0;
	playerShip.srcrect.w = 320;
	playerShip.srcrect.h = 480;

/*Default ship speed*/
	playerShip.speed = 4;

/*Various booleans for different movements*/
	playerShip.moveForward = 0;
	playerShip.strafeLeft = 0;
	playerShip.moveBackward = 0;
	playerShip.strafeRight = 0;
	
	playerShip.rotateLeft = 0;
	playerShip.rotateRight = 0;

	playerShip.actionShoot = 0;
/*Variable for ship rotation angle*/
	playerShip.rotate = 0;

	playerShip.deltaX = 0;
	playerShip.deltaY = 0;

	playerShip.lives = 3;

	playerShip.animationFrame = AT_REST;

	playerShip.shootSFX = Mix_LoadWAV("resources/sfx/shoot.wav");

	playerShip.defaultOrientation = NORTH;

	if (playerShip.defaultOrientation == NORTH)
		playerShip.defaultOrientation = 90;
		playerShip.rotate = playerShip.defaultOrientation;

/*Creates variables for laser beam*/
	SDL_Point laser_origin = {8, 26};
	int laserTotal = 20;
	SDL_Texture *laserTexture;
	laserInstance laser[20];
	for (i = 0; i < laserTotal; i++) {
		createSprite(renderer, &laser[i].dstrect.w, &laser[i].dstrect.h, &laserTexture, "resources/gfx/lasers_spritesheet_160x320.png");
		laser[i].dstrect.w -= 320;
		laser[i].dstrect.w /= 10;
		laser[i].dstrect.h /= 10;	
	
		laser[i].srcrect.x = 0;
		laser[i].srcrect.y = 0;
		laser[i].srcrect.w = 160;
		laser[i].srcrect.h = 320;

		laser[i].dstrect.x = -20;
		laser[i].dstrect.y = 0;
		laser[i].rotate = playerShip.defaultOrientation;
		
		laser[i].deltaX = 0;
		laser[i].deltaY = 0;
//		printf("Laser %d Original W: %d H: %d\n", i, laser[i].dstrect.w, laser[i].dstrect.h);
	}
	
	int laserCount = 0;
	int laserDelay = 0;

/*Creates variables for asteroids. Uses the struct above and same process as the laser*/
	int asteroid_spawnRate = 60;
//	int asteroid_spawnRateTick = 0;
	int asteroidTick = 0;
	int asteroidCount = 0;

	int asteroidDefault = 20;
//	int asteroidDouble = 4;
//	int asteroidGold = 1;
	SDL_Texture *asteroidTexture;
	asteroidInstance asteroid[20];
	for (i = 0; i < asteroidDefault; i++) {
		createSprite(renderer, &asteroid[i].asteroid_dstrect.w, &asteroid[i].asteroid_dstrect.h, &asteroidTexture, "resources/gfx/asteroid_spritesheet_640x640.png");
		asteroid[i].asteroid_dstrect.w -= 640;
		asteroid[i].asteroid_dstrect.h -= 1920;
		asteroid[i].asteroid_dstrect.w /= (rand() % 30) + 5;
		asteroid[i].asteroid_dstrect.h = asteroid[i].asteroid_dstrect.w;

		asteroid[i].size = asteroid[i].asteroid_dstrect.w;

		asteroid[i].health = 1;
		/*ALPHA FEATURE TESTING*/
		/*if (asteroid[i].size < 25)
			asteroid[i].health = 1;
		if (asteroid[i].size >= 25 && asteroid[i].size < 40)
			asteroid[i].health = 2;
		if (asteroid[i].size >= 40)
			asteroid[i].health = 2;*/

		asteroid[i].asteroid_srcrect.x = 0;
		asteroid[i].asteroid_srcrect.y = 0;
		asteroid[i].asteroid_srcrect.w = 640;
		asteroid[i].asteroid_srcrect.h = 640;

		asteroid[i].deltaX = 0;
		asteroid[i].deltaY = 0;

		asteroid[i].spawnLocation = rand() % 4;

		switch(asteroid[i].spawnLocation) {
			case TOP:
				asteroid[i].asteroid_dstrect.x = (rand() % WINDOW_WIDTH);
				asteroid[i].asteroid_dstrect.y = -100;
				break;
			case LEFT:
				asteroid[i].asteroid_dstrect.x = -100;
				asteroid[i].asteroid_dstrect.y = (rand() % WINDOW_HEIGHT);
				break;
			case BOTTOM:
				asteroid[i].asteroid_dstrect.x = (rand() % WINDOW_WIDTH);
				asteroid[i].asteroid_dstrect.y = WINDOW_HEIGHT + 100;
				break;
			case RIGHT:
				asteroid[i].asteroid_dstrect.x = WINDOW_WIDTH + 100;
				asteroid[i].asteroid_dstrect.y = (rand() % WINDOW_HEIGHT);
				break;
		}

//		printf("%d: %d, %d, %d\n", i, asteroid[i].asteroid_dstrect.x, asteroid[i].asteroid_dstrect.y, asteroid[i].spawnLocation);

		asteroid[i].type = (rand() % 3) + 1;

		switch(asteroid[i].type) {
			case 1:
				asteroid[i].hitBox.w = asteroid[i].asteroid_dstrect.w / 2;
				asteroid[i].hitBox.h = asteroid[i].asteroid_dstrect.h * 5 / 8;
				asteroid[i].hitBox.x = asteroid[i].asteroid_dstrect.x + (asteroid[i].size / 4);
				asteroid[i].hitBox.y = asteroid[i].asteroid_dstrect.y + (asteroid[i].size - (asteroid[i].size * 5 / 8)) / 2;
				break;
			case 2:
				asteroid[i].asteroid_srcrect.y = 640;
				asteroid[i].hitBox.w = asteroid[i].asteroid_dstrect.w * 19 / 32;
				asteroid[i].hitBox.h = asteroid[i].asteroid_dstrect.h * 19 / 32;
				asteroid[i].hitBox.x = asteroid[i].asteroid_dstrect.x + (asteroid[i].size - (asteroid[i].size * 19 / 32)) / 2;
				asteroid[i].hitBox.y = asteroid[i].asteroid_dstrect.y + (asteroid[i].size - (asteroid[i].size * 19 / 32)) / 2;
				break;
			case 3:
				asteroid[i].asteroid_srcrect.y = 1280;
				asteroid[i].hitBox.w = asteroid[i].asteroid_dstrect.w * 19 / 32;
				asteroid[i].hitBox.h = asteroid[i].asteroid_dstrect.h * 5 / 8;
				asteroid[i].hitBox.x = asteroid[i].asteroid_dstrect.x + (asteroid[i].size - (asteroid[i].size * 19 / 32)) / 2;
				asteroid[i].hitBox.y = asteroid[i].asteroid_dstrect.y + (asteroid[i].size - (asteroid[i].size * 5 / 8)) / 2;
				break;
		}
//		printf("%d, %d\n\n", asteroid[i].asteroid_dstrect.x, asteroid[i].asteroid_dstrect.y);
	}

/*THIS WHOLE SECITION IS CODE FOR ADD ONS THAT IS NOT CURRENTLY WORKING*/
/*	for (i = 0; i < laserTotal; i++) {
		printf("Laser %d W: %d H: %d\n", i, laser[i].dstrect.w, laser[i].dstrect.h);
	}*/

/*	for (i = (asteroidTotal - asteroidDouble); i < (asteroidTotal - asteroidGold); i++) {
		createAsteroid(&gameWindow, renderer, &asteroid[i].asteroid_dstrect.w, &asteroid[i].asteroid_dstrect.h, &asteroidTexture);
		asteroid[i].asteroid_dstrect.w -= 640;
		asteroid[i].asteroid_dstrect.h -= 1920;
		asteroid[i].asteroid_dstrect.w /= (rand() % 30) + 30;
		asteroid[i].asteroid_dstrect.h = asteroid[i].asteroid_dstrect.w;

		asteroid[i].size = asteroid[i].asteroid_dstrect.w;

		if (asteroid[i].size < 25)
			asteroid[i].health = 1;
		if (asteroid[i].size >= 25 && asteroid[i].size < 40)
			asteroid[i].health = 3;
		if (asteroid[i].size >= 40)
			asteroid[i].health = 5;

		asteroid[i].asteroid_srcrect.x = 640;
		asteroid[i].asteroid_srcrect.y = 1920;
		asteroid[i].asteroid_srcrect.w = 640;
		asteroid[i].asteroid_srcrect.h = 640;

		asteroid[i].spawnLocation = i % 4;

		switch(asteroid[i].spawnLocation) {
			case TOP:
				asteroid[i].asteroid_dstrect.x = (rand() % WINDOW_WIDTH);
				asteroid[i].asteroid_dstrect.y = -100;
				break;
			case LEFT:
				asteroid[i].asteroid_dstrect.x = 100;
				asteroid[i].asteroid_dstrect.y = (rand() % WINDOW_HEIGHT);
				break;
			case BOTTOM:
				asteroid[i].asteroid_dstrect.x = (rand() % WINDOW_WIDTH);
				asteroid[i].asteroid_dstrect.y = 100;
				break;
			case RIGHT:
				asteroid[i].asteroid_dstrect.x = -100;
				asteroid[i].asteroid_dstrect.y = (rand() % WINDOW_HEIGHT);
				break;
		}
		
		asteroid[i].type = 5;

		asteroid[i].asteroid_srcrect.y = 1280;
		asteroid[i].hitBox.w = asteroid[i].asteroid_dstrect.w * 19 / 32;
		asteroid[i].hitBox.h = asteroid[i].asteroid_dstrect.h * 5 / 8;
		asteroid[i].hitBox.x = asteroid[i].asteroid_dstrect.x + (asteroid[i].size - (asteroid[i].size * 19 / 32)) / 2;
		asteroid[i].hitBox.y = asteroid[i].asteroid_dstrect.y + (asteroid[i].size - (asteroid[i].size * 5 / 8)) / 2;
	}*/

/*	for (i = (asteroidTotal - asteroidGold); i < asteroidTotal; i++) {
		createAsteroid(&gameWindow, renderer, &asteroid[i].asteroid_dstrect.w, &asteroid[i].asteroid_dstrect.h, &asteroidTexture);
		asteroid[i].asteroid_dstrect.w -= 640;
		asteroid[i].asteroid_dstrect.h -= 1920;
		asteroid[i].asteroid_dstrect.w /= (rand() % 50) + 10;
		asteroid[i].asteroid_dstrect.h = asteroid[i].asteroid_dstrect.w;

		asteroid[i].size = asteroid[i].asteroid_dstrect.w;

		asteroid[i].health = 1;

		asteroid[i].asteroid_srcrect.x = 0;
		asteroid[i].asteroid_srcrect.y = 1920;
		asteroid[i].asteroid_srcrect.w = 640;
		asteroid[i].asteroid_srcrect.h = 640;

		asteroid[i].spawnLocation = i % 4;

		switch(asteroid[i].spawnLocation) {
			case TOP:
				asteroid[i].asteroid_dstrect.x = (rand() % WINDOW_WIDTH);
				asteroid[i].asteroid_dstrect.y = -100;
				break;
			case LEFT:
				asteroid[i].asteroid_dstrect.x = 100;
				asteroid[i].asteroid_dstrect.y = (rand() % WINDOW_HEIGHT);
				break;
			case BOTTOM:
				asteroid[i].asteroid_dstrect.x = (rand() % WINDOW_WIDTH);
				asteroid[i].asteroid_dstrect.y = 100;
				break;
			case RIGHT:
				asteroid[i].asteroid_dstrect.x = -100;
				asteroid[i].asteroid_dstrect.y = (rand() % WINDOW_HEIGHT);
				break;
		}

		asteroid[i].type = 4;
		asteroid[i].hitBox.w = asteroid[i].asteroid_dstrect.w * 19 / 32;
		asteroid[i].hitBox.h = asteroid[i].asteroid_dstrect.h * 19 / 32;
		asteroid[i].hitBox.x = asteroid[i].asteroid_dstrect.x + (asteroid[i].size - (asteroid[i].size * 19 / 32)) / 2;
		asteroid[i].hitBox.y = asteroid[i].asteroid_dstrect.y + (asteroid[i].size - (asteroid[i].size * 19 / 32)) / 2;
	} */

/*	for (i = 0; i < laserTotal; i++) {
		printf("Laser %d W: %d H: %d\n", i, laser[i].dstrect.w, laser[i].dstrect.h);
	}*/
	/*close requested variable for controlling closed window*/
	/*Test for various events from the keyboard*/
	int close_requested = 0;

	int splash_screenAlpha = 255;
	while (splash_screenAlpha > 0) {
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, backgroundTexture, NULL, &backgroundRect);
		SDL_RenderCopy(renderer, splash_screenTexture, NULL, &splash_screenRect);
		SDL_SetTextureAlphaMod(splash_screenTexture, splash_screenAlpha);
		SDL_RenderPresent(renderer);
		splash_screenAlpha -= 5;
	}

	int hudAlpha = 0;
	
	while (!close_requested) {																						
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch(event.type) {																					
				case SDL_QUIT:
					close_requested = 1;
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode) {
						case SDL_SCANCODE_W:
							playerShip.moveForward = 1;
							break;
						case SDL_SCANCODE_A:
							playerShip.strafeLeft = 1;
							break;
						case SDL_SCANCODE_S:
							playerShip.moveBackward = 1;
							break;
						case SDL_SCANCODE_D:
							playerShip.strafeRight = 1;
							break;
						case SDL_SCANCODE_SPACE:
							playerShip.actionShoot = 1;
							break;
						case SDL_SCANCODE_LEFT:
							playerShip.rotateLeft = 1;
							break;
						case SDL_SCANCODE_RIGHT:
							playerShip.rotateRight = 1;
							break;
					}
					break;
				case SDL_KEYUP:
					switch(event.key.keysym.scancode) {
						case SDL_SCANCODE_W:
							playerShip.moveForward = 0;
							break;
						case SDL_SCANCODE_A:
							playerShip.strafeLeft = 0;
							break;
						case SDL_SCANCODE_S:
							playerShip.moveBackward = 0;
							break;
						case SDL_SCANCODE_D:
							playerShip.strafeRight = 0;
							break;
						case SDL_SCANCODE_SPACE:
							playerShip.actionShoot = 0;
							break;
						case SDL_SCANCODE_LEFT:
							playerShip.rotateLeft = 0;
							break;
						case SDL_SCANCODE_RIGHT:
							playerShip.rotateRight = 0;
							break;
					}
					break;

			}
		}

		/*Test for end of game*/
		if (playerShip.lives == 0) {
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, endTexture, NULL, &endRect);
			SDL_RenderPresent(renderer);
		continue;
		}

		
/*Copies ship's position into deltaX and deltaY*/
		playerShip.deltaX = playerShip.dstrect.x;																	
		playerShip.deltaY = playerShip.dstrect.y;

/*Checks for ship rotation based on arrow keys*/
		if (playerShip.rotateLeft == 1)																				
			playerShip.rotate += 9;
		if (playerShip.rotateRight == 1)
			playerShip.rotate -= 9;
		if (playerShip.rotate >= 360)
			playerShip.rotate -= 360;
		if (playerShip.rotate < 0)
			playerShip.rotate += 360;

//		printf("%d\n", playerShip.rotate);
		
/*Tests for different key presses*/
		if (playerShip.moveForward == 1) {																			
			calculateMovement(&playerShip.deltaX, &playerShip.deltaY, playerShip.rotate, playerShip.speed);
			playerShip.dstrect.x += playerShip.deltaX;
			playerShip.dstrect.y += playerShip.deltaY;
			playerShip.animationFrame++;
			backgroundRect.x -= playerShip.deltaX / 2;
			backgroundRect.y -= playerShip.deltaY / 2;
		}
		if (playerShip.moveBackward == 1) {
			calculateMovement(&playerShip.deltaX, &playerShip.deltaY, playerShip.rotate + 180, playerShip.speed - 2);
			playerShip.dstrect.x += playerShip.deltaX;
			playerShip.dstrect.y += playerShip.deltaY;
			backgroundRect.x -= playerShip.deltaX;
			backgroundRect.y -= playerShip.deltaY;
		}
		if (playerShip.strafeLeft == 1) {
			calculateMovement(&playerShip.deltaX, &playerShip.deltaY, playerShip.rotate + 90, playerShip.speed - 2);
			playerShip.dstrect.x += playerShip.deltaX;
			playerShip.dstrect.y += playerShip.deltaY;
			backgroundRect.x -= playerShip.deltaX;
			backgroundRect.y -= playerShip.deltaY;
		}
		if (playerShip.strafeRight == 1) {
			calculateMovement(&playerShip.deltaX, &playerShip.deltaY, playerShip.rotate - 90, playerShip.speed - 2);
			playerShip.dstrect.x += playerShip.deltaX;
			playerShip.dstrect.y += playerShip.deltaY;
			backgroundRect.x -= playerShip.deltaX;
			backgroundRect.y -= playerShip.deltaY;
		}

		if (playerShip.actionShoot == 1) {
			if (laserCount < laserTotal) {
				if (laserDelay == 0) {
					calculateMovement(&laser[laserCount].deltaX, &laser[laserCount].deltaY, playerShip.rotate, 10);
					laser[laserCount].dstrect.x = playerShip.dstrect.x + 8;
					laser[laserCount].dstrect.y = playerShip.dstrect.y - 2;
					laser[laserCount].rotate = playerShip.rotate;

					Mix_PlayChannel(-1, playerShip.shootSFX, 0);
					
					laserCount++;
					laserDelay++;
				}
			}
		}

 		/*Collision Detection between the ship and the walls*/
		if (playerShip.dstrect.x <= 0)
			playerShip.dstrect.x = 0;
		if (playerShip.dstrect.x >= WINDOW_WIDTH - playerShip.dstrect.w)
			playerShip.dstrect.x = WINDOW_WIDTH - playerShip.dstrect.w ;
		if (playerShip.dstrect.y <= -6)
			playerShip.dstrect.y = -6;
		if (playerShip.dstrect.y >= WINDOW_HEIGHT - playerShip.dstrect.h + 6)
			playerShip.dstrect.y = WINDOW_HEIGHT - playerShip.dstrect.h + 6;

		/*Proper Animation for the Ship*/
		if (playerShip.moveForward == 0) {
			if (playerShip.animationFrame <= AT_REST)
				playerShip.animationFrame = AT_REST;
			else
				playerShip.animationFrame--;
		}

		if (playerShip.animationFrame > ENGINE_4)
			playerShip.animationFrame = ENGINE_1;

		playerShip.srcrect.x = 320 * (playerShip.animationFrame);
		/*Laser timer*/
		if (laserDelay != 0)
			laserDelay++;
		if (laserDelay == 15)
			laserDelay = 0;

		/*Asteroid timer and process*/
		if (asteroidTick >= 0) {
			asteroidTick++;
		}
		if (asteroidTick == asteroid_spawnRate) {
			asteroidTick = 0;
		}
		if (asteroidTick == 0 && asteroidCount < asteroidDefault) {
			calculate_asteroidMovement(&asteroid[asteroidCount].rotate, &asteroid[asteroidCount].deltaX, &asteroid[asteroidCount].deltaY, asteroid[asteroidCount].spawnLocation);
			//printf("A: %d, R: %d, deltaX: %d, deltaY: %d, spawnLocation %d\n", asteroidCount, asteroid[asteroidCount].rotate, asteroid[asteroidCount].deltaX, asteroid[asteroidCount].deltaY, asteroid[asteroidCount].spawnLocation);
			asteroidCount++;
		}
		/*Tests for collision between asteroid and laser beam*/
		for (i = 0; i < asteroidDefault; i++) {
			for (j = 0; j < laserTotal; j++) {
				if (laser[j].dstrect.x + laser[j].dstrect.w < WINDOW_WIDTH && laser[j].dstrect.x  > 0 && laser[j].dstrect.y + laser[j].dstrect.h < WINDOW_HEIGHT && laser[j].dstrect.y > 0) {
					if ((laser[j].dstrect.y >= asteroid[i].asteroid_dstrect.y && laser[j].dstrect.y <= asteroid[i].asteroid_dstrect.y + asteroid[i].asteroid_dstrect.h) && (laser[j].dstrect.x <= asteroid[i].asteroid_dstrect.x + asteroid[i].asteroid_dstrect.w && laser[j].dstrect.x > asteroid[i].asteroid_dstrect.x)) {
						asteroid[i].health--;
						//printf("Veritcal Hit Size: %d  Health: %d\n", asteroid[i].size, asteroid[i].health);
						if (asteroid[i].health == 0) {
							asteroid[i].asteroid_dstrect.x = -80;
							asteroid[i].hitBox.x = -80;
							asteroid[i].health = 1;
							playerScore++;
							printf("Score: %d\n", playerScore);
						}
						//laser[j].dstrect.x = -50;
						//laser[j].dstrect.y = 0;
					}
					if ((laser[j].dstrect.x >= asteroid[i].asteroid_dstrect.x && laser[j].dstrect.x <= asteroid[i].asteroid_dstrect.x + asteroid[i].asteroid_dstrect.w) && (laser[j].dstrect.y <= asteroid[i].asteroid_dstrect.y + asteroid[i].asteroid_dstrect.h && laser[j].dstrect.y > asteroid[i].asteroid_dstrect.y)) {
						asteroid[i].health--;	
						//printf("Horizontal Hit Size: %d  Health %d\n", asteroid[i].size, asteroid[i].health);
						if (asteroid[i].health == 0) {
							asteroid[i].asteroid_dstrect.x = WINDOW_WIDTH + 80;
							asteroid[i].hitBox.x = -80;
							asteroid[i].health = 1;
							playerScore++;
							printf("Score: %d\n", playerScore);
						}
					}
				}
			}
				if (asteroid[i].health == 0) {
					asteroid[i].asteroid_dstrect.x = -80;
					asteroid[i].hitBox.x = -80;
					asteroid[i].health = 1;
					playerScore++;
				}
			if ((playerShip.dstrect.y >= asteroid[i].asteroid_dstrect.y && playerShip.dstrect.y <= asteroid[i].asteroid_dstrect.y + asteroid[i].asteroid_dstrect.h) && (playerShip.dstrect.x <= asteroid[i].asteroid_dstrect.x + asteroid[i].asteroid_dstrect.w && playerShip.dstrect.x >= asteroid[i].asteroid_dstrect.x)) {
				asteroid[i].asteroid_dstrect.y = -80;
				playerShip.dstrect.x = WINDOW_WIDTH / 2;
				playerShip.dstrect.y = WINDOW_HEIGHT / 2;
				playerShip.lives--;
			}
		}
		if (hudAlpha < 255) {
			SDL_SetTextureAlphaMod(hudTexture, hudAlpha);
			hudAlpha += 5;
		}


/*Renders and updates position based on the different changes in X and Y*/
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, backgroundTexture, NULL, &backgroundRect);
		for (i = 0; i < asteroidDefault; i++) {
			asteroid[i].asteroid_dstrect.x += asteroid[i].deltaX;
			asteroid[i].asteroid_dstrect.y += asteroid[i].deltaY;
			asteroid[i].hitBox.x += asteroid[i].deltaX;
			asteroid[i].hitBox.y += asteroid[i].deltaY;
			asteroid[i].rotate += asteroid[i].rotate;
			if (asteroid[i].asteroid_dstrect.x <= -175 || asteroid[i].asteroid_dstrect.x >= WINDOW_WIDTH + 175 - asteroid[i].asteroid_dstrect.w) {
				asteroid[i].deltaX *= -1;
			}
			if (asteroid[i].asteroid_dstrect.y <= -175 ||asteroid[i].asteroid_dstrect.y >= WINDOW_HEIGHT + 175 - asteroid[i].asteroid_dstrect.h) {
				asteroid[i].deltaY *= -1;
			}
			SDL_RenderCopyEx(renderer, asteroidTexture, &asteroid[i].asteroid_srcrect, &asteroid[i].asteroid_dstrect, asteroid[i].rotate, NULL, SDL_FLIP_NONE);
		}
		for (i = 0; i < laserTotal; i++) {
			if (laser[i].dstrect.x + laser[i].dstrect.h < 0 || laser[i].dstrect.x + laser[i].dstrect.h > WINDOW_WIDTH || laser[i].dstrect.y + laser[i].dstrect.h < 0 || laser[i].dstrect.y + laser[i].dstrect.h > WINDOW_HEIGHT) {
				if (laserCount == 20) {
					laserCount = 0;
				}
			}
			laser[i].dstrect.x += laser[i].deltaX;
			laser[i].dstrect.y += laser[i].deltaY;
			SDL_RenderCopyEx(renderer, laserTexture, &laser[i].srcrect, &laser[i].dstrect, -1 * (laser[i].rotate - playerShip.defaultOrientation), &laser_origin, SDL_FLIP_NONE);
//			printf("%d\tW:%d\tH:%d\n", i, laser[i].dstrect.w, laser[i].dstrect.h);

		}/*Copies the texture onto the rect, and rotates it correctly*/
			/*Presents the renderer and draws everything in renderer*/
		SDL_RenderCopyEx(renderer, playerShip.texture, &playerShip.srcrect, &playerShip.dstrect, -1 * (playerShip.rotate - playerShip.defaultOrientation), NULL, SDL_FLIP_NONE);

		SDL_RenderCopy(renderer, hudTexture, NULL, &hudRect);
		SDL_RenderPresent(renderer);

		SDL_Delay(1000/60);
	}

	/*Free Sounds*/
	Mix_FreeChunk(playerShip.shootSFX);
	/*Destroys Texture to ensure no memory leaks*/
	SDL_DestroyTexture(playerShip.texture);
	SDL_DestroyTexture(laserTexture);
	SDL_DestroyTexture(asteroidTexture);
	SDL_DestroyTexture(splash_screenTexture);
	SDL_DestroyTexture(endTexture);
	SDL_DestroyTexture(backgroundTexture);
	SDL_DestroyTexture(hudTexture);
	/*Destroys Renderer*/
	SDL_DestroyRenderer(renderer);
	/*Destroys the window that gameWindow is pointing to*/
	SDL_DestroyWindow(gameWindow);

	Mix_Quit();
	IMG_Quit();

	/*Cleans up and quits out of SDL*/
	SDL_Quit();

	*debug = 0;
}

int main(int argc, char *argv[]) {
	/*Variable for testing if initialization was successful*/
	int debug;
	initializeAstroshark(&debug);
	if (debug == 1)
		return 1;
	else
		return 0;
}
