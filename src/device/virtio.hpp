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
#ifndef EBBRT_DEVICE_VIRTIO_HPP
#define EBBRT_DEVICE_VIRTIO_HPP

#include "arch/io.hpp"

namespace ebbrt {
  namespace virtio {
    class QueueDescriptor {
    public:
      uint64_t address;
      uint32_t length;
      union {
        uint16_t raw;
        struct {
          uint16_t next :1;
          uint16_t write :1;
          uint16_t indirect :1;
        uint16_t :13;
        };
      } flags;
      uint16_t next;
    };
    class Available {
    public:
      union {
        uint16_t raw;
        struct {
          uint16_t no_interrupt :1;
          uint16_t :15;
        };
      };
      uint16_t index;
      uint16_t ring[];
    };
    class UsedElement {
    public:
      uint32_t index;
      uint32_t length;
    };
    class Used {
    public:
      union {
        uint16_t raw;
        struct {
          uint16_t no_notify :1;
          uint16_t :15;
        };
      };
      uint16_t index;
      UsedElement ring[];
    };

    inline uint32_t
    device_features(uint16_t io_addr)
    {
      return in32(io_addr);
    }

    inline void
    guest_features(uint16_t io_addr, uint32_t features)
    {
      out32(features, io_addr + 4);
    }

    inline void
    queue_address(uint16_t io_addr, uint32_t address)
    {
      out32(address, io_addr + 8);
    }

    inline uint16_t
    queue_size(uint16_t io_addr)
    {
      return in16(io_addr + 12);
    }

    inline void
    queue_select(uint16_t io_addr, uint16_t n)
    {
      out16(n, io_addr + 14);
    }

    inline void
    queue_notify(uint16_t io_addr, uint16_t n)
    {
      out16(n, io_addr + 16);
    }

    inline void
    queue_vector(uint16_t io_addr, uint16_t n)
    {
      out16(n, io_addr + 22);
    }

    inline uint16_t
    queue_vector(uint16_t io_addr)
    {
      return in16(io_addr + 22);
    }

    inline size_t
    qsz_bytes(uint16_t qsz)
    {
      return ((sizeof(QueueDescriptor) * qsz +
               6 + 2 * qsz + 4095) & ~4095) +
        ((6 + sizeof(UsedElement) * qsz + 4095) & ~4095);
    }

    union DeviceStatus {
      uint8_t raw;
      struct {
        uint8_t acknowledge :1;
        uint8_t driver :1;
        uint8_t driver_ok :1;
        uint8_t :4;
        uint8_t failed :1;
      };
    };

    inline DeviceStatus device_status(uint16_t io_addr)
    {
      DeviceStatus status;
      status.raw = in8(io_addr + 18);
      return status;
    }

    inline void device_status(uint16_t io_addr, DeviceStatus status)
    {
      out8(status.raw, io_addr + 18);
    }

    inline void acknowledge(uint16_t io_addr)
    {
      DeviceStatus status = device_status(io_addr);
      status.acknowledge = true;
      device_status(io_addr, status);
    }

    inline void driver(uint16_t io_addr)
    {
      DeviceStatus status = device_status(io_addr);
      status.driver = true;
      device_status(io_addr, status);
    }

    inline void driver_ok(uint16_t io_addr)
    {
      DeviceStatus status = device_status(io_addr);
      status.driver_ok = true;
      device_status(io_addr, status);
    }
  }
}
#endif
