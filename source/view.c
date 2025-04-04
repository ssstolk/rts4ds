// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#include "view.h"

#include "environment.h"
#include "shared.h"

#define SCREEN_SWAP_ANIMATION_DURATION  (8 / (SCREEN_REFRESH_RATE / FPS))  /*(FPS/6)*/
#define SCREEN_SCROLL_PIXELS_PART1      ( SCREEN_HEIGHT / 3 )
#define SCREEN_SCROLL_PIXELS_PART2      ( SCREEN_HEIGHT / 3 )



enum View view_View;
enum ViewMovement view_ViewMovement;
enum ScreenSetUp view_ScreenSetUp;
int view_X;
int view_Y;

int    view_ScreenSwapped;
int    view_ScreenScrollingTimer;
int    view_ScreenScrolling; 
uint16 view_ScreenScrollingShort; 




void initView(enum View view, int x, int y) {
    view_View = view;
    view_ViewMovement = VM_OTHER;
    view_ScreenSetUp = MAIN_TOUCH;
    view_X = x;
    view_Y = y;
}

void setView(enum View view) {
    if (view == view_View) { view_ViewMovement = VM_NONE; return; }

    // load in a different set of tiles. here?
    
    // adjust x and y values properly
    if (view_View == HORIZONTAL && view == HORIZONTAL_ZO) { // switching between both horizontal modes
        view_X -= ((HORIZONTAL_ZO_WIDTH - HORIZONTAL_WIDTH) / 2);
        if (view_X < 0) view_X = 0;
        view_Y -= ((HORIZONTAL_ZO_HEIGHT - HORIZONTAL_HEIGHT) / 2);
        if (view_Y < 0) view_Y = 0;
    } else if (view_View == HORIZONTAL_ZO && view == HORIZONTAL) {
        view_X += ((HORIZONTAL_ZO_WIDTH - HORIZONTAL_WIDTH) / 2);
        if (view_X + HORIZONTAL_WIDTH > environment.width) view_X = environment.width - HORIZONTAL_WIDTH;
        view_Y += ((HORIZONTAL_ZO_HEIGHT - HORIZONTAL_HEIGHT) / 2);
        if (view_Y + HORIZONTAL_HEIGHT > environment.height) view_Y = environment.height - HORIZONTAL_HEIGHT;
    } else if ((view_View == VERTICAL_RIGHT && view == VERTICAL_RIGHT_ZO) || (view_View == VERTICAL_LEFT && view == VERTICAL_LEFT_ZO)) { // switching between both vertical modes
        view_X -= ((VERTICAL_ZO_WIDTH - VERTICAL_WIDTH) / 2);
        if (view_X < 0) view_X = 0;
        view_Y -= ((VERTICAL_ZO_HEIGHT - VERTICAL_HEIGHT) / 2);
        if (view_Y < 0) view_Y = 0;
    } else if ((view_View == VERTICAL_RIGHT_ZO && view == VERTICAL_RIGHT) && (view_View == VERTICAL_LEFT_ZO && view == VERTICAL_LEFT)) {
        view_X += ((VERTICAL_ZO_WIDTH - VERTICAL_WIDTH) / 2);
        if (view_X + VERTICAL_WIDTH > environment.width) view_X = environment.width - VERTICAL_WIDTH;
        view_Y += ((VERTICAL_ZO_HEIGHT - VERTICAL_HEIGHT) / 2);
        if (view_Y + VERTICAL_HEIGHT > environment.height) view_Y = environment.height - VERTICAL_HEIGHT;
    } else if (view_View == HORIZONTAL && (view == VERTICAL_RIGHT || view == VERTICAL_LEFT)) { // switching between both regular modes
        view_X += ((HORIZONTAL_WIDTH - VERTICAL_WIDTH) / 2);
        view_Y -= ((VERTICAL_HEIGHT - HORIZONTAL_HEIGHT) / 2);
        if (view_Y < 0) view_Y = 0;
        else if (view_Y + VERTICAL_HEIGHT > environment.height) view_Y = environment.height - VERTICAL_HEIGHT;
    } else if ((view_View == VERTICAL_RIGHT || view_View == VERTICAL_LEFT) && view == HORIZONTAL) {
        view_X -= ((HORIZONTAL_WIDTH - VERTICAL_WIDTH) / 2);
        if (view_X < 0) view_X = 0;
        else if (view_X + HORIZONTAL_WIDTH > environment.width) view_X = environment.width - HORIZONTAL_WIDTH;
        view_Y += ((VERTICAL_HEIGHT - HORIZONTAL_HEIGHT) / 2);
    } else if (view_View == HORIZONTAL_ZO && (view == VERTICAL_RIGHT_ZO || view == VERTICAL_LEFT_ZO)) { // switching between both zoomed out modes
        view_X += ((HORIZONTAL_ZO_WIDTH - VERTICAL_ZO_WIDTH) / 2);
        view_Y -= ((VERTICAL_ZO_HEIGHT - HORIZONTAL_ZO_HEIGHT) / 2);
        if (view_Y < 0) view_Y = 0;
        else if (view_Y + VERTICAL_ZO_HEIGHT > environment.height) view_Y = environment.height - VERTICAL_ZO_HEIGHT;
    } else if ((view_View == VERTICAL_RIGHT_ZO || view_View == VERTICAL_LEFT_ZO) && view == HORIZONTAL_ZO) {
        view_X -= ((HORIZONTAL_ZO_WIDTH - VERTICAL_ZO_WIDTH) / 2);
        if (view_X < 0) view_X = 0;
        else if (view_X + HORIZONTAL_ZO_WIDTH > environment.width) view_X = environment.width - HORIZONTAL_ZO_WIDTH;
        view_Y += ((VERTICAL_ZO_HEIGHT - HORIZONTAL_ZO_HEIGHT) / 2);
    }
    view_View = view;
    view_ViewMovement = VM_OTHER;
}

enum View getView() { return view_View; }



inline int getScreenScrollingPlayScreen() {
    return -view_ScreenScrolling;
}

inline int getScreenScrollingInfoScreen() {
    return view_ScreenScrolling;
}

inline uint16 getScreenScrollingPlayScreenShort() {
    return -view_ScreenScrollingShort;
}

inline uint16 getScreenScrollingInfoScreenShort() {
    return view_ScreenScrollingShort;
}

#define SCREEN_SWAP_ON_TIMER  ((SCREEN_SWAP_ANIMATION_DURATION * SCREEN_SCROLL_PIXELS_PART1) / (SCREEN_SCROLL_PIXELS_PART1 + SCREEN_SCROLL_PIXELS_PART2))
void doScreenSetUpLogic() {
    if (view_ScreenSetUp != MAIN_TO_SUB_TOUCH && view_ScreenSetUp != SUB_TO_MAIN_TOUCH) // no scrolling required
        return;
    
    view_ScreenScrollingTimer++;

    if (view_ScreenScrollingTimer == SCREEN_SWAP_ON_TIMER) { // swapping screens here
        lcdSwap();
        view_ScreenSwapped = 1;
    } else if (!view_ScreenSwapped) { // screens have not swapped here yet
        view_ScreenScrolling = (view_ScreenScrollingTimer * SCREEN_SCROLL_PIXELS_PART1) / SCREEN_SWAP_ON_TIMER;
        if (view_ScreenSetUp == SUB_TO_MAIN_TOUCH)
            view_ScreenScrolling = -view_ScreenScrolling;
        view_ScreenScrollingShort = (short) view_ScreenScrolling;
        return;
    }
    
    // screens have swapped (either this exact frame or sometime before)
    if (view_ScreenScrollingTimer >= SCREEN_SWAP_ANIMATION_DURATION) {
        if (view_ScreenSetUp == MAIN_TO_SUB_TOUCH)
            setScreenSetUp(SUB_TOUCH);
        else
            setScreenSetUp(MAIN_TOUCH);
        return;
    }
    
    view_ScreenScrolling = SCREEN_SCROLL_PIXELS_PART2 - (((view_ScreenScrollingTimer - SCREEN_SWAP_ON_TIMER) * SCREEN_SCROLL_PIXELS_PART2) / (SCREEN_SWAP_ANIMATION_DURATION - SCREEN_SWAP_ON_TIMER));
    if (view_ScreenSetUp == MAIN_TO_SUB_TOUCH)
        view_ScreenScrolling = -view_ScreenScrolling;
    view_ScreenScrollingShort = (short) view_ScreenScrolling;
}

void setScreenSetUp(enum ScreenSetUp setUp) {
    if (view_ScreenSetUp == setUp)
        return;
    
    if ((view_ScreenSetUp == MAIN_TO_SUB_TOUCH && setUp == SUB_TO_MAIN_TOUCH) ||
        (view_ScreenSetUp == SUB_TO_MAIN_TOUCH && setUp == MAIN_TO_SUB_TOUCH)) {
        view_ScreenSwapped = !view_ScreenSwapped;
        view_ScreenScrollingTimer = SCREEN_SWAP_ANIMATION_DURATION - view_ScreenScrollingTimer; // in frames
        view_ScreenScrolling      = -view_ScreenScrolling;        // in pixels
        view_ScreenScrollingShort = (short) view_ScreenScrolling; // in pixels
        
        view_ScreenSetUp = setUp;
        return;
    }
    
    if ((view_ScreenSetUp == MAIN_TO_SUB_TOUCH && setUp == SUB_TOUCH  && !view_ScreenSwapped) ||
        (view_ScreenSetUp == SUB_TO_MAIN_TOUCH && setUp == MAIN_TOUCH && !view_ScreenSwapped) ||
        (view_ScreenSetUp == MAIN_TOUCH && setUp == SUB_TOUCH) || 
        (view_ScreenSetUp == SUB_TOUCH  && setUp == MAIN_TOUCH))
        lcdSwap();
    
    view_ScreenSwapped = 0;
    view_ScreenScrollingTimer = 0; // in frames
    view_ScreenScrolling      = 0; // in pixels
    view_ScreenScrollingShort = 0; // in pixels
    
    view_ScreenSetUp = setUp;
}

enum ScreenSetUp getScreenSetUp() {
    return view_ScreenSetUp;
}



void setViewCurrentXY(int x, int y) {
    if (x < 0) x = 0;
    else if (view_View == HORIZONTAL && x + HORIZONTAL_WIDTH > environment.width) x = environment.width - HORIZONTAL_WIDTH;
    else if (view_View == HORIZONTAL_ZO && x + HORIZONTAL_ZO_WIDTH > environment.width) x = environment.width - HORIZONTAL_ZO_WIDTH;
    else if ((view_View == VERTICAL_RIGHT || view_View == VERTICAL_LEFT) && x + VERTICAL_WIDTH > environment.width) x = environment.width - VERTICAL_WIDTH;
    else if ((view_View == VERTICAL_RIGHT_ZO || view_View == VERTICAL_LEFT_ZO) && x + VERTICAL_ZO_WIDTH > environment.width) x = environment.width - VERTICAL_ZO_WIDTH;
    if (y < 0) y = 0;
    else if (view_View == HORIZONTAL && y + HORIZONTAL_HEIGHT > environment.height) y = environment.height - HORIZONTAL_HEIGHT;
    else if (view_View == HORIZONTAL_ZO && y + HORIZONTAL_ZO_HEIGHT > environment.height) y = environment.height - HORIZONTAL_ZO_HEIGHT;
    else if ((view_View == VERTICAL_RIGHT || view_View == VERTICAL_LEFT) && y + VERTICAL_HEIGHT > environment.height) y = environment.height - VERTICAL_HEIGHT;
    else if ((view_View == VERTICAL_RIGHT_ZO || view_View == VERTICAL_LEFT_ZO) && y + VERTICAL_ZO_HEIGHT > environment.height) y = environment.height - VERTICAL_ZO_HEIGHT;

    if (x == view_X) {
        if (y == view_Y) view_ViewMovement = VM_NONE;
        else if (y == view_Y + 1) view_ViewMovement = VM_DOWN;
        else if (y == view_Y - 1) view_ViewMovement = VM_UP;
        else view_ViewMovement = VM_OTHER;
    } else if (y == view_Y) {
        if (x == view_X + 1) view_ViewMovement = VM_RIGHT;
        else if (x == view_X - 1) view_ViewMovement = VM_LEFT;
        else view_ViewMovement = VM_OTHER;
    } else if (x == view_X - 1) {
        if (y == view_Y - 1) view_ViewMovement = VM_LEFT_UP;
        else if (y == view_Y + 1) view_ViewMovement = VM_LEFT_DOWN;
        else view_ViewMovement = VM_OTHER;
    } else if (x == view_X + 1) {
        if (y == view_Y - 1) view_ViewMovement = VM_RIGHT_UP;
        else if (y == view_Y + 1) view_ViewMovement = VM_RIGHT_DOWN;
        else view_ViewMovement = VM_OTHER;
    } else view_ViewMovement = VM_OTHER;
    
    view_X = x;
    view_Y = y;
}

int getViewCurrentX() { return view_X; }
int getViewCurrentY() { return view_Y; }


enum ViewMovement getViewMovement() { return view_ViewMovement; }
