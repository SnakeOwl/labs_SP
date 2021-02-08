#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

using boost::asio::ip::tcp;
namespace bp = boost::process;

std::string make_daytime_string()
{
    std::time_t now = std::time(0);
    return std::ctime(&now);
}

int main()
{
    try
    {
        const short buf_size = 128;
        /* Any program that uses asio need to have at least one io_service object */
        boost::asio::io_service io_service;
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 13)); /* 13 - port*/
        for (;;)
        {
            tcp::socket socket(io_service);
            acceptor.accept(socket);

            /***** receive a message from a client *****/
            boost::array<char, buf_size> buf;
            boost::system::error_code error;
            size_t len = socket.read_some(boost::asio::buffer(buf), error);
            std::cout << "message received: " << buf.data() << std::endl;

            /***** invoke process *****/
            std::string command = buf.data();
            bp::ipstream pipe_stream;
            bp::child process(command, bp::std_out > pipe_stream);
            command.clear();
            buf.fill('\0');

            std::string line = "";
            std::string message_to = "";

            while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
            {
                message_to += line + "\n";
            }

            /***** sent message to the client *****/
            boost::system::error_code ignored_error;
            boost::asio::write(socket, boost::asio::buffer(message_to), ignored_error);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}