// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _VIEW_H_
#define _VIEW_H_

#include <nds.h>


#define HORIZONTAL_WIDTH        16
#define HORIZONTAL_HEIGHT       11
#define HORIZONTAL_ZO_WIDTH     32
#define HORIZONTAL_ZO_HEIGHT    22
#define VERTICAL_WIDTH          12
#define VERTICAL_HEIGHT         15
#define VERTICAL_ZO_WIDTH       24
#define VERTICAL_ZO_HEIGHT      30
#define ACTION_BAR_SIZE          1




enum View { HORIZONTAL, VERTICAL_RIGHT, VERTICAL_LEFT, HORIZONTAL_ZO, VERTICAL_RIGHT_ZO, VERTICAL_LEFT_ZO };
enum ViewMovement { VM_NONE, VM_LEFT_UP, VM_RIGHT_UP, VM_LEFT_DOWN, VM_RIGHT_DOWN, VM_LEFT, VM_RIGHT, VM_UP, VM_DOWN, VM_OTHER };
enum ScreenSetUp { MAIN_TOUCH, MAIN_TO_SUB_TOUCH, SUB_TOUCH, SUB_TO_MAIN_TOUCH };

void initView(enum View view, int x, int y);

enum View getView();
void setView(enum View view);

int getScreenScrollingPlayScreen();
int getScreenScrollingInfoScreen();
uint16 getScreenScrollingPlayScreenShort();
uint16 getScreenScrollingInfoScreenShort();
void doScreenSetUpLogic();
enum ScreenSetUp getScreenSetUp();
void setScreenSetUp(enum ScreenSetUp setUp);

void setViewCurrentXY(int x, int y);
int getViewCurrentX();
int getViewCurrentY();

enum ViewMovement getViewMovement();

#endif
