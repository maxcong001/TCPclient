
#include "logger/logger.hpp"
#include <sys/socket.h> /*  socket definitions        */
#include <sys/types.h>  /*  socket types              */
#include <arpa/inet.h>  /*  inet (3) funtions         */
#include <unistd.h>     /*  misc. UNIX functions      */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcpClient/tcpClient.h"
#include <thread>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class session
    : public std::enable_shared_from_this<session>
{
  public:
    session(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }

  private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                [this, self](boost::system::error_code ec, std::size_t length) {
                                    if (!ec)
                                    {
                                        do_write(length);
                                    }
                                });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                                 [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                     if (!ec)
                                     {
                                         do_read();
                                     }
                                 });
    }

    tcp::socket socket_;
    enum
    {
        max_length = 1024
    };
    char data_[max_length];
};

class server
{
  public:
    server(boost::asio::io_service &io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
          socket_(io_service)
    {
        do_accept();
    }

  private:
    void do_accept()
    {
        acceptor_.async_accept(socket_,
                               [this](boost::system::error_code ec) {
                                   if (!ec)
                                   {
                                       std::make_shared<session>(std::move(socket_))->start();
                                   }

                                   do_accept();
                               });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int echoServer()
{
    try
    {
        boost::asio::io_service io_service;

        server s(io_service, 2002);

        io_service.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
int main()
{
    set_log_level(logger_iface::log_level::debug);

    std::thread server_thread(echoServer);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::shared_ptr<Loop> loop_sptr(new Loop);
    loop_sptr->init(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::shared_ptr<tcpClient> client_sptr = std::make_shared<tcpClient>(loop_sptr);

    CONN_INFO ip_info;
    ip_info.ip = "127.0.0.1";
    ip_info.port = "2002";
    std::string test("this is a test...");
    client_sptr->post_connect(ip_info);
    while (!client_sptr->isConnected())
    {
        __LOG(debug, "connection is not ready");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    for (int i = 0; i < 5; i++)
    {
        __LOG(debug, "now send message");
        if (!client_sptr->send(test.c_str(), test.size()))
        {
            __LOG(error, "send fail!!!");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    client_sptr->disconnect();
    for (int i = 0; i < 5; i++)
    {
        __LOG(debug, "now send message");
        if (!client_sptr->send(test.c_str(), test.size()))
        {
            __LOG(error, "send fail!!!");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    __LOG(debug, "now exiting!");
    client_sptr->post_connect(ip_info);
    client_sptr->send(test.c_str(), test.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    server_thread.join();
}
