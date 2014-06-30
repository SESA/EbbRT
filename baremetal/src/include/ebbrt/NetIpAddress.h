//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETIPADDRESS_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETIPADDRESS_H_

#include <functional>

#include <boost/functional/hash.hpp>

namespace ebbrt {
class Ipv4Address;
}

namespace std {
template <> class hash<ebbrt::Ipv4Address>;
}

namespace ebbrt {
class __attribute__((packed)) Ipv4Address {
 public:
  Ipv4Address() noexcept { addr_.fill(0); }
  Ipv4Address(std::array<uint8_t, 4> a) noexcept {
    std::copy(a.begin(), a.end(), addr_.begin());
  }

  static Ipv4Address Broadcast() {
    Ipv4Address ret;
    ret.addr_[0] = 0xff;
    ret.addr_[1] = 0xff;
    ret.addr_[2] = 0xff;
    ret.addr_[3] = 0xff;

    return ret;
  }

  Ipv4Address Mask(const Ipv4Address& mask) const {
    Ipv4Address ret;
    ret.addr_[0] = addr_[0] & mask.addr_[0];
    ret.addr_[1] = addr_[1] & mask.addr_[1];
    ret.addr_[2] = addr_[2] & mask.addr_[2];
    ret.addr_[3] = addr_[3] & mask.addr_[3];
    return ret;
  }

  bool operator==(const Ipv4Address& other) const {
    return addr_ == other.addr_;
  }

  bool operator!=(const Ipv4Address& other) const { return !(*this == other); }

  Ipv4Address operator~() const {
    Ipv4Address ret;
    ret.addr_[0] = ~addr_[0];
    ret.addr_[1] = ~addr_[1];
    ret.addr_[2] = ~addr_[2];
    ret.addr_[3] = ~addr_[3];

    return ret;
  }

  bool Any() const {
    return addr_[0] == 0 && addr_[1] == 0 && addr_[2] == 0 && addr_[3] == 0;
  }

  bool isBroadcast() const {
    return Any() || (addr_[0] == 0xff && addr_[1] == 0xff && addr_[2] == 0xff &&
                     addr_[3] == 0xff);
  }

  bool isMulticast() const { return (addr_[0] & 0xf0) == 0xe0; }

  bool isLinkLocal() const { return addr_[0] == 169 && addr_[1] == 254; }

 private:
  std::array<uint8_t, 4> addr_;

  friend class std::hash<Ipv4Address>;
};

}  // namespace ebbrt
namespace std {
template <> struct hash<ebbrt::Ipv4Address> {
  size_t operator()(const ebbrt::Ipv4Address& addr) const {
    return boost::hash<std::array<uint8_t, 4>>()(addr.addr_);
  }
};
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETIPADDRESS_H_
