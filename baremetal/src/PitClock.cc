//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/PitClock.h>

#include <atomic>

#include <ebbrt/Cpu.h>
#include <ebbrt/Cpuid.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Io.h>
#include <ebbrt/Msr.h>
#include <ebbrt/Rdtsc.h>

namespace {
thread_local uint64_t boot_system_time;  // Boot time [ticks]
double ns_per_tick;  // CPU speed [nanosec / tick]

// Calculate CPU speed
double CalibrateSystemTime() {
  // Set PIT to Channel 0, Access lobyte/hibyte Mode, Mode 2 (Rate Generator)
  //   Mode 1 (One Shot) is 0x34 (from Detect CPU Speed link) but seems like
  //   could
  //   be an error and is actually Mode 2
  ebbrt::io::Out8(0x43, 0x34);
  ebbrt::io::Out8(0x40, 0);
  ebbrt::io::Out8(0x40, 0);

  // Read tick count (uses latch command)
  ebbrt::io::Out8(0x43, 0x04);
  uint8_t lo = ebbrt::io::In8(0x40);
  uint8_t hi = ebbrt::io::In8(0x40);

  // Set boot time values
  uint16_t start_tick = (hi << 8) + lo;
  uint16_t end_tick = 0;
  uint16_t total_tick = 0;
  uint64_t start_tsc = ebbrt::rdtsc();
  uint64_t end_tsc = 0;
  uint64_t total_tsc = 0;
  uint64_t step = 10000000;
  uint64_t freq = 1193182;

  // Read tsc some number of times (cheap to look at tsc)
  do {
    end_tsc = ebbrt::rdtsc();
  } while ((end_tsc - start_tsc) < step);

  // Read tick count after loop
  ebbrt::io::Out8(0x43, 0x04);
  lo = ebbrt::io::In8(0x40);
  hi = ebbrt::io::In8(0x40);
  end_tick = (hi << 8) + lo;

  // Calculate elapsed ticks (in PIT and tsc)
  total_tick = (start_tick - end_tick);
  total_tsc = (end_tsc - start_tsc);

  // Calculate speed
  return ((static_cast<double>(total_tsc) * freq) / total_tick);
}

ebbrt::ExplicitlyConstructed<ebbrt::clock::PitClock> the_clock;
}  // namespace

// Per core initialize boot time (b/c thread local)
void ebbrt::clock::PitClock::ApInit() { boot_system_time = ebbrt::rdtsc(); }

// Return the Pit clock
ebbrt::clock::PitClock* ebbrt::clock::PitClock::GetClock() {
  the_clock.construct();
  return &*the_clock;
}

// Pit clock constructor
ebbrt::clock::PitClock::PitClock() {
  boot_system_time = ebbrt::rdtsc();
  ns_per_tick = static_cast<double>(1000000000) / CalibrateSystemTime();
}

// Determine current time since boot up
ebbrt::clock::Wall::time_point ebbrt::clock::PitClock::Now() noexcept {
  // Read timestamp counter (tsc)
  uint64_t now_tsc = ebbrt::rdtsc();
  uint64_t total_tsc = 0;

  // Determine total number of ticks elapsed since boot
  uint64_t bst = boot_system_time;
  total_tsc = now_tsc - bst;

  // Calculate current time
  auto now_time = std::chrono::nanoseconds((uint64_t)(total_tsc * ns_per_tick));

  return std::chrono::time_point<ebbrt::clock::Wall>(now_time);
}

// Determine current time since boot up (same as Now())
std::chrono::nanoseconds ebbrt::clock::PitClock::Uptime() noexcept {
  auto now = Wall::Now().time_since_epoch().count();
  return std::chrono::nanoseconds(now);
}

// Convert a timestamp count to nanoseconds
std::chrono::nanoseconds
ebbrt::clock::PitClock::TscToNano(uint64_t tsc) noexcept {
  return std::chrono::nanoseconds((uint64_t)(ns_per_tick * tsc));
}
