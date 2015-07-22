/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: irtkObject.cc 2 2008-12-23 12:40:14Z dr $
  Copyright : Imperial College, Department of Computing
              Visual Information Processing (VIP), 2008 onwards
  Date      : $Date: 2008-12-23 12:40:14 +0000 (Tue, 23 Dec 2008) $
  Version   : $Revision: 2 $
  Changes   : $Author: dr $

=========================================================================*/

#include <irtkCommon.h>

irtkObject::irtkObject()
{
  this->DeleteMethod = NULL;
}

irtkObject::~irtkObject()
{
}

void irtkObject::SetDeleteMethod(void (*f)(void *))
{
  if (f != this->DeleteMethod) {
    this->DeleteMethod = f;
  }
}
