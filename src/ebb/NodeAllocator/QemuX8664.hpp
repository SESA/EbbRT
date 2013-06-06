#ifndef EBBRT_EBB_NODEALLOCATOR_QEMUX8664_HPP
#define EBBRT_EBB_NODEALLOCATOR_QEMUX8664_HPP

#include "ebb/NodeAllocator/NodeAllocator.hpp"
#include "misc/network.hpp"

namespace ebbrt {
  class QemuX8664 : public NodeAllocator {
    /**
     *  Stored state of QEMU instance.
     */
    class Instance {
      public:
        NetworkId addr;
    }; //__attribute__((packed))
  public:
    QemuX8664();
    /**
     * Fork and execute a new Qemu instance.
     *
     * @param[in] count Number of instances to allocate
     * @param[in] img Path to boot image 
     *
     * @return nullptr 
     */
    NodeId Allocate(std::string img) override;
    /**
     * Shut down an existing QEMU process.
     *
     * @param[in] target node instance to kill
     */
    void Deallocate(NodeId target) override;
    /**
     *  Generate at random a valid MAC address
     *
     * @return 
     */
    static ebbrt::NetworkId RandomAddress();
    static ebbrt::EbbRoot* ConstructRoot();
  private:
    std::string cmd_;
    std::string args_;
    std::unordered_map<ebbrt::NodeId, ebbrt::QemuX8664::Instance> instance_table_;
  };
}

#endif
