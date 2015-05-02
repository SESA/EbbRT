//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/PvClock.h>

#include <atomic>

#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Msr.h>
#include <ebbrt/Rdtsc.h>

namespace {
struct pvclock_wall_clock {
  std::atomic<volatile uint32_t> version;
  std::atomic<volatile uint32_t> sec;
  std::atomic<volatile uint32_t> nsec;
};

static_assert(sizeof(pvclock_wall_clock) == 12,
              "pvclock_wall_clock packing issue");

pvclock_wall_clock wall_clock;

struct pvclock_vcpu_time_info {
  std::atomic<volatile uint32_t> version;
  std::atomic<volatile uint32_t> pad0;
  std::atomic<volatile uint64_t> tsc_timestamp;
  std::atomic<volatile uint64_t> system_time;
  std::atomic<volatile uint32_t> tsc_to_system_mul;
  std::atomic<volatile int8_t> tsc_shift;
  std::atomic<volatile uint8_t> flags;
  std::atomic<volatile uint8_t> pad[2];
};

static_assert(sizeof(pvclock_vcpu_time_info) == 32,
              "pvclock_vcpu_time_info packing issue");

thread_local pvclock_vcpu_time_info vcpu_time_info;
std::chrono::nanoseconds boot_system_time;

void SetupSystemTime() {
  ebbrt::msr::Write(ebbrt::msr::kKvmSystemTimeNew,
                    reinterpret_cast<uint64_t>(&vcpu_time_info) | 1);
}

ebbrt::clock::Wall::time_point WallClockBoot() {
  uint32_t version, sec, nsec;
  do {
    // if the version value is odd, then an update is in progress, retry
    if ((version = wall_clock.version.load(std::memory_order_relaxed)) % 2)
      continue;
    std::atomic_thread_fence(std::memory_order_acquire);
    sec = wall_clock.sec.load(std::memory_order_relaxed);
    nsec = wall_clock.nsec.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
  } while (version != wall_clock.version.load(std::memory_order_relaxed));
  return ebbrt::clock::Wall::time_point(std::chrono::seconds(sec) +
                                        std::chrono::nanoseconds(nsec));
}

std::chrono::nanoseconds SystemTime() noexcept {
  uint32_t version;
  uint64_t tsc = 0;
  uint64_t system_time = 0;
  uint32_t tsc_to_system_mul = 0;
  int8_t tsc_shift = 0;
  do {
    if ((version = vcpu_time_info.version.load(std::memory_order_relaxed)) % 2)
      continue;

    std::atomic_thread_fence(std::memory_order_acquire);
    tsc = vcpu_time_info.tsc_timestamp.load(std::memory_order_relaxed);
    system_time = vcpu_time_info.system_time.load(std::memory_order_relaxed);
    tsc_to_system_mul =
        vcpu_time_info.tsc_to_system_mul.load(std::memory_order_relaxed);
    tsc_shift = vcpu_time_info.tsc_shift.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
  } while (version != vcpu_time_info.version.load(std::memory_order_relaxed));

  auto time = ebbrt::rdtsc() - tsc;
  if (tsc_shift >= 0)
    time <<= tsc_shift;
  else
    time >>= -tsc_shift;
  uint64_t tmp;
  asm("mulq %[multiplier];"
      "shrd $32, %[hi], %[lo]"
      : [lo] "+a"(time), [hi] "=d"(tmp)
      : [multiplier] "rm"(static_cast<uint64_t>(tsc_to_system_mul)));
  return std::chrono::nanoseconds(system_time + time);
}

ebbrt::ExplicitlyConstructed<ebbrt::clock::PvClock> the_clock;
}  // namespace

void ebbrt::clock::PvClock::ApInit() { SetupSystemTime(); }

ebbrt::clock::PvClock* ebbrt::clock::PvClock::GetClock() {
  the_clock.construct();
  return &*the_clock;
}

ebbrt::clock::PvClock::PvClock() {
  msr::Write(msr::kKvmWallClockNew, reinterpret_cast<uint64_t>(&wall_clock));

  SetupSystemTime();
  boot_system_time = SystemTime();
}

ebbrt::clock::Wall::time_point ebbrt::clock::PvClock::Now() noexcept {
  return WallClockBoot() + SystemTime();
}

std::chrono::nanoseconds ebbrt::clock::PvClock::Uptime() noexcept {
  return SystemTime() - boot_system_time;
}

std::chrono::nanoseconds
ebbrt::clock::PvClock::TscToNano(uint64_t tsc) noexcept {
  uint32_t version;
  uint32_t tsc_to_system_mul = 0;
  int8_t tsc_shift = 0;
  do {
    if ((version = vcpu_time_info.version.load(std::memory_order_relaxed)) % 2)
      continue;

    std::atomic_thread_fence(std::memory_order_acquire);
    tsc_to_system_mul =
        vcpu_time_info.tsc_to_system_mul.load(std::memory_order_relaxed);
    tsc_shift = vcpu_time_info.tsc_shift.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
  } while (version != vcpu_time_info.version.load(std::memory_order_relaxed));

  if (tsc_shift >= 0)
    tsc <<= tsc_shift;
  else
    tsc >>= -tsc_shift;
  uint64_t tmp;
  asm("mulq %[multiplier];"
      "shrd $32, %[hi], %[lo]"
      : [lo] "+a"(tsc), [hi] "=d"(tmp)
      : [multiplier] "rm"(static_cast<uint64_t>(tsc_to_system_mul)));
  return std::chrono::nanoseconds(tsc);
}
