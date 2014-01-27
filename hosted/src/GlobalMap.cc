//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Config.h>
#include <ebbrt/GlobalMap.h>

namespace bai = boost::asio::ip;

ebbrt::GlobalMap::GlobalMap()
    : acceptor_(
          active_context->io_service_,
          bai::tcp::endpoint(bai::address_v4::from_string(frontend_ip), 0)),
      socket_(active_context->io_service_) {
  DoAccept();
}

void ebbrt::GlobalMap::DoAccept() {
  acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
    if (!ec) {
      std::make_shared<session>(std::move(socket_));
    }
    DoAccept();
  });
}

std::pair<uint16_t, uint32_t> ebbrt::GlobalMap::GetAddress() {
  const auto &endpoint = acceptor_.local_endpoint();
  return std::make_pair(endpoint.port(), endpoint.address().to_v4().to_ulong());
}

ebbrt::GlobalMap::session::session(bai::tcp::socket socket)
    : socket_(std::move(socket)) {
  std::cout << "GlobalMap: New Session" << std::endl;
}
