// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "infoscreen.h"
#include "playscreen.h"

#include "fileio.h"

#include "view.h"
#include "info.h"
#include "radar.h"
#include "shared.h"
#include "sprites.h"
#include "inputx.h"
#include "game.h"
#include "settings.h"
#include "soundeffects.h"

#include "factions.h"
#include "structures.h"
#include "units.h"
#include "objectives.h"
#include "menu_gameinfo.h"
#include "menu_gameinfo_item_select.h"


#define MAX_ITEMS_BAR_STRUCTURES    MAX_DIFFERENT_STRUCTURES
#define MAX_ITEMS_BAR_BUILDABLES    MAX_DIFFERENT_STRUCTURES + MAX_DIFFERENT_UNITS

#define RADAR_OVERLAY_DEFAULT_BLEND_A       13
#define MAX_RADAR_OVERLAY_BLEND_A_DEVIATION  3 /* 3 is the highest possible considering 13 is the default, and 16 is the max blend factor */
#define RADAR_OVERLAY_DEFAULT_BLEND_B        3
#define MAX_RADAR_OVERLAY_BLEND_B_DEVIATION  3 /* 3 is the highest possible considering  3 is the default, and  0 is the min blend factor */

#define STRUCTURE_ICON_ANIMATION_DURATION  (2*FPS)
#define ARROWS_ANIMATION_DURATION          (FPS/2)
#define MINPLUS_ANIMATION_DURATION         (FPS/2)
#define PLACE_ICON_ANIMATION_DURATION      (FPS/3)
#define BUILDQUEUE_ANIMATION_DURATION      (FPS/2)
#define ITEM_BAR_ANIMATION_DURATION        (18 / (SCREEN_REFRESH_RATE / FPS)) /* can be 63 max */


struct BarItem {
    short unsigned int animation; // includes both type and frame-counter (using mask IBAT_ADD)
    short int          info;
};

int infoScreenTouchedTimer = 0;

enum InfoScreenType { IS_RADAR, IS_BUILDINFO, IS_HOTKEYS, IS_BUILDQUEUE };
enum InfoScreenType infoScreenType = IS_RADAR;
int infoScreenBuildInfoIsStructure;
int infoScreenBuildInfoNr;
int infoScreenBuildInfoBuilderNr;
int infoScreenBuildInfoCanBuild;

struct BarItem itemsBarStructures[MAX_ITEMS_BAR_STRUCTURES+1];
int infoScreenItemBarStructuresFirstShown;
//int infoScreenItemBarStructuresSelected;
struct BarItem itemsBarBuildables[MAX_ITEMS_BAR_BUILDABLES+1];
int amountOfStructuresInBarBuildables;
int infoScreenItemBarBuildablesFirstShown;
int infoScreenItemBarBuildablesStructureNr;
//int infoScreenItemBarBuildablesSelected;

int infoScreenStructureIconAnimationTimer;
int infoScreenMinplusAnimationTimer;
int infoScreenArrowsAnimationTimer;
int infoScreenPlaceIconAnimationTimer;
int infoScreenBuildQueueAnimationTimer;
int haveDrawnBarStructuresSelect;

static uint16 fb_font_small[48 * 8*8]; // font_small is also loaded in RAM for use with framebuffer mode
static int base_infoscreen_bar;
static int base_arrows;
static int base_digits_credits;
static int base_digits_stats;
static int base_power_slider;
static int base_emptyslot_icons;
static int base_structure_icons;
static int base_unit_icons;
static int base_sidebars_animation;
static int base_place_icon;
static int base_size_icons;
static int base_size_icon[4*3];
static int base_queue_words;
static int base_queue_building;
static int base_minplus;
static int base_buildqueue_flash;
static int base_font_small;
static int base_hotkey;
static int base_gameinfo_button;
static int base_select_graphics;
static int base_glitch_hide_sprite;
static int base_select_all;



void inputBarBuildables();


enum ItemBarAnimationType { IBAT_NONE = 0, IBAT_ADD = 1,  IBAT_REMOVE = 2, IBAT_MASK = 3 };

struct BarItem createBarItem(int nr, int animation) {
    struct BarItem result;
    
    result.animation = animation;
    result.info      = nr;
    
    return result;
}

void changeBarItemAnimation(struct BarItem *barItem, enum ItemBarAnimationType ibat) {
    if (ibat == IBAT_NONE) {
        *barItem = createBarItem(barItem->info, IBAT_NONE);
        return;
    }
    if ((barItem->animation & IBAT_MASK) == IBAT_NONE) {
        *barItem = createBarItem(barItem->info, ibat | ((ibat == IBAT_ADD ? 0 : ITEM_BAR_ANIMATION_DURATION) << 2));
    } else { // just set ibat, and leave the animation frame setting
        *barItem = createBarItem(barItem->info, ibat | (barItem->animation & (~IBAT_MASK)));
    }
}

void doBarItemAnimationLogic(struct BarItem *barItem) {
    int i;
    
    if ((barItem->animation & IBAT_MASK) == IBAT_ADD) {
        i = (barItem->animation >> 2) + 1;
        if (i >= ITEM_BAR_ANIMATION_DURATION)
            *barItem = createBarItem(barItem->info, IBAT_NONE);
        else
            *barItem = createBarItem(barItem->info, IBAT_ADD | (i << 2));
    } else if ((barItem->animation & IBAT_MASK) == IBAT_REMOVE) {
        i = (barItem->animation >> 2) - 1;
        if (i < 0)
            *barItem = createBarItem(-1, IBAT_NONE);
        else
            *barItem = createBarItem(barItem->info, IBAT_REMOVE | (i << 2));
    }
}


void setInfoScreenRadar() {
    infoScreenType = IS_RADAR;
    REG_BG0HOFS_SUB = 0;
    initKeysHeldTime(KEY_TOUCH);
    REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE; // activate radar overlay
    REG_DISPCNT_SUB |= DISPLAY_BG1_ACTIVE; // activate covering up background behind radar
    REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // temporarily deactivate radar background
    drawRadarBG();
//        REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE; // activate radar background
    setAllDirtyRadarDirtyBitmap();
    REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_SRC_BG2;
}

void inputButtonRadar(int x, int y) {
    if (x >= 181 && x < 213 && y >= 128 && y < 160) {
        setInfoScreenRadar();
        playSoundeffect(SE_BUTTONCLICK);
    }
}

void drawButtonRadarSelect(int x, int y) {
    if (x >= 181 && x < 213 && y >= 128 && y < 160)
        setSpriteInfoScreen(128, ATTR0_SQUARE,
                            181, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_select_graphics);
}


void inputButtonHotKeys(int x, int y) {
    if (x >= BUTTON_HOTKEYS_X1 && x < BUTTON_HOTKEYS_X2 && y >= BUTTON_HOTKEYS_Y1 && y < BUTTON_HOTKEYS_Y2) {
        infoScreenType = IS_HOTKEYS;
        REG_BG0HOFS_SUB = 0;
        initKeysHeldTime(KEY_TOUCH);
        REG_DISPCNT_SUB &= (~DISPLAY_BG0_ACTIVE); // disable queue percentages background or radar overlay
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // disable radar background (might already be disabled)
        REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
        REG_BLDCNT_SUB = BLEND_NONE;
        playSoundeffect(SE_BUTTONCLICK);
    }
}

void drawButtonHotKeysSelect(int x, int y) {
    if (x >= BUTTON_HOTKEYS_X1 && x < BUTTON_HOTKEYS_X2 && y >= BUTTON_HOTKEYS_Y1 && y < BUTTON_HOTKEYS_Y2)
        setSpriteInfoScreen(BUTTON_HOTKEYS_Y1, ATTR0_WIDE,
                            BUTTON_HOTKEYS_X1, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_select_graphics + 4);
}

int inputButtonSwapScreens(int x, int y) {
    return (x >= BUTTON_SWAPSCREENS_X1 && x < BUTTON_SWAPSCREENS_X2 && y >= BUTTON_SWAPSCREENS_Y1 && y < BUTTON_SWAPSCREENS_Y2);
}

void drawButtonSwapScreensSelect(int x, int y) {
    if (x >= BUTTON_SWAPSCREENS_X1 && x < BUTTON_SWAPSCREENS_X2 && y >= BUTTON_SWAPSCREENS_Y1 && y < BUTTON_SWAPSCREENS_Y2)
        setSpriteInfoScreen(BUTTON_SWAPSCREENS_Y1, ATTR0_WIDE,
                            BUTTON_SWAPSCREENS_X1, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_select_graphics + 4 + 2 + 1 + 1 + 1);
}

void inputButtonBuildQueue(int x, int y) {
    if (x >= BUTTON_BUILDQUEUE_X1 && x <= BUTTON_BUILDQUEUE_X2 && y >= BUTTON_BUILDQUEUE_Y1 && y <= BUTTON_BUILDQUEUE_Y2) {
        infoScreenType = IS_BUILDQUEUE;
        REG_BG0HOFS_SUB = 0;
        initKeysHeldTime(KEY_TOUCH);
        REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE;    // activate queue percentages background
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // disable radar background (might already be disabled)
        REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
        REG_BLDCNT_SUB = BLEND_NONE;
        playSoundeffect(SE_BUTTONCLICK);
    }
}

void drawButtonBuildQueueSelect(int x, int y) {
    if (x >= BUTTON_BUILDQUEUE_X1 && x <= BUTTON_BUILDQUEUE_X2 && y >= BUTTON_BUILDQUEUE_Y1 && y <= BUTTON_BUILDQUEUE_Y2)
        setSpriteInfoScreen(BUTTON_BUILDQUEUE_Y1, ATTR0_WIDE,
                            BUTTON_BUILDQUEUE_X1, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_select_graphics + 4);
}


int inputBuildQueue(int x, int y) {
    int item;
    
    if (x >= 48 && x < 208 && y >= 32) {
        if (y < 56) {
            /* an item in the structures queue was clicked */
            item = (x-48)/32;
            if (getStructuresQueue(FRIENDLY)[item].enabled) {
                if (getKeysHeldTimeMax(KEY_TOUCH) >= FPS/2) {
                    if (removeItemStructuresQueueNr(FRIENDLY, item, 1, 0))
                        playSoundeffect(SE_BUTTONCLICK);
                } else {
                    if (getStructuresQueue(FRIENDLY)[item].status == DONE) {
                        setModifierPlaceStructure(item);
                        playSoundeffect(SE_BUTTONCLICK);
                        return 1;
                    }
                    if (togglePauseItemStructuresQueueNr(FRIENDLY, item))
                        playSoundeffect(SE_BUTTONCLICK);
                }
            }
        } else if (y < 104) {
            /* an item in the units queue was clicked */
            item = ((y-56)/24)*5 + ((x-48)/32);
            if (getUnitsQueue(FRIENDLY)[item].enabled) {
                if (getKeysHeldTimeMax(KEY_TOUCH) >= FPS/2) {
                    if (removeItemUnitsQueueNr(FRIENDLY, item, 1, 0))
                        playSoundeffect(SE_BUTTONCLICK);
                } else {
                    if (togglePauseItemUnitsQueueNr(FRIENDLY, item))
                        playSoundeffect(SE_BUTTONCLICK);
                }
            }
        }
    }
    return 0;
}


void inputButtonMinplus(int x, int y) {
    struct Structure *curStructure;
    struct Unit *curUnit;
    int amountOfItems;
    int i, j;
    
    if (y >= 88 && y < 104) {
        if (x >= 48) {
            if (x < 64) {
                /* minus clicked */
                if (infoScreenBuildInfoIsStructure) {
                    if (removeItemStructuresQueueWithInfo(FRIENDLY, infoScreenBuildInfoNr, 1, 0))
                        playSoundeffect(SE_BUTTONCLICK);
                } else {
                    if (removeItemUnitsQueueWithInfo(FRIENDLY, infoScreenBuildInfoNr, 1, 0))
                        playSoundeffect(SE_BUTTONCLICK);
                }
            } else if (x < 80) {
                /* plus clicked */
                if (infoScreenBuildInfoIsStructure) {
                    amountOfItems = MAX_DIFFERENT_FACTIONS;
                    curStructure = structure;
                    for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
                        if (curStructure->enabled)
                            amountOfItems++;
                    }
                    for (i=0; i<getAmountOfSides(); i++) {
                        int maxItems = (i == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
                        for (j=0; j<maxItems; j++) {
                            if (getStructuresQueue(i)[j].enabled && !structureInfo[getStructuresQueue(i)[j].itemInfo].foundation)
                                amountOfItems++;
                        }
                    }
                    if (amountOfItems < MAX_STRUCTURES_ON_MAP) {
                        if (addItemStructuresQueue(FRIENDLY, infoScreenBuildInfoNr, infoScreenBuildInfoBuilderNr)) {
                            amountOfItems++;
                            if (amountOfItems == MAX_STRUCTURES_ON_MAP)
                                playMiscSoundeffect("Sound_structureLimitReached");
                            else
                                playSoundeffect(SE_BUTTONCLICK);
                        }
                    }
                } else {
                    amountOfItems = 0;
                    curUnit = unit;
                    for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
                        if (curUnit->enabled && curUnit->side == FRIENDLY)
                            amountOfItems++;
                    }
                    for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                        if (getUnitsQueue(FRIENDLY)[i].enabled)
                            amountOfItems++;
                    }
                    if (amountOfItems < getUnitLimit(FRIENDLY)) {
                        if (addItemUnitsQueue(FRIENDLY, infoScreenBuildInfoNr, infoScreenBuildInfoBuilderNr)) {
                            amountOfItems++;
                            if (amountOfItems == getUnitLimit(FRIENDLY))
                                playMiscSoundeffect("Sound_unitLimitReached");
                            else
                                playSoundeffect(SE_BUTTONCLICK);
                        }
                    }
                }
            }
        }
    }
}

void drawButtonMinplusSelect(int x, int y) {
    struct Structure *curStructure;
    struct Unit *curUnit;
    int amountOfItems;
    int i, j;
    
    if (y >= 88 && y < 104) {
        if (x >= 48) {
            if (x < 64) { /* minus */
                if (infoScreenBuildInfoIsStructure) {
                    for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
                        if (getStructuresQueue(FRIENDLY)[i].enabled && getStructuresQueue(FRIENDLY)[i].itemInfo == infoScreenBuildInfoNr) {
                            setSpriteInfoScreen(88, ATTR0_SQUARE,
                                                48, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_select_graphics + 4 + 2 + 1);
                            return;
                        }
                    }
                } else {
                    for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                        if (getUnitsQueue(FRIENDLY)[i].enabled && getUnitsQueue(FRIENDLY)[i].itemInfo == infoScreenBuildInfoNr) {
                            setSpriteInfoScreen(88, ATTR0_SQUARE,
                                                48, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_select_graphics + 4 + 2 + 1);
                            return;
                        }
                    }
                }
            } else if (x < 80) { /* plus */
                if (infoScreenBuildInfoIsStructure) {
                    for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
                        if (!getStructuresQueue(FRIENDLY)[i].enabled) {
                            amountOfItems = MAX_DIFFERENT_FACTIONS;
                            curStructure = structure;
                            for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
                                if (curStructure->enabled)
                                    amountOfItems++;
                            }
                            for (i=0; i<getAmountOfSides(); i++) {
                                int maxItems = (i == FRIENDLY) ? FRIENDLY_STRUCTURES_QUEUE : ENEMY_STRUCTURES_QUEUE;
                                for (j=0; j<maxItems; j++) {
                                    if (getStructuresQueue(i)[j].enabled && !structureInfo[getStructuresQueue(i)[j].itemInfo].foundation)
                                        amountOfItems++;
                                }
                            }
                            if (amountOfItems < MAX_STRUCTURES_ON_MAP)
                                setSpriteInfoScreen(88, ATTR0_SQUARE,
                                                    64, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_select_graphics + 4 + 2 + 1);
                            return;
                        }
                    }
                } else {
                    for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                        if (!getUnitsQueue(FRIENDLY)[i].enabled) {
                            amountOfItems = 0;
                            curUnit = unit;
                            for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
                                if (curUnit->enabled && curUnit->side == FRIENDLY)
                                    amountOfItems++;
                            }
                            for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                                if (getUnitsQueue(FRIENDLY)[i].enabled)
                                    amountOfItems++;
                            }
                            if (amountOfItems < getUnitLimit(FRIENDLY))
                                setSpriteInfoScreen(88, ATTR0_SQUARE,
                                                    64, SPRITE_SIZE_16, 0, 0,
                                                    0, 0, base_select_graphics + 4 + 2 + 1);
                            return;
                        }
                    }
                }
            }
        }
    }
}


void createBarBuildables(int structureNr) {
    int i, j, k, l;
    int buildableInfo;
    int mayInsert;
    int insertedItems = 0;
    int tItemCount;
    struct TechtreeItem *tItem;
    struct BarItem bItem, bItemTemp;
    
    if (structureNr == -1) { // set all items in the bar to "remove" animation
        for (i=0; i<MAX_ITEMS_BAR_BUILDABLES+1 && itemsBarBuildables[i].info >= 0; i++)
            changeBarItemAnimation(&itemsBarBuildables[i], IBAT_REMOVE);
        infoScreenItemBarBuildablesStructureNr = structureNr;
        return;
    }
    
    if (structureNr != infoScreenItemBarBuildablesStructureNr) { // first delete all entries
        infoScreenItemBarBuildablesFirstShown = 0;
        amountOfStructuresInBarBuildables = 0;
        for (i=0; i<MAX_ITEMS_BAR_BUILDABLES; i++)
            itemsBarBuildables[i] = createBarItem(-1, IBAT_NONE);
    } else {
        for (i=0; i<MAX_ITEMS_BAR_BUILDABLES && itemsBarBuildables[i].info != -1; i++) {
            if (availableBuilderForItem(FRIENDLY, i<amountOfStructuresInBarBuildables, itemsBarBuildables[i].info) == -1) // set item in the bar to "remove" animation
                changeBarItemAnimation(&itemsBarBuildables[i], IBAT_REMOVE);
            else if ((itemsBarBuildables[i].animation & IBAT_MASK) == IBAT_REMOVE)
                changeBarItemAnimation(&itemsBarBuildables[i], IBAT_ADD);
            insertedItems++;
        }
    }
    
    tItemCount = getTechtree(FRIENDLY)->amountOfItems;
    tItem = getTechtree(FRIENDLY)->item;
    
    int insertAtStructures=0;
    int insertAtUnits=0;
    
//error("at least we got here", "");
    for (i=0; i<tItemCount; i++) {
        mayInsert = 0;
        if (tItem[i].buildingInfo == structureNr) {
            buildableInfo = tItem[i].buildableInfo;
            
            if (tItem[i].buildStructure) { // to be inserted as structure, if allowed
                l = insertAtStructures;
                
                if (i>0 && buildableInfo == tItem[i-1].buildableInfo && tItem[i-1].buildStructure &&
                    insertAtStructures>0 && itemsBarBuildables[l-1].info == buildableInfo) // already exists in the bar
                    continue;
                
                if (itemsBarBuildables[l].info == buildableInfo)
                    insertAtStructures++;
                else {
                    // check whether allowed to insert
                    mayInsert = 1;
                    for (j=0; j<MAX_REQUIREDINFO_COUNT; j++) {
                        if (tItem[i].requiredInfo[j] >= 0) {
                            for (k=0; k<MAX_STRUCTURES_ON_MAP; k++) {
                                if (structure[k].enabled && structure[k].side == FRIENDLY && tItem[i].requiredInfo[j] == structure[k].info)
                                    break;
                            }
                            if (k == MAX_STRUCTURES_ON_MAP) {
                                mayInsert = 0;
                                break;
                            }
                        }
                    }
                    if (mayInsert) {
                        amountOfStructuresInBarBuildables++;
                        insertAtStructures++;
                    }
                }
                 // either it would be inserted now or it already existed
            } else { // to be inserted as unit, if allowed                
                l = amountOfStructuresInBarBuildables + insertAtUnits;
                
                if (i>0 && buildableInfo == tItem[i-1].buildableInfo && !tItem[i-1].buildStructure &&
                    insertAtUnits>0 && itemsBarBuildables[l-1].info == buildableInfo)
                    continue;
                
                if (itemsBarBuildables[l].info == buildableInfo)
                    insertAtUnits++;
                else {
                    // check whether allowed to insert
                    mayInsert = 1;
                    for (j=0; j<MAX_REQUIREDINFO_COUNT; j++) {
                        if (tItem[i].requiredInfo[j] != -1) {
                            for (k=0; k<MAX_STRUCTURES_ON_MAP; k++) {
                                if (structure[k].enabled && structure[k].side == FRIENDLY && tItem[i].requiredInfo[j] == structure[k].info)
                                    break;
                            }
                            if (k == MAX_STRUCTURES_ON_MAP) {
                                mayInsert = 0;
                                break;
                            }
                        }
                    }
                    if (mayInsert)
                        insertAtUnits++;
                }
                
            }
            
            if (mayInsert) { // l should be left at place of insertion
                bItem = createBarItem(buildableInfo, IBAT_ADD);
                while (itemsBarBuildables[l].info != -1) {
                    bItemTemp = itemsBarBuildables[l]; // tempinfo is what needs shoving up
                    itemsBarBuildables[l] = bItem;
                    bItem = bItemTemp;
                    l++;
                }
                itemsBarBuildables[l]=bItem;
                insertedItems++;
            }
        }
    }
    
    /* making sure the first shown is still at a suitable position */
    if (infoScreenItemBarBuildablesFirstShown > 0 && infoScreenItemBarBuildablesFirstShown + 7 > insertedItems)
        infoScreenItemBarBuildablesFirstShown = insertedItems - 7;
    if (infoScreenItemBarBuildablesFirstShown < 0) // 'wall-barrier issue' was related to adjusting the first shown to a negative number
        infoScreenItemBarBuildablesFirstShown = 0;
    
    infoScreenItemBarBuildablesStructureNr = structureNr;
}


void inputBarStructures(int x, int y) { // structures which can build
    int selectedItem = -1;
    
    if (infoScreenItemBarStructuresFirstShown == -1)
        return;
    
    if (x < 36 && y >= 8 && y < SCREEN_HEIGHT - 16) {
        y -= 8;
        // user wants this one in view:  y/24  (range 0 to 6)
        selectedItem = infoScreenItemBarStructuresFirstShown + (y/24);
    }
    if (selectedItem != -1) {
        if (itemsBarStructures[selectedItem].info != -1) {
            if (infoScreenItemBarBuildablesFirstShown == -1 || infoScreenItemBarBuildablesStructureNr != itemsBarStructures[selectedItem].info) {
//                infoScreenItemBarStructuresSelected = selectedItem;
//                infoScreenItemBarBuildablesStructureNr = itemsBarStructures[selectedItem];
                initKeysHeldTime(KEY_TOUCH);
                infoScreenItemBarBuildablesFirstShown = -1;
                createBarBuildables(itemsBarStructures[selectedItem].info);
                if (infoScreenType == IS_BUILDINFO && itemsBarBuildables[0].info != -1)
                    inputBarBuildables(SCREEN_WIDTH - 18, 8+24/2); // first item. function performs playSoundeffect
                else
                    playSoundeffect(SE_BUTTONCLICK);
            } else
                playSoundeffect(SE_BUTTONCLICK);
        }
    }
}

void drawBarStructuresSelect(int x, int y) {
    if (infoScreenItemBarStructuresFirstShown == -1)
        return;
    
    if (x < 36 && y >= 8 && y < SCREEN_HEIGHT - 16) {
        y -= 8;
        // user wants this one in view:  y/24  (range 0 to 6)
        if (itemsBarStructures[infoScreenItemBarStructuresFirstShown + (y/24)].info != -1) {
            setSpriteInfoScreen(8 + (y/24) * 24, ATTR0_SQUARE,
                                4, SPRITE_SIZE_32, 0, 0,
                                0, 0, base_select_graphics);
            haveDrawnBarStructuresSelect = 1;
        }
    }
}


void inputBarStructuresArrows(int x, int y, int timeHeld) {
    int key=0; // 1==up, 2==down
    int maxItemsInBar=7;
    
    if (infoScreenItemBarStructuresFirstShown == -1)
        return;
    
    if (y >= SCREEN_HEIGHT - 16) {
        if (x < 16)
            key = 1;
        else if (x < 32)
            key = 2;
    }
    if (timeHeld > FPS / 2) {
        if (key == 2) { // up
            if (infoScreenItemBarStructuresFirstShown > 0) {
                infoScreenItemBarStructuresFirstShown = 0;
                playSoundeffect(SE_BUTTONCLICK);
            }
        } else if (key == 1) { // down
            if (itemsBarStructures[infoScreenItemBarStructuresFirstShown + maxItemsInBar].info != -1) {
                while (itemsBarStructures[infoScreenItemBarStructuresFirstShown + maxItemsInBar].info != -1) {
                    infoScreenItemBarStructuresFirstShown++;
                }
                playSoundeffect(SE_BUTTONCLICK);
            }
        }
    } else if (!timeHeld) {
        if (key == 2) { // up
            if (infoScreenItemBarStructuresFirstShown > 0) {
                infoScreenItemBarStructuresFirstShown--;
                playSoundeffect(SE_BUTTONCLICK);
            }
        } else if (key == 1) { // down
            if (itemsBarStructures[infoScreenItemBarStructuresFirstShown + maxItemsInBar].info != -1) {
                infoScreenItemBarStructuresFirstShown++;
                playSoundeffect(SE_BUTTONCLICK);
            }
        }
    }
}

void drawBarStructuresArrowsSelect(int x, int y) {
    int maxItemsInBar=7;
    
    if (y >= SCREEN_HEIGHT - 16) {
        if (x < 20) { // down
            if (itemsBarStructures[infoScreenItemBarStructuresFirstShown + maxItemsInBar].info != -1)
                setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                                    4, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_select_graphics + 4 + 2);
        } else if (x < 36) { // up
            if (infoScreenItemBarStructuresFirstShown > 0)
                setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                                    20, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_select_graphics + 4 + 2);
        }
    }
}


void blitTileWH(int nr, int x, int y, int w, int h) {
    int i, j;
    uint16 *dest = BG_GFX_SUB + y*128 + x;
    uint16 *src = fb_font_small + nr*8*8;
    
    for (i=0; i<h; i++) {
        for (j=0; j<w; j++) {
            *dest = *src;
            dest++;
            src++;
        }
        dest += (128 - w);
        src += (8-w);
    }
}


void blitTile(int nr, int x, int y) { // cheating by knowing we're actually talking about a font of max 5x6 here.
    blitTileWH(nr, x, y, 5, 6);
}



extern uint32 radarCX, radarCY; // only required for screenshotting purposes
extern uint16 radarScale;

void drawInformationBG(char *string) {
    int i;
    int x = 0;
    int y = 0;
    char curChar;
    
    // first clear the framebuffer background, bg2
    for (i=0; i<128*128; i++)
        BG_GFX_SUB[i] = 0;
    
    // now, let's write string to it
    while (*string != 0) {
        curChar = *string;
        if (curChar == '\\') { // next line indicated by '\'
            x = 0;
            y += 8;
            string++;
            continue;
        }
        
        if (curChar >= 'a' && curChar <= 'z')
            blitTile(curChar - 'a', x, y);
        else if (curChar >= 'A' && curChar <= 'Z') {
            *string += 32;
            curChar += 32;
            blitTile(curChar - 'a', x, y);
        }
        else if (curChar >= '0' && curChar <= '9')
            blitTile(26 + (curChar - '0'), x, y);
        else if (curChar == ':')
            blitTile(36, x, y);
        else if (curChar == '/') {
            x += 1;
            blitTile(37, x, y);
            x += 1;
        }
        else if (curChar == '.')
            blitTile(38, x, y);
        else if (curChar == ',')
            blitTile(39, x, y);
        else if (curChar >= '"' && curChar <= ')')
            blitTileWH(40 + (curChar - '"'), x, y, 8, 8);
        
        if (curChar != '.') {
            if (curChar == 'i')
                x += 1;
            else if (curChar == ':')
                x += 3;
            else if (curChar == 'm' || curChar == 'w')
                x += 4;
            else if (curChar == 'n' || curChar == 'q')
                x += 4;
            else if (curChar >= '"' && curChar <= '(') { // special case.
                x += 6;
    //            string++;
    //            continue;
            } else if (curChar <= ')')
                x += 2;
            else
                x += 3;
        }
        x += 1; // spacing
        
        string++;
    }
    
    // set correct scaling and position for bg2
    REG_BG2PA_SUB = 1<<8; // scaling of 1
    REG_BG2PB_SUB = 0;
    REG_BG2PC_SUB = 0;
    REG_BG2PD_SUB = 1<<8; // scaling of 1
    REG_BG2X_SUB =  -(84 << 8);                                    // x position
    REG_BG2Y_SUB = (-(22 << 8)) - (getScreenScrollingInfoScreen() * 256); // y position
    REG_BG2HOFS_SUB = 0;
    REG_BG2VOFS_SUB = 0;
    
    radarCX = -(84 << 8); // only required for screenshotting purposes
    radarCY = -(22 << 8);
    radarScale = 1<<8;
}


void inputBarBuildables(int x, int y) { // buildable structures or units
    int selectedItem = -1;
    
    if (infoScreenItemBarBuildablesFirstShown == -1)
        return;
    
    if (x >= SCREEN_WIDTH - 36 && y >= 8 && y < SCREEN_HEIGHT - 16) {
        y -= 8;
        // user wants this one in view:  y/24  (range 0 to 6)
        selectedItem = infoScreenItemBarBuildablesFirstShown + (y/24);
    }
    if (selectedItem != -1) {
        if (itemsBarBuildables[selectedItem].info != -1 && (itemsBarBuildables[selectedItem].animation & IBAT_MASK) != IBAT_REMOVE) {
            if (infoScreenType != IS_BUILDINFO || (infoScreenBuildInfoIsStructure != (selectedItem < amountOfStructuresInBarBuildables)) ||
                infoScreenBuildInfoNr != itemsBarBuildables[selectedItem].info || infoScreenBuildInfoBuilderNr != infoScreenItemBarBuildablesStructureNr) {
                infoScreenBuildInfoIsStructure = (selectedItem < amountOfStructuresInBarBuildables);
                infoScreenBuildInfoNr = itemsBarBuildables[selectedItem].info;
                infoScreenBuildInfoBuilderNr = infoScreenItemBarBuildablesStructureNr;
                infoScreenBuildInfoCanBuild = 1;
                infoScreenType = IS_BUILDINFO;
                REG_BG0HOFS_SUB = -(4); //  x position slightly shifted
                infoScreenMinplusAnimationTimer = 0;
                infoScreenPlaceIconAnimationTimer = 0;
                initKeysHeldTime(KEY_TOUCH);
                REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE;    // enable background for queue percentages
                REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // temporarily disable the radar background until it is rewritten here to be used as information text display
                REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
                drawInformationBG( (infoScreenBuildInfoIsStructure) ? 
                                    structureInfo[infoScreenBuildInfoNr].description : 
                                    unitInfo[infoScreenBuildInfoNr].description );
//                REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE; // activate radar background, used now for text-display
                REG_BLDCNT_SUB = BLEND_NONE;
            }
            playSoundeffect(SE_BUTTONCLICK);
        }
    }
}

void drawBarBuildablesSelect(int x, int y) {
    if (infoScreenItemBarBuildablesFirstShown == -1)
        return;
    
    if (x >= SCREEN_WIDTH - 36 && y >= 8 && y < SCREEN_HEIGHT - 16) {
        y -= 8;
        // user wants this one in view:  y/24  (range 0 to 6)
        if (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + (y/24)].info != -1)
            setSpriteInfoScreen(8 + (y/24) * 24, ATTR0_SQUARE,
                                SCREEN_WIDTH - 36, SPRITE_SIZE_32, 0, 0,
                                0, 0, base_select_graphics);
    }
}



void inputBarBuildablesArrows(int x, int y, int timeHeld) {
    int key=0; // 1==up, 2==down
    int maxItemsInBar=7;
    
    if (infoScreenItemBarBuildablesFirstShown == -1)
        return;
    
    if (y >= SCREEN_HEIGHT - 16) {
        if (x >= SCREEN_WIDTH - 16)
            key = 1;
        else if (x >= SCREEN_WIDTH - 32)
            key = 2;
    }
    if (timeHeld > FPS / 2) {
        if (key == 1) { // up
            if (infoScreenItemBarBuildablesFirstShown > 0) {
                infoScreenItemBarBuildablesFirstShown = 0;
                playSoundeffect(SE_BUTTONCLICK);
            }
        } else if (key == 2) { // down
            if (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + maxItemsInBar].info != -1) {
                while (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + maxItemsInBar].info != -1) {
                    infoScreenItemBarBuildablesFirstShown++;
                }
                playSoundeffect(SE_BUTTONCLICK);
            }
        }
    } else if (!timeHeld) {
        if (key == 1) { // up
            if (infoScreenItemBarBuildablesFirstShown > 0) {
                infoScreenItemBarBuildablesFirstShown--;
                playSoundeffect(SE_BUTTONCLICK);
            }
        } else if (key == 2) { // down
            if (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + maxItemsInBar].info != -1) {
                infoScreenItemBarBuildablesFirstShown++;
                playSoundeffect(SE_BUTTONCLICK);
            }
        }
    }
}

void drawBarBuildablesArrowsSelect(int x, int y) {
    int maxItemsInBar=7;
    
    if (y >= SCREEN_HEIGHT - 16) {
        if (x >= SCREEN_WIDTH - 20) { // up
            if (infoScreenItemBarBuildablesFirstShown > 0)
                setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                                    SCREEN_WIDTH - 20, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_select_graphics + 4 + 2);
        } else if (x >= SCREEN_WIDTH - 36) { // down
            if (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + maxItemsInBar].info != -1)
                setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                                    SCREEN_WIDTH - 36, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_select_graphics + 4 + 2);
        }
    }
}


int inputButtonPlace(int x, int y) {
    int i;
    
    if (x >= 48 && x < 80 &&
        ((y >= 56 && y < 80) || (y >= 8 && y < 32))) { // place button & pic
        for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
            if (getStructuresQueue(FRIENDLY)[i].enabled && getStructuresQueue(FRIENDLY)[i].itemInfo == infoScreenBuildInfoNr && getStructuresQueue(FRIENDLY)[i].status == DONE) {
                setModifierPlaceStructure(i);
                playSoundeffect(SE_BUTTONCLICK);
                return 1;
            }
        }
    }
    return 0;
}

void drawButtonPlaceSelect(int x, int y) {
    int i;
    
    if (x >= 48 && x < 80) {
        if (y >= 56 && y < 80)
            y = 56;
        else if (y >= 8 && y < 32)
            y = 8;
        else
            return;
        
        for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
            if (getStructuresQueue(FRIENDLY)[i].enabled && getStructuresQueue(FRIENDLY)[i].itemInfo == infoScreenBuildInfoNr && getStructuresQueue(FRIENDLY)[i].status == DONE) {
                setSpriteInfoScreen(y, ATTR0_SQUARE,
                                    48, SPRITE_SIZE_32, 0, 0,
                                    0, 0, base_select_graphics);
                return;
            }
        }
    }
}

int inputGlobalGameinfoButton(int x, int y) {
    if (x >= 158 && x < 174 && y >= 137 && y < 152) {
        if (getObjectivesState() == OBJECTIVES_INCOMPLETE) {
            playSoundeffect(SE_BUTTONCLICK);
            return 1;
        }
        playSoundeffect(SE_CANNOT);
    }
    return 0;
}

void drawGlobalGameinfoButtonSelect(int x, int y) {
    if (x >= 158 && x < 174 && y >= 137 && y < 152)
        setSpriteInfoScreen(137, ATTR0_WIDE,
                            154, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_select_graphics + 4 + 2 + 1 + 1 + 1 + 2);
}

int inputGameinfoButton(int x, int y) {
    if (x >= 186 && x < 201 && y >= 89 && y < 105) {
        if (getObjectivesState() == OBJECTIVES_INCOMPLETE) {
            playSoundeffect(SE_BUTTONCLICK);
            return 1;
        }
        playSoundeffect(SE_CANNOT);
    }
    return 0;
}

void drawGameinfoButtonSelect(int x, int y) {
    if (x >= 186 && x < 201 && y >= 89 && y < 105)
        setSpriteInfoScreen(89, ATTR0_WIDE,
                            180, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_select_graphics + 4 + 2 + 1 + 1 + 1 + 2);
}

void drawGameinfoButton() {
    setSpriteInfoScreen(89, ATTR0_WIDE,
                        180, SPRITE_SIZE_32, 0, 0,
                        0, 0, base_gameinfo_button);
}


void inputHotKeysManipulation(int x, int y) {
    int x_center = 128;
    int y_center = 64;
    int key = 0;
   
    if (x < 100)
        x_center = 72;
    else if (x > 156)
        x_center = 184;
    
    if (x > x_center - 6 && x <= x_center + 6) {
        if (y > y_center - 17 && y <= y_center - 5)
            key = 1; // up
        else if (y > y_center + 5 && y <= y_center + 17)
            key = 2; // down
    }
    if (y > y_center - 6 && y <= y_center + 6) {
        if (x > x_center - 17 && x <= x_center - 5)
            key = 3; // left
        else if (x > x_center + 5 && x <= x_center + 17)
            key = 4; // right
    }
    if (key) {
        if (getPrimaryHand() == LEFT_HANDED) {
            if (key == 1)
                key = KEY_UP;
            else if (key == 2)
                key = KEY_DOWN;
            else if (key == 3)
                key = KEY_LEFT;
            else
                key = KEY_RIGHT;
        } else {
            if (key == 1)
                key = KEY_X;
            else if (key == 2)
                key = KEY_B;
            else if (key == 3)
                key = KEY_Y;
            else
                key = KEY_A;
        }
    
        if (x_center == 72) { // clear
            clearHotKey(key);
            playSoundeffect(SE_BUTTONCLICK);
        } else if (x_center == 184) { // add: only possible for friendly unit selection to friendly unit group
            addHotKey(key) ? playSoundeffect(SE_BUTTONCLICK) : playSoundeffect(SE_CANNOT);
        } else { // save: possible for any selection other than an enemy unit
            saveHotKey(key) ? playSoundeffect(SE_BUTTONCLICK) : playSoundeffect(SE_CANNOT);
        }
    }
}

void drawHotKeysManipulationSelect(int x, int y) {
    int x_center = 128;
     int y_center = 64;
    int key = 0;
   
    if (x < 100)
        x_center = 72;
    else if (x > 156)
        x_center = 184;
    
    if (x > x_center - 6 && x <= x_center + 6) {
        if (y > y_center - 17 && y <= y_center - 5)
            key = 1; // up
        else if (y > y_center + 5 && y <= y_center + 17)
            key = 2; // down
    }
    if (y > y_center - 6 && y <= y_center + 6) {
        if (x > x_center - 17 && x <= x_center - 5)
            key = 3; // left
        else if (x > x_center + 5 && x <= x_center + 17)
            key = 4; // right
    }
    if (key) {
        if (key == 1)
            setSpriteInfoScreen(y_center - 19, ATTR0_SQUARE,
                                x_center - 8, SPRITE_SIZE_16, 0, 0,
                                0, 0, base_select_graphics + 4 + 2 + 1 + 1);
        else if (key == 2)
            setSpriteInfoScreen(y_center + 3, ATTR0_SQUARE,
                                x_center - 8, SPRITE_SIZE_16, 0, 0,
                                0, 0, base_select_graphics + 4 + 2 + 1 + 1);
        else if (key == 3)
            setSpriteInfoScreen(y_center - 8, ATTR0_SQUARE,
                                x_center - 19, SPRITE_SIZE_16, 0, 0,
                                0, 0, base_select_graphics + 4 + 2 + 1 + 1);
        else
            setSpriteInfoScreen(y_center - 8, ATTR0_SQUARE,
                                x_center + 3, SPRITE_SIZE_16, 0, 0,
                                0, 0, base_select_graphics + 4 + 2 + 1 + 1);
    }
}

void drawSelectAll() {
    // 32x16 = SPRITE_SIZE_32 + ATTR0_WIDE
    // Two sprites (instead of 4)
    setSpriteInfoScreen(90, ATTR0_WIDE,
                        96, SPRITE_SIZE_32, 0, 0,
                        0, 0, base_select_all);
    setSpriteInfoScreen(90, ATTR0_WIDE,
                        96+32, SPRITE_SIZE_32, 0, 0,
                        0, 0, base_select_all+2);
}

void inputButtonSelectAll (int x, int y) {
    if (x >= 96 && x < (96+64) && y >= 90 && y < (90+16)) {
        doUnitsSelectAll(); // select all FRIENDLY units, other than those that collect ore
        playSoundeffect(SE_BUTTONCLICK);
    }
}


void inputInfoScreen() {
    int swapScreensRequest = 0;
    touchPosition touchXY = touchReadLast();
    
//    startProfilingFunction("inputInfoScreen");
    
    inputButtonScrolling();
    inputHotKeys();
    
    if (infoScreenType == IS_RADAR)
        inputRadar(touchXY.px, touchXY.py);
    if (keysHeld() & KEY_TOUCH) {
        inputBarStructuresArrows(touchXY.px, touchXY.py, getKeysHeldTimeMin(KEY_TOUCH));
        inputBarBuildablesArrows(touchXY.px, touchXY.py, getKeysHeldTimeMin(KEY_TOUCH));
    }
    if (getKeysOnUp() & KEY_TOUCH) { // everything that may only be tapped
        /* these buttons are present on every infoscreen */
        if (getObjectivesState() != OBJECTIVES_FAILED) {
            inputButtonHotKeys(touchXY.px, touchXY.py);
            inputButtonBuildQueue(touchXY.px, touchXY.py);
            inputBarStructures(touchXY.px, touchXY.py);
            inputBarStructuresArrows(touchXY.px, touchXY.py, 0);
            inputBarBuildables(touchXY.px, touchXY.py);
            inputBarBuildablesArrows(touchXY.px, touchXY.py, 0);
        }
        
        switch (infoScreenType) {
            case IS_RADAR:
                break;
            case IS_BUILDINFO:
                if (inputGlobalGameinfoButton(touchXY.px, touchXY.py)) {
                    setGameState(MENU_GAMEINFO);
                    return;
                }
                inputButtonRadar(touchXY.px, touchXY.py);
                if (infoScreenBuildInfoCanBuild) {
                    inputButtonMinplus(touchXY.px, touchXY.py);
                    swapScreensRequest = inputButtonPlace(touchXY.px, touchXY.py);
                }
                if (inputGameinfoButton(touchXY.px, touchXY.py)) {
                    setGameinfoIsStructure(infoScreenBuildInfoIsStructure);
                    setGameinfoItemNr(infoScreenBuildInfoNr);
                    setGameState(MENU_GAMEINFO_ITEM_INFO);
                    return;
                }
                break;
            case IS_HOTKEYS:
                if (inputGlobalGameinfoButton(touchXY.px, touchXY.py)) {
                    setGameState(MENU_GAMEINFO);
                    return;
                }
                inputHotKeysManipulation(touchXY.px, touchXY.py);
                inputButtonSelectAll(touchXY.px, touchXY.py);
                inputButtonRadar(touchXY.px, touchXY.py);
                break;
            case IS_BUILDQUEUE:
                if (inputGlobalGameinfoButton(touchXY.px, touchXY.py)) {
                    setGameState(MENU_GAMEINFO);
                    return;
                }
                inputButtonRadar(touchXY.px, touchXY.py);
                swapScreensRequest = inputBuildQueue(touchXY.px, touchXY.py);
                break;
        }
    }
    
    if (((getKeysOnUp() & KEY_TOUCH) && inputButtonSwapScreens(touchXY.px, touchXY.py)) ||
        (swapScreensRequest /*&& getScreenSwitchMethod() == PRESS_TRIGGER*/) ||
/*
        (getScreenSwitchMethod() == PRESS_TRIGGER && ((getPrimaryHand() == RIGHT_HANDED && (keysDown() & KEY_L)) ||
                                                       (getPrimaryHand() == LEFT_HANDED && (keysDown() & KEY_R)))) ||
        (getScreenSwitchMethod() == HOLD_TRIGGER && ((getPrimaryHand() == RIGHT_HANDED && (keysUp() & KEY_L)) ||
                                                      (getPrimaryHand() == LEFT_HANDED && (keysUp() & KEY_R))))
*/
        (getScreenSwitchMethod() == PRESS_TRIGGER && (keysDown() & (KEY_L | KEY_R))) ||
        (getScreenSwitchMethod() == HOLD_TRIGGER  && (keysUp() & (KEY_L | KEY_R)))
    ) {
//      lcdSwap();
        setScreenSetUp(SUB_TO_MAIN_TOUCH);
        initKeysHeldTime(~0);
        playSoundeffect(SE_BUTTONCLICK);
    }
    
//    stopProfilingFunction();
}



void doBarLogic(struct BarItem *bar, int max_items) {
    int i, j;
    
    for (i=0; i<max_items && bar[i].info != -1; i++)
        doBarItemAnimationLogic(&bar[i]);
    
    for (i=0; i<max_items; i++) {
        if (bar[i].info == -1) {
            for (j=i+1; j<max_items; j++) {
                if (bar[j].info != -1) {
                    bar[i] = bar[j];
                    bar[j] = createBarItem(-1, IBAT_NONE);
                    break;
                }
            }
            if (j == max_items)
                break;
        }
    }
}


void doBarStructuresLogic() {
    doBarLogic(itemsBarStructures, MAX_ITEMS_BAR_STRUCTURES);
}


void doBarBuildablesLogic() {
    doBarLogic(itemsBarBuildables, MAX_ITEMS_BAR_BUILDABLES);
}


void doInfoScreenLogic() {
    int i;
    struct Structure *curStructure;
    
    startProfilingFunction("doInfoScreenLogic");
    
    if (getObjectivesState() != OBJECTIVES_FAILED) {
        doBarStructuresLogic();
        doBarBuildablesLogic();
    }
    
    for (i=0; i<getAmountOfSides(); i++) {
        updateStructuresQueue(i);
        updateUnitsQueue(i);
    }
    
    doRadarLogic();
    updateMatchTime();
    
    for (i=0, curStructure=structure; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
        if (curStructure->selected && curStructure->enabled && curStructure->side == FRIENDLY) {
            infoScreenStructureIconAnimationTimer++;
            if (infoScreenStructureIconAnimationTimer > STRUCTURE_ICON_ANIMATION_DURATION)
                infoScreenStructureIconAnimationTimer = 1;
            break;
        }
    }
    if (i == MAX_STRUCTURES_ON_MAP)
        infoScreenStructureIconAnimationTimer =  0;
    
    infoScreenArrowsAnimationTimer++;
    if (infoScreenArrowsAnimationTimer > ARROWS_ANIMATION_DURATION)
        infoScreenArrowsAnimationTimer = 0;
    
    infoScreenMinplusAnimationTimer++;
    if (infoScreenMinplusAnimationTimer > MINPLUS_ANIMATION_DURATION)
        infoScreenMinplusAnimationTimer = 0;
    
    infoScreenPlaceIconAnimationTimer++;
    if (infoScreenPlaceIconAnimationTimer > PLACE_ICON_ANIMATION_DURATION)
        infoScreenPlaceIconAnimationTimer = 0;
    
    if (infoScreenType == IS_BUILDQUEUE)
        infoScreenBuildQueueAnimationTimer = 0;
    else {
        for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
            if (getStructuresQueue(FRIENDLY)[i].enabled && getStructuresQueue(FRIENDLY)[i].status == DONE)
                break;
        }
        if (i < FRIENDLY_STRUCTURES_QUEUE)
            infoScreenBuildQueueAnimationTimer++;
        if (i >= FRIENDLY_STRUCTURES_QUEUE || (infoScreenBuildQueueAnimationTimer > BUILDQUEUE_ANIMATION_DURATION))
            infoScreenBuildQueueAnimationTimer = 0;
    }
    
    stopProfilingFunction();
}


void drawAlphaCaption(char *string, int x, int y) {
    int i=0;
    int curChar;
    
    while (*string != 0) { // 0 being end of string
        i++;
        if (*string == ' ' || *string == '_') {
            x += 4;
        } else {
            curChar = ((int)*string);
            if (curChar == ':') {
                setSpriteInfoScreen(y, ATTR0_SQUARE,
                                    x, SPRITE_SIZE_8, 0, 0,
                                    0, 0, base_font_small + 36);
                x += 3;
            } else if (curChar == '/') {
                x += 1;
                setSpriteInfoScreen(y, ATTR0_SQUARE,
                                    x, SPRITE_SIZE_8, 0, 0,
                                    0, 0, base_font_small + 37);
                x += 4;
            } else if (curChar < 48 || curChar >= 58)  {
                if (curChar < 97) // capital letter
                    curChar -= 65;
                else // small letter
                    curChar -= 97;
                setSpriteInfoScreen(y, ATTR0_SQUARE,
                                    x, SPRITE_SIZE_8, 0, 0,
                                    0, 0, base_font_small + curChar);
                
                curChar += 97;
                if (curChar == 'i')
                    x += 1;
                else if (curChar == 'm' || curChar == 'w')
                    x += 4;
                else if (curChar == 'n' || curChar == 'q')
                    x += 4;
                else
                    x += 3;
            }
            x += 1; // spacing
        }
        string++;
    }
}

void drawDigitsCaption(int number, int x, int y) {
    int curDigit;
    int i=0;
    
    do {
        i++;
        curDigit = number % 10;
        number = number / 10;
        if (curDigit == 1)
            x -= 2;
        else
            x -= 4;
        setSpriteInfoScreen(y, ATTR0_SQUARE,
                            x, SPRITE_SIZE_8, 0, 0,
                            0, 0, base_font_small + 26 + curDigit);
        x -= 1; // spacing
    } while (number);
}

void drawCredits() {
    int credits = getCredits(FRIENDLY);
    int i=0;
    
    do {
        setSpriteInfoScreen(181, ATTR0_SQUARE,
                            79 - 9*i, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_digits_credits + (credits%10));
        credits /= 10;
        i++;
    } while (credits != 0);
}

void drawPower() {
    int diff;
    int x = 181 - 16/2;
    
    diff = getPowerGeneration(FRIENDLY) - getPowerConsumation(FRIENDLY);
    if (diff < 0) // position in center of red
        x += 1;
    else if (diff > ((28 * 100) / 8))
        x += 30;
    else
        x += 3 + ((diff * 8) / 100);
    
    setSpriteInfoScreen(SCREEN_HEIGHT-16, ATTR0_SQUARE,
                        x, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_power_slider);
}

int getPositionOfItemInStructuresBar(int info) {
    int i = 0;
    
    while (itemsBarStructures[i].info != -1) {
        if (itemsBarStructures[i].info == info)
            return i;
        i++;
    }
    return -1;
}

void drawBarStructures() {
    struct Structure *curStructure;
    int i,j;
    int y;
    
    if (infoScreenItemBarStructuresFirstShown == -1)
        return;
    
    if (!haveDrawnBarStructuresSelect && infoScreenStructureIconAnimationTimer > 0 && infoScreenStructureIconAnimationTimer < STRUCTURE_ICON_ANIMATION_DURATION/2) {
        for (i=0, curStructure=structure; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
            if (curStructure->selected && curStructure->enabled && curStructure->side == FRIENDLY) {
                j = getPositionOfItemInStructuresBar(curStructure->info);
                if (j >= 0 && infoScreenItemBarStructuresFirstShown <= j && j < infoScreenItemBarStructuresFirstShown + 7)
                    setSpriteInfoScreen(8 + (j - infoScreenItemBarStructuresFirstShown) * 24, ATTR0_SQUARE,
                                        4, SPRITE_SIZE_32, 0, 0,
                                        0, 0, base_select_graphics);
                break;
            }
        }
    }
    
    y = 8;
    for (i=0; i<7; i++) {
        j = itemsBarStructures[infoScreenItemBarStructuresFirstShown + i].info;
        if (j == -1) break;
        if ((itemsBarStructures[infoScreenItemBarStructuresFirstShown + i].animation & IBAT_MASK) != IBAT_NONE) {
            setSpriteInfoScreen(y, ATTR0_SQUARE,
                                4, SPRITE_SIZE_32, 0, 0,
                                0, 0, base_sidebars_animation + (((itemsBarStructures[infoScreenItemBarStructuresFirstShown + i].animation >> 2) * 3) / ITEM_BAR_ANIMATION_DURATION)*4);
        } else {
            setSpriteInfoScreen(y, ATTR0_SQUARE,
                                4, SPRITE_SIZE_32, 0, 0,
                                0, 0, base_structure_icons + j*4);
        }
        y += 24;
    }
    if (i==7 && itemsBarStructures[infoScreenItemBarStructuresFirstShown + 7].info != -1)
        setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                            4, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_arrows + 1 + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
    if (infoScreenItemBarStructuresFirstShown > 0 && itemsBarStructures[infoScreenItemBarStructuresFirstShown - 1].info != -1)
        setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                            20, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_arrows + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
 }

void drawBarBuildables() {
    int i,j;
    int y;

    if (infoScreenItemBarBuildablesFirstShown == -1 || infoScreenItemBarBuildablesStructureNr == -1)
        return;
    
    y = 8;
    for (i=0; i<7; i++) {
        j = infoScreenItemBarBuildablesFirstShown + i;
        if (itemsBarBuildables[j].info == -1) break;
        if ((itemsBarBuildables[j].animation & IBAT_MASK) != IBAT_NONE) {
            setSpriteInfoScreen(y, ATTR0_SQUARE,
                                SCREEN_WIDTH - 36, SPRITE_SIZE_32, 0, 0,
                                0, 0, base_sidebars_animation + (((itemsBarBuildables[j].animation >> 2) * 3) / ITEM_BAR_ANIMATION_DURATION)*4);
        } else {
            if (j < amountOfStructuresInBarBuildables)
                setSpriteInfoScreen(y, ATTR0_SQUARE,
                                    SCREEN_WIDTH - 36, SPRITE_SIZE_32, 0, 0,
                                    0, 0, base_structure_icons + itemsBarBuildables[j].info*4);
            else
                setSpriteInfoScreen(y, ATTR0_SQUARE,
                                    SCREEN_WIDTH - 36, SPRITE_SIZE_32, 0, 0,
                                    0, 0, base_unit_icons + itemsBarBuildables[j].info*4);
        }
        y += 24;
    }
    if (i==7 && itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + 7].info != -1)
        setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                            SCREEN_WIDTH - 36, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_arrows + 1 + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
    if (infoScreenItemBarBuildablesFirstShown > 0 && itemsBarBuildables[infoScreenItemBarBuildablesFirstShown - 1].info != -1)
        setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_SQUARE,
                            SCREEN_WIDTH - 20, SPRITE_SIZE_16, 0, 0,
                            0, 0, base_arrows + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
}

void drawQueuePercentage(int x, int y, int percentage) {
    int i;

    percentage = 32 - ((percentage * 32) / 100); /* how many pixels need to be checkered out */
    y = (y/8)*32;
    
    if (x%8 == 0)
        x = x/8 + 3; /* starting at the last tile to checker out */
    else { // special situation when aligned on a 4px boundary (assuming it is)
        x = x/8 + 4;
        if (percentage > 0) {
            for (i=4; i>0; i--) {
                if (percentage >= i) {
                    mapSubBG0[y + x]      = base_queue_building + 7 + i;
                    mapSubBG0[y + x + 32] = base_queue_building + 7 + i;
                    mapSubBG0[y + x + 64] = base_queue_building + 7 + i;
                    break;
                }
            }
            percentage -= 4;
        }
        x--;
    }
    
    while (percentage > 0) {
        for (i=8; i>0; i--) {
            if (percentage >= i) {
                mapSubBG0[y + x]      = base_queue_building + i - 1;
                mapSubBG0[y + x + 32] = base_queue_building + i - 1;
                mapSubBG0[y + x + 64] = base_queue_building + i - 1;
                percentage -= 8;
                x--;
                break;
            }
        }
    }
}

void drawHotKeys(int x_center, int y_center) {
    setSpriteInfoScreen(y_center - 19, ATTR0_SQUARE,
                        x_center - 8, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_hotkey);
    setSpriteInfoScreen(y_center + 3, ATTR0_SQUARE,
                        x_center - 8, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_hotkey);
    setSpriteInfoScreen(y_center - 8, ATTR0_SQUARE,
                        x_center - 19, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_hotkey);
    setSpriteInfoScreen(y_center - 8, ATTR0_SQUARE,
                        x_center + 3, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_hotkey);
}

void drawStatistics() {
    int number;
    
    // match time
    number = getMatchTime();
    setSpriteInfoScreen(132, ATTR0_SQUARE,
                        140, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number % 10));
    setSpriteInfoScreen(132, ATTR0_SQUARE,
                        133, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + ((number % 60) / 10));
    setSpriteInfoScreen(132, ATTR0_SQUARE,
                        122, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + ((number / 60) % 10));
    setSpriteInfoScreen(132, ATTR0_SQUARE,
                        115, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (((number / 60) % 60) / 10));
    setSpriteInfoScreen(132, ATTR0_SQUARE,
                        104, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number / 3600));
    // current amount of friendly units
    number = getUnitCount(FRIENDLY);
    if (number > 99) number = 99;
    setSpriteInfoScreen(141, ATTR0_SQUARE,
                        122, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number % 10));
    setSpriteInfoScreen(141, ATTR0_SQUARE,
                        115, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number / 10));
    // max amount of friendly units
    number = getUnitLimit(FRIENDLY);
    if (number > 99) number = 99;
    setSpriteInfoScreen(141, ATTR0_SQUARE,
                        140, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number % 10));
    setSpriteInfoScreen(141, ATTR0_SQUARE,
                        133, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number / 10));
    // enemy unit kills
    number = getUnitDeaths(ENEMY1) + getUnitDeaths(ENEMY2) + getUnitDeaths(ENEMY3) + getUnitDeaths(ENEMY4);
    if (number > 999) number = 999;
    setSpriteInfoScreen(150, ATTR0_SQUARE,
                        115, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number % 10));
    setSpriteInfoScreen(150, ATTR0_SQUARE,
                        108, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + ((number % 100) / 10));
    setSpriteInfoScreen(150, ATTR0_SQUARE,
                        101, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + ((number % 1000) / 100));
    // friendly unit deaths
    number = getUnitDeaths(FRIENDLY);
    if (number > 999) number = 999;
    setSpriteInfoScreen(150, ATTR0_SQUARE,
                        140, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + (number % 10));
    setSpriteInfoScreen(150, ATTR0_SQUARE,
                        133, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + ((number % 100) / 10));
    setSpriteInfoScreen(150, ATTR0_SQUARE,
                        126, SPRITE_SIZE_16, 0, 0,
                        0, 0, base_digits_stats + ((number % 1000) / 100));
}

void drawInfoScreen() {
    struct Structure *curStructure;
    struct Unit *curUnit;
    int amountOfItems;
    int i, j;
    int amountInQueue = 0;
    int amountDoneInQueue = 0;
    int bestInQueue = -1;
    touchPosition touchXY = touchReadLast();
    
    startProfilingFunction("drawInfoScreen");
    
    initSpritesUsedInfoScreen();
    
    REG_BG0VOFS_SUB = -getScreenScrollingInfoScreenShort();
    REG_BG1VOFS_SUB = -getScreenScrollingInfoScreenShort();
    REG_BG3Y_SUB = -(getScreenScrollingInfoScreen() * 256);
    if (infoScreenType == IS_RADAR)
        positionRadar();
    else
        REG_BG2Y_SUB = (-(22 << 8)) - (getScreenScrollingInfoScreen() * 256); // y position
    
    if (getScreenSetUp() == MAIN_TO_SUB_TOUCH || getScreenSetUp() == SUB_TO_MAIN_TOUCH) {
        for (i=0; i<4; i++) {
            setSpriteInfoScreen(SCREEN_HEIGHT, ATTR0_SQUARE,
                                i*64, SPRITE_SIZE_64, 0, 0,
                                0, 0, base_glitch_hide_sprite);
        }
    }
    
    haveDrawnBarStructuresSelect = 0;
    if (getObjectivesState() != OBJECTIVES_FAILED) {
        if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH)) {
            drawBarStructuresSelect(touchXY.px, touchXY.py);
            drawBarBuildablesSelect(touchXY.px, touchXY.py);
            drawBarStructuresArrowsSelect(touchXY.px, touchXY.py);
            drawBarBuildablesArrowsSelect(touchXY.px, touchXY.py);
            drawButtonHotKeysSelect(touchXY.px, touchXY.py);
            drawButtonSwapScreensSelect(touchXY.px, touchXY.py);
            drawButtonBuildQueueSelect(touchXY.px, touchXY.py);
        }
        
        drawBarStructures();
        drawBarBuildables();
    }
    
    if (getScreenSetUp() != MAIN_TO_SUB_TOUCH && getScreenSetUp() != SUB_TO_MAIN_TOUCH) {
        drawCredits();
        drawPower();
    }
    
    if (infoScreenBuildQueueAnimationTimer >= BUILDQUEUE_ANIMATION_DURATION/2)
        setSpriteInfoScreen(176, ATTR0_WIDE,
                            137, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_buildqueue_flash);
    
    switch (infoScreenType) {
        case IS_RADAR:
            REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE;
            drawRadar();
            REG_BLDALPHA_SUB = ((RADAR_OVERLAY_DEFAULT_BLEND_A - MAX_RADAR_OVERLAY_BLEND_A_DEVIATION) + (rand() % (MAX_RADAR_OVERLAY_BLEND_A_DEVIATION*2 + 1))) |
                              (((RADAR_OVERLAY_DEFAULT_BLEND_B - MAX_RADAR_OVERLAY_BLEND_B_DEVIATION) + (rand() % (MAX_RADAR_OVERLAY_BLEND_B_DEVIATION*2 + 1))) << 8);
            break;
        case IS_BUILDINFO:
            REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE;
            if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH)) {
                if (infoScreenBuildInfoCanBuild)
                    drawButtonMinplusSelect(touchXY.px, touchXY.py);
                drawButtonRadarSelect(touchXY.px, touchXY.py);
                drawGlobalGameinfoButtonSelect(touchXY.px, touchXY.py);
                drawGameinfoButtonSelect(touchXY.px, touchXY.py);
            }
            /* zero out the building percentages */
            for (i=0; i<768; i++)
                mapSubBG0[i] = 0;
            if (infoScreenBuildInfoCanBuild) {
                /* display queue statistics text */
                drawAlphaCaption("in-queue:", 111, 90);
                drawAlphaCaption("complete:", 111, 98);
                /* display costs text */
                drawAlphaCaption("c:", 172, 10);
            } else {
                /* display can't build text */
                drawAlphaCaption("unable to build.", 84, 90);
                //drawAlphaCaption("consult techtree.", 93, 98);
            }
            /* draw statistics */
            drawStatistics();
            /* draw info button */
            drawGameinfoButton();
            
            if (infoScreenBuildInfoIsStructure) {
                // need to display information on the selected buildable structure
                
                /* display pic */
                setSpriteInfoScreen(8, ATTR0_SQUARE,
                                    48, SPRITE_SIZE_32, 0, 0,
                                    1, 0, base_structure_icons + infoScreenBuildInfoNr*4);
                /* display name */
                drawAlphaCaption(structureInfo[infoScreenBuildInfoNr].name, 84, 10);
                if (infoScreenBuildInfoCanBuild) {
                    /* display costs*/
                    drawDigitsCaption(structureInfo[infoScreenBuildInfoNr].build_cost, 200, 10);
                }
                /* display "size" */
                setSpriteInfoScreen(32, ATTR0_SQUARE,
                                    48, SPRITE_SIZE_32, 0, 0,
                                    0, 0, base_size_icon[(structureInfo[infoScreenBuildInfoNr].height-1)*4+(structureInfo[infoScreenBuildInfoNr].width-1)]);
                                    // used to be: base_size_icons + (3*(structureInfo[infoScreenBuildInfoNr].width-1)+(structureInfo[infoScreenBuildInfoNr].height-1))*4);
                
                if (!infoScreenBuildInfoCanBuild)
                    break;
                
                for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
                    if (getStructuresQueue(FRIENDLY)[i].enabled && getStructuresQueue(FRIENDLY)[i].itemInfo == infoScreenBuildInfoNr) {
                        amountInQueue++;
                        if (getStructuresQueue(FRIENDLY)[i].status == DONE || getStructuresQueue(FRIENDLY)[i].status == FROZEN)
                            amountDoneInQueue++;
                        if (bestInQueue == -1 ||
                            (getStructuresQueue(FRIENDLY)[bestInQueue].status != DONE && getStructuresQueue(FRIENDLY)[bestInQueue].status != FROZEN && getStructuresQueue(FRIENDLY)[i].status >= BUILDING) ||
                            (getStructuresQueue(FRIENDLY)[bestInQueue].status <= PAUSED && (getStructuresQueue(FRIENDLY)[bestInQueue].completion < getStructuresQueue(FRIENDLY)[i].completion)))
                            bestInQueue = i;
                    }
                }
                /* display status of most completed item in the queue */
                if (bestInQueue != -1) {
                    drawQueuePercentage(44, 8, (getStructuresQueue(FRIENDLY)[bestInQueue].completion * 100) / structureInfo[getStructuresQueue(FRIENDLY)[bestInQueue].itemInfo].build_time);
                    if (getStructuresQueue(FRIENDLY)[bestInQueue].status == PAUSED)
                        setSpriteInfoScreen(16, ATTR0_WIDE,
                                            48, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_queue_words);
                    else if (getStructuresQueue(FRIENDLY)[bestInQueue].status == DONE || getStructuresQueue(FRIENDLY)[bestInQueue].status == FROZEN)
                        setSpriteInfoScreen(16, ATTR0_WIDE,
                                            48, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_queue_words + 1);
                }
                
                /* display queue statistics */
                drawDigitsCaption(amountInQueue, 158, 90);
                drawDigitsCaption(amountDoneInQueue, 158, 98);
                
                /* possibly display "place" */
                if (amountDoneInQueue > 0) {
                    if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH))
                        drawButtonPlaceSelect(touchXY.px, touchXY.py);
                    setSpriteInfoScreen(56, ATTR0_SQUARE,
                                        48, SPRITE_SIZE_32, 0, 0,
                                        0, 0, base_place_icon + (infoScreenPlaceIconAnimationTimer >= PLACE_ICON_ANIMATION_DURATION/2) * 4);
                }
                
                /* possibly display minus icon */
                if (amountInQueue > 0) {
                    setSpriteInfoScreen(88, ATTR0_SQUARE,
                                        48, SPRITE_SIZE_16, 0, 0,
                                        0, 0, base_minplus + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
                }
                /* possibly display plus icon */
                for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
                    if (!getStructuresQueue(FRIENDLY)[i].enabled) {
                        amountOfItems = MAX_DIFFERENT_FACTIONS;
                        curStructure = structure;
                        for (i=MAX_DIFFERENT_FACTIONS; i<MAX_STRUCTURES_ON_MAP; i++, curStructure++) {
                            if (curStructure->enabled)
                                amountOfItems++;
                        }
                        for (i=0; i<getAmountOfSides(); i++) {
                            for (j=0; j<FRIENDLY_STRUCTURES_QUEUE; j++) {
                                if (getStructuresQueue(i)[j].enabled && !structureInfo[getStructuresQueue(i)[j].itemInfo].foundation)
                                    amountOfItems++;
                            }
                        }
                        if (amountOfItems < MAX_STRUCTURES_ON_MAP)
                            setSpriteInfoScreen(88, ATTR0_SQUARE,
                                                64, SPRITE_SIZE_16, 0, 0,
                                                0, 0, base_minplus + 1  + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
                        break;
                    }
                }
                break;
            }
            /* else: (the above "break" makes sure this is an "else") */
            // need to display information on the selected buildable unit
            
            /* display pic */
            setSpriteInfoScreen(8, ATTR0_SQUARE,
                                48, SPRITE_SIZE_32, 0, 0,
                                1, 0, base_unit_icons + infoScreenBuildInfoNr*4);
            /* display name */
            drawAlphaCaption(unitInfo[infoScreenBuildInfoNr].name, 84, 10);
            if (infoScreenBuildInfoCanBuild) {
                /* display costs*/
                drawDigitsCaption(unitInfo[infoScreenBuildInfoNr].build_cost, 200, 10);
            }
            
            if (!infoScreenBuildInfoCanBuild)
                break;
            
            for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                if (getUnitsQueue(FRIENDLY)[i].enabled && getUnitsQueue(FRIENDLY)[i].itemInfo == infoScreenBuildInfoNr) {
                    amountInQueue++;
                    if (getUnitsQueue(FRIENDLY)[i].status == DONE)
                        amountDoneInQueue++;
                    if (bestInQueue == -1 ||
                        (getUnitsQueue(FRIENDLY)[bestInQueue].status != DONE && getUnitsQueue(FRIENDLY)[bestInQueue].status != FROZEN && getUnitsQueue(FRIENDLY)[i].status >= BUILDING) ||
                        (getUnitsQueue(FRIENDLY)[bestInQueue].status <= PAUSED && (getUnitsQueue(FRIENDLY)[bestInQueue].completion < getUnitsQueue(FRIENDLY)[i].completion)))
                        bestInQueue = i;
                }
            }
            /* display status of most completed item in the queue */
            if (bestInQueue != -1) {
                drawQueuePercentage(44, 8, (getUnitsQueue(FRIENDLY)[bestInQueue].completion * 100) / unitInfo[getUnitsQueue(FRIENDLY)[bestInQueue].itemInfo].build_time);
                if (getUnitsQueue(FRIENDLY)[bestInQueue].status == PAUSED)
                    setSpriteInfoScreen(16, ATTR0_WIDE,
                                        48, SPRITE_SIZE_16, 0, 0,
                                        0, 0, base_queue_words);
                else if (getUnitsQueue(FRIENDLY)[bestInQueue].status == DONE || getUnitsQueue(FRIENDLY)[bestInQueue].status == FROZEN)
                    setSpriteInfoScreen(16, ATTR0_WIDE,
                                        48, SPRITE_SIZE_16, 0, 0,
                                        0, 0, base_queue_words + 1);
            }
            
            /* display queue statistics */
            drawDigitsCaption(amountInQueue, 158, 90);
            drawDigitsCaption(amountDoneInQueue, 158, 98);
            
            /* possibly display minus icon */
            if (amountInQueue > 0) {
                setSpriteInfoScreen(88, ATTR0_SQUARE,
                                    48, SPRITE_SIZE_16, 0, 0,
                                    0, 0, base_minplus + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
            }
            
            /* possibly display plus icon */
            for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                if (!getUnitsQueue(FRIENDLY)[i].enabled) {
                    amountOfItems = 0;
                    curUnit = unit;
                    for (i=0; i<MAX_UNITS_ON_MAP; i++, curUnit++) {
                        if (curUnit->enabled && curUnit->side == FRIENDLY)
                            amountOfItems++;
                    }
                    for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                        if (getUnitsQueue(FRIENDLY)[i].enabled)
                            amountOfItems++;
                    }
                    if (amountOfItems < getUnitLimit(FRIENDLY))
                        setSpriteInfoScreen(88, ATTR0_SQUARE,
                                            64, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_minplus + 1 + (infoScreenMinplusAnimationTimer >= MINPLUS_ANIMATION_DURATION/2) * 2);
                    break;
                }
            }
            break;
        case IS_HOTKEYS:
            if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH)) {
                drawHotKeysManipulationSelect(touchXY.px, touchXY.py);
                drawButtonRadarSelect(touchXY.px, touchXY.py);
                drawGlobalGameinfoButtonSelect(touchXY.px, touchXY.py);
            }
            /* display informational text regarding usage */
// 28+29+2extra == 59
//
// TODO
            drawAlphaCaption("manipulation of selection hotkeys", 60, 11);
            if (getPrimaryHand() == LEFT_HANDED)
                drawAlphaCaption("mapped to dpad or slot two grip", 64, 20);
            else
                drawAlphaCaption("mapped to abxy or slot two grip", 64, 20);
            /* display hotkeys */
            drawAlphaCaption("clear", 61, 36);
            drawHotKeys(72, 64);
            drawAlphaCaption("save", 120, 36);
            drawHotKeys(128, 64);
            drawAlphaCaption("add", 178, 36);
            drawHotKeys(184, 64);
            
            /* draw select all units button */
            drawSelectAll();
            
            /* draw statistics */
            drawStatistics();
            break;
        case IS_BUILDQUEUE:
            if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH)) {
                drawButtonRadarSelect(touchXY.px, touchXY.py);
                drawGlobalGameinfoButtonSelect(touchXY.px, touchXY.py);
            }
            /* display informational text regarding usage */
            drawAlphaCaption("top: structures        bottom: units", 49, 11);
            drawAlphaCaption("tap: place/pause         hold: remove", 49, 20);
            /* zero out the building percentages */
            for (i=0; i<768; i++)
                mapSubBG0[i] = 0;
            /* display structure queue */
            for (i=0; i<FRIENDLY_STRUCTURES_QUEUE; i++) {
                if (getStructuresQueue(FRIENDLY)[i].enabled) {
                    if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH) && touchXY.py >= 32 && touchXY.py < 32+24 && touchXY.px >= (48 + i*32) && touchXY.px < (48 + (i+1)*32))
                        setSpriteInfoScreen(32, ATTR0_SQUARE,
                                            48 + i*32, SPRITE_SIZE_32, 0, 0,
                                            0, 0, base_select_graphics);
                    setSpriteInfoScreen(32, ATTR0_SQUARE,
                                        48 + i*32, SPRITE_SIZE_32, 0, 0,
                                        1, 0, base_structure_icons + getStructuresQueue(FRIENDLY)[i].itemInfo*4);
                    drawQueuePercentage(48 + i*32, 32, (getStructuresQueue(FRIENDLY)[i].completion * 100) / structureInfo[getStructuresQueue(FRIENDLY)[i].itemInfo].build_time);
                    if (getStructuresQueue(FRIENDLY)[i].status == PAUSED)
                        setSpriteInfoScreen(40, ATTR0_WIDE,
                                            48 + i*32, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_queue_words);
                    else if (getStructuresQueue(FRIENDLY)[i].status == DONE)
                        setSpriteInfoScreen(40, ATTR0_WIDE,
                                            48 + i*32, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_queue_words + 1);
                } else
                    setSpriteInfoScreen(32, ATTR0_SQUARE,
                                        48 + i*32, SPRITE_SIZE_32, 0, 0,
                                        1, 0, base_emptyslot_icons);
            }
            /* display unit queue */
            for (i=0; i<FRIENDLY_UNITS_QUEUE; i++) {
                if (getUnitsQueue(FRIENDLY)[i].enabled) {
                    if (getScreenSetUp() == SUB_TOUCH && (getKeysDown() & KEY_TOUCH) && touchXY.py >= (56 + (i/5)*24) && touchXY.py < (80 + (i/5)*24) && touchXY.px >= (48 + (i%5)*32) && touchXY.px < (48 + ((i%5)+1)*32))
                        setSpriteInfoScreen(56 + (i/5)*24, ATTR0_SQUARE,
                                            48 + (i%5)*32, SPRITE_SIZE_32, 0, 0,
                                            0, 0, base_select_graphics);
                    setSpriteInfoScreen(56 + (i/5)*24, ATTR0_SQUARE,
                                        48 + (i%5)*32, SPRITE_SIZE_32, 0, 0,
                                        1, 0, base_unit_icons + getUnitsQueue(FRIENDLY)[i].itemInfo*4);
                    drawQueuePercentage(48 + (i%5)*32, 56 + (i/5)*24, (getUnitsQueue(FRIENDLY)[i].completion * 100) / unitInfo[getUnitsQueue(FRIENDLY)[i].itemInfo].build_time);
                    if (getUnitsQueue(FRIENDLY)[i].status == PAUSED)
                        setSpriteInfoScreen(64 + (i/5)*24, ATTR0_WIDE,
                                            48 + (i%5)*32, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_queue_words);
                    else if (getUnitsQueue(FRIENDLY)[i].status == DONE)
                        setSpriteInfoScreen(64 + (i/5)*24, ATTR0_WIDE,
                                            48 + (i%5)*32, SPRITE_SIZE_16, 0, 0,
                                            0, 0, base_queue_words + 1);
                } else
                    setSpriteInfoScreen(56 + (i/5)*24, ATTR0_SQUARE,
                                        48 + (i%5)*32, SPRITE_SIZE_32, 0, 0,
                                        1, 0, base_emptyslot_icons + 4);
            }
            /* draw statistics */
            drawStatistics();
            break;
    }
    
    // draw the infoscreen bar at the bottom
    for (i=0; i<6; i++)
        setSpriteInfoScreen(SCREEN_HEIGHT - 16, ATTR0_WIDE,
                            40 + i*32, SPRITE_SIZE_32, 0, 0,
                            0, 0, base_infoscreen_bar + 2*i);
    
    stopProfilingFunction();
}

struct BarItem *getItemsBarStructures() {
    return itemsBarStructures;
}

struct BarItem *getItemsBarBuildables() {
    return itemsBarBuildables;
}


void loadInfoScreenGraphics() {
    char filename[256];
    int offset;
    int i, j, k;
    unsigned int l;
    uint16 *src, *dest;
    
    // first loading in palette
    copyFileVRAM(SPRITE_PALETTE_SUB, "ingameInfoscreenSprites", FS_PALETTES);
    strcpy(filename, "ingameInfoscreenBGs_");
    strcat(filename, factionInfo[getFaction(FRIENDLY)].name);
    copyFileVRAM(BG_PALETTE_SUB, filename, FS_PALETTES);
    BG_PALETTE_SUB[0] = 0;
    
//    if (getGameState() != MENU_INGAME) {
        // loading in bg graphics
        strcpy(filename, "ingame_");
        strcat(filename, factionInfo[getFaction(FRIENDLY)].name);
        copyFileVRAM((uint16*)(CHAR_BASE_BLOCK_SUB(5)), filename, FS_MENU_GRAPHICS);
//    }
    
    offset = 0;
    
    for (i=0; i<8*8/2; i++)
        ((uint16*) CHAR_BASE_BLOCK_SUB(2))[i] = 0; // transparent tile
    offset += 8*8;
    
    l = ~0;
    j = 0;
    for (i=1; i<256; i++) {
        if (BG_PALETTE_SUB[i] < l) {
            l = BG_PALETTE_SUB[i];
            j = i;
        }
    }
    j |= (j<<8);
    for (i=0; i<8*8/2; i++)
        ((uint16*) CHAR_BASE_BLOCK_SUB(2))[8*8/2 + i] = j; // darkest tile possible
    offset += 8*8;
    
    base_queue_building = offset/(8*8);
    offset += copyFileVRAM((uint16*)(CHAR_BASE_BLOCK_SUB(2) + offset), "queue_building", FS_GUI_GRAPHICS);
    
    loadRadarGraphics(CHAR_BASE_BLOCK_SUB(2), &offset, 0);
    
    // now loading in sprite graphics
    offset = 0;
    
    base_glitch_hide_sprite = offset/(16*16);
    for (i=0; i<64*64/2; i++)
        SPRITE_GFX_SUB[(offset>>1) + i] = (1<<8) | 1;
    offset += 64*64;
    
    base_arrows = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "arrows", FS_GUI_GRAPHICS);
    
    base_digits_credits = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "digits_credits", FS_GUI_GRAPHICS);
    
    base_digits_stats = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "digits_stats", FS_GUI_GRAPHICS);
    
    base_place_icon = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "Place", FS_ICONS_GRAPHICS);
    for (j=0; j<(256>>1); j++) // set the remaining bytes of a 32x32 sprites to be transparent
        SPRITE_GFX_SUB[(offset>>1) + j] = 0;
    offset += 256;
    
    base_power_slider = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "power_slider", FS_GUI_GRAPHICS);
    
    base_infoscreen_bar = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "infoscreen_bar", FS_GUI_GRAPHICS);
    
    if (getGameState() == MENU_INGAME || getGameState() == INGAME) // pause menu and ingame briefing only overwrite +- 10kb of sprite space
        return;
    
    base_emptyslot_icons = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "EmptySlot_Structure", FS_ICONS_GRAPHICS);
    for (j=0; j<(256>>1); j++) // set the remaining bytes of a 32x32 sprites to be transparent
        SPRITE_GFX_SUB[(offset>>1) + j] = 0;
    offset += 256;
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "EmptySlot_Unit", FS_ICONS_GRAPHICS);
    for (j=0; j<(256>>1); j++) // set the remaining bytes of a 32x32 sprites to be transparent
        SPRITE_GFX_SUB[(offset>>1) + j] = 0;
    offset += 256;
    
    base_structure_icons = offset/(16*16);
    for (i=0; i<MAX_DIFFERENT_STRUCTURES && structureInfo[i].enabled; i++) {
        offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), structureInfo[i].name, FS_ICONS_GRAPHICS);
        for (j=0; j<(256>>1); j++) // set the remaining bytes of a 32x32 sprites to be transparent
            SPRITE_GFX_SUB[(offset>>1) + j] = 0;
        offset += 256;
    }
    
    base_unit_icons = offset/(16*16);
    for (i=0; i<MAX_DIFFERENT_UNITS && unitInfo[i].enabled; i++) {
        offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), unitInfo[i].name, FS_ICONS_GRAPHICS);
        for (j=0; j<(256>>1); j++) // set the remaining bytes of a 32x32 sprites to be transparent
            SPRITE_GFX_SUB[(offset>>1) + j] = 0;
        offset += 256;
    }
    
    base_size_icons = offset/(16*16);
    for (i=1; i<=4; i++) { // 4 being max width of a structure
        for (j=1; j<=3; j++) { // 3 being max height of a structure
			base_size_icon[(j-1)*4+(i-1)] = offset/(16*16);
            sprintf(filename, "Size%ix%i", i, j);
            offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), filename, FS_ICONS_GRAPHICS);
            for (k=0; k<(256>>1); k++) // set the remaining bytes of a 32x32 sprites to be transparent
                SPRITE_GFX_SUB[(offset>>1) + k] = 0;
            offset += 256;
        }
    }
    
    base_sidebars_animation = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "sidebars_animation", FS_GUI_GRAPHICS);
    
    base_minplus = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "minplus", FS_GUI_GRAPHICS);
    
    base_buildqueue_flash = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "buildqueue_flash", FS_GUI_GRAPHICS);
    
    base_queue_words = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "queue_paused", FS_GUI_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "queue_done", FS_GUI_GRAPHICS);
    
    base_gameinfo_button = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "gameinfo_button", FS_GUI_GRAPHICS);
    
    // copying font_small somewhat differently to speed conversion to framebuffer up a bit
    base_font_small = offset/(16*16);
    /*offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "font_small", FS_GUI_GRAPHICS);*/
    copyFile(fb_font_small + 48 * 8*8/2, "font_small", FS_GUI_GRAPHICS);
    for (i=0; i<48; i++) {
        for (j=0; j<8*8/2; j++) {
            (SPRITE_GFX_SUB + (offset>>1))[i*16*16/2 + j] = (fb_font_small + 48 * 8*8/2)[i*8*8/2 + j];
        }
    }
    offset += 48 * 16*16;
    
    base_hotkey = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "hotkey", FS_GUI_GRAPHICS);
    
    base_select_graphics = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "icon_select", FS_GUI_GRAPHICS);
    for (j=0; j<(256>>1); j++) // set the remaining bytes of a 32x32 sprites to be transparent
        SPRITE_GFX_SUB[(offset>>1) + j] = 0;
    offset += 256;
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "modekeys_select",        FS_GUI_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "arrows_select",          FS_GUI_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "minplus_select",         FS_GUI_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "hotkey_select",          FS_GUI_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "swapscreens_select",     FS_GUI_GRAPHICS);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "gameinfo_button_select", FS_GUI_GRAPHICS);
    
    base_select_all = offset/(16*16);
    offset += copyFileVRAM(SPRITE_GFX_SUB + (offset>>1), "select_all_button", FS_GUI_GRAPHICS);
    
    if (offset > 128 * 1024)
        errorSI("Sprites for infoscreen exceeding VRAM limit,\nmeasured in bytes:", offset - 128 * 1024);
    
    src = fb_font_small + 48 * 8*8/2;
    dest = fb_font_small;
    for (i=0; i<48*8*8/2; i++) {
        j = (*src) & 0xFF;
        *dest = (j == 0) ? 0 : BIT(15) | SPRITE_PALETTE_SUB[j];
        dest++;
        j = (*src) >> 8;
        *dest = (j == 0) ? 0 : BIT(15) | SPRITE_PALETTE_SUB[j];
        dest++;
        src++;
    }
}


int getPositionOfItemInBuildablesBar(int isStructure, int info) {
    int i = 0;
    
    if (isStructure) {
        while (itemsBarBuildables[i].info != -1 && i < amountOfStructuresInBarBuildables) {
            if (itemsBarBuildables[i].info == info)
                return i;
            i++;
        }
    } else {
        i = amountOfStructuresInBarBuildables;
        while (itemsBarBuildables[i].info != -1) {
            if (itemsBarBuildables[i].info == info)
                return i;
            i++;
        }
    }
    return -1;
}


int canStructureBuildAnItem(int structure_info) { // prerequisite: structure_info needs to be an existing FRIENDLY structure
    int i,j,k;

    for (i=0; i<getTechtree(FRIENDLY)->amountOfItems; i++) {
        if (getTechtree(FRIENDLY)->item[i].buildingInfo == structure_info) {
            
            for (j=0; j<MAX_REQUIREDINFO_COUNT && getTechtree(FRIENDLY)->item[i].requiredInfo[j] != -1; j++) {
                for (k=0; k<MAX_STRUCTURES_ON_MAP; k++) {
                    if (structure[k].info == getTechtree(FRIENDLY)->item[i].requiredInfo[j] && structure[k].enabled && structure[k].side == FRIENDLY)
                        break; // the structure may yet be added to the bar, as the other required structures exist so far
                    if (k == MAX_STRUCTURES_ON_MAP)
                        break; // the structure may not be added to the bar (according to the current rule)
                }
                if (j == MAX_REQUIREDINFO_COUNT || getTechtree(FRIENDLY)->item[i].requiredInfo[j] == -1) // the structure can build an item, can be added to the bar now
                    return 1;
            }
            
        }
    }
    
    return 0;
}


void createBarStructures() {
    int amountOfItems = 0;
    int i, j, k, l;
    int insertAt;
    struct BarItem bItem, bItemTemp;
    
    for (i=0; i<MAX_ITEMS_BAR_STRUCTURES && itemsBarStructures[i].info != -1; i++) {
        for (j=0; j<MAX_STRUCTURES_ON_MAP; j++) {
            if (structure[j].enabled && structure[j].side == FRIENDLY && structure[j].info == itemsBarStructures[i].info) {
                if ((itemsBarStructures[i].animation & IBAT_MASK) == IBAT_REMOVE && canStructureBuildAnItem(itemsBarStructures[i].info))
                    changeBarItemAnimation(&itemsBarStructures[i], IBAT_ADD);
                break;
            }
        }
        if (j == MAX_STRUCTURES_ON_MAP)
            changeBarItemAnimation(&itemsBarStructures[i], IBAT_REMOVE);
    }
    
    for (i=0; i<MAX_STRUCTURES_ON_MAP; i++) {
        if (structure[i].enabled && structure[i].side == FRIENDLY) {
            if (getPositionOfItemInStructuresBar(structure[i].info) == -1) { // if that structure is not already in the bar
                insertAt = 0;
                for (j=0; j<getTechtree(FRIENDLY)->amountOfItems; j++) {
                    if (getTechtree(FRIENDLY)->item[j].buildingInfo != structure[i].info) {
                        if (getTechtree(FRIENDLY)->item[j].buildingInfo == itemsBarStructures[insertAt].info)
                            insertAt++;
                    } else {
                    
                        for (k=0; k<MAX_REQUIREDINFO_COUNT && getTechtree(FRIENDLY)->item[j].requiredInfo[k] != -1; k++) {
                            for (l=0; l<MAX_STRUCTURES_ON_MAP; l++) {
                                if (structure[l].info == getTechtree(FRIENDLY)->item[j].requiredInfo[k] && structure[l].enabled && structure[l].side == FRIENDLY)
                                    break; // the structure may yet be added to the bar, as the other required structures exist so far
                            }
                            if (l == MAX_STRUCTURES_ON_MAP)
                                break; // the structure may not be added to the bar
                        }
                        if (k == MAX_REQUIREDINFO_COUNT || getTechtree(FRIENDLY)->item[j].requiredInfo[k] == -1) { // the structure can build an item, can be added to the bar now
                    
    //                        if (insertAt < MAX_ITEMS_BAR_STRUCTURES) {
                                bItem = createBarItem(structure[i].info, IBAT_ADD);
                                for (j=insertAt; j<MAX_ITEMS_BAR_STRUCTURES && itemsBarStructures[j].info != -1; j++) {
                                    bItemTemp = itemsBarStructures[j]; // tempinfo is what needs shoving up
                                    itemsBarStructures[j] = bItem;
                                    bItem = bItemTemp;
                                }
                                itemsBarStructures[j]=bItem;
                                amountOfItems++;
    //                        }
                            if (infoScreenItemBarStructuresFirstShown == -1)
                                infoScreenItemBarStructuresFirstShown = 0;
                            break;
                    
                        }
                    }
                }
            }
        }
    }
    
    // check if the build info screen should switch to radar
    if (infoScreenType == IS_BUILDINFO) {
        k = 0; // switch to radar?
        if (getPositionOfItemInStructuresBar(infoScreenBuildInfoBuilderNr) == -1)
            k = 1; // switch to radar
        else
            k = (availableBuilderForItem(FRIENDLY, infoScreenBuildInfoIsStructure, infoScreenBuildInfoNr) == -1); // switch to radar if it can't be built
        
        if (k) { // switching to radar now
            infoScreenType = IS_RADAR;
            REG_BG0HOFS_SUB = 0;
            REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE; // activate radar overlay
            REG_DISPCNT_SUB |= DISPLAY_BG1_ACTIVE; // activate covering up background behind radar
            REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // temporarily deactivate radar background
            drawRadarBG();
//            REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE; // activate radar background
            setAllDirtyRadarDirtyBitmap();
            REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_SRC_BG2;
        }
    }
    
    if (infoScreenItemBarBuildablesFirstShown != -1) {
        if (getPositionOfItemInStructuresBar(infoScreenItemBarBuildablesStructureNr) == -1)
            infoScreenItemBarBuildablesStructureNr = -1;
        createBarBuildables(infoScreenItemBarBuildablesStructureNr);
    }
    if (infoScreenItemBarStructuresFirstShown > 0 && infoScreenItemBarStructuresFirstShown + 7 > amountOfItems)
        infoScreenItemBarStructuresFirstShown = amountOfItems - 7;
}


void setInfoScreenBuildInfo(int isStructure, int info) {
    if (infoScreenType != IS_BUILDINFO || infoScreenBuildInfoIsStructure != isStructure || infoScreenBuildInfoNr != info) {
        infoScreenBuildInfoBuilderNr = availableBuilderForItem(FRIENDLY, isStructure, info);
        
        infoScreenBuildInfoCanBuild = (infoScreenBuildInfoBuilderNr >= 0);
        
        infoScreenBuildInfoIsStructure = isStructure;
        infoScreenBuildInfoNr = info;
        infoScreenType = IS_BUILDINFO;
        REG_BG0HOFS_SUB = -(4); //  x position slightly shifted
        infoScreenMinplusAnimationTimer = 0;
        infoScreenPlaceIconAnimationTimer = 0;
        REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE;    // enable background for queue percentages
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // temporarily disable the radar background until it is rewritten here to be used as information text display
        REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
        drawInformationBG( (infoScreenBuildInfoIsStructure) ? 
                            structureInfo[infoScreenBuildInfoNr].description : 
                            unitInfo[infoScreenBuildInfoNr].description );
//                REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE; // activate radar background, used now for text-display
        REG_BLDCNT_SUB = BLEND_NONE;
    }
}


void initInfoScreen() {
    char oneline[256];
    FILE *fp;
    int i, j;
    
    infoScreenStructureIconAnimationTimer = 0;
    infoScreenArrowsAnimationTimer = 0;
    infoScreenMinplusAnimationTimer = 0;
    infoScreenPlaceIconAnimationTimer = 0;
    infoScreenBuildQueueAnimationTimer = 0;
    infoScreenItemBarBuildablesStructureNr = -1;
    
    initRadar();
    infoScreenType = IS_RADAR;
    REG_BG0HOFS_SUB = 0;
    infoScreenTouchedTimer = 0;
    infoScreenItemBarStructuresFirstShown = -1;
    infoScreenItemBarBuildablesFirstShown = -1;
    for (i=0; i<MAX_ITEMS_BAR_STRUCTURES+1; i++)
        itemsBarStructures[i] = createBarItem(-1, IBAT_NONE);
    for (i=0; i<MAX_ITEMS_BAR_BUILDABLES+1; i++)
        itemsBarBuildables[i] = createBarItem(-1, IBAT_NONE);

    for (i=0; i<getAmountOfSides(); i++) {
        initStructuresQueue(i);
        initUnitsQueue(i);
    }
    
    // going to read quota and credits from the scenario
    fp = openFile("", FS_CURRENT_SCENARIO_FILE);
    
    // FRIENDLY section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[FRIENDLY]", strlen("[FRIENDLY]")));
    readstr(fp, oneline); // skip Faction=
    readstr(fp, oneline); // skip TechLevel=
    readstr(fp, oneline);
    sscanf(oneline, "Credits=%i", &j);
//    if (getGameType() == MULTIPLAYER_CLIENT)
//        initCredits(ENEMY1, j);
//    else
        initCredits(FRIENDLY, j);
    readstr(fp, oneline);
    sscanf(oneline, "MaxUnit=%i", &j);
    setUnitLimit(FRIENDLY, j);
    
    // ENEMY section
    do {
        readstr(fp, oneline);
    } while (strncmp(oneline, "[ENEMY", strlen("[ENEMY")));
    for (i=1; i<MAX_DIFFERENT_FACTIONS; i++) {
        readstr(fp, oneline); // skip Faction=
        readstr(fp, oneline); // skip TechLevel=
        readstr(fp, oneline);
        if (oneline[0] == 'R') // skip RepairSpeed=
            readstr(fp, oneline);
        sscanf(oneline, "Credits=%i", &j);
//        if (getGameType() == MULTIPLAYER_CLIENT)
//            initCredits(FRIENDLY, j);
//        else
            initCredits(i, j);
        readstr(fp, oneline);
        sscanf(oneline, "MaxUnit=%i", &j);
        setUnitLimit(i, j);

        do {
            readstr(fp, oneline);
        } while (oneline[0] != '[');
        if (strncmp(oneline, "[ENEMY", strlen("[ENEMY")))
            break;
    }
    
    closeFile(fp);
    
    for (i=0; i<getAmountOfSides(); i++)
        initTechtree(i);
    
    createBarStructures();
    if (itemsBarStructures[0].info != -1) {
        infoScreenItemBarBuildablesFirstShown = -1;
        createBarBuildables(itemsBarStructures[0].info);
        
        int maxItemsInBar = 7;
        if (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + maxItemsInBar].info != -1) {
            while (itemsBarBuildables[infoScreenItemBarBuildablesFirstShown + maxItemsInBar].info != -1) {
                infoScreenItemBarBuildablesFirstShown++;
            }
        }
    }
    
    initMatchTime();
}


void drawInfoScreenBG() {
    int i, j;
    
    REG_DISPCNT_SUB = (MODE_5_2D |
                      DISPLAY_BG0_ACTIVE |    /* bg for radar overlay (static noise) and building queue percentage showing */
                      DISPLAY_BG1_ACTIVE |    /* bg for covering up main bg behind radar */
                      DISPLAY_BG2_ACTIVE |    /* bg for radar and informational text */
                      DISPLAY_BG3_ACTIVE |    /* bg for background */
                      DISPLAY_SPR_ACTIVE |    /* icons, buttons, and some text */
                      DISPLAY_SPR_1D |
//                      DISPLAY_SPR_1D_SIZE_64 // getting sprite references to be 64 byte (8x8 8-bit tiles)
                      DISPLAY_SPR_1D_SIZE_256 // getting sprite references to be 256 bytes (16x16 8-bit tiles)
                     );

    REG_BG0CNT_SUB = BG_PRIORITY_0 | BG_TILE_BASE(2) | BG_COLOR_256 | BG_MAP_BASE(31) | BG_32x32;
    REG_BG1CNT_SUB = BG_PRIORITY_2 | BG_TILE_BASE(2) | BG_COLOR_256 | BG_MAP_BASE(30) | BG_32x32;
    REG_BG2CNT_SUB = BG_PRIORITY_1 | BG_BMP_BASE(0) | BG_BMP16_128x128; // 256x256 is not needed for radar; just stretch this one to fit.
    REG_BG3CNT_SUB = BG_PRIORITY_3 | BG_BMP_BASE(5) | BG_BMP8_256x256;  // of which only 256x192 is actually used
    REG_BG2PA_SUB = 1<<8; // scaling of 1
    REG_BG2PB_SUB = 0;
    REG_BG2PC_SUB = 0;
    REG_BG2PD_SUB = 1<<8; // scaling of 1
    REG_BG2X_SUB  = 0;
    REG_BG2Y_SUB  = 0 - (getScreenScrollingInfoScreen() * 256);
    REG_BG2HOFS_SUB = 0;
    REG_BG2VOFS_SUB = 0;
    REG_BG3PA_SUB = 1<<8; // scaling of 1
    REG_BG3PB_SUB = 0;
    REG_BG3PC_SUB = 0;
    REG_BG3PD_SUB = 1<<8; // scaling of 1
    REG_BG3X_SUB  = 0;
    REG_BG3Y_SUB  = 0 - (getScreenScrollingInfoScreen() * 256);
    REG_BG3HOFS_SUB = 0;
    REG_BG3VOFS_SUB = 0;
    
    //  0- 32 kb used.   by radar.
    // 32- .. kb used.   by various bg graphics. (including bg1)
    // 60- 64 kb used.   by tile maps.
    // 80-128 kb used.   by bg3. (main background)

    if (infoScreenType == IS_BUILDINFO) {
        REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE;    // enable background for queue percentages
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // temporarily disable the radar background until it is rewritten here to be used as information text display
        REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
        drawInformationBG( (infoScreenBuildInfoIsStructure) ? 
                            structureInfo[infoScreenBuildInfoNr].description : 
                            unitInfo[infoScreenBuildInfoNr].description );
//        REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE; // activate radar background, used now for text-display
        REG_BLDCNT_SUB = BLEND_NONE;
    } else if (infoScreenType == IS_BUILDQUEUE) {
        REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE;    // activate queue percentages background
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // disable radar background (might already be disabled)
        REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
        REG_BLDCNT_SUB = BLEND_NONE;
    } else if (infoScreenType == IS_HOTKEYS) {
        REG_DISPCNT_SUB &= (~DISPLAY_BG0_ACTIVE); // disable queue percentages background or radar overlay
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // disable radar background (might already be disabled)
        REG_DISPCNT_SUB &= (~DISPLAY_BG1_ACTIVE); // disable covering up background behind radar
        REG_BLDCNT_SUB = BLEND_NONE;
    } else {
        REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG0 | BLEND_DST_BG2;
        REG_DISPCNT_SUB &= (~DISPLAY_BG2_ACTIVE); // temporarily disable the radar background
        drawRadarBG();
//        REG_DISPCNT_SUB |= DISPLAY_BG2_ACTIVE; // activate radar background
    }
    
    for (i=0; i<32*24; i++)
        mapSubBG0[i] = 0;
    
    for (i=0; i<RADAR_MAX_SIZE/8; i++) {
        for (j=0; j<(SCREEN_WIDTH - RADAR_MAX_SIZE)/(2*8); j++)
            mapSubBG1[i * 32 + j] = 0;
        for (; j<32-((SCREEN_WIDTH - RADAR_MAX_SIZE)/(2*8)); j++)
            mapSubBG1[i * 32 + j] = 1;
        for (; j<32; j++)
            mapSubBG1[i * 32 + j] = 0;
    }
    i *= 32;
    for (; i<32*24; i++)
        mapSubBG1[i] = 0;
}
