// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "screenshot.h"

#include "shared.h"
#ifndef DEBUG_BUILD
void takeScreenshot(int mainScreenOnTop) {}
#else

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileio.h"
#include "debug.h"
#include "game.h"


extern uint32 radarCX, radarCY; // only required for screenshotting purposes
extern uint16 radarScale;


static unsigned char headerBMP[] =
 {
  0x42, 0x4D,
  0, 0, 0, 0,   // size of file
  0, 0,
  0, 0,
  54, 0, 0, 0,
  40, 0, 0, 0,
  0, 0, 0, 0,   // width of bitmap in pixels
  0, 0, 0, 0,   // height of bitmap in pixels
  0x01, 0,
  16, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,   // size of bmp raw data (excluding this header)
  0x13, 0x0B, 0, 0,
  0x13, 0x0B, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
 };


static void captureMainScreen(/*uint16 *buffer*/) {
//  int i;
    
    REG_DISPCAPCNT=DCAP_BANK(3)|DCAP_ENABLE|DCAP_SIZE(3);
	while(REG_DISPCAPCNT & DCAP_ENABLE);
    
    swiWaitForVBlank();
//    for (i=0; i<256*192; i++)
//        buffer[i] = VRAM_D[i];
}

static void writeHeaderBMP(FILE *fp, int width, int height) {
    unsigned char header[sizeof(headerBMP)];
    unsigned int size;
    
    memcpy(header, headerBMP, sizeof(headerBMP));
    memcpy(header + 18, &width, sizeof(unsigned int)); // set width
    memcpy(header + 22, &height, sizeof(unsigned int)); // set height
    size = width * height * 2;
    memcpy(header + 34, &size, sizeof(unsigned int)); // set raw bmp data size
    size += sizeof(headerBMP);
    memcpy(header + 2, &size, sizeof(unsigned int)); // set file size
    
    if (fwrite(header, sizeof(headerBMP), 1, fp) != 1) error("Failed to write screenshot header", "");
}

static void writeScreenAsBMP(char *location_destination, uint16 *screen) {
    FILE *fp;
    int i, j;
    uint16 color;
    
    fp = openScreenshotFile(location_destination, "wb");
    if (fp == NULL) error("Failed to open file for screenshot at:", location_destination);
    writeHeaderBMP(fp, 256, 192);
    
    for (i=0; i<192; i++) {
        for (j=0; j<256; j++) {
            color = screen[(191-i)*256 + j]; // bmp format is from bottom to top. awkward, I know.
            #ifndef SCREENSHOT_INVERT_CHANNELS
            color =
                ( ((color & 0x7C00) >> 10 ) /* blue  */       ) |
                ( ((color & 0x03E0) >> 5 )  /* green */ << 5  ) |
                ( ((color & 0x001F))        /* red   */ << 10 );
            #endif
            if (fwrite(&color, sizeof(color), 1, fp) != 1) error("Failed to write screenshot pixel at:", location_destination);
        }
    }
    
    fclose(fp);
}

static void writeScreensBMP(char *location_destination, char *location_top_screen, char*location_bottom_screen/*, uint16 *screen*/) {
    FILE *fp1, *fp2;
/*  int need_to_free = (screen == NULL);*/
    int i;
    
/*  if (need_to_free) {
        screen = (uint16 *) malloc(256*192 *sizeof(uint16));
        if (screen == NULL) error("Failed to malloc for screenshot", "");
    }*/
    
    fp1 = openScreenshotFile(location_destination, "wb");
    if (fp1 == NULL) error("Failed to open file for screenshot at:", location_destination);
    writeHeaderBMP(fp1, 256, 192*2);

    fp2 = openScreenshotFile(location_bottom_screen, "rb");
    if (fp2 == NULL) error("Failed to open screenshot file at:", location_bottom_screen);
    fseek(fp2, sizeof(headerBMP), SEEK_CUR); // set read position after header
    for (i=0; i<256*192*sizeof(uint16); i++) {
        fputc(fgetc(fp2), fp1);
    }
    fclose(fp2);
    
/*  if (fread(screen, 256*192*sizeof(uint16), 1, fp2) != 1) error("Failed to read screenshot at:", location_bottom_screen);
    fclose(fp2);
    if (fwrite(screen, 256*192*sizeof(uint16), 1, fp1) != 1) error("Failed to write screenshot at:", location_destination);*/
    
    fp2 = openScreenshotFile(location_top_screen, "rb");
    if (fp2 == NULL) error("Failed to open screenshot file at:", location_top_screen);
    fseek(fp2, sizeof(headerBMP), SEEK_CUR); // set read position after header
    for (i=0; i<256*192*sizeof(uint16); i++) {
        fputc(fgetc(fp2), fp1);
    }
    fclose(fp2);
/*  if (fread(screen, 256*192*sizeof(uint16), 1, fp2) != 1) error("Failed to read screenshot at:", location_top_screen);
    fclose(fp2);
    if (fwrite(screen, 256*192*sizeof(uint16), 1, fp1) != 1) error("Failed to write screenshot at:", location_destination);*/
    
    fclose(fp1);
    
/*  if (need_to_free) {
        free(screen);
    }*/
}

static void backupVRAM(char *location_destination, uint16 *vram) {
    uint16 buffer/*[128]*/;
    int i, j;
    
    FILE *fp = openScreenshotFile(location_destination, "wb");
    if (!fp) error("Failed to create file for screenshotting:", location_destination);
    for (i=0; i<512; i++) {
        for (j=0; j<128; j++) {
            buffer/*[j]*/ = vram[i*128 + j];
            fwrite(&buffer, sizeof(uint16), 1, fp);
        }
//      fwrite(buffer, 128*sizeof(uint16), 1, fp);
    }
    fclose(fp);
}

static void restoreVRAM(char *location_source, uint16 *vram) {
    uint16 buffer/*[128]*/;
    int i, j;
    
    FILE *fp = openScreenshotFile(location_source, "rb");
    if (!fp) error("Failed to open file for screenshotting:", location_source);
    for (i=0; i<512; i++) {
//      fread(buffer, 128*sizeof(uint16), 1, fp);
        for (j=0; j<128; j++) {
            fread(&buffer, sizeof(uint16), 1, fp);
            vram[i*128 + j] = buffer;
        }
    }
    fclose(fp);
}


extern unsigned int infoScreenType;

void takeScreenshot(int mainScreenOnTop) {
    FILE *fp;
//  uint16 *screen/*, *bmpFormat*/;
    char filenameTopBMP[50];
    char filenameBottomBMP[50];
    char filenameBMP[50];
    int i;
    
    uint32 old_REG_DISPCNT_SUB = REG_DISPCNT_SUB;
    uint32 old_REG_DISPCNT = REG_DISPCNT;
    uint16 old_BG0_CR = REG_BG0CNT;
    uint16 old_BG1_CR = REG_BG1CNT;
    uint16 old_BG2_CR = REG_BG2CNT;
    uint16 old_BG3_CR = REG_BG3CNT;
    uint16 old_BG_PALETTE[256];
    uint16 old_SPRITE_PALETTE[256];
    for (i=0; i<256; i++) {
        old_BG_PALETTE[i] = BG_PALETTE[i];
        old_SPRITE_PALETTE[i] = SPRITE_PALETTE[i];
    }
    
    // backup VRAM D and set it to be used for capturing
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;
    backupVRAM("vramD.tmp", VRAM_D);
    
    for (i=1; i<1024; i++) {
        sprintf(filenameBMP, "screenshot%i.bmp", i);
        fp = openScreenshotFile(filenameBMP, "rb");
        if (!fp) break;
        fclose(fp);
    }
    sprintf(filenameTopBMP,    "screenshot%i_top.bmp", i);
    sprintf(filenameBottomBMP, "screenshot%i_bottom.bmp", i);
    
//  screen    = (uint16 *) malloc(256*192 *sizeof(uint16));*/
/*  bmpFormat = (uint16 *) malloc(256*192 *sizeof(uint16));*/
//  if (screen == NULL /*|| bmpFormat == NULL*/) error("Failed to malloc for screenshot", "");
    
    swiWaitForVBlank();
    drawGame();
    
    captureMainScreen(/*screen*/);
/*  convertScreenToFormatBMP(screen, bmpFormat);*/
    if (mainScreenOnTop)
/*      writeScreenBMP(filenameTopBMP, bmpFormat);*/
        writeScreenAsBMP(filenameTopBMP, VRAM_D);
    else
/*      writeScreenBMP(filenameBottomBMP, bmpFormat);*/
        writeScreenAsBMP(filenameBottomBMP, VRAM_D);
    
// here I need to make sure to let sub-screen show on main-screen. pure madness.
    
    REG_DISPCNT = MODE_0_2D;
    REG_DISPCNT_SUB = MODE_0_2D;
    // backup VRAM A
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_LCD;
    backupVRAM("vramA.tmp", VRAM_A);
    // hijacking sub-screen's bg VRAM.
    VRAM_A_CR = 0;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_MAIN_BG_0x06000000;
    // read in backup VRAM D to VRAM A
    VRAM_G_CR = 0;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_LCD;
    restoreVRAM("vramD.tmp", VRAM_A);
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_SPRITE;
    // copy all sprite OAM for sub to sprite OAM for main
    for(i=0; i < 128 * sizeof(SpriteEntry) / 2; i++)
        OAM[i] = OAM_SUB[i];
    
    REG_DISPCNT = old_REG_DISPCNT_SUB;
    REG_BG0CNT = REG_BG0CNT_SUB;
    REG_BG1CNT = REG_BG1CNT_SUB;
    REG_BG2CNT = REG_BG2CNT_SUB;
    REG_BG3CNT = REG_BG3CNT_SUB;
    REG_BG2PA = radarScale;
    REG_BG2PD = radarScale;
    REG_BG2X = radarCX;
    REG_BG2Y = radarCY;
    
    if (getGameState() == INGAME && infoScreenType == 1 /* IS_BUILDINFO */)
        REG_BG0HOFS = -(4);
    
    for (i=0; i<256; i++) {
        BG_PALETTE[i] = BG_PALETTE_SUB[i];
        SPRITE_PALETTE[i] = SPRITE_PALETTE_SUB[i];
    }
    
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_SRC_BG2;
    REG_BLDALPHA = 13 | (3 << 8); //RADAR_OVERLAY_DEFAULT_BLEND_A | (RADAR_OVERLAY_DEFAULT_BLEND_B << 8);
    
    for (i=0; i<2; i++)
        swiWaitForVBlank();
    
    captureMainScreen(/*screen*/);
/*  convertScreenToFormatBMP(screen, bmpFormat);*/
    if (mainScreenOnTop)
/*      writeScreenBMP(filenameBottomBMP, bmpFormat);*/
        writeScreenAsBMP(filenameBottomBMP, VRAM_D);
    else
/*      writeScreenBMP(filenameTopBMP, bmpFormat);*/
        writeScreenAsBMP(filenameTopBMP, VRAM_D);

// afterwards, make sure that's reverted to what it used to be.

    // read in backup VRAM A to VRAM A
    VRAM_G_CR = VRAM_ENABLE | VRAM_G_MAIN_SPRITE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_LCD;
    restoreVRAM("vramA.tmp", VRAM_A);
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    // stop hijacking VRAM C
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
    // read in backup VRAM D to VRAM D
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;
    restoreVRAM("vramD.tmp", VRAM_D);
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_SUB_SPRITE;
    
    REG_DISPCNT = old_REG_DISPCNT;
    REG_BG0CNT = old_BG0_CR;
    REG_BG1CNT = old_BG1_CR;
    REG_BG2CNT = old_BG2_CR;
    REG_BG3CNT = old_BG3_CR;
    
    if (getGameState() == INGAME && infoScreenType == 1 /* IS_BUILDINFO */)
        REG_BG0HOFS = 0;
    
    for (i=0; i<256; i++) {
        BG_PALETTE[i] = old_BG_PALETTE[i];
        SPRITE_PALETTE[i] = old_SPRITE_PALETTE[i];
    }
    
    REG_BLDCNT = BLEND_NONE;
    REG_DISPCNT_SUB = old_REG_DISPCNT_SUB;
    
/*  free(screen);*/
/*  free(bmpFormat);*/
    
    writeScreensBMP(filenameBMP, filenameTopBMP, filenameBottomBMP/*, screen*/ /* already malloc'ed buffer! */);
    
/*  free(screen);*/
}

#endif
