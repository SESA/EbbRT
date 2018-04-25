//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Printer.h"

#include <iostream>

#include <ebbrt/GlobalIdMap.h>

#include <ebbrt/UniqueIOBuf.h>

EBBRT_PUBLISH_TYPE(, Printer);

Printer::Printer() : ebbrt::Messagable<Printer>(kPrinterEbbId) {}

ebbrt::Future<void> Printer::Init() {
  return ebbrt::global_id_map->Set(
      kPrinterEbbId, ebbrt::GlobalIdMap::OptArgs({.data=ebbrt::messenger->LocalNetworkId().ToBytes()}));
}

void Printer::Print(const char* str) {}

void Printer::Execute(ebbrt::Messenger::NetworkId nid) {
  const char* str = "exec";
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char*>(buf->MutData()), len, "%s", str);
  std::cout << "Sending to " << nid.ToString() << std::endl;
  SendMessage(nid, std::move(buf));

}

void Printer::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             std::unique_ptr<ebbrt::IOBuf>&& buffer) {
  auto output = std::string(reinterpret_cast<const char*>(buffer->Data()),
                            buffer->Length());
  std::cout << nid.ToString() << ": " << output;
}
