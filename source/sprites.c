// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "sprites.h"

#include "shared.h"
#include "view.h"

#include <string.h>

extern void fastCopyOAM(void *src);


SpriteEntry spritesInfoScreen[128];
int spritesUsedInfoScreen=0;

SpriteEntry spritesPlayScreen[128];
int spritesUsedPlayScreen = 0;
int spritesPlayScreenPriorityZValue[4]; // there are four sprite priorities, like with regular 2D
enum PlayScreenSpritesMode playScreenSpritesMode = PSSM_NORMAL;


void setPlayScreenSpritesMode(enum PlayScreenSpritesMode mode) {
    playScreenSpritesMode = mode;
}

void updateOAMafterVBlank() {
    unsigned int i;
    
    for(i=0; i < 128 * sizeof(SpriteEntry) / 2; i++) {
        OAM[i]     = ((uint16*)spritesPlayScreen)[i];
        OAM_SUB[i] = ((uint16*)spritesInfoScreen)[i];
    }
}



void initSpritesUsedInfoScreen() {
    int i;
    
    spritesUsedInfoScreen = 0;
    for (i=0; i<128; i++)
	   spritesInfoScreen[i].attribute[0] = ATTR0_DISABLED;
}

void setSpriteInfoScreen(int y, unsigned int shape,
                         int x, unsigned int size, unsigned int flipX, unsigned int flipY,
                         unsigned int priority, unsigned int palette, unsigned int graphics) {
    if (spritesUsedInfoScreen == 128) return;
    y += getScreenScrollingInfoScreen();
    spritesInfoScreen[spritesUsedInfoScreen].attribute[0] = (y&0x00FF) | shape | ATTR0_COLOR_256;
    spritesInfoScreen[spritesUsedInfoScreen].attribute[1] = (x&0x01FF) | (size<<14) | (flipX*ATTR1_FLIP_X) | (flipY*ATTR1_FLIP_Y);
    spritesInfoScreen[spritesUsedInfoScreen].attribute[2] = ATTR2_PRIORITY(priority) | ATTR2_PALETTE(palette) | graphics;
    spritesUsedInfoScreen++;
}

void initSpritesUsedPlayScreen() {
    int i;
    
    spritesUsedPlayScreen = 0;
    for (i=0; i<128; i++)
        spritesPlayScreen[i].attribute[0] = ATTR0_DISABLED;
    
    spritesPlayScreenPriorityZValue[0] = 4000;
    spritesPlayScreenPriorityZValue[1] = 1500;
    spritesPlayScreenPriorityZValue[2] = -1500;
    spritesPlayScreenPriorityZValue[3] = -4000; // assuming priority 3 isn't actually used a lot unlike the others
}


void setSpriteBorderPlayScreen(int x, int y, int x1, int y1) {
    int z = 4000+1;
    
    GFX_TEX_FORMAT = 0;
    GFX_COLOR = 0x0; // black borders

//GFX_BEGIN = GL_QUADS;
    // top-left
    GFX_VERTEX16 = ((u16)y << 16) | (u16)x;
    GFX_VERTEX16 = z;
    
    // top-right
    GFX_VERTEX16 = ((u16)y << 16) | (u16)x1;
    GFX_VERTEX16 = z;
    
    // bottom-right
    GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x1;
    GFX_VERTEX16 = z;
    
    // bottom-left
    GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x;
    GFX_VERTEX16 = z;
//GFX_END = 0;

    GFX_COLOR = 0x7FFF; // white, not reducing any colours in textures
}


void setSpritePlayScreen(int y, unsigned int shape,
                         int x, unsigned int size, unsigned int flipX, unsigned int flipY,
                         unsigned int priority, unsigned int palette, unsigned int graphics) {
    
    if (playScreenSpritesMode == PSSM_NORMAL) {
        if (spritesUsedPlayScreen == 128) return;
        y += getScreenScrollingPlayScreen();
        spritesPlayScreen[spritesUsedPlayScreen].attribute[0] = (y&0x00FF) | shape | ATTR0_COLOR_256;
        spritesPlayScreen[spritesUsedPlayScreen].attribute[1] = (x&0x01FF) | (size<<14) | (flipX*ATTR1_FLIP_X) | (flipY*ATTR1_FLIP_Y);
        spritesPlayScreen[spritesUsedPlayScreen].attribute[2] = ATTR2_PRIORITY(priority) | ATTR2_PALETTE(palette) | graphics;
        spritesUsedPlayScreen++;
    } else {
        
        int sizeInPixels = 8 * math_power(2, size);
        int x1 = x + sizeInPixels;
        int y1 = y + sizeInPixels;
        int z = spritesPlayScreenPriorityZValue[priority]--;
        u16 t1 = (8<<size)<<4;
        
        GFX_TEX_FORMAT = 
            TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | GL_TEXTURE_FLIP_S | GL_TEXTURE_FLIP_T |
            (size << 20) | (size << 23) | (graphics * (16*16/8)) | (GL_RGB256 << 26);
        GFX_PAL_FORMAT = palette << 5; // (palette * 256*2) >> 4;
        
        if (flipX) {
            if (flipY) {
                
//                GFX_BEGIN = GL_QUADS;
                    // top-left
                    GFX_TEX_COORD = t1 | (t1 << 16); // 1, 1
                    GFX_VERTEX16 = ((u16)y << 16) | (u16)x;
                    GFX_VERTEX16 = z;
                    
                    // top-right
                    GFX_TEX_COORD = (2*t1) | (t1 << 16); // 2, 1
                    GFX_VERTEX16 = ((u16)y << 16) | (u16)x1;
                    GFX_VERTEX16 = z;
                    
                    // bottom-right
                    GFX_TEX_COORD = (2*t1) | ((2*t1) << 16); // 2, 2
                    GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x1;
                    GFX_VERTEX16 = z;
                    
                    // bottom-left
                    GFX_TEX_COORD = t1 | ((2*t1) << 16); // 1, 2
                    GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x;
                    GFX_VERTEX16 = z;
//                GFX_END = 0;
                
            } else {
                
//                GFX_BEGIN = GL_QUADS;
                    // top-left
                    GFX_TEX_COORD = t1; // 1, 0
                    GFX_VERTEX16 = ((u16)y << 16) | (u16)x;
                    GFX_VERTEX16 = z;
                    
                    // top-right
                    GFX_TEX_COORD = 2*t1; // 2, 0
                    GFX_VERTEX16 = ((u16)y << 16) | (u16)x1;
                    GFX_VERTEX16 = z;
                    
                    // bottom-right
                    GFX_TEX_COORD = (2*t1) | (t1 << 16); // 2, 1
                    GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x1;
                    GFX_VERTEX16 = z;
                    
                    // bottom-left
                    GFX_TEX_COORD = t1 | (t1 << 16); // 1, 1
                    GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x;
                    GFX_VERTEX16 = z;
//                GFX_END = 0;
                
            }
        } else if (flipY) {
            
//            GFX_BEGIN = GL_QUADS;
                // top-left
                GFX_TEX_COORD = (t1 << 16); // 0, 1
                GFX_VERTEX16 = ((u16)y << 16) | (u16)x;
                GFX_VERTEX16 = z;
                
                // top-right
                GFX_TEX_COORD = t1 | (t1 << 16); // 1, 1
                GFX_VERTEX16 = ((u16)y << 16) | (u16)x1;
                GFX_VERTEX16 = z;
                
                // bottom-right
                GFX_TEX_COORD = t1 | ((2*t1) << 16); // 1, 2
                GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x1;
                GFX_VERTEX16 = z;
                
                // bottom-left
                GFX_TEX_COORD = (2*t1) << 16; // 0, 2
                GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x;
                GFX_VERTEX16 = z;
//            GFX_END = 0;
            
        } else { // no flipping
            
//            GFX_BEGIN = GL_QUADS;
                // top-left
                GFX_TEX_COORD = 0; // 0, 0
                GFX_VERTEX16 = ((u16)y << 16) | (u16)x;
                GFX_VERTEX16 = z;
                
                // top-right
                GFX_TEX_COORD = t1; // 1, 0
                GFX_VERTEX16 = ((u16)y << 16) | (u16)x1;
                GFX_VERTEX16 = z;
                
                // bottom-right
                GFX_TEX_COORD = t1 | (t1 << 16); // 1, 1
                GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x1;
                GFX_VERTEX16 = z;
                
                // bottom-left
                GFX_TEX_COORD = t1 << 16; // 0, 1
                GFX_VERTEX16 = ((u16)y1 << 16) | (u16)x;
                GFX_VERTEX16 = z;
//            GFX_END = 0;
            
        }
    }
}


void init3DforExpandedSprites() {
    glInit();
    glEnable(GL_TEXTURE_2D);
//    glEnable(GL_ANTIALIAS);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(31, 0, 31, 0);
//    glClearColor(31, 0, 31, 31); // BG must be opaque for AA to work
//    glClearPolyID(63);
    glClearDepth(GL_MAX_DEPTH);
    
    
    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glOrthof32(0, 256, 192, 0, -4090, 4090);
    glViewport(0, 0, 255, 191);
    
    // ds specific, several attributes can be set here
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT);
    
    // Set the current matrix to be the model matrix
    glMatrixMode(GL_MODELVIEW);
//    glMatrixMode(GL_PROJECTION);

    GFX_COLOR = 0x7FFF; // white, not reducing any colours in textures
    
    spritesPlayScreenPriorityZValue[0] = 4000;
    spritesPlayScreenPriorityZValue[1] = 1500;
    spritesPlayScreenPriorityZValue[2] = -1500;
    spritesPlayScreenPriorityZValue[3] = -4000; // assuming priority 3 isn't actually used a lot unlike the others
}
