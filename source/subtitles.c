// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "subtitles.h"

#include "sprites.h"
#include "fileio.h"


#define SUBTITLES_FIRST_SYMBOL  32
#define SUBTITLES_LAST_SYMBOL   175
#define SUBTITLES_NUMBER_OF_SYMBOLS   ((SUBTITLES_LAST_SYMBOL + 1) - SUBTITLES_FIRST_SYMBOL)

#define SUBTITLES_SPACING  1


uint8 subtitleFontWidth[SUBTITLES_NUMBER_OF_SYMBOLS];
char subtitleLines[128 + 1]; // can't be more than 128 due to it using sprites




int calculateSubtitleLineWidth(char *line, int maxLength) {
    int i;
    int result = 0;
    
    if (maxLength == 0 || *line == 0)
        return result;
    
    for (i=0; i<maxLength && *line; i++, line++) {
        result += subtitleFontWidth[*line - SUBTITLES_FIRST_SYMBOL];
        result += SUBTITLES_SPACING;
    }
    return result - SUBTITLES_SPACING;
}

void drawSubtitle(char *line, int maxLength, int y) {
    int i;
    int x = SCREEN_WIDTH / 2 - calculateSubtitleLineWidth(line, maxLength) / 2;
    
    for (i=0; i<maxLength && *line; i++, line++) {
        setSpriteInfoScreen(y, ATTR0_SQUARE,
                            x, SPRITE_SIZE_8, 0, 0,
                            0, 0, *line - SUBTITLES_FIRST_SYMBOL);
        x += subtitleFontWidth[*line - SUBTITLES_FIRST_SYMBOL];
        x += SUBTITLES_SPACING;
    }
}

int searchSubtitleNewlineSymbol(char *lines) {
    int i;
    for (i=0; i<128 && *lines; i++, lines++) {
        if (*lines == '\\')
            return i;
    }
    return -1;
}

void drawSubtitles() {
    int newLineSymbolAt = searchSubtitleNewlineSymbol(subtitleLines);
    if (newLineSymbolAt == -1)
        drawSubtitle(subtitleLines, 128, 192 - 28);
    else {
        drawSubtitle(subtitleLines, newLineSymbolAt, 192 - 28);
        drawSubtitle(subtitleLines + newLineSymbolAt + 1, (128 - newLineSymbolAt) - 1, 192 - 14);
    }
}

void setSubtitles(char *lines) {
    strncpy(subtitleLines, lines, 128);
}

void loadSubtitlesGraphics() {
    // assuming it's the subscreen, and that no other sprites are used. scandalous.
    copyFileVRAM(SPRITE_PALETTE_SUB, "subtitles", FS_PALETTES);
    copyFileVRAM(SPRITE_GFX_SUB, "subtitles", FS_SUBTITLES_GRAPHICS);
}

void initSubtitles() {
    char oneline[256];
    FILE *fp;
    int i, j;
    
    fp = openFile("subtitles", FS_SUBTITLES_INFO);
    for (i=0; i<SUBTITLES_NUMBER_OF_SYMBOLS; i++) {
        readstr(fp, oneline);
        sscanf(oneline, "%i", &j);
        subtitleFontWidth[i] = j;
    }
    closeFile(fp);
}
