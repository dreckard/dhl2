//=============================================================================//
// Purpose:	Distraction Half-Life 2 shared definitions
//
// Author: Skillet
//=============================================================================/
#ifndef DHL_SHAREDDEFS_H
#define DHL_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

//Default Models
#define DHL_DEFAULTMODEL_MOB "models/player/dhl/mobsters/yakuza.mdl"
#define DHL_DEFAULTMODEL_PRO "models/player/dhl/professionals/swat.mdl"
#define DHL_DEFAULTMODEL DHL_DEFAULTMODEL_PRO

//Movement speeds
#define DHL_SPEED_SCALAR 1.25f
#define DHL_PRONESPEED_SCALAR 0.25f
#define DHL_WALK_SPEED 150.0f * DHL_SPEED_SCALAR
#define DHL_NORM_SPEED 190.0f * DHL_SPEED_SCALAR
#define DHL_SPRINT_SPEED 320.0f * DHL_SPEED_SCALAR

//Stunt Status
#define STUNT_NONE 0
#define STUNT_PRONE 1
#define STUNT_DIVE 2
#define STUNT_JUMPED 3
#define STUNT_CLIMBED 4

//Stunt Directions
#define STUNTDIR_FORWARDS 0
#define STUNTDIR_LEFT 1
#define STUNTDIR_BACKWARDS 2
#define STUNTDIR_RIGHT 3

//Bullet types
#define DHL_PROJECTILE_TYPE_BULLET  0 //A regular bullet
#define DHL_PROJECTILE_TYPE_PELLET  1 //Shotgun pellet
#define DHL_PROJECTILE_TYPE_COMBATKNIFE 2

//Weapon indices (for networking)
#define WEAPON_FIRST_MELEE 1
#define WEAPON_BASEBALLBAT 1
#define WEAPON_COMBATKNIFE 2
#define WEAPON_KATANA 3
#define WEAPON_LAST_MELEE 3

#define WEAPON_FIRST_HANDGUN 4
#define WEAPON_BERETTA 4
#define WEAPON_BERETTA_AKIMBO 5
#define WEAPON_DEAGLE 6
#define WEAPON_SAA 7
#define WEAPON_DERINGER 8
#define WEAPON_LAST_HANDGUN 8

#define WEAPON_FIRST_UNIQUE 9
#define WEAPON_AK47 9
#define WEAPON_MAC11 10
#define WEAPON_THOMPSON 11
#define WEAPON_REMINGTON 12
#define WEAPON_SAWEDOFF 13
#define WEAPON_MOSINNAGANT 14
#define WEAPON_WINCHESTER 15
#define WEAPON_LAST_UNIQUE 15

#define WEAPON_GRENADE 16

//Item inventory bit flags and networking incices
#define FIRST_ITEM 17
#define DHL_IFLAG_KEVLAR (1<<0)
#define ITEM_KEVLAR 17
#define DHL_IFLAG_NIGHTVISION (1<<1)
#define ITEM_NIGHTVISION 18
#define DHL_IFLAG_FLASHLIGHT (1<<2)
#define ITEM_FLASHLIGHT 19
#define DHL_IFLAG_COUNT 3
#define LAST_ITEM 19

//DHL_Player entity message IDs
#define DHL_PLAYER_RESPAWN 0
#define DHL_PLAYER_DEATH 1

//Inventory values for each type of selectable item
#define DHL_INV_VAL_HANDGUN 4
#define DHL_INV_VAL_UNIQUE 6
#define DHL_INV_VAL_ITEM 3
#define DHL_INV_VAL_SELMAX 10
#define DHL_INV_VAL_ABSMAX 16

//Style Points
#define DHL_MAXSTYLEPOINTS 20
#define DHL_SLOWMOSTYLEPOINTS 20

//Used for ImpulseCommands, starts at 245 to avoid interference with HL2 impulses but stay under byte limit as well
#define DHL_IMPULSE_START 245

#endif //DHL_SHAREDDEFS_H