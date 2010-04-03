/**
 * WiiSX - controller-Classic.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Classic controller input module
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
 *                emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#ifdef HW_RVL

#include <string.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include "controller.h"

#ifndef PI
#define PI 3.14159f
#endif

enum { STICK_X, STICK_Y };
static int getStickValue(joystick_t* j, int axis, int maxAbsValue){
	double angle = PI * j->ang/180.0f;
	double magnitude = (j->mag > 1.0f) ? 1.0f :
	                    (j->mag < -1.0f) ? -1.0f : j->mag;
	double value;
	if(axis == STICK_X)
		value = magnitude * sin( angle );
	else
		value = magnitude * cos( angle );
	return (int)(value * maxAbsValue);
}

enum {
	L_STICK_AS_ANALOG = 1, R_STICK_AS_ANALOG = 2,
};

enum {
	L_STICK_L = 0x01 << 16,
	L_STICK_R = 0x02 << 16,
	L_STICK_U = 0x04 << 16,
	L_STICK_D = 0x08 << 16,
	R_STICK_L = 0x10 << 16,
	R_STICK_R = 0x20 << 16,
	R_STICK_U = 0x40 << 16,
	R_STICK_D = 0x80 << 16,
};

static button_t buttons[] = {
	{  0, ~0,                         "None" },
	{  1, CLASSIC_CTRL_BUTTON_UP,     "D-Up" },
	{  2, CLASSIC_CTRL_BUTTON_LEFT,   "D-Left" },
	{  3, CLASSIC_CTRL_BUTTON_RIGHT,  "D-Right" },
	{  4, CLASSIC_CTRL_BUTTON_DOWN,   "D-Down" },
	{  5, CLASSIC_CTRL_BUTTON_FULL_L, "L" },
	{  6, CLASSIC_CTRL_BUTTON_FULL_R, "R" },
	{  7, CLASSIC_CTRL_BUTTON_ZL,     "Left Z" },
	{  8, CLASSIC_CTRL_BUTTON_ZR,     "Right Z" },
	{  9, CLASSIC_CTRL_BUTTON_A,      "A" },
	{ 10, CLASSIC_CTRL_BUTTON_B,      "B" },
	{ 11, CLASSIC_CTRL_BUTTON_X,      "X" },
	{ 12, CLASSIC_CTRL_BUTTON_Y,      "Y" },
	{ 13, CLASSIC_CTRL_BUTTON_PLUS,   "+" },
	{ 14, CLASSIC_CTRL_BUTTON_MINUS,  "-" },
	{ 15, CLASSIC_CTRL_BUTTON_HOME,   "Home" },
	{ 16, R_STICK_U,                  "RS-Up" },
	{ 17, R_STICK_L,                  "RS-Left" },
	{ 18, R_STICK_R,                  "RS-Right" },
	{ 19, R_STICK_D,                  "RS-Down" },
	{ 20, L_STICK_U,                  "LS-Up" },
	{ 21, L_STICK_L,                  "LS-Left" },
	{ 22, L_STICK_R,                  "LS-Right" },
	{ 23, L_STICK_D,                  "LS-Down" },
};

static button_t analog_sources[] = {
	{ 0, L_STICK_AS_ANALOG,  "Left Stick" },
	{ 1, R_STICK_AS_ANALOG,  "Right Stick" },
};

static button_t menu_combos[] = {
	{ 0, CLASSIC_CTRL_BUTTON_X|CLASSIC_CTRL_BUTTON_Y, "X+Y" },
	{ 1, CLASSIC_CTRL_BUTTON_ZL|CLASSIC_CTRL_BUTTON_ZR, "ZL+ZR" },
};


static unsigned int getButtons(classic_ctrl_t* controller)
{
	unsigned int b = (unsigned)controller->btns;
	s8 stickX      = getStickValue(&controller->ljs, STICK_X, 7);
	s8 stickY      = getStickValue(&controller->ljs, STICK_Y, 7);
	s8 substickX   = getStickValue(&controller->rjs, STICK_X, 7);
	s8 substickY   = getStickValue(&controller->rjs, STICK_Y, 7);
	
	if(stickX    < -3) b |= L_STICK_L;
	if(stickX    >  3) b |= L_STICK_R;
	if(stickY    >  3) b |= L_STICK_U;
	if(stickY    < -3) b |= L_STICK_D;
	
	if(substickX < -3) b |= R_STICK_L;
	if(substickX >  3) b |= R_STICK_R;
	if(substickY >  3) b |= R_STICK_U;
	if(substickY < -3) b |= R_STICK_D;
	
	return b;
}

static int available(int Control) {
	int err;
	u32 expType;
	err = WPAD_Probe(Control, &expType);
	if(err == WPAD_ERR_NONE &&
	   expType == WPAD_EXP_CLASSIC){
		controller_Classic.available[Control] = 1;
		return 1;
	} else {
		controller_Classic.available[Control] = 0;
		if(err == WPAD_ERR_NONE &&
		   expType == WPAD_EXP_NUNCHUK){
			controller_WiimoteNunchuk.available[Control] = 1;
		}
		else if (err == WPAD_ERR_NONE &&
		   expType == WPAD_EXP_NONE){
			controller_Wiimote.available[Control] = 1;
		}
		return 0;
	}
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));
	c->btns.All = 0xFFFF;  //needed

	// Only use a connected classic controller
	if(!available(Control))
		return 0;

	unsigned int b = getButtons(&wpad->exp.classic);
	inline int isHeld(button_tp button){
		return (b & button->mask) == button->mask ? 0 : 1;
	}
	
	c->btns.SQUARE_BUTTON    = isHeld(config->SQU);
	c->btns.CROSS_BUTTON     = isHeld(config->CRO);
	c->btns.CIRCLE_BUTTON    = isHeld(config->CIR);
	c->btns.TRIANGLE_BUTTON  = isHeld(config->TRI);

	c->btns.R1_BUTTON    = isHeld(config->R1);
	c->btns.L1_BUTTON    = isHeld(config->L1);
	c->btns.R2_BUTTON    = isHeld(config->R2);
	c->btns.L2_BUTTON    = isHeld(config->L2);

	c->btns.L_DPAD       = isHeld(config->DL);
	c->btns.R_DPAD       = isHeld(config->DR);
	c->btns.U_DPAD       = isHeld(config->DU);
	c->btns.D_DPAD       = isHeld(config->DD);

	c->btns.START_BUTTON  = isHeld(config->START);
	c->btns.SELECT_BUTTON = isHeld(config->SELECT);

	//adjust values by 128 cause PSX sticks range 0-255 with a 128 center pos
	if(config->analogL->mask == L_STICK_AS_ANALOG){
		c->leftStickX = getStickValue(&wpad->exp.classic.ljs, STICK_X, 127);
		c->leftStickY = getStickValue(&wpad->exp.classic.ljs, STICK_Y, 127);
	} else if(config->analogL->mask == R_STICK_AS_ANALOG){
		c->leftStickX = getStickValue(&wpad->exp.classic.rjs, STICK_X, 127);
		c->leftStickY = getStickValue(&wpad->exp.classic.rjs, STICK_Y, 127);
	}
	if(config->invertedYL) c->leftStickY = (u8)(127-((int)c->leftStickY-127));

	if(config->analogR->mask == L_STICK_AS_ANALOG){
		c->rightStickX = getStickValue(&wpad->exp.classic.ljs, STICK_X, 127);
		c->rightStickY = getStickValue(&wpad->exp.classic.ljs, STICK_Y, 127);
	} else if(config->analogR->mask == R_STICK_AS_ANALOG){
		c->rightStickX = getStickValue(&wpad->exp.classic.rjs, STICK_X, 127);
		c->rightStickY = getStickValue(&wpad->exp.classic.rjs, STICK_Y, 127);
	}
	if(config->invertedYR) c->rightStickY = (u8)(127-((int)c->rightStickY-127));

	// Return 1 if whether the exit button(s) are pressed
	return isHeld(config->exit) ? 0 : 1;

/*	Previous code
	c->btns.R_DPAD          = (b & CLASSIC_CTRL_BUTTON_RIGHT) ? 0 : 1;
	c->btns.L_DPAD          = (b & CLASSIC_CTRL_BUTTON_LEFT)  ? 0 : 1;
	c->btns.D_DPAD          = (b & CLASSIC_CTRL_BUTTON_DOWN)  ? 0 : 1;
	c->btns.U_DPAD          = (b & CLASSIC_CTRL_BUTTON_UP)    ? 0 : 1;
	c->btns.START_BUTTON    = (b & CLASSIC_CTRL_BUTTON_PLUS)  ? 0 : 1;
	c->btns.SELECT_BUTTON   = (b & CLASSIC_CTRL_BUTTON_MINUS) ? 0 : 1;
	
	c->btns.SQUARE_BUTTON   = (b & CLASSIC_CTRL_BUTTON_Y)  ? 0 : 1;
	c->btns.CROSS_BUTTON    = (b & CLASSIC_CTRL_BUTTON_B)  ? 0 : 1;
	c->btns.CIRCLE_BUTTON   = (b & CLASSIC_CTRL_BUTTON_A)  ? 0 : 1;
	c->btns.TRIANGLE_BUTTON = (b & CLASSIC_CTRL_BUTTON_X)  ? 0 : 1;
	
	c->btns.R1_BUTTON       = (b & CLASSIC_CTRL_BUTTON_ZR)  ? 0 : 1;
	c->btns.L1_BUTTON       = (b & CLASSIC_CTRL_BUTTON_ZL)  ? 0 : 1;
	c->btns.R2_BUTTON       = (b & CLASSIC_CTRL_BUTTON_FULL_R)  ? 0 : 1;
	c->btns.L2_BUTTON       = (b & CLASSIC_CTRL_BUTTON_FULL_L)  ? 0 : 1;

	s8 substickX = getStickValue(&wpad->exp.classic.rjs, STICK_X, 7);
	s8 substickY = getStickValue(&wpad->exp.classic.rjs, STICK_Y, 7);
	c->rightStickX  = (u8)(substickX+127) & 0xFF;
	c->rightStickY  = (u8)(substickY+127) & 0xFF;
	c->leftStickX   = (u8)(getStickValue(&wpad->exp.classic.ljs, STICK_X, 127)+127)    & 0xFF;
	c->leftStickY   = (u8)(-getStickValue(&wpad->exp.classic.ljs, STICK_Y, 127)+127)   & 0xFF;

	// X+Y quits to menu
	return (b & CLASSIC_CTRL_BUTTON_X) && (b & CLASSIC_CTRL_BUTTON_Y);
*/
}

static void pause(int Control){
	WPAD_Rumble(Control, 0);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	WPAD_Rumble(Control, rumble ? 1 : 0);
}

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void refreshAvailable(void);

controller_t controller_Classic =
	{ 'C',
	  _GetKeys,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailable,
	  {0, 0, 0, 0},
	  sizeof(buttons)/sizeof(buttons[0]),
	  buttons,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .SQU        = &buttons[10], // B
	    .CRO        = &buttons[9],  // A
	    .CIR        = &buttons[11], // X
	    .TRI        = &buttons[12], // Y
	    .R1         = &buttons[8],  // Right Z
	    .L1         = &buttons[7],  // Left Z
	    .R2         = &buttons[6],  // Full R
	    .L2         = &buttons[5],  // Full L
	    .DL         = &buttons[2],  // D-Pad Left
	    .DR         = &buttons[3],  // D-Pad Right
	    .DU         = &buttons[1],  // D-Pad Up
	    .DD         = &buttons[4],  // D-Pad Down
	    .START      = &buttons[13], // +
	    .SELECT     = &buttons[14], // -
	    .analogL    = &analog_sources[0], // Left Stick
	    .analogR    = &analog_sources[1], // Right stick
	    .exit       = &menu_combos[1], // ZL+ZR
	    .invertedYL = 0,
	    .invertedYR = 0,
	  }
	 };

static void refreshAvailable(void){

	int i, err;
	u32 expType;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		err = WPAD_Probe(i, &expType);
		if(err == WPAD_ERR_NONE &&
		   expType == WPAD_EXP_CLASSIC){
			controller_Classic.available[i] = 1;
			WPAD_SetDataFormat(i, WPAD_DATA_EXPANSION);
		} else
			controller_Classic.available[i] = 0;
	}
}
#endif
