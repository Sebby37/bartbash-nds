// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: NightFox & Co., 2009-2011
//
// Basic text example.
// http://www.nightfoxandco.com

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

Object* load_sprite(int screen, const char *path, s32 w, s32 h, s32 id, s32 pal, s32 gfx) {
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

float randf_range(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

int randi_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void rand_bart_velocity(Vec2 *vec, u32 round) {
    const float pi = 3.1415926535897932384626;

    int speed = randi_range(100 + round * 30, 600 + round * 30) / 2;
    float angle = randf_range(0.261f, 1.309f) + randi_range(0, 3) * pi * 0.5;

    vec->x = speed * cosf(angle);
    vec->y = speed * sinf(angle);
}

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

inline bool point_in_rect(float px, float py, float rx, float ry, float rw, float rh) {
    return (px >= rx && px <= rx+rw && py >= ry && py <= ry+rh);
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    
    // Prepare a NitroFS initialization screen
    NF_Set2D(SCREEN_TOP, 0);
    NF_Set2D(SCREEN_BOTTOM, 0);
    consoleDemoInit();
    printf("\n NitroFS init. Please wait.\n\n");
    swiWaitForVBlank();

    // Initialize NitroFS and set it as the root folder of the filesystem
    if (!nitroFSInit(NULL)) return 1;
    NF_SetRootFolder("NITROFS");

    { // Maxmod (sound) inits
        mus_init();
        mmInitDefault("maxmod/soundbank.bin");
        mmLoadEffect(SFX_CONGRATS);
        mmLoadEffect(SFX_OW);
        mus_play("nitro:/mus/bartbash.raw");
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
        NF_LoadTiledBg("bg/bartbash", "bartbash", 256, 256);
        NF_LoadTiledBg("bg/barttop", "barttop", 256, 256);

        // Create backgrounds
        NF_CreateTiledBg(SCREEN_TOP, 3, "barttop");
        NF_CreateTiledBg(SCREEN_BOTTOM, 3, "bartbash");
    }

    { // Setup sprites
        NF_InitSpriteBuffers();
        NF_InitSpriteSys(SCREEN_BOTTOM);
    }

    const int num_barts = 25;
    Object *barts[num_barts];
    { // Load the barts!
        const char *path = "spr/bart";
        const int bart_pal = 0, bart_gfx = 0;

        // Load bart palette
        NF_LoadSpritePal(path, bart_pal);
        NF_VramSpritePal(SCREEN_BOTTOM, bart_pal, bart_pal);

        // Load bart sprite
        NF_LoadSpriteGfx(path, bart_gfx, 16, 32);
        NF_VramSpriteGfx(SCREEN_BOTTOM, bart_gfx, bart_gfx, false);

        // Create barts!
        for (int i = 0; i < num_barts; i++) {
            Object *bart = load_sprite(SCREEN_BOTTOM, path, 16, 32, i, bart_pal, bart_gfx);
            bart->pos.x = randf_range(0, 256-bart->w);
            bart->pos.y = randf_range(0, 192-bart->h);
            rand_bart_velocity(&bart->vel, 1);
            barts[i] = bart;
        }
    }

    Boom booms[num_barts];
    const char *boom_path = "spr/boom_half";
    const int boom_pal = 1, boom_gfx = 1;
    int boom_available_id = num_barts; // The next available ID for the boom is after the barts
    { // Load the booms!
        NF_LoadSpritePal(boom_path, boom_pal);
        NF_VramSpritePal(SCREEN_BOTTOM, boom_pal, boom_pal);

        NF_LoadSpriteGfx(boom_path, boom_gfx, 32, 32);
        NF_VramSpriteGfx(SCREEN_BOTTOM, boom_gfx, boom_gfx, false); // Anim: keep unused frames in RAM

        for (int i = 0; i < num_barts; i++) {
            booms[i].obj = NULL;
            booms[i].frame = 0;
            booms[i].maxFrames = 17;
        }
    }

    // Create text layers
    NF_InitTextSys(SCREEN_TOP); // Top screen
    NF_LoadTextFont("fnt/ComicMono", "comic", 256, 256, 0); // Load normal text
    NF_CreateTextLayer(SCREEN_TOP, 0, 0, "comic");

    // Update text layers
    NF_UpdateTextLayers();
    
    bool just_tapped = false;
    while (1)
    {
        scanKeys();
        if (keysDown() & KEY_A)
            mmEffect(SFX_CONGRATS);
        if (keysDown() & KEY_B)
            mus_stop();
        if (keysDown() & KEY_START)
            mus_play("nitro:/mus/bartbash.raw");
        
        if (keysDown() & KEY_TOUCH) {
            if (!just_tapped) just_tapped = true;
            else              just_tapped = false;
        } else {
            just_tapped = false;
        }
        
        // Touch!
        touchPosition touch;
        touchRead(&touch);
        
        // Bart!
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
                }
            } else if (bart != NULL && !bart->enabled) {
                // Boom! But only if we have the room :)
                if (boom_available_id < 127) {
                    boom->obj = load_sprite(SCREEN_BOTTOM, boom_path, 32, 32, boom_available_id++, boom_pal, boom_gfx);
                    boom->obj->pos.x = bart->pos.x;
                    boom->obj->pos.y = bart->pos.y;
                }

                NF_DeleteSprite(bart->screen, bart->id);
                barts[i] = NULL;
            }

            // Boom update
            if (boom->obj) {
                NF_MoveSprite(SCREEN_BOTTOM, boom->obj->id, boom->obj->pos.x, boom->obj->pos.y);
                NF_SpriteFrame(SCREEN_BOTTOM, boom->obj->id, boom->frame);
                
                // Boom done!
                boom->frame++;
                if (boom->frame >= boom->maxFrames) {
                    NF_DeleteSprite(SCREEN_BOTTOM, boom->obj->id);
                    boom_available_id--;
                    free(boom->obj);
                    boom->obj = NULL;
                }
            }
        }

        // Update text layers
        // NF_WriteText(SCREEN_TOP, 0, 100, 100, "Woah");
        NF_UpdateTextLayers();

        // Update sprite stuff
        NF_SpriteOamSet(SCREEN_BOTTOM);
        oamUpdate(&oamSub);

        // Wait for the screen refresh
        mus_update();
        swiWaitForVBlank();
    }

    return 0;
}
