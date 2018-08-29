//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_SPLASH_H_
#define COMMON_SRC_INCLUDE_EBBRT_SPLASH_H_

#define COLOR_RESET "\033[0m"
#define RED "\033[31m" /* Red */
#define GREEN "\033[32m" /* Green */
#define YELLOW "\033[33m" /* Yellow */
#define BLUE "\033[34m" /* Blue */
#define MAGENTA "\033[35m" /* Magenta */
#define CYAN "\033[36m" /* Cyan */
#define WHITE "\033[37m" /* White */

#define C0 COLOR_RESET
#define C1 CYAN
#define C2 MAGENTA
#define C3 GREEN
#define C4 YELLOW
#define C5 RED
#define C6 BLUE
#define C7 WHITE

#include "Debug.h"

void EbbRTSplash(void){
  ebbrt::kprintf_force(C1 "                 " C2 "                    " C5 "    ___        \n");
  ebbrt::kprintf_force(C1 "  ~~~~~   ~     ~" C2 "     ~~~~~  ~~~~~~~~" C5 "   |  _|" C6 "   ___ \n");
  ebbrt::kprintf_force(C1 " /       /     / " C2 "    /    /     /    " C5 "   |_|  " C4 " _" C6 "|" C6 "_  |\n");
  ebbrt::kprintf_force(C1 " ~~~~~  ~~~~  ~~~~" C2 "   ~~~~      /     " C5 "       " C7 "_" C4 "|_  " C4 "|" C6 "_|\n");
  ebbrt::kprintf_force(C1 "/      /   / /   / " C2 " /    \\    /     " C5 "       " C7 "|   " C4 "|_| " C3 "|\n");
  ebbrt::kprintf_force(C1 "~~~~~  ~~~~  ~~~~ " C2 " /      \\  /      " C5 "       " C7 "|___" C3 "|___|\n" C0);
}

#endif //COMMON_SRC_INCLUDE_EBBRT_SPLASH_H_

