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

/*
#include <stdio.h>
#define ROW     3
#define COL     3
int main(void)
{
    // 2d array
    char aiData [ROW][COL] = { { 'a','b','c' }, { 'd', 'e', 'f' }, {'g', 'h', 'i'} };
    char *piData = NULL; //pointer to integer
    int i =0, j =0;
    piData = (char *)aiData; //You can also write *aiData
    for (i = 0; i < ROW; ++i) //Loop of row
    {
        for (j = 0; j < COL; ++j)// Loop for coloum
        {
            //Read element of 2D array
            printf("aiData[%d][%d] = %c\n",i,j, *(piData + ( i * COL) + j));
        }
        printf("\n");
    }
    return 0;
}
 */

/*** Includes ***/
#include "keyPad.h"

/*** Globals ***/
static KEYPAD kaypadInfo;

/****************************************************************
* Function Definition
****************************************************************/
/* Private */

/* Public */
int setKeypadInfo(	uint16_t numOfRow,uint16_t numOfCol, const uint8_t *keyMap,
					int (*iGpio)(void), uint8_t (*cGpio)(int16_t,uint8_t,uint8_t,uint8_t) )
{
	kaypadInfo.kRow = numOfRow;
	kaypadInfo.kCol = numOfCol;

	if(keyMap != NULL)
		kaypadInfo.kMap = (uint8_t *)keyMap;
	else
		return -1;

	if( (iGpio != NULL) && (cGpio != NULL) )
	{
		kaypadInfo.initGpio = iGpio;
		kaypadInfo.ctlGpio = cGpio;
	}
	else
		return -1;

	return 0;
}

int keyPadInit(void)
{
	return kaypadInfo.initGpio();
}

uint8_t readKey(void)
{
	uint8_t kVal = GPIO_NON, i=0, j=0, k=0;

    for (i = 0; i < kaypadInfo.kRow; ++i) //Loop of row
    {
		for (k = 0; k < kaypadInfo.kRow; ++k)
		{
			if(i == k)
				kaypadInfo.ctlGpio(k,ROW_PIN,GPIO_LOW,GPIO_SET);
			else
				kaypadInfo.ctlGpio(k,ROW_PIN,GPIO_HIGH,GPIO_SET);
		}

		for (j = 0; j < kaypadInfo.kCol; ++j)// Loop for coloum
		{
			if(kaypadInfo.ctlGpio(j,COL_PIN,GPIO_NON,GPIO_GET) == GPIO_LOW)
				kVal = *(kaypadInfo.kMap + ( i * kaypadInfo.kCol) + j);
		}
	}
	return kVal;
}

/* EOF */