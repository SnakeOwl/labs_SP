/*
*   Simple server C++
*/
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <experimental/filesystem>

namespace bf = std::experimental::filesystem;
using boost::asio::ip::tcp;

int main()
{
    const long port = 8000;
    const long buffer_size = 2048;

    try
    {
        /* Any program that uses asio need to have at least one io_service object */
        boost::asio::io_service io_service;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

        for (;;)
        {
            tcp::socket socket(io_service);
            acceptor.accept(socket);
            std::cout << "Server is ready." << std::endl;

            /***** receive a message from a browser *****/
            boost::array<char, buffer_size> buf;
            boost::system::error_code error;

            size_t len = socket.read_some(boost::asio::buffer(buf), error);
            std::string data = buf.data();
            std::cout << data << std::endl << std::endl;

            /**** parsing browser's data ****/
            char file_path[buffer_size]{};
            const std::string get_ = "GET ";
            std::size_t check_url = data.find_first_of(get_);

            if (check_url != std::string::npos)
            {
                std::size_t begin = get_.size();
                std::size_t end = data.find_first_of(" ", get_.size());
                
                 data.copy(file_path, end - begin, begin);
            }

            /***** search the file and making responce_body *****/
            bool error_404 = false;
            std::stringstream response_body;

            response_body << "";
            std::string file_p(file_path);

            if (file_p.size() > 0)
            {
                bf::path file(file_p);
                
                if (bf::exists(file))
                {
                    if (bf::is_regular_file(file))
                    {
                        FILE *fin;
                        char buffer[buffer_size+1];
                        buffer[buffer_size] = '\0';
                        if((fin = fopen(file_p.c_str(), "rb")) == NULL)
                        {
                            response_body << "Can't open file.";
                        }
                        else
                            while (fread(buffer, sizeof(char), buffer_size, fin))
                            {
                                response_body << buffer; 
                            }

                        memset (buffer,'\0',buffer_size);
                        fclose (fin);
                    }
                    else if (bf::is_directory(file))
                    {
                        for (auto x : bf::directory_iterator(file))
                            response_body << "<a href=" << x.path() << ">"<< x.path() << "</a><br>";
                    }
                }
                else
                    error_404 = true;
            }

            /***** sent message to the browser *****/
            boost::system::error_code ignored_error;
            std::stringstream response;
            
            
            if (error_404)
            {
                response_body << "error 404";
                response << "HTTP/1.1 404 Not Found"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html; charset=utf-8\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();
            }
            else
                response << "HTTP/1.1 200 OK\r\n"
                        << "Version: HTTP/1.1\r\n"
                        << "Content-Type: text/html; charset=utf-8\r\n"
                        << "Content-Length: " << response_body.str().length()
                        << "\r\n\r\n"
                        << response_body.str();


    std::string message_to = "it is work!";
            boost::asio::write(socket, boost::asio::buffer(response.str()), ignored_error);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    std::cout  << "Server stopped." << std::endl;
    
    return 0;
}
