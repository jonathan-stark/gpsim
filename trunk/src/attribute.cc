/*
   Copyright (C) 2001 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <string.h>

#include "attribute.h"


/*********************************************************************
 *
       Attributes




 *********************************************************************/

/*
AttributeType::AttributeType (ATTRIBUTE_VALUE_TYPES t)
{ 
  type = t;


  switch(type) {

  case ATTRIBUTE_INT:
    val.n = 0;
    break;

  case ATTRIBUTE_FLOAT:
    val.f = 0.0;
    break;

  case ATTRIBUTE_STRING:
    val.s = NULL;

  }

}


-------------------------------------------------------------------
void AttributeType::set(char *s) {

  if(type == ATTRIBUTE_STRING) {
    if (val.s)
      delete val.s;

    if(s)
      val.s = strdup(s); 
    else
      val.s = NULL;

  }

}

*/
//-------------------------------------------------------------------
Attribute::Attribute(void)
{

  name = NULL;
  //xref = NULL;


}

//-------------------------------------------------------------------
Attribute::Attribute(char *init_name)
{

  if(init_name)
    name = strdup(init_name);
  else
    name = NULL;
  

}
//-------------------------------------------------------------------
void Attribute::new_name(char *s)
{
  if(name)
    delete name;

  if(s)
    name = strdup(s);
  else
    name = NULL;

}


//-------------------------------------------------------------------
// char * IntAttribute::sGet(char *buffer, int buf_size)
//
//  Write the current value of this integer attribute
// into the buffer that the caller provides.
//
//INPUTS:
//
//  char *buffer   - pointer to buffer where the conversion is made
//  int   buf_size - the size of the character buffer.
//
//RETURN:
//  buffer
//
char * IntAttribute::sGet(char *buffer, int buf_size)
{

  if(!buffer || buf_size==0)
    return NULL;


  snprintf(buffer,buf_size,"%d",value);

  return buffer;
}

//-------------------------------------------------------------------
void IntAttribute::set(char *new_value)
{

  value = 1;

}

//-------------------------------------------------------------------
void IntAttribute::set(char *val1, int val2)
{

  value = 1;

}

//-------------------------------------------------------------------
void IntAttribute::set(int n)
{
  value = n;
}

//-------------------------------------------------------------------
void IntAttribute::set(double f)
{
  value = (int)f;
}





//-------------------------------------------------------------------
// char * FloatAttribute::sGet(char *buffer, int buf_size)
//
//  Write the current value of this integer attribute
// into the buffer that the caller provides.
//
//INPUTS:
//
//  char *buffer   - pointer to buffer where the conversion is made
//  int   buf_size - the size of the character buffer.
//
//RETURN:
//  buffer
//
char * FloatAttribute::sGet(char *buffer, int buf_size)
{

  if(!buffer || buf_size==0)
    return NULL;


  snprintf(buffer,buf_size,"%g",value);

  return buffer;
}

//-------------------------------------------------------------------
void FloatAttribute::set(char *new_value)
{

  //value = 1.0;
  cout << "Warning: this attribute prefers numbers over strings\n";

}


//-------------------------------------------------------------------
void FloatAttribute::set(char *val1, int val2)
{

  cout << "Warning: this attribute prefers numbers over strings\n";

}

//-------------------------------------------------------------------
void FloatAttribute::set(int n)
{
  value = n;
}

//-------------------------------------------------------------------
void FloatAttribute::set(double f)
{
  value = f;
}







//-------------------------------------------------------------------
char * StringAttribute::sGet(char *buffer, int buf_size)
{

  if(!buffer || buf_size==0)
    return NULL;

  if(value)
    strncpy(buffer,value,buf_size);
  else
    *buffer = 0;

  return buffer;
}

//-------------------------------------------------------------------
void StringAttribute::set(char *new_value)
{

  if (value)
    delete value;

  if(new_value)
    value = strdup(new_value); 
  else
    value = NULL;

}

//-------------------------------------------------------------------
void StringAttribute::set(char *val1, int val2)
{

  set(val1);
}

//-------------------------------------------------------------------
void StringAttribute::set(int n)
{
  #define LOC_BSIZE 64
  char buf[LOC_BSIZE];

  if (value)
    delete value;

  snprintf(buf,LOC_BSIZE,"%d",n);
  value = strdup(buf); 
  nvalue = n;
}

//-------------------------------------------------------------------
void StringAttribute::set(double f)
{
  #define LOC_BSIZE 64
  char buf[LOC_BSIZE];

  if (value)
    delete value;

  snprintf(buf,LOC_BSIZE,"%g",f);
  value = strdup(buf); 
  nvalue = (int)f;
}

