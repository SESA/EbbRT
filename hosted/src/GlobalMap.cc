//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/GlobalMap.h>

namespace bai = boost::asio::ip;

ebbrt::GlobalMap::GlobalMap()
    : acceptor_(active_context->io_service_,
                bai::tcp::endpoint(bai::address_v4(), 0)),
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

uint16_t ebbrt::GlobalMap::GetPort() {
  const auto& endpoint = acceptor_.local_endpoint();
  return endpoint.port();
}

ebbrt::GlobalMap::session::session(bai::tcp::socket socket)
    : socket_(std::move(socket)) {
  std::cout << "GlobalMap: New Session" << std::endl;
}
