/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: irtkScalarFunction.h 2 2008-12-23 12:40:14Z dr $
  Copyright : Imperial College, Department of Computing
              Visual Information Processing (VIP), 2008 onwards
  Date      : $Date: 2008-12-23 12:40:14 +0000 (Tue, 23 Dec 2008) $
  Version   : $Revision: 2 $
  Changes   : $Author: dr $

=========================================================================*/

#ifndef _IRTKSCALARFUNCTION_H

#define _IRTKSCALARFUNCTION_H

/**

  Scalar function class.

*/

class irtkScalarFunction : public irtkObject
{

public:

  /// Virtual destructor
  virtual ~irtkScalarFunction();

  /// Evaluation function (pure virtual)
  virtual double Evaluate(double x, double y, double z) = 0;
};

#include <irtkScalarGaussian.h>

#endif
