// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "cutscene_debriefing.h"

#include "gif/gif.h"
#include "animation.h"
#include "factions.h"
#include "info.h"
#include "settings.h"
#include "inputx.h"
#include "fileio.h"

// win / stats / cinematic / tally [/ outtro]*              *=this one isn't handled here, but later on
// lose        / cinematic
enum CutsceneDebriefingState { CDS_WIN_YOUWIN, CDS_WIN_MENU_STATISTICS, CDS_WIN_INI, CDS_WIN_MENU_TALLY,
                               CDS_LOSE_GAMEOVER, CDS_LOSE_INI };

enum MedalTypeCondition { MTC_NONE, MTC_TIME, MTC_UNITS_LOST, MTC_UNITS_KILL, MTC_STRUCTURES_LOST, MTC_STRUCTURES_KILL };

static enum CutsceneDebriefingState state;
static char animationFilename[256];
static enum MedalTypeCondition medalTypeCondition;
static int medalGoldCondition;
static int medalSilverCondition;


static void drawNumberRightAligned(int number, int x, int y, int precision) {
    int digit;
    
    x -= 8;
    
    while (number > 0 || precision > 0) {
        digit = number % 10;
        
        setSpritePlayScreen(y, ATTR0_TALL,
                            x, SPRITE_SIZE_8, 0, 0,
                            0, 0, digit*2);
        
        number = number / 10;
        
        if (precision > 0)
            precision--;
        
        x -= 8;
    }
}


void drawCutsceneDebriefing() {
    enum Medal medalType;
    int medalCount[3];
    int deathCount;
    int i;
    
    initSpritesUsedPlayScreen();
    initSpritesUsedInfoScreen();
    
    if (state == CDS_WIN_MENU_STATISTICS) {
        // enemy unit deaths
        deathCount = 0;
        for (i=1; i<getAmountOfSides(); i++)
            deathCount += getUnitDeaths(i);
        if (deathCount > 9999)
            deathCount = 9999;
        drawNumberRightAligned(deathCount, 247, 61, 1);
        
        // enemy structure deaths
        deathCount = 0;
        for (i=1; i<getAmountOfSides(); i++)
            deathCount += getStructureDeaths(i);
        if (deathCount > 9999)
            deathCount = 9999;
        drawNumberRightAligned(deathCount, 247, 75, 1);
        
        // friendly unit deaths
        deathCount = getUnitDeaths(FRIENDLY); // already capped at 9999
        drawNumberRightAligned(deathCount, 247, 93, 1);
        
        // friendly structure deaths
        deathCount = getStructureDeaths(FRIENDLY); // already capped at 9999
        drawNumberRightAligned(deathCount, 247, 106, 1);
        
        // time played
        drawNumberRightAligned(getMatchTime() % 60, 247, 125, 2);
        drawNumberRightAligned((getMatchTime() / 60) % 60, 229, 125, 2);
        drawNumberRightAligned(getMatchTime() / (60*60), 210, 125, 1);
        setSpritePlayScreen(125, ATTR0_TALL, // :
                            210, SPRITE_SIZE_8, 0, 0,
                            0, 0, 10*2);
        setSpritePlayScreen(125, ATTR0_TALL, // .
                            229, SPRITE_SIZE_8, 0, 0,
                            0, 0, 11*2);
        
        if (medalTypeCondition == MTC_TIME) {
            // gold medal time
            drawNumberRightAligned(medalGoldCondition % 60, 247, 146, 2);
            drawNumberRightAligned((medalGoldCondition / 60) % 60, 229, 146, 2);
            drawNumberRightAligned(medalGoldCondition / (60*60), 210, 146, 1);
            setSpritePlayScreen(146, ATTR0_TALL, // :
                                210, SPRITE_SIZE_8, 0, 0,
                                0, 0, 10*2);
            setSpritePlayScreen(146, ATTR0_TALL, // .
                                229, SPRITE_SIZE_8, 0, 0,
                                0, 0, 11*2);
            
            // silver medal time
            drawNumberRightAligned(medalSilverCondition % 60, 144, 146, 2);
            drawNumberRightAligned((medalSilverCondition / 60) % 60, 126, 146, 2);
            drawNumberRightAligned(medalSilverCondition / (60*60), 107, 146, 1);
            setSpritePlayScreen(146, ATTR0_TALL, // :
                                107, SPRITE_SIZE_8, 0, 0,
                                0, 0, 10*2);
            setSpritePlayScreen(146, ATTR0_TALL, // .
                                126, SPRITE_SIZE_8, 0, 0,
                                0, 0, 11*2);
        }
        
        if (medalTypeCondition != MTC_NONE) { // "time" or "kill"
            setSpritePlayScreen(145, ATTR0_WIDE,
                                15, SPRITE_SIZE_32, 0, 0,
                                0, 0, (8*1024) / (8*8));
        }
        
        if (getMedalReceived(getFaction(FRIENDLY), getLevel()-1) != MEDAL_GOLD)
            setSpritePlayScreen(168, ATTR0_WIDE, // retry
                                0, SPRITE_SIZE_64, 0, 0,
                                0, 0, (12*8*16) / (8*8));
    }
    
    if (state == CDS_WIN_MENU_TALLY) {
        for (i=0; i<3; i++)
            medalCount[i] = 0;
        
        for (i=1; i<=9; i++) {
            medalType = getMedalReceived(getFaction(FRIENDLY), i);
            if (medalType >= MEDAL_BRONZE && medalType <= MEDAL_GOLD)
                medalCount[medalType]++;
        }
        
        for (i=0; i<3; i++)
            setSpritePlayScreen(120 - 24*i, ATTR0_SQUARE,
                                86, SPRITE_SIZE_16, 0, 0,
                                0, 0, medalCount[i] * 4);
    }
    
    drawAnimation();
}

void drawCutsceneDebriefingBG() {
    drawAnimationBG();
    REG_DISPCNT |= DISPLAY_BG3_ACTIVE;
}

void setCutsceneDebriefingState(enum CutsceneDebriefingState state_requested) {
    enum Medal medalAnimationType;
    pGifHandler medalPictureAni;
    int i, j, k;
    
        REG_MASTER_BRIGHT = (2<<14) | 16; // 2 for downing brightness from original
    REG_MASTER_BRIGHT_SUB = (2<<14) | 16;
    
    initSpritesUsedPlayScreen();
    initSpritesUsedInfoScreen();
    updateKeys(); // make sure key presses don't carry over to other states
    updateOAMafterVBlank();
    
    state = state_requested;
    switch (state) {
        case CDS_WIN_MENU_STATISTICS:
            copyFileVRAM(BG_GFX, "menu_statistics", FS_MENU_GRAPHICS);
            copyFileVRAM(BG_PALETTE, "menu_statistics", FS_PALETTES);
            copyFileVRAM(SPRITE_GFX, "menu_statistics_font", FS_MENU_GRAPHICS);
            copyFileVRAM(SPRITE_GFX + ((12*8*16)>>1), "menu_bit_retry", FS_MENU_GRAPHICS);
            
            if (medalTypeCondition != MTC_NONE) {
                if (medalTypeCondition == MTC_TIME)
                    copyFileVRAM(SPRITE_GFX + ((8*1024)>>1), "stats_text_time", FS_MENU_GRAPHICS);
                else
                    copyFileVRAM(SPRITE_GFX + ((8*1024)>>1), "stats_text_kills", FS_MENU_GRAPHICS);
            }


            copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
            medalAnimationType = getMedalReceived(getFaction(FRIENDLY), getLevel()-1);
            if (medalAnimationType == MEDAL_GOLD)
                initAnimation("medal_gold", 1, 0, 0);
            else if (medalAnimationType == MEDAL_SILVER)
                initAnimation("medal_silver", 1, 0, 0);
            else
                initAnimation("medal_bronze", 1, 0, 0);
            break;
        case CDS_WIN_INI:
            if (!initAnimation(animationFilename, 0, 0, 0))
                loadAnimationGraphics(CUTSCENE_DEBRIEFING);
            else { // set gamestate to next level, or next state if TALLY exists
                if (getLevel() <= 9) {
                    setGameState(MENU_REGIONS);
                    return;
                } else {
                    setCutsceneDebriefingState(CDS_WIN_MENU_TALLY);
                    return;
                }
            }
            break;
        case CDS_WIN_MENU_TALLY:
            copyFileVRAM(BG_GFX, "menu_tally", FS_MENU_GRAPHICS);
            copyFileVRAM(BG_PALETTE, "menu_tally", FS_PALETTES);
            copyFileVRAM(SPRITE_GFX, "menu_tally_font", FS_MENU_GRAPHICS);
            copyFileVRAM(SPRITE_PALETTE, "menu_bits", FS_PALETTES);
            
            medalAnimationType = getFactionMedalReceived(getFaction(FRIENDLY));
            
            if (medalAnimationType == MEDAL_GOLD)
                medalPictureAni = gifHandlerLoad(openFile("medal_gold_small", FS_ANIMATIONS_GRAPHICS), NULL);
            else if (medalAnimationType == MEDAL_SILVER)
                medalPictureAni = gifHandlerLoad(openFile("medal_silver_small", FS_ANIMATIONS_GRAPHICS), NULL);
            else
                medalPictureAni = gifHandlerLoad(openFile("medal_bronze_small", FS_ANIMATIONS_GRAPHICS), NULL);
            
            if (medalPictureAni != NULL) {
                for (i=0; i<medalPictureAni->dimY; i++) {
                    k = ((48*1024) >> 1) + i*128;
                    for (j=0; j<medalPictureAni->dimX; j++, k++)
                        BG_GFX[k] = BIT(15) | medalPictureAni->gifPalette[medalPictureAni->gifFrame[i*medalPictureAni->dimX + j]];
                    for (; j<128; j++, k++)
                        BG_GFX[k] = 0;
                }
                for (; i<128; i++) {
                    k = ((48*1024) >> 1) + i*128;
                    for (j=0; j<128; j++, k++)
                        BG_GFX[k] = 0;
                }
                gifHandlerDestroy(medalPictureAni);
                // just to reiterate (apparently I overwrite these somewhere badly?)
                REG_DISPCNT = ( MODE_5_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 | DISPLAY_BG3_ACTIVE | DISPLAY_BG2_ACTIVE);
                REG_BG3CNT  = BG_BMP8_256x256  | BG_PRIORITY_3;
                REG_BG2CNT  = BG_BMP16_128x128 | BG_PRIORITY_2 | BG_BMP_BASE(3);
                REG_BG2PA   = 1<<8; // scaling of 1
                REG_BG2PB   = 0;
                REG_BG2PC   = 0;
                REG_BG2PD   = 1<<8; // scaling of 1
                REG_BG2X    = -(172 << 8);
                REG_BG2Y    = -( 72 << 8);
            }
            
            if (medalAnimationType == MEDAL_GOLD)
                initAnimation("medal_gold", 1, 0, 0);
            else if (medalAnimationType == MEDAL_SILVER)
                initAnimation("medal_silver", 1, 0, 0);
            else
                initAnimation("medal_bronze", 1, 0, 0);
            break;
        case CDS_LOSE_INI:
            if (!initAnimation(animationFilename, 0, 0, 0))
                loadAnimationGraphics(CUTSCENE_DEBRIEFING);
            else { // go to MENU_REGIONS
                setGameState(MENU_REGIONS);
                return;
            }
            break;
        default:
            break;
    }
    
        REG_MASTER_BRIGHT = 0;
    REG_MASTER_BRIGHT_SUB = 0;
}

void doCutsceneDebriefingLogic() {
    enum Medal medalAnimationType;
    
    if (state == CDS_WIN_MENU_STATISTICS) {
        medalAnimationType = getMedalReceived(getFaction(FRIENDLY), getLevel()-1);
        
        if (medalAnimationType != MEDAL_GOLD && ((keysDown() & KEY_B) || ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px < 48 && touchReadLast().py >= 169))) { // Replay Level
            setLevel(getLevel()-1);
            setGameState(INGAME);
            return;
        } else if ((keysDown() & KEY_A) || ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 160 && touchReadLast().py >= 169)) { // Continue
            setCutsceneDebriefingState(CDS_WIN_INI);
            return;
        }
    } else if (state == CDS_WIN_MENU_TALLY) {
        if ((keysDown() & KEY_A) || ((getKeysOnUp() & KEY_TOUCH) && touchReadLast().px >= 160 && touchReadLast().py >= 169)) { // Continue
            if (isLevelLoadedViaLevelSelect())
                setGameState(MENU_LEVELSELECT);
            else
                setGameState(OUTTRO);
            return;
        }
    }
    
    if (doAnimationLogic()) {
        switch (state) {
            case CDS_WIN_YOUWIN: // next state
                setCutsceneDebriefingState(CDS_WIN_MENU_STATISTICS);
                break;
            case CDS_WIN_MENU_STATISTICS: // remain in the same state, loop animation
                medalAnimationType = getMedalReceived(getFaction(FRIENDLY), getLevel()-1);
                if (medalAnimationType == MEDAL_GOLD)
                    initAnimation("medal_gold", 1, 1, 0);
                else if (medalAnimationType == MEDAL_SILVER)
                    initAnimation("medal_silver", 1, 1, 0);
                else
                    initAnimation("medal_bronze", 1, 1, 0);
                break;
            case CDS_WIN_INI: // set gamestate to next level, or next state if TALLY exists
                if (isLevelLoadedViaLevelSelect()) {
                    if (getMedalReceived(getFaction(FRIENDLY), getLevel()-1) == MEDAL_GOLD &&
                        getFactionMedalReceived(getFaction(FRIENDLY)) == MEDAL_GOLD) {
                        setCutsceneDebriefingState(CDS_WIN_MENU_TALLY);
                    } else
                        setGameState(MENU_LEVELSELECT);
                    break;
                }
                
                if (getLevel() <= 9)
                    setGameState(MENU_REGIONS);
                else
                    setCutsceneDebriefingState(CDS_WIN_MENU_TALLY);
                break;
            case CDS_WIN_MENU_TALLY: // remain in the same state, loop animation
                medalAnimationType = getFactionMedalReceived(getFaction(FRIENDLY));
                if (medalAnimationType == MEDAL_GOLD)
                    initAnimation("medal_gold", 1, 1, 0);
                else if (medalAnimationType == MEDAL_SILVER)
                    initAnimation("medal_silver", 1, 1, 0);
                else
                    initAnimation("medal_bronze", 1, 1, 0);
                break;
            case CDS_LOSE_GAMEOVER:
                setCutsceneDebriefingState(CDS_LOSE_INI);
                break;
            case CDS_LOSE_INI:
                if (isLevelLoadedViaLevelSelect())
                    setGameState(MENU_LEVELSELECT);
                else
                    setGameState(MENU_REGIONS);
                break;
        }
    }
}

void loadCutsceneDebriefingGraphics(enum GameState oldState) {
    loadAnimationGraphics(oldState);
}

int initCutsceneDebriefing() {
    enum Medal medalReceived, factionMedalReceived;
    char filename[256];
    int i, j;
    
    REG_DISPCNT = ( MODE_5_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 );
    REG_BG3CNT  = BG_BMP8_256x256  | BG_PRIORITY_3;
/*  REG_BG3PA   = 1<<8; // scaling of 1
    REG_BG3PB   = 0;
    REG_BG3PC   = 0;
    REG_BG3PD   = 1<<8; // scaling of 1*/
    REG_BG3X    = 0;
    REG_BG3Y    = 0;
    
    if (state == CDS_WIN_YOUWIN) {
        
        medalReceived = MEDAL_BRONZE;
        if (medalTypeCondition == MTC_TIME) {
            medalGoldCondition = (medalGoldCondition * 2) / getGameSpeed();
            medalSilverCondition = (medalSilverCondition * 2) / getGameSpeed();
            if (getMatchTime() <= medalGoldCondition)
                medalReceived = MEDAL_GOLD; 
            else if (getMatchTime() <= medalSilverCondition)
                medalReceived = MEDAL_SILVER;
        } else if (medalTypeCondition == MTC_UNITS_LOST) {
            if (getUnitDeaths(FRIENDLY) <= medalGoldCondition)
                medalReceived = MEDAL_GOLD;
            else if (getUnitDeaths(FRIENDLY) <= medalSilverCondition)
                medalReceived = MEDAL_SILVER;
        } else if (medalTypeCondition == MTC_UNITS_KILL) {
            j = 0;
            for (i=1; i<getAmountOfSides(); i++)
                j += getUnitDeaths(i);
            if (j >= medalGoldCondition)
                medalReceived = MEDAL_GOLD;
            else if (j >= medalSilverCondition)
                medalReceived = MEDAL_SILVER;
        } else if (medalTypeCondition == MTC_STRUCTURES_LOST) {
            if (getStructureDeaths(FRIENDLY) <= medalGoldCondition)
                medalReceived = MEDAL_GOLD;
            else if (getStructureDeaths(FRIENDLY) <= medalSilverCondition)
                medalReceived = MEDAL_SILVER;
        } else if (medalTypeCondition == MTC_UNITS_LOST) {
            j = 0;
            for (i=1; i<getAmountOfSides(); i++)
                j += getStructureDeaths(i);
            if (j >= medalGoldCondition)
                medalReceived = MEDAL_GOLD;
            else if (j >= medalSilverCondition)
                medalReceived = MEDAL_SILVER;
        } else
            medalReceived = MEDAL_GOLD;
        
//        if (medalReceived > getMedalReceived(getFaction(FRIENDLY), getLevel() - 1))
        setMedalReceived(getFaction(FRIENDLY), getLevel() - 1, medalReceived);
        
//        if (getLevel() > 9) { // there are only 9 levels usually, let's immediately calculate and save the tally of medals
            j = 0;
            for (i=1; i<=9; i++) {
                if (getMedalReceived(getFaction(FRIENDLY), i) < MEDAL_BRONZE || getMedalReceived(getFaction(FRIENDLY), i) > MEDAL_GOLD)
                    break;
                j += getMedalReceived(getFaction(FRIENDLY), i);
            }
            if (i>9) { // all nine levels have been completed at least once
                if (j >= 2*9)
                    factionMedalReceived = MEDAL_GOLD;
                else if (j >= 12)
                    factionMedalReceived = MEDAL_SILVER;
                else
                    factionMedalReceived = MEDAL_BRONZE;
                if (getFactionMedalReceived(getFaction(FRIENDLY)) < MEDAL_BRONZE ||
                    getFactionMedalReceived(getFaction(FRIENDLY)) > MEDAL_GOLD ||
                    factionMedalReceived > getFactionMedalReceived(getFaction(FRIENDLY)))
                    setFactionMedalReceived(getFaction(FRIENDLY), factionMedalReceived);
            }
//        }
        
        writeSaveFile();
        
        sprintf(filename, "win_%s", factionInfo[getFaction(FRIENDLY)].name);
        if (!initAnimation(filename, 0, 0, 0))
            return 0;
    }
    
    if (state == CDS_LOSE_GAMEOVER && !initAnimation("lose", 0, 0, 0))
        return 0;
    
    return 1;
}

void initCutsceneDebriefingFilename(int win) {
    char oneline[256];
    FILE *fp;
    
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    // CUTSCENES section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[CUTSCENES]", strlen("[CUTSCENES]")));
    readstr(fp, oneline);
    readstr(fp, oneline);
    if (win) {
        readstr(fp, oneline);
        replaceEOLwithEOF(oneline, 255);
        strcpy(animationFilename, oneline + strlen("Win="));
        state = CDS_WIN_YOUWIN;
        
        // MEDALS section
        do {
            readstr(fp, oneline);
        } while (strncmp(oneline, "[MEDALS]", strlen("[MEDALS]")) && strncmp(oneline, "[FRIENDLY]", strlen("[FRIENDLY]")));
        if (!strncmp(oneline, "[FRIENDLY]", strlen("[FRIENDLY]")))
            medalTypeCondition = MTC_NONE;
        else {
            readstr(fp, oneline); // Type=
            if (oneline[strlen("Type=")] == 'T') // Type=Time
                medalTypeCondition = MTC_TIME;
            else if (oneline[strlen("Type=")] == 'K') // Type=Kill (read: UnitsKill)
                medalTypeCondition = MTC_UNITS_KILL;
            else if (oneline[strlen("Type=") + strlen("Units")] == 'L') // Type=UnitsLost
                medalTypeCondition = MTC_UNITS_LOST;
            else if (oneline[strlen("Type=") + strlen("Units")] == 'K') // Type=UnitsKill
                medalTypeCondition = MTC_UNITS_KILL;
            else if (oneline[strlen("Type=") + strlen("Structures")] == 'L') // Type=StructuresLost
                medalTypeCondition = MTC_STRUCTURES_LOST;
            else if (oneline[strlen("Type=") + strlen("Structures")] == 'K') // Type=StructuresKill
                medalTypeCondition = MTC_STRUCTURES_KILL;
            else
                error("Unrecognised medal type condition", oneline);
            readstr(fp, oneline);
            sscanf(oneline, "Gold=%i", &medalGoldCondition);
            readstr(fp, oneline);
            sscanf(oneline, "Silver=%i", &medalSilverCondition);
        }
    } else {
        replaceEOLwithEOF(oneline, 255);
        strcpy(animationFilename, oneline + strlen("Lose="));
        state = CDS_LOSE_GAMEOVER;
    }
    closeFile(fp);
}
