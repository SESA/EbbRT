//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include <ebbrt/Debug.h>

#include <gsl/gsl_sf_bessel.h>

#include "irtkImage.h"

void AppMain()
{
    double x, y;
    irtkRealImage *img = NULL;

    //testing GNU Scientific Library
    x = 5.0;
    y = gsl_sf_bessel_J0(x);
    ebbrt::kprintf("GSL TEST: %g = %g\n", x, y);
    
    //Testing IRTK
    //creates a new irtkRealImage with x = 5, y = 5, z = 5, irtkRealImage means each 3D point is represented using a double value
    img = new irtkRealImage(5, 5, 5, 1);
    //getting the number of voxels in created image, should be 125
    ebbrt::kprintf("IRTK TEST: Num voxels: %d\n", img->GetNumberOfVoxels());

    //sends a string message to the front end node
    printer->Print("Hello World\n");
}
