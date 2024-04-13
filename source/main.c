#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "tilemap.h"
#include "music.h"
#include "window.h"
#include "collisions.h"
#include "menu.h"
#include <stdlib.h> // rand(), srand() 
#include <time.h>   // for seeding srand()    
#include "character.h"  

#define SPEED 300
#define TILE_SIZE 64
#define TILE_W_AMOUNT 19  // changed from 60 to test collisions 
#define TILE_H_AMOUNT 11  // changed from 60 to test collisions
#define GAME_W TILE_W_AMOUNT*TILE_SIZE
#define GAME_H TILE_H_AMOUNT*TILE_SIZE
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 700  // changed from 800 to adjust to my screen size

/// * temporary defines. Set to 1 to enable and 0 to disable
#define FOLLOW_PLAYER 0
#define VINGETTE 0

//! explanation of #if, #else, #endif 
// They are like if, and else statements, but they are used at compile time and not at runtime. Therefor they can only be used with #define statements.
// This is okay for Testing purposes, but it is not recommended to use #if, #else, #endif in the main code. I am using them now in the code to quickly turn on and off some features. This will be changed in the future.
#if FOO
    // if FOO is set to 1, everything between the #if FOO and #else will be included. The #else will be ignored.
#else
    // if FOO is set to 0, everything between the #else and #endif will be included. Everything between #if FOO and #else will be ignored.
#endif

SDL_Window* pWindow = NULL;
SDL_Renderer* pRenderer = NULL;



typedef struct _char{
    SDL_Rect rect;
    SDL_Texture* texture;
} _char;


void create_texture(SDL_Texture** texture, const char* path)
{
    SDL_Surface* surface = IMG_Load(path);
    if(!surface)
    {
        printf("Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(pRenderer);
        SDL_DestroyWindow(pWindow);
        SDL_Quit();
        return;
    }
    *texture = SDL_CreateTextureFromSurface(pRenderer, surface);
    SDL_FreeSurface(surface);
    if(!*texture)
    {
        printf("Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(pRenderer);
        SDL_DestroyWindow(pWindow);
        SDL_Quit();
        return;
    }
}
#if FOLLOW_PLAYER
SDL_Rect follow_camera(SDL_Rect* camera, SDL_Rect* target)
{
    SDL_Rect new_camera = *target;
    new_camera.x -= camera->x;
    new_camera.y -= camera->y;
    return new_camera;
}
#endif


int main(int argv, char** args)
{
    if(init_SDL_window(&pWindow, &pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT) != 0) {
        printf("Failed to initialize window and renderer.\n");
        return 1;
    }

    char soundPathbgm[] = "resources/music/bgm1.mp3";
    BackgroundMusic *bgm =init_background_music(soundPathbgm, 20);
    play_background_music(bgm);
    free_bgm(bgm);

    // TODO: Move this to a function, maybe in a separate file
    SDL_Texture* pWhite = NULL;
    SDL_Texture* pBlack = NULL;
    SDL_Texture* pTexture = NULL;
    SDL_Texture* pVingette = NULL;

    create_texture(&pBlack, "resources/black.png");
    create_texture(&pWhite, "resources/white.png");



    _char shipRect;
    // * ship texture
    // SDL_Rect shipRect;
    create_texture(&shipRect.texture, "resources/ship.png");
    SDL_QueryTexture(shipRect.texture, NULL, NULL, &shipRect.rect.x, &shipRect.rect.y);
    shipRect.rect.x = 100;
    shipRect.rect.y = 100;
    shipRect.rect.w /= 4;
    shipRect.rect.h /= 4;

#if VINGETTE
    // * psudo vingette effect
    create_texture(&pVingette, "resources/vingette.png");
    SDL_Rect vingetteRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_QueryTexture(pVingette, NULL, NULL, &vingetteRect.w, &vingetteRect.h);
#endif

    TileMap tilemap;
    SDL_Rect rect = { 0, 0, TILE_SIZE, TILE_SIZE };
    tilemap_init(&tilemap, TILE_W_AMOUNT, TILE_H_AMOUNT, TILE_SIZE);
    Tile white_tile = { 0,0, pWhite, rect };
    Tile black_tile = { 1,0, pBlack, rect };
    // TODO: implement a LoadFromFile function
    for(int y = 0; y < tilemap.height; y++)
    {
        for(int x = 0; x < tilemap.width; x++)
        {
            if((x + y) % 2 == 0)
            {
                tilemap_set_tile(&tilemap, x, y, &white_tile);
            }
            else
            {
                tilemap_set_tile(&tilemap, x, y, &black_tile);
            }
        }
    }
    

    bool menu = false;
    bool music = true;
    bool up, down, left, right, space, m, lower_volume, inc_volume;
    up = down = left = right = space = m = lower_volume = inc_volume = false;   


    ////////////////////////////////////////////////////////
    // TODO: remove. these are to test collisions only.      

    SDL_Texture* bush = NULL;
    SDL_Texture* cactus = NULL;                
    SDL_Texture* stone = NULL; 
    SDL_Texture* tile2 = NULL;  
    SDL_Texture* tile5 = NULL; 

    create_texture(&bush, "resources/testpack/Bush.png");    
    create_texture(&cactus, "resources/testpack/Cactus.png"); 
    create_texture(&stone, "resources/testpack/Stone.png"); 
    create_texture(&tile2, "resources/testpack/tile2.png"); 
    create_texture(&tile5, "resources/testpack/tile5.png"); 

    srand(time(NULL)); 

    Tile testTiles[] = {
        {2, 0, bush, rect},       // not solid (decoration)   
        {3, 0, cactus, rect},     // not solid
        {4, 0, stone, rect},      // not solid   
        {5, 1, tile2, rect},      // solid (blocks movement) 
        {6, 1, tile5, rect}       // solid    
    };         
    

    // place 3 of each randomly 
    bool positions[TILE_W_AMOUNT][TILE_H_AMOUNT] = {false};  // Track whether a position is occupied
    for (int i = 0; i < 5; i++) {
        int count = 0;
        while (count < 3) {
            int x = rand() % TILE_W_AMOUNT;
            int y = rand() % TILE_H_AMOUNT;

            if (!positions[x][y]) {  // Check if the position is already occupied
                tilemap_set_tile(&tilemap, x, y, &testTiles[i]);
                positions[x][y] = true;  // Mark the position as occupied
                count++;
            }
        }
    }


#if FOLLOW_PLAYER
    // camera is centered on the player
    SDL_Rect camera = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    camera.x = (shipRect.x + shipRect.w / 2) + WINDOW_WIDTH / 2;
    camera.y = (shipRect.y + shipRect.h / 2) + WINDOW_HEIGHT / 2;
#endif
    bool closeWindow = false;
    while(!closeWindow)
    {

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                closeWindow = true;
                break;
            // * this is the movement code from simpleSDLexample1.
            case SDL_KEYDOWN:
                switch(event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    menu = true;
                    break;
                case SDL_SCANCODE_M:
                    m = true;
                    break;
                case SDL_SCANCODE_SPACE:
                    space = true;
                    break;
                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP:
                    up = true;
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    left = true;
                    break;
                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN:
                    down = true;
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    right = true;
                    break;
                }
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.scancode)
                {
                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP:
                    up = false;
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    left = false;
                    break;
                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN:
                    down = false;
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    right = false;
                    break;
                }
                break;
            }   
        }   

        // made some changes to the movement logic to enable 
        // collision detection.  

        // movement change per update (assume 60 is fps?)
        int speedPerFrame = SPEED / 60;   // suggest better name?


        SDL_Rect nextPosition = shipRect.rect; 

        // we have bool (1 or 0) direction variables (up, down etc)  
        nextPosition.y += (down - up) * speedPerFrame;  // vertical movement 
        nextPosition.x += (right - left) *speedPerFrame;  // horisontal 
        char soundPath2[] = "resources/music/sse2.mp3";
        Single_sound *wall = init_sound_effect(soundPath2, 10);
        // check for collision 
        if (!collides(&nextPosition, &tilemap, WINDOW_WIDTH, WINDOW_HEIGHT)) {
            shipRect.rect = nextPosition; // here: no collision, updates position 
        } 
        // here: collision, doesnt update position (stops),  notifies collision
        else play_sound_once(wall);     

        char soundPath[] = "resources/music/sse1.mp3";
        Single_sound *kill_sound = init_sound_effect(soundPath, 30);

        if(space)
        {
            play_sound_once(kill_sound);
            free_sse(kill_sound);
            space = false;
        }
        if(m)
        {
            toggle_music();
            m = false;
        }
        if(menu)
        {
            closeWindow = mainMenu(pRenderer);
            menu = false;
        }

        SDL_RenderClear(pRenderer);
#if FOLLOW_PLAYER
        camera.x = (shipRect.x + shipRect.w / 2) - WINDOW_WIDTH / 2;
        camera.y = (shipRect.y + shipRect.h / 2) - WINDOW_HEIGHT / 2;
        SDL_Rect shipdest = follow_camera(&camera, &shipRect);
        tilemap_draw(&tilemap, pRenderer, &camera);
#else
        SDL_Rect none = { 0,0,0,0 };
        SDL_Rect shipdest = shipRect.rect;
        tilemap_draw(&tilemap, pRenderer, &none);
#endif
        SDL_RenderCopy(pRenderer, shipRect.texture, NULL, &shipdest);
#if VINGETTE
        // * psudo vingette effect
        SDL_RenderCopy(pRenderer, pVingette, NULL, &vingetteRect);
#endif
        SDL_RenderPresent(pRenderer);
        SDL_Delay(1000 / 120);//60 frames/s
    }

#if VINGETTE
    SDL_DestroyTexture(pVingette);
#endif
    tilemap_free(&tilemap);
    // SDL_DestroyTexture(pTexture);
    cleanup_SDL(pWindow, pRenderer);
    return 0;
}