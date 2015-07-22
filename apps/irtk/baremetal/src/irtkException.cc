/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: irtkException.cc 2 2008-12-23 12:40:14Z dr $
  Copyright : Imperial College, Department of Computing
              Visual Information Processing (VIP), 2008 onwards
  Date      : $Date: 2008-12-23 12:40:14 +0000 (Tue, 23 Dec 2008) $
  Version   : $Revision: 2 $
  Changes   : $Author: dr $

=========================================================================*/

#include "irtkCommon.h"

std::ostream& operator<<(std::ostream& os, const irtkException& ex)
{
  os << "File name: ";
  if (ex.GetFileName() != "")
    os << ex.GetFileName();
  else
    os << "UNKNOWN";
  os << std::endl;

  os << "Line: ";
  if (ex.GetLine() != 0)
    os << ex.GetLine();
  else
    os << "UNKNOWN";
  os << std::endl;

  os << "Message: " << ex.GetMessage();

  return os;
}
