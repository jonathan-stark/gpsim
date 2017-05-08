/*
   Copyright (C) 2017	Roy R Rankin

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#include "../config.h"
#include "14bit-processors.h"
#include "14bit-registers.h"
#include "a2d_v2.h"


#include "lcd_module.h"

#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

LCD_MODULE::LCD_MODULE(Processor *pCpu, bool p16f917)
{
    char lcdsex[] = "lcdsex"; 
    char lcddataX[10];
    int i;

    lcdcon = new LCDCON(pCpu, "lcdcon", "LCD control register", this);
    lcdps = new LCDPS(pCpu, "lcdps", "LCD prescaler select register", this);
    for (i = 0; i < 3 ; i++)
    {
	lcdsex[5] = '0' + i;
	if ( i < 2 || p16f917)
	    lcdsen[i] = new LCDSEn(pCpu, (const char *)lcdsex, "LCD Segment register", this, i);
	else
	    lcdsen[i] = 0;
    }
    printf("\n");
    for (i=0; i < 12; i++)
    {
	sprintf(lcddataX, "lcddata%d", i);
 	if (((i+1)%3 != 0) || p16f917)
 	{
	    lcddatax[i] = new LCDDATAx(pCpu, (const char *)lcddataX, "LCD Data register", this, i);
	}
	else
	{
	    lcddatax[i] = 0;
	}
    }
}

LCDCON::LCDCON(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module) :
    sfr_register(pCpu, pName, pDesc)
{
    pt_lcd_module = _lcd_module;
}
void LCDCON::put(unsigned int new_value)
{
}


LCDPS::LCDPS(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module) :
    sfr_register(pCpu, pName, pDesc)
{
    pt_lcd_module = _lcd_module;
}
void LCDPS::put(unsigned int new_value)
{
}
LCDSEn::LCDSEn(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module, unsigned int n) :
    sfr_register(pCpu, pName, pDesc)
{
    pt_lcd_module = _lcd_module;
}
void LCDSEn::put(unsigned int new_value)
{
}
LCDDATAx::LCDDATAx(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *_lcd_module, unsigned int n) :
    sfr_register(pCpu, pName, pDesc)
{
    pt_lcd_module = _lcd_module;
}
void LCDDATAx::put(unsigned int new_value)
{
}
