#include "../config.h"
#include "xref.h"
#include "gpsim_interface.h"

//-------------------------------------------------------------------
XrefObject::XrefObject()
{
    data=0;
}
XrefObject::XrefObject(unsigned int *value)
{
    data=value;
}

XrefObject::~XrefObject()
{
    list<void*>::iterator ioi;

    ioi=xrefs.begin();
    for(;ioi!=xrefs.end();ioi++) {
      gi.remove_object(*ioi);
//      delete *ioi;
    }
}

void XrefObject::add(void *xref)
{
    xrefs.push_back(xref);
}

void XrefObject::clear(void *xref)
{
    xrefs.remove(xref);
}

void XrefObject::update()
{
  list<void*>::iterator ioi;

  ioi=xrefs.begin();
  for(;ioi!=xrefs.end();++ioi)
  {

    gpointer *xref = (gpointer *) *ioi;

    gi.update_object(xref,get_val());
  }
}


int XrefObject::get_val(void)
{

  if(data)
    return *data;

  return 0;

}
