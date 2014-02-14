//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Messenger.h>

#include <ebbrt/GlobalIdMap.h>

namespace bai = boost::asio::ip;

ebbrt::Messenger::Messenger()
    : acceptor_(active_context->io_service_,
                bai::tcp::endpoint(bai::address_v4(), 0)),
      socket_(active_context->io_service_) {
  DoAccept();
}

ebbrt::Future<void> ebbrt::Messenger::Send(NetworkId to,
                                           std::shared_ptr<const Buffer> data) {
  auto ip = to.ip_.to_ulong();
  {
    std::lock_guard<std::mutex> lock(m_);
    auto it = connection_map_.find(ip);
    if (it == connection_map_.end()) {
      throw std::runtime_error(
          "UNIMPLEMENTED Messenger::Send create connection");
    }
  }
  return connection_map_[ip].Then(
      MoveBind([](std::shared_ptr<const Buffer> data,
                  Future<Session> f) { return f.Get().Send(std::move(data)); },
               std::move(data)));
}

void ebbrt::Messenger::DoAccept() {
  acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
    if (!ec) {
      auto addr = socket_.remote_endpoint().address().to_v4().to_ulong();
      std::lock_guard<std::mutex> lock(m_);
      if (connection_map_.find(addr) != connection_map_.end())
        throw std::runtime_error("Store to promise");

      connection_map_.emplace(addr,
                              MakeReadyFuture<Session>(std::move(socket_)));
    }
    DoAccept();
  });
}

uint16_t ebbrt::Messenger::GetPort() {
  return acceptor_.local_endpoint().port();
}

ebbrt::Messenger::Session::Session(bai::tcp::socket socket)
    : socket_(std::move(socket)) {
  ReadHeader();
}

namespace {
class BufferToCBS {
 public:
  explicit BufferToCBS(std::shared_ptr<const ebbrt::Buffer> buf)
      : buf_(std::move(buf)) {
    buf_vec_.reserve(buf_->length());
    for (auto& b : *buf_) {
      buf_vec_.emplace_back(b.data(), b.size());
    }
  }

  typedef boost::asio::const_buffer value_type;
  typedef std::vector<boost::asio::const_buffer>::const_iterator const_iterator;

  const_iterator begin() const { return buf_vec_.cbegin(); }
  const_iterator end() const { return buf_vec_.cend(); }

 private:
  std::shared_ptr<const ebbrt::Buffer> buf_;
  std::vector<boost::asio::const_buffer> buf_vec_;
};
}  // namespace

ebbrt::Future<void>
ebbrt::Messenger::Session::Send(std::shared_ptr<const Buffer> data) {
  auto buf = std::make_shared<Buffer>(sizeof(Header));
  auto h = static_cast<Header*>(buf->data());
  h->size = data->total_size();
  buf->append(std::move(std::const_pointer_cast<Buffer>(data)));
  auto p = new Promise<void>();
  auto ret = p->GetFuture();
  boost::asio::async_write(
      socket_, BufferToCBS(std::move(buf)),
      [p](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
          p->SetValue();
          delete p;
        }
      });
  return ret;
}

void ebbrt::Messenger::Session::ReadHeader() {
  boost::asio::async_read(
      socket_,
      boost::asio::buffer(static_cast<void*>(&header_), sizeof(header_)),
      [this](const boost::system::error_code& ec, std::size_t length) {
        if (!ec) {
          ReadMessage();
        }
      });
}

void ebbrt::Messenger::Session::ReadMessage() {
  if (header_.size <= 0) {
    throw std::runtime_error("Request to read undersized message!");
  }

  std::cout << header_.size << std::endl;

  auto buf = malloc(header_.size);
  if (buf == nullptr)
    throw std::bad_alloc();

  boost::asio::async_read(
      socket_, boost::asio::buffer(buf, header_.size),
      [this, buf](const boost::system::error_code& ec, std::size_t length) {
        if (!ec) {
          global_id_map->HandleMessage(
              NetworkId(socket_.remote_endpoint().address().to_v4()),
              Buffer(buf, length, [](void* p, size_t sz) { free(p); }));
        }
        ReadHeader();
      });
}
