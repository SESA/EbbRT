//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Compiler.h>
#include <ebbrt/NetChecksum.h>

namespace {
// Last step, fold a 32 bit sum to 16 bit and invert it
uint16_t CsumFold(uint32_t sum) {
  asm("addl %[lower_sum], %[sum];"
      "adcl $0xffff,%[sum];"
      : [sum] "+r"(sum)
      : [lower_sum] "r"(static_cast<uint32_t>(sum << 16)));
  return ~(sum >> 16);
}

uint16_t From32To16(uint32_t val) {
  uint16_t ret = val >> 16;
  asm("addw %w[val], %w[ret];"
      "adcw $0, %w[ret];"
      : [ret] "+r"(ret)
      : [val] "r"(val));
  return ret;
}

uint32_t Add32WithCarry(uint32_t a, uint32_t b) {
  asm("addl %[b], %[a];"
      "adcl $0, %[a];"
      : [a] "+r"(a)
      : [b] "r"(b));
  return a;
}

uint32_t Csum(const uint8_t* buf, size_t len) {
  if (unlikely(len == 0))
    return 0;

  uint64_t result = 0;

  auto odd = reinterpret_cast<uintptr_t>(buf) & 1;
  if (unlikely(odd)) {
    result = *buf << 8;
    --len;
    ++buf;
  }

  auto count = len >> 1;  // num 16 bit words
  if (count) {
    // two byte aligned but not larger
    if (reinterpret_cast<uintptr_t>(buf) & 2) {
      result += *reinterpret_cast<const uint16_t*>(buf);
      count--;
      len -= 2;
      buf += 2;
    }
    count >>= 1;  // num 32 bit words
    if (count) {
      // four byte aligned but not larger
      if (reinterpret_cast<uintptr_t>(buf) & 4) {
        result += *reinterpret_cast<const uint32_t*>(buf);
        count--;
        len -= 4;
        buf += 4;
      }
      count >>= 1;  // num 64 bit words

      uint32_t count64 = count >> 3;  // cacheline at a time
      while (count64) {
        asm("addq 0*8(%[src]),%[res];"
            "adcq 1*8(%[src]),%[res];"
            "adcq 2*8(%[src]),%[res];"
            "adcq 3*8(%[src]),%[res];"
            "adcq 4*8(%[src]),%[res];"
            "adcq 5*8(%[src]),%[res];"
            "adcq 6*8(%[src]),%[res];"
            "adcq 7*8(%[src]),%[res];"
            "adcq $0,%[res];"
            : [res] "+r"(result)
            : [src] "r"(buf));
        buf += 64;
        --count64;
      }

      // up to 7 * 8 bytes remain
      count %= 8;
      while (count) {
        asm("addq %[src], %[res];"
            "adcq $0, %[res];"
            : [res] "+r"(result)
            : [src] "m"(*reinterpret_cast<const uint64_t*>(buf)));
        --count;
        buf += 8;
      }

      result = Add32WithCarry(result >> 32, result & 0xffffffff);

      if (len & 4) {
        result += *reinterpret_cast<const uint32_t*>(buf);
        buf += 4;
      }
    }
    if (len & 2) {
      result += *reinterpret_cast<const uint16_t*>(buf);
      buf += 2;
    }
  }
  if (len & 1)
    result += *buf;
  result = Add32WithCarry(result >> 32, result & 0xffffffff);
  if (unlikely(odd)) {
    result = From32To16(result);
    result = ((result >> 8) & 0xff) | ((result & 0xff) << 8);
  }
  return result;
}
}  // namespace

uint16_t ebbrt::IpCsum(const IOBuf& buf) {
  uint32_t acc = 0;
  bool swapped = false;
  for (auto& b : buf) {
    acc += Csum(b.Data(), b.Length());
    acc = CsumFold(acc);
    if (b.Length() % 2) {
      swapped = !swapped;
      acc = __builtin_bswap16(acc);
    }
  }

  if (swapped) {
    acc = __builtin_bswap16(acc);
  }
  return ~(acc & 0xffff);
}

uint16_t ebbrt::IpCsum(const uint8_t* buf, size_t len) {
  return CsumFold(Csum(buf, len));
}
