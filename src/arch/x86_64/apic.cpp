#include <new>

#include "arch/cpu.hpp"
#include "arch/io.hpp"
#include "arch/x86_64/acpi.hpp"
#include "arch/x86_64/apic.hpp"
#include "lrt/mem.hpp"

ebbrt::apic::IoApic** ebbrt::apic::io_apics;
int ebbrt::apic::num_io_apics;

bool
ebbrt::apic::init()
{
  //Check that we actually have a lapic
  uint32_t features, dummy;
  cpuid(CPUID_FEATURES, &dummy, &dummy, &dummy, &features);
  if (!features & CPUID_EDX_HAS_LAPIC) {
    return false;
  }

  //disable 8259 by masking IRQS on master and slave
  out8(0x21, 0xff);
  out8(0xa1, 0xff);

  num_io_apics = acpi::get_num_io_apics();
  io_apics = new (lrt::mem::malloc(sizeof(IoApic) * num_io_apics, 0))
    IoApic*[num_io_apics];

  for (auto i = 0; i < num_io_apics; ++i) {
    const acpi::MadtIoApic* madt_io_apic = acpi::find_io_apic(i);
    io_apics[i] = reinterpret_cast<IoApic*>(madt_io_apic->addr);
  }
  return true;
}

void
ebbrt::apic::disable_irqs()
{
  IoApic::IoApicRedEntry entry;
  entry.Disable();

  for (auto i = 0; i < num_io_apics; ++i) {
    //how many entries?
    int entries = io_apics[i]->MaxRedEntry() + 1;

    //disable all entries
    for (int j = 0; j < entries; j++) {
      io_apics[i]->WriteEntry(j, entry);
    }
  }
}

uint32_t
ebbrt::apic::IoApic::Read(int index)
{
  io_reg_sel_ = index;
  return io_win_;
}

void
ebbrt::apic::IoApic::Write(int index, uint32_t val)
{
  io_reg_sel_ = index;
  io_win_ = val;
}

ebbrt::apic::IoApic::IoApicVersionRegister
ebbrt::apic::IoApic::ReadVersion()
{
  return Read(IO_APIC_VER);
}

int
ebbrt::apic::IoApic::MaxRedEntry()
{
  return ReadVersion().max_redir_entry;
}

void
ebbrt::apic::IoApic::WriteEntry(int i, IoApicRedEntry entry)
{
  Write(IO_APIC_REDTBL_START + (i * 2), entry & 0xFFFFFFFFF);
  Write(IO_APIC_REDTBL_START + (i * 2) + 1, (entry >> 32) & 0xFFFFFFFFF);
}

ebbrt::apic::IoApic::IoApicRedEntry::IoApicRedEntry() : raw_(0)
{
}

void
ebbrt::apic::IoApic::IoApicRedEntry::Disable()
{
  interrupt_mask_ = 1;
}

void
ebbrt::apic::Lapic::Enable()
{
  uint64_t base = rdmsr(MSR_APIC_BASE);
  base |= 1 << 11;
  wrmsr(MSR_APIC_BASE, base);

  Lapic* lapic = reinterpret_cast<Lapic*>
    (base & ((1 << 12) - 1));

  lapic->SwEnable();
}

void
ebbrt::apic::Lapic::SwEnable()
{
  sivr_.sw_enable_ = 1;
}

void
ebbrt::apic::Lapic::SendIpi(IcrLow icr_low, IcrHigh icr_high)
{
  lapic->lih_.raw_ = icr_high.raw_;
  lapic->lil_.raw_ = icr_low.raw_;
}
