/***************************************************************
*
*	Copyright (c) 2021
*	All rights reserved.
*
*	Project 		:
*	Last Updated on	:
*	Author			: Ganesh
*
*	Revision History
****************************************************************
*	Date			Version		Name		Description
****************************************************************
*	07/02/2020		1.0			Ganesh		Initial Development
*
****************************************************************/
#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#include <stdint.h>
#include <stdlib.h>
/*
*Macros
*/
#define	GPIO_SET	0x01
#define	GPIO_GET	0x02
#define	ROW_PIN		0x01
#define	COL_PIN		0x02

#define	GPIO_NON	0xFF
#define	GPIO_HIGH	0x01
#define	GPIO_LOW	0x00

/*
*Structure
*/
#pragma pack(push,1)

typedef struct
{
	uint16_t	kRow;		//number row in keypad
	uint16_t	kCol;		//number column in keypad
	uint8_t 	*kMap;		//2d array pointer for key map

	/* Two function pointer to initialize and access the GPIO */
	int			(*initGpio)(void);
	uint8_t		(*ctlGpio)(int16_t,uint8_t,uint8_t,uint8_t);
}KEYPAD;

#pragma pack(pop)

/*
*Function declarations
*/
int setKeypadInfo(	uint16_t numOfRow,uint16_t numOfCol, const uint8_t *keyMap,
					int (*iGpio)(void), uint8_t (*ctlGpio)(int16_t,uint8_t,uint8_t,uint8_t) );
int keyPadInit(void);
uint8_t readKey(void);

#endif

/* EOF */