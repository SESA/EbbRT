#include <cstdint>

#include "arch/io.hpp"
#include "lrt/console.hpp"


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
  outb(0, PORT + INT_ENABLE); //disable interrupts

  outb(LINE_CNTL_REG_DLAB, PORT + LINE_CNTL_REG); //enable dlab
  //set divisor to 1 (115200 baud)
  outb(1, PORT + BAUD_DIV_LSB);
  outb(0, PORT + BAUD_DIV_MSB);

  //set as 8N1 (8 bits, no parity, one stop bit)
  outb(LINE_CNTL_REG_CHARLEN_8, PORT + LINE_CNTL_REG);
}

void
ebbrt::lrt::console::write(char c)
{
  while (!(inb(PORT + LINE_STATUS_REG) & LINE_STATUS_REG_THR_EMPTY))
    ;

  outb(c, PORT + DATA_REG);
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
