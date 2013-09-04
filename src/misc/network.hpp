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
#ifndef EBBRT_MISC_NETWORK_HPP
#define EBBRT_MISC_NETWORK_HPP

#include <cstring>

#include <functional>
#include <string>

namespace ebbrt {
#ifndef __bg__
  class NetworkId {
  public:
    char mac_addr[6];
  };
  inline bool operator==(const NetworkId& lhs, const NetworkId& rhs)
  {
    return std::strncmp(lhs.mac_addr, rhs.mac_addr, 6);
  }
#else
  class NetworkId {
  public:
    NetworkId() = default;
    NetworkId(const NetworkId& other) : rank{other.rank} {}
    int rank;
  };
  inline bool operator==(const NetworkId& lhs, const NetworkId& rhs)
  {
    return lhs.rank == rhs.rank;
  }
  inline bool operator!=(const NetworkId& lhs, const NetworkId& rhs) {
    return !(lhs == rhs);
  }
#endif
}

#ifndef __bg__
namespace std {
  template <> struct hash<ebbrt::NetworkId>
  {
    size_t operator()(const ebbrt::NetworkId& x) const
    {
      return hash<std::string>()(std::string(x.mac_addr, 6));
    }
  };
}
#else
namespace std {
  template <> struct hash<ebbrt::NetworkId>
  {
    size_t operator()(const ebbrt::NetworkId& x) const
    {
      return hash<int>()(x.rank);
    }
  };
}
#endif

#endif
