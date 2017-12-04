//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Messenger.h"
#include "../UniqueIOBuf.h"
#include "GlobalIdMap.h"
#include "NodeAllocator.h"

#include <iostream>

namespace bai = boost::asio::ip;

ebbrt::Messenger::Messenger() {
  auto acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(
      active_context->io_service_, bai::tcp::endpoint(bai::address_v4(), 0));
  auto socket = std::make_shared<boost::asio::ip::tcp::socket>(
      active_context->io_service_);
  port_ = acceptor->local_endpoint().port();
  DoAccept(std::move(acceptor), std::move(socket));
}

ebbrt::Future<void> ebbrt::Messenger::Send(NetworkId to, EbbId id,
                                           uint64_t type_code,
                                           std::unique_ptr<IOBuf>&& data) {
  // make sure we have a pending connection
  auto ip = to.ip_.to_ulong();
  {
    std::lock_guard<std::mutex> lock(m_);
    auto it = connection_map_.find(ip);
    if (it == connection_map_.end()) {
      auto endpoint = bai::tcp::endpoint(to.ip_, port_);
      auto& p = promise_map_[ip];
      connection_map_.emplace(ip, p.GetFuture().Share());
      std::queue<message_queue_entry_t> foo;
      message_queue_.emplace(ip, std::move(foo));
      auto socket =
          std::make_shared<bai::tcp::socket>(active_context->io_service_);
      async_connect(
          *socket, &endpoint, (&endpoint) + 1,
          EventManager::WrapHandler([socket, ip,
                                     this](const boost::system::error_code& ec,
                                           bai::tcp::endpoint* /* unused */) {
            if (!ec) {
              auto session = std::make_shared<Session>(std::move(*socket));
              session->Start();
              {
                std::lock_guard<std::mutex> lock(m_);
                while (!message_queue_[ip].empty()) {
                  session->Send(std::move(message_queue_[ip].front().second));
                  message_queue_[ip].front().first.SetValue();
                  message_queue_[ip].pop();
                }
                promise_map_[ip].SetValue(
                    std::weak_ptr<Session>(std::move(session)));
              }
            }
          }));
    }
  }

  // construct message
  auto buf = MakeUniqueIOBuf(sizeof(Header));
  auto dp = buf->GetMutDataPointer();
  auto& h = dp.Get<Header>();
  h.length = data->ComputeChainDataLength();
  h.type_code = type_code;
  h.id = id;
  buf->PrependChain(std::move(data));
  {
    std::lock_guard<std::mutex> lock(m_);
    if (!connection_map_[ip].Ready()) {
      // add to message queue
      Promise<void> p;
      auto f = p.GetFuture();
      message_queue_entry_t pair = std::make_pair(std::move(p), std::move(buf));
      message_queue_[ip].emplace(std::move(pair));
      return f;
    }
  }
  // send message immediately
  auto f = connection_map_[ip].Get();
  return f.lock()->Send(std::move(buf));
}

void ebbrt::Messenger::DoAccept(
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor,
    std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
  acceptor->async_accept(*socket, EventManager::WrapHandler([acceptor, socket,
                                                             this](
                                      boost::system::error_code ec) {
    if (!ec) {
      auto addr = socket->remote_endpoint().address().to_v4().to_ulong();
      std::lock_guard<std::mutex> lock(m_);
      if (connection_map_.find(addr) != connection_map_.end())
        throw std::runtime_error("Store to promise");

      auto session = std::make_shared<Session>(std::move(*socket));
      session->Start();
      connection_map_.emplace(
          addr,
          MakeReadyFuture<std::weak_ptr<Session>>(std::move(session)).Share());
      DoAccept(std::move(acceptor), std::move(socket));
    }
  }));
}

uint16_t ebbrt::Messenger::GetPort() { return port_; }

ebbrt::Messenger::Session::Session(bai::tcp::socket socket)
    : socket_(std::move(socket)) {
  socket_.set_option(boost::asio::ip::tcp::no_delay(true));
}

void ebbrt::Messenger::Session::Start() { ReadHeader(); }

namespace {
class IOBufToCBS {
 public:
  explicit IOBufToCBS(std::unique_ptr<const ebbrt::IOBuf>&& buf)
      : buf_(std::move(buf)) {
    buf_vec_.reserve(buf_->ComputeChainDataLength());
    for (auto& b : *buf_) {
      buf_vec_.emplace_back(b.Data(), b.Length());
    }
  }

  typedef boost::asio::const_buffer value_type;
  typedef std::vector<boost::asio::const_buffer>::const_iterator const_iterator;

  const_iterator begin() const { return buf_vec_.cbegin(); }
  const_iterator end() const { return buf_vec_.cend(); }

 private:
  std::shared_ptr<const ebbrt::IOBuf> buf_;
  std::vector<boost::asio::const_buffer> buf_vec_;
};
}  // namespace

ebbrt::Future<void>
ebbrt::Messenger::Session::Send(std::unique_ptr<IOBuf>&& data) {
  auto p = new Promise<void>();
  auto ret = p->GetFuture();
  auto self(shared_from_this());

  boost::asio::async_write(
      socket_, IOBufToCBS(std::move(data)),
      EventManager::WrapHandler(
          [p, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
              p->SetValue();
              delete p;
            }
          }));
  return ret;
}

void ebbrt::Messenger::Session::ReadHeader() {
  auto self(shared_from_this());
  boost::asio::async_read(
      socket_,
      boost::asio::buffer(static_cast<void*>(&header_), sizeof(header_)),
      EventManager::WrapHandler([this, self](
          const boost::system::error_code& ec, std::size_t length) {
        if (!ec) {
          ReadMessage();
        }
      }));
}

void ebbrt::Messenger::Session::ReadMessage() {
  auto size = header_.length;
  if (size <= 0) {
    throw std::runtime_error("Request to read undersized message!");
  }

  auto buf = MakeUniqueIOBuf(size).release();

  auto self(shared_from_this());
  boost::asio::async_read(
      socket_, boost::asio::buffer(buf->MutData(), size),
      EventManager::WrapHandler([this, buf, size, self](
          const boost::system::error_code& ec, std::size_t /*length*/) {
        if (!ec) {
          auto& ref = GetMessagableRef(header_.id, header_.type_code);
          ref.ReceiveMessageInternal(
              NetworkId(socket_.remote_endpoint().address().to_v4()),
              std::unique_ptr<MutIOBuf>(buf));
        }
        ReadHeader();
      }));
}

ebbrt::Messenger::NetworkId ebbrt::Messenger::LocalNetworkId() {
  auto net_addr = node_allocator->GetNetAddr();
  return NetworkId(boost::asio::ip::address_v4(net_addr));
}
