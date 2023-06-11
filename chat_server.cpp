#include "chat_server.hpp"
#include <cstdlib>                                                                                                            
#include <deque>                                                                                                              
#include <iostream>                                                                                                           
#include <list>    
#include <map>                                                                                                           
#include <memory>                                                                                                             
#include <set>                                                                                                                
#include <utility>    
#include <string>                                                                                                           
#include <boost/asio.hpp> 

using boost::asio::ip::tcp;

int main(int argc, const char * argv[]){
    try{
        if(argc < 2){
            std::cerr << "Usage: chat server <port> [<port>...], please at least enter one port\n";
            return 1;
        }

        boost::asio::io_service io_service;

        std::string host = boost::asio::ip::host_name();
        std::cout << "Hostname: " << host << std::endl;

        std::list<Chat_server*> servers; //how many servers depend on arg
        for(int i = 1; i < argc; i++){
            tcp::endpoint endpoint(tcp::v4(),std::atoi(argv[i])); //tcp::v4()：这是一个函数调用，返回一个表示 IPv4 协议的对象。它用于指定创建的端点使用 IPv4。
            servers.push_back(new Chat_server(io_service,endpoint));
        }

        io_service.run();

    } catch(std::exception &e){
        std::cout << e.what() << std::endl;
    }
    return 0;
}