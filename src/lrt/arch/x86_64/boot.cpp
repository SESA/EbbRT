#include "arch/x86_64/multiboot.hpp"
#include "lrt/boot.hpp"

const ebbrt::MultibootInformation* ebbrt::lrt::boot::multiboot_information;

extern "C"
void __attribute__((noreturn))
init_arch(ebbrt::MultibootInformation* mbi)
{
  ebbrt::lrt::boot::multiboot_information = mbi;
  ebbrt::lrt::boot::init();
}
