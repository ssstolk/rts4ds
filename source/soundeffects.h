// SPDX-License-Identifier: MIT
// Copyright © 2007-2025 Sander Stolk

#ifndef _SOUNDEFFECTS_H_
#define _SOUNDEFFECTS_H_

#include "music.h"
#include "projectiles.h"

#define MAX_SOUNDEFFECT_CHANNELS   (16-MAX_MUSIC_CHANNELS)

#define MAX_SE_UNIT_DEATHS  10

#define DISTANCE_TO_HALVE_SE_PLACE_STRUCTURE         15
#define DISTANCE_TO_HALVE_SE_IMPACT_SAND             15
#define DISTANCE_TO_HALVE_SE_EXPLOSION_STRUCTURE     45
#define DISTANCE_TO_HALVE_SE_EXPLOSION_UNIT_WHEELED  30 /* the actual sounds used are SE_UNIT_DEATHs */
#define DISTANCE_TO_HALVE_SE_EXPLOSION_UNIT_TRACKED  30 /* the actual sounds used are SE_UNIT_DEATHs */
#define DISTANCE_TO_HALVE_SE_FOOT_CRUSH              15
#define DISTANCE_TO_HALVE_SE_FOOT_SCREAM             15 /* the actual sounds used are SE_UNIT_DEATHs */
#define DISTANCE_TO_HALVE_SE_PROJECTILE              22

enum Soundeffect {  // the following are low priority, reserved channel #5-(MAX_SOUNDEFFECT_CHANNELS-1)
                    SE_PROJECTILE, SE_LAST_PROJECTILE = ((SE_PROJECTILE+2*MAX_DIFFERENT_PROJECTILES) - 1),
                    SE_CREDIT,
                    SE_IMPACT_SAND, SE_IMPACT_UNIT, SE_IMPACT_STRUCTURE, 
                    
                    // the following are medium priority, reserved channel #3-4
                    SE_EXPLOSION_STRUCTURE, /*SE_EXPLOSION_UNIT_WHEELED, SE_EXPLOSION_UNIT_TRACKED,*/
                    SE_FOOT_CRUSH, /*SE_FOOT_SCREAM,*/ SE_FOOT_HURT,
                    SE_UNIT_DEATH, SE_LAST_UNIT_DEATH = ((SE_UNIT_DEATH+MAX_SE_UNIT_DEATHS) - 1),
                    SE_MINING, SE_MINER_FULL,
                    
                    // the following are high priority, reserved channel #1-2
                    SE_POWER_ON, SE_POWER_OFF, SE_CREDITS_MAXED, SE_CREDITS_NONE,
                    
                    SE_BUTTONCLICK, SE_CANNOT, SE_PLACE_STRUCTURE,
                    SE_FOOT_SELECT, SE_FOOT_ATTACK, SE_FOOT_MOVE, 
                    SE_WHEELED_SELECT, SE_WHEELED_ATTACK, SE_WHEELED_MOVE,
                    SE_TRACKED_SELECT, SE_TRACKED_ATTACK, SE_TRACKED_MOVE,
                    SE_STRUCTURE_SELECT,
                    
                    SE_MENU_SELECT, SE_MENU_BACK, SE_MENU_REGION, SE_MENU_OK,
                    
                    SE_CONSTRUCTION_COMPLETE,
//                  SE_ENEMY_UNIT_DESTROYED,
/* not done */      SE_ENEMY_UNIT_APPROACHING,
/* not done */      SE_FRIENDLY_BASE_ATTACKED,
                    SE_MINER_DEPLOYED, SE_UNIT_DEPLOYED, SE_UNIT_REPAIRED,
                    
                    // the following are top priority, reserved channel #0
                    SE_MISC,
                    SE_GAMETIMER,
                    SE_OBJECTIVE,
                    
                    SE_COUNT
                  };


void stopSoundeffects();
void updateSEafterVBlank();
int playSoundeffectControlled(enum Soundeffect se, unsigned int volumePercentage, unsigned int sp);
int playSoundeffect(enum Soundeffect se);
int playMiscSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp);
int playMiscSoundeffect(char *name);
int playGameTimerSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp);
int playGameTimerSoundeffect(char *name);
int playObjectiveSoundeffectControlled(char *name, unsigned int volumePercentage, unsigned int sp);
int playObjectiveSoundeffect(char *name);
void requestContinueBufferingSoundeffect(char *name);

void initSoundeffectsWithScenario();
void initSoundeffects();

#endif
