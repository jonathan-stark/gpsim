/*
   Copyright (C) 2017   Roy R Rankin

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
#ifndef __LCD_H__
#define __LCD_H__

class LCD_MODULE;

// LCDCON - LIQUID CRYSTAL DISPLAY CONTROL REGISTER

class LCDCON : public sfr_register
{
public:

  enum
  {
      LMUX0	= 1<<0,	//LMUX<1:0> Commons Select bits
      LMUX1	= 1<<1,	
      CS0	= 1<<2,	//CS<1:0> Clock Source Select bits
      CS1	= 1<<3,
      VLCDEN	= 1<<4,	// LCD Bias Voltage Pins Enable bit
      WERR	= 1<<5, // LCD Write Failed Error bit
      SLPEN	= 1<<6,	// LCD Driver Enable in Sleep mode bit
      LCDEN	= 1<<7	// LCD Driver Enable bit
  };

  LCDCON(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *);
  virtual void put(unsigned int new_value);

  LCD_MODULE *pt_lcd_module;
};

// LCDPS - LCD PRESCALER SELECT REGISTER
class LCDPS : public sfr_register
{
public:

  enum
  {
	LP0	= 1<<0,	//LP<3:0>: LCD Prescaler Select bits
	LP1	= 1<<1,
   	LP2	= 1<<2,
	LP3	= 1<<3,
	WA	= 1<<4,	// LCD Write Allow Status bit
	LCDA	= 1<<5,	// LCD Active Status bit
	BIASMD	= 1<<6,	// Bias Mode Select bit
	WFT	= 1<<7 // Waveform Type Select bit
  };
  LCDPS(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *);
  virtual void put(unsigned int new_value);
  LCD_MODULE *pt_lcd_module;
};

// LCDSEn - LCD SEGMENT REGISTERS

class LCDSEn : public sfr_register
{
public:

  LCDSEn(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *, unsigned int _n);
  virtual void put(unsigned int new_value);
  LCD_MODULE *pt_lcd_module;
  unsigned int n;
};

// LCDDATAx - LCD DATA REGISTERS
class LCDDATAx : public sfr_register
{
public:

  LCDDATAx(Processor *pCpu, const char *pName, const char *pDesc, LCD_MODULE *, unsigned int _n);
  virtual void put(unsigned int new_value);
  LCD_MODULE *pt_lcd_module;
  unsigned int n;
};

class LCD_MODULE
{
public:
    LCD_MODULE(Processor *pCpu, bool p16f917);

    LCDCON	*lcdcon;
    LCDPS	*lcdps;
    LCDSEn   	*lcdsen[3];
    LCDDATAx 	*lcddatax[12];

};    

#endif // __LCD_H__
