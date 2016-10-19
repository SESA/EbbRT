//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

///
/// This file implements (hopefully) high performance network checksum routines.
///
#include "../Compiler.h"
#include "NetChecksum.h"

namespace {
// Fold a 32 bit sum to 16 bit then invert it
uint16_t CsumFold(uint32_t sum) {
  // Add the two 16 bit values in the top of the registers so the carry flag is
  // set. Then, add the carry in. Finally, shift back so we get a 16 bit return
  auto lower_sum = sum << 16;
  sum &= 0xffff0000;
  asm("addl %[lower_sum], %[sum];"
      "adcl $0xffff,%[sum];"
      : [sum] "+r"(sum)
      : [lower_sum] "r"(lower_sum));
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

// Compute checksum over a contiguous region of memory
uint32_t Csum(const uint8_t* buf, size_t len, size_t offset = 0) {
  if (unlikely(len == 0))
    return 0;

  uint64_t result = 0;

  auto odd_ptr = reinterpret_cast<uintptr_t>(buf) & 1;
  if (unlikely(odd_ptr)) {
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
  if (unlikely(odd_ptr) ^ (offset & 1)) {
    result = From32To16(result);
    result = ((result >> 8) & 0xff) | ((result & 0xff) << 8);
  }
  return result;
}

// Checksum all buffers in an IOBuf without folding
uint32_t IpCsumNoFold(const ebbrt::IOBuf& buf) {
  uint32_t ret = 0;
  size_t offset = 0;
  for (auto& b : buf) {
    ret = Add32WithCarry(ret, Csum(b.Data(), b.Length(), offset));
    offset += b.Length();
  }
  return ret;
}

// Calculate IPv4 pseudo header checksum
uint32_t PseudoCsum(uint16_t len, uint8_t proto, ebbrt::Ipv4Address src,
                    ebbrt::Ipv4Address dst) {
  uint32_t sum = 0;
  uint32_t lenproto = (ebbrt::ntohs(len) << 16) + (proto << 8);
  asm("addl %[dst], %[sum];"
      "adcl %[src], %[sum];"
      "adcl %[lenproto], %[sum];"
      "adcl $0, %[sum];"
      : [sum] "+r"(sum)
      :
      [dst] "g"(dst.toU32()), [src] "g"(src.toU32()), [lenproto] "g"(lenproto));
  return sum;
}
}  // namespace

uint16_t ebbrt::OffloadPseudoCsum(const IOBuf& buf, uint8_t proto,
                                  Ipv4Address src, Ipv4Address dst) {
  return From32To16(PseudoCsum(buf.ComputeChainDataLength(), proto, src, dst));
}

// Calculate the Ipv4 pseudo checksum with the provided header information
uint16_t ebbrt::IpPseudoCsum(const IOBuf& buf, uint8_t proto, Ipv4Address src,
                             Ipv4Address dst) {
  auto sum = IpCsumNoFold(buf);
  auto header_sum = PseudoCsum(buf.ComputeChainDataLength(), proto, src, dst);

  return CsumFold(Add32WithCarry(sum, header_sum));
}

// Checksum all buffers in an IOBuf
uint16_t ebbrt::IpCsum(const IOBuf& buf) { return CsumFold(IpCsumNoFold(buf)); }

// Checksum a region of memory
uint16_t ebbrt::IpCsum(const uint8_t* buf, size_t len) {
  return CsumFold(Csum(buf, len));
}
