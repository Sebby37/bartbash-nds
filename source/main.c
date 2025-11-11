// Written by SebC :)
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <nds.h>
#include <filesystem.h>

#include <nf_lib.h>
#include <maxmod9.h>

#include "nitrofs/soundbank_info.h"

#include "music.h"

#define SCREEN_TOP 0
#define SCREEN_BOTTOM 1

const float dt = 0.0167f; // Deltatime, fixed on the NDS which runs at 60fps

typedef struct Vec2 {
    float x;
    float y;
} Vec2;

typedef struct Object {
    int screen;
    s32  id;
    bool enabled;
    Vec2 pos;
    s32  w,h;
    Vec2 vel;
} Object;

typedef struct Boom {
    Object *obj;
    u8 frame;
    u8 maxFrames;
} Boom;

// Util: Returns a random float between min and max
float randf_range(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

// Util: Returns a random int between min and max
int randi_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Util: Return whether a point resides within some rect
inline bool point_in_rect(float px, float py, float rx, float ry, float rw, float rh) {
    return (px >= rx && px <= rx+rw && py >= ry && py <= ry+rh);
}

// Loads a sprite and returns an Object struct reference to it
Object* load_sprite(int screen, s32 w, s32 h, s32 id, s32 pal, s32 gfx) {
    Object *obj = (Object*)malloc(sizeof(Object)); // Yucky heap memory, but I feel like we need it here :(

    obj->screen = screen;
    obj->id = id;
    obj->enabled = true;

    obj->pos.x = 0;
    obj->pos.y = 0;

    obj->w = w;
    obj->h = h;

    obj->vel.x = 0;
    obj->vel.y = 0;
    
    // Finally, create the sprite!
    NF_CreateSprite(screen, id, gfx, pal, 0, 0);

    return obj;
}

// Basic update loop for an object, which in this case is just barts and having him bounce off edges
void update_obj(Object *obj) {
    // Apply velocity
    obj->pos.x += obj->vel.x * dt;
    obj->pos.y += obj->vel.y * dt;

    // Bounce off edge of screen
    if (obj->pos.x < 0)           { obj->pos.x = 0;           obj->vel.x *= -1; }
    if (obj->pos.x+obj->w > 256)  { obj->pos.x = 256-obj->w;  obj->vel.x *= -1; }
    if (obj->pos.y < 0)           { obj->pos.y = 0;           obj->vel.y *= -1; }
    if (obj->pos.y+obj->h > 192)  { obj->pos.y = 192-obj->h;  obj->vel.y *= -1; }

    // Draw in real life!
    NF_MoveSprite(SCREEN_BOTTOM, obj->id, obj->pos.x, obj->pos.y);
}

// Given the current round, give bart a random speed in a random direction
// Logic straight from the actual game
void rand_bart_velocity(Vec2 *vec, u32 round) {
    const float pi = 3.1415926535897932384626;

    int speed = randi_range(100 + round * 30, 600 + round * 30) / 2; // Halve it because the original game runs at a ~512x512 world
    float angle = randf_range(0.261f, 1.309f) + randi_range(0, 3) * pi * 0.5;

    vec->x = speed * cosf(angle);
    vec->y = speed * sinf(angle);
}

// Global game vars!
unsigned int score = 0;
const int bart_pal = 0, bart_gfx = 0;
const int boom_pal = 1, boom_gfx = 1;
// The actual game itself!
void bartbash(int round) {
    int num_barts = round < 13 ? round * 5 : 64; // Can't really be bigger than 64 because of my lazy solution for the explosions
    int barts_left = num_barts;
    
    // Create barts!
    Object *barts[num_barts];
    for (int i = 0; i < num_barts; i++) {
        Object *bart = load_sprite(SCREEN_BOTTOM, 16, 32, i, bart_pal, bart_gfx);
        bart->pos.x = randf_range(0, 256-bart->w);
        bart->pos.y = randf_range(0, 192-bart->h);
        rand_bart_velocity(&bart->vel, round);
        barts[i] = bart;
    }

    // Setup booms (no load though)
    Boom booms[num_barts];
    for (int i = 0; i < num_barts; i++) {
        booms[i].obj = NULL;
        booms[i].frame = 0;
        booms[i].maxFrames = 17;
    }

    // Main loop time!
    bool just_tapped = false;
    u8 timer = 30, frame = 0, end_timer = 0;
    while (1)
    {
        // TAP TIME
        scanKeys();
        if (keysDown() & KEY_TOUCH) {
            if (!just_tapped) just_tapped = true;
            else              just_tapped = false;
        } else {
            just_tapped = false;
        }
        
        // Touch!
        touchPosition touch;
        touchRead(&touch);
        
        // Bart! (Object updates)
        for (int i = 0; i < num_barts; i++) {
            Object *bart = barts[i];
            Boom *boom = &booms[i];
            
            // Bart update
            if (bart != NULL && bart->enabled) {
                update_obj(bart);

                // Did the bart be clicked?
                if (just_tapped && point_in_rect(touch.px, touch.py, bart->pos.x, bart->pos.y, bart->w, bart->h)) {
                    bart->enabled = false;
                    mmEffect(SFX_OW);
                    if (--barts_left <= 0)
                        mmEffect(SFX_CONGRATS);
                    score += 50;
                }
            } else if (bart != NULL && !bart->enabled) {
                // Boom! But only if we have the room :)
                // Cheat hack awful silly solution, but basically each bart has a corresponding boom
                // It's a waste of memory and it means we can't have >64 barts that have explosions, but its sooooo easy to do
                if (num_barts+i <= 127) {
                    boom->obj = load_sprite(SCREEN_BOTTOM, 32, 32, num_barts+i, boom_pal, boom_gfx);
                    boom->obj->pos.x = bart->pos.x - 8;
                    boom->obj->pos.y = bart->pos.y;
                }

                NF_DeleteSprite(bart->screen, bart->id);
                free(barts[i]); // MEMORY MANAGEMENT WOOOOOO
                barts[i] = NULL;
            }

            // Boom update
            if (boom->obj) {
                NF_MoveSprite(SCREEN_BOTTOM, boom->obj->id, boom->obj->pos.x, boom->obj->pos.y);
                NF_SpriteFrame(SCREEN_BOTTOM, boom->obj->id, (boom->frame/2));
                
                // Boom done!
                boom->frame++;
                if ((boom->frame/2) >= boom->maxFrames) {
                    NF_DeleteSprite(SCREEN_BOTTOM, num_barts+i);
                    free(boom->obj); // Heap memory babyyyy!
                    boom->obj = NULL;
                }
            }
        }

        // Update text layers
        char top_str[32];
        NF_ClearTextLayer(SCREEN_TOP, 0); NF_ClearTextLayer16(SCREEN_TOP, 1);
        sprintf(top_str, "ROUND %d", round);            NF_WriteText(SCREEN_TOP,   0, 12, 5, top_str);
        sprintf(top_str, "Score: %u", score);           NF_WriteText(SCREEN_TOP,   0, 2, 8,  top_str);
        sprintf(top_str, "Barts Left: %d", barts_left); NF_WriteText(SCREEN_TOP,   0, 2, 10, top_str);
        sprintf(top_str, "%hhu", timer);                NF_WriteText16(SCREEN_TOP, 1, 15, 9, top_str); 
                                                        NF_WriteText16(SCREEN_TOP, 1, 24, 4, top_str);
        NF_UpdateTextLayers();

        // Update sprite stuff
        NF_SpriteOamSet(SCREEN_BOTTOM);
        oamUpdate(&oamSub);

        // Wait for the screen refresh
        mus_update();
        swiWaitForVBlank();
        
        // Timer timing
        frame++;
        if (frame >= 60 && barts_left > 0) {
            frame = 0;
            timer--;
        }
        if (timer == 0)
            break;
        
        // Wait for sound to finito
        if (barts_left <= 0)
            end_timer++;
        if (end_timer >= 120) // Wait 2 seconds
            break;
    }

    // Did we suck?
    if (timer == 0 && num_barts != 0) {
        // Uh oh we did! EAT MY SHORTS!

        // Im having the strangest bug where the text on the top is mirrored to the bottom
        // So I tried to fix it by clearing the other text layers, but now it mirrors the bottom text to the top
        // And if I tried to create the text layer before the bartbash() call I run out of memory I think
        // So I'm just gonna settle with it saying "eat my shorts" on the top and bottom screen when you lose
        NF_UpdateTextLayers();
        NF_ClearTextLayer(SCREEN_TOP, 0); NF_ClearTextLayer16(SCREEN_TOP, 1); NF_UpdateTextLayers();
        NF_CreateTextLayer(SCREEN_BOTTOM, 2, 0, "comic");
        NF_WriteText(SCREEN_BOTTOM, 2, 2, 9, "GAME OVER");
        NF_WriteText(SCREEN_BOTTOM, 2, 2, 11, "\"Eat my shorts!\"");
        NF_UpdateTextLayers();

        // Remove the barts
        for (int i = 0; i < num_barts; i++) {
            if (barts[i] != NULL) {
                NF_DeleteSprite(SCREEN_BOTTOM, barts[i]->id);
                free(barts[i]);
            }
        }
        NF_SpriteOamSet(SCREEN_BOTTOM);
        oamUpdate(&oamSub);

        // You STINK!
        while (true) {
            mus_update();
            swiWaitForVBlank();
        }
    }
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    
    // Prepare a NitroFS initialization screen
    NF_Set2D(SCREEN_TOP, 0);
    NF_Set2D(SCREEN_BOTTOM, 0);
    swiWaitForVBlank();

    // Initialize NitroFS and set it as the root folder of the filesystem
    if (!nitroFSInit(NULL)) return 1;
    NF_SetRootFolder("NITROFS");

    { // Maxmod (sound) inits
        mus_init();
        mmInitDefault("maxmod/soundbank.bin");
        mmLoadEffect(SFX_CONGRATS);
        mmLoadEffect(SFX_OW);
    }

    // Initialize 2D engine in both screens and use mode 0
    NF_Set2D(SCREEN_TOP, 0);
    NF_Set2D(SCREEN_BOTTOM, 0);

    { // Background inits
        // Initialize tiled backgrounds system
        NF_InitTiledBgBuffers();    // Initialize storage buffers
        NF_InitTiledBgSys(SCREEN_TOP);       // Top screen
        NF_InitTiledBgSys(SCREEN_BOTTOM);       // Bottom screen

        // Load background files from NitroFS
        NF_LoadTiledBg("bg/bartbash-ready", "bartbash-ready", 256, 256);
        NF_LoadTiledBg("bg/barttop", "barttop", 256, 256);

        // Create backgrounds
        NF_CreateTiledBg(SCREEN_TOP, 3, "barttop");
        NF_CreateTiledBg(SCREEN_BOTTOM, 3, "bartbash-ready");
    }

    { // Setup sprites
        NF_InitSpriteBuffers();
        NF_InitSpriteSys(SCREEN_BOTTOM);
    }
    
    { // Load bart gfx/sprites
        const char *path = "spr/bart";

        // Load bart palette
        NF_LoadSpritePal(path, bart_pal);
        NF_VramSpritePal(SCREEN_BOTTOM, bart_pal, bart_pal);
        NF_UnloadSpritePal(bart_pal);

        // Load bart sprite
        NF_LoadSpriteGfx(path, bart_gfx, 16, 32);
        NF_VramSpriteGfx(SCREEN_BOTTOM, bart_gfx, bart_gfx, false);
        NF_UnloadSpriteGfx(bart_gfx);
    }

    { // Load the boom gfx/sprites
        const char *boom_path = "spr/boom_half";

        NF_LoadSpritePal(boom_path, boom_pal);
        NF_VramSpritePal(SCREEN_BOTTOM, boom_pal, boom_pal);
        NF_UnloadSpritePal(boom_pal); // Its already in VRAM so free da slot

        NF_LoadSpriteGfx(boom_path, boom_gfx, 32, 32);
        NF_VramSpriteGfx(SCREEN_BOTTOM, boom_gfx, boom_gfx, false);
        NF_UnloadSpriteGfx(boom_gfx);
    }

    { // Create text layers
        NF_InitTextSys(SCREEN_TOP); // Top screen
        NF_InitTextSys(SCREEN_BOTTOM);
        NF_LoadTextFont("fnt/default", "comic", 256, 256, 0); // Load normal text
        NF_CreateTextLayer(SCREEN_TOP, 0, 0, "comic");
        NF_LoadTextFont16("fnt/font16", "impact", 256, 256, 0); // 8x16 font
        NF_CreateTextLayer16(SCREEN_TOP, 1, 0, "impact");
        NF_UpdateTextLayers();
    }

    // Wait for the user to consent to bashing barts
    swiWaitForVBlank();
    while (true) {
        scanKeys();
        if (keysDown() & ~(KEY_LID | KEY_DEBUG))
            break;
        swiWaitForVBlank();
    }

    // Begin the game!
    NF_UnloadTiledBg("bartbash-ready");
    NF_LoadTiledBg("bg/bartbash", "bartbash", 256, 256);
    NF_CreateTiledBg(SCREEN_BOTTOM, 3, "bartbash");
    swiWaitForVBlank();
    mus_play("nitro:/mus/bartbash.raw", 22050);
    for (int round = 1;;round++) bartbash(round); // Boy do I love funny for loops

    return 0;
}
