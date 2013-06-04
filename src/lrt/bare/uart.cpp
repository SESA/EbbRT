/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <cstdint>

#include "arch/io.hpp"
#include "lrt/bare/console.hpp"


const int16_t PORT = 0x3F8;

/* when DLAB = 0 : */
const int16_t DATA_REG = 0; /* Data Register */
const int16_t INT_ENABLE = 1; /* Interrupt Enable Register */
/* when DLAB = 1 : */
const int16_t BAUD_DIV_LSB = 0; /* Least-signficant byte of the
buad rate divsior */

const int16_t BAUD_DIV_MSB = 1; /* Most-signficant byte of the
buad rate divsior */

/* Regardless of DLAB value : */
const int16_t INT_ID_FIFO = 2; /* Interrupt identification and
fifo cntl registers */

const int16_t LINE_CNTL_REG = 3; /* Line control register (DLAB
is the most signifcant bit)
*/
const int8_t LINE_CNTL_REG_CHARLEN_8 = 0x3;
const int8_t LINE_CNTL_REG_DLAB = 1 << 7;

const int16_t MODEM_CNTL_REG = 4; /* Modem control register */

const int16_t LINE_STATUS_REG = 5; /* Line status register */
const int8_t LINE_STATUS_REG_THR_EMPTY = 1 << 5;

const int16_t MODEM_STATUS_REG = 6; /* Modem status register */

const int16_t SCRATCH_REG = 7; /* Scratch register */

void
ebbrt::lrt::console::init()
{
  out8(0, PORT + INT_ENABLE); //disable interrupts

  out8(LINE_CNTL_REG_DLAB, PORT + LINE_CNTL_REG); //enable dlab
  //set divisor to 1 (115200 baud)
  out8(1, PORT + BAUD_DIV_LSB);
  out8(0, PORT + BAUD_DIV_MSB);

  //set as 8N1 (8 bits, no parity, one stop bit)
  out8(LINE_CNTL_REG_CHARLEN_8, PORT + LINE_CNTL_REG);
}

void
ebbrt::lrt::console::write(char c)
{
  while (!(in8(PORT + LINE_STATUS_REG) & LINE_STATUS_REG_THR_EMPTY))
    ;

  out8(c, PORT + DATA_REG);
}

int
ebbrt::lrt::console::write(const char *str, int len)
{
  for (int i = 0; i < len; i++) {
    write(str[i]);
  }
  return len;
}

void
ebbrt::lrt::console::write(const char *str)
{
  while (*str != '\0') {
    write(*str++);
  }
}
