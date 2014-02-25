//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Printer.h"

#include <iostream>

#include <ebbrt/GlobalIdMap.h>

EBBRT_PUBLISH_TYPE(, Printer);

Printer::Printer() : ebbrt::Messagable<Printer>(kPrinterEbbId) {}

ebbrt::Future<void> Printer::Init() {
  return ebbrt::global_id_map->Set(
      kPrinterEbbId, ebbrt::messenger->LocalNetworkId().ToBytes());
}

void Printer::Print(std::string str) {}

void Printer::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             ebbrt::Buffer buffer) {
  auto output = std::string(static_cast<char*>(buffer.data()), buffer.size());
  std::cout << nid.ToString() << ": " << output;
}
