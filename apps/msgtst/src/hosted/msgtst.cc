//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/* Messenger Test
 * Send fixed-size messages from the front-end to back-end and wait for
 * acks from back-end.
 *
 * Usage:
 * ./msgtst <msg_size [B]> <msg_count>
 *
 * */
#include <signal.h>

#include <boost/filesystem.hpp>

#include <ebbrt/Future.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/hosted/Context.h>
#include <ebbrt/hosted/ContextActivation.h>
#include <ebbrt/hosted/NodeAllocator.h>

#include "../MsgTst.h"

using namespace ebbrt;

int main(int argc, char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/msgtst.elf32";

  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  boost::asio::signal_set sig(c.io_service_, SIGINT);
  {
    ebbrt::ContextActivation activation(c);

    uint32_t msg_size;
    uint32_t msg_count;
    if (argc == 1) {
      // prompt user for size/amount
      std::cout << "Message size (bytes): ";
      std::cin >> msg_size;
      std::cout << "Message count: ";
      std::cin >> msg_count;
    } else if (argc == 2) {
      // default amount to 1
      msg_size = std::stoi(argv[1]);
      msg_count = 1;
    } else {
      msg_size = std::stoi(argv[1]);
      msg_count = std::stoi(argv[2]);
    }

    if (!msg_size || !msg_count)
      exit(0);
    // ensure clean quit on ctrl-c
    sig.async_wait([&c](const boost::system::error_code& ec,
                        int signal_number) { c.io_service_.stop(); });

    try {
      auto node_desc = node_allocator->AllocateNode(bindir.string(), 2, 2, 2);
      node_desc.NetworkId().Then([msg_size, msg_count](auto f) {
        auto nid = f.Get();
        auto msgtst_ebb = MsgTst::Create();
        if (msg_size > 0) {
          std::cout << "Sending " << msg_count << " " << msg_size
                    << "B messages..." << std::endl;
          auto fs = msgtst_ebb->SendMessages(nid, msg_count, msg_size);
          WhenAll(fs.begin(), fs.end()).Then([=](auto f) {
            std::cout << "Messages sent successfully!" << std::endl;
          });
        }
        return;
      });
    } catch (std::runtime_error& e) {
      std::cout << e.what() << std::endl;
      exit(1);
    }
  }
  c.Run();
  return 0;
}
