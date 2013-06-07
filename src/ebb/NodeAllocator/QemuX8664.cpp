#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <signal.h>
#include <unistd.h>
#include <unordered_map>

#include "ebb/SharedRoot.hpp"
#include "ebb/NodeAllocator/NodeAllocator.hpp"
#include "ebb/NodeAllocator/QemuX8664.hpp"
#include "misc/network.hpp"

ebbrt::QemuX8664::QemuX8664():
  cmd_{"qemu-system-x86_64"}
{}

ebbrt::NodeId
ebbrt::QemuX8664::Allocate(std::string path){

  /* execvp structure */
  char *args[] = {
    const_cast<char*>(cmd_.c_str()), // command
    const_cast<char*>("-netdev"),
    const_cast<char*>("tap,id=vlan0,vhost=on,ifname=tap0,script=no,downscript=no"),
    const_cast<char*>("-device"),
    const_cast<char*>("virtio-net-pci,netdev=vlan0"),
    const_cast<char*>("-net"),
    const_cast<char*>("nic,macaddr=52:54:be:36:42:a9"),
    const_cast<char*>("-nographic"),
    const_cast<char*>(path.c_str()), // boot image
    (char *)0 }; // null terminator

  /* fork & spawn */
  pid_t pid = fork();
  if (pid == 0) // child
  { 
    if(freopen("/dev/null","w+",stdin) == NULL)
      std::cout << "Failure! unable to reroute stdin" << std::endl;
    if(execvp(args[0], args) == -1)
      std::cout << "Failure! execv errored" << std::endl;
    _exit(0); // If exec fails exit forked process.
  } assert(pid > 0);
  
  return reinterpret_cast<NodeId>(pid);
}

void
ebbrt::QemuX8664::Deallocate(NodeId target)
{
  if( kill(reinterpret_cast<pid_t>(target),SIGQUIT) != 0)
    std::cout << "Could not deallocate node " << target << std::endl;
  return;
}

ebbrt::NetworkId
ebbrt::QemuX8664::RandomAddress()
{
  //TODO: return a valid MAC address at random
  // for use w/ QEMU virtual networks
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  return id; 
}

ebbrt::EbbRoot*
ebbrt::QemuX8664::ConstructRoot()
{
  return new SharedRoot<QemuX8664>();
}
