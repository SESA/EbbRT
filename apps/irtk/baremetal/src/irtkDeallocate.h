/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: irtkDeallocate.h 845 2013-05-10 01:35:31Z as12312 $
  Copyright : Imperial College, Department of Computing
              Visual Information Processing (VIP), 2008 onwards
  Date      : $Date: 2013-05-10 02:35:31 +0100 (Fri, 10 May 2013) $
  Version   : $Revision: 845 $
  Changes   : $Author: as12312 $

=========================================================================*/

#ifndef _IRTKDEALLOCATE_H

#define _IRTKDEALLOCATE_H

#include "irtkCommon.h"

/// Deallocate 1-dimensional array
template <typename T> inline T *Deallocate(T *p)
{
  delete[] p;
  return NULL;
}

template <class Type> inline Type **Deallocate(Type **matrix)
{
  delete []matrix[0];
  delete []matrix;
  matrix = NULL;

#ifdef DEBUG
  memory_allocated -= x*y*sizeof(Type);
  cout << "Deallocate: Memory allocated is " << memory_allocated << " bytes.\n";
#endif

  return NULL;
}

template <class Type> inline Type ***Deallocate(Type ***matrix)
{
  delete []matrix[0][0];
  delete []matrix[0];
  delete []matrix;

#ifdef DEBUG
  memory_allocated -= x*y*z*sizeof(Type);
  cout << "Deallocate: Memory allocated is " << memory_allocated << " bytes.\n";
#endif

  matrix = NULL;
  return NULL;
}

template <class Type> inline Type ****Deallocate(Type ****matrix)
{
  delete []matrix[0][0][0];
  delete []matrix[0][0];
  delete []matrix[0];
  delete []matrix;

  matrix = NULL;
  return NULL;
}

#endif
