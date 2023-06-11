#ifndef chat_client_hpp
#define chat_client_hpp
#include <cstdlib>                                                                                                            
#include <deque>                                                                                                              
#include <iostream>                                                                                                           
#include <thread> 
#include <string>                                                                                                               
#include <boost/asio.hpp>                                                                                                     
                                                                                                                              
#include "message.hpp"                                                                                                   
                                                                                                                              
using boost::asio::ip::tcp;
using std::string;
using std::cout;
using std::endl;
                                                                                                                              
using chat_message_queue = std::deque<chat_message>;  

bool login_state = false;
string user_name = "anonymous";
string server_name = "__server__";

class chat_client{                                                                                                            
    boost::asio::io_service &m_io_service;                                                                                    
    tcp::socket m_socket;                                                                                                     
    chat_message m_read_msg;                                                                                                  
    chat_message_queue m_write_msgs;
public:
    chat_client(boost::asio::io_service &io_service,
                tcp::resolver::iterator endpoint_iterator):
    m_io_service(io_service), m_socket(io_service){
        do_connect(endpoint_iterator);
    }
    
    void write(const chat_message &msg){
        // 让 lambda 在 m_io_service 的 run 所在线程执行
        m_io_service.post([this, msg](){
            bool write_in_progress = !m_write_msgs.empty();
            m_write_msgs.push_back(msg);
            
            if (!write_in_progress) {
                do_write();
            }
        });
    }
    
    void close(){
        m_io_service.post([this](){
            m_socket.close();
        });
    }
    
    void do_connect(tcp::resolver::iterator endpoint_iterator){
        boost::asio::async_connect(m_socket, endpoint_iterator,
                                   [this](boost::system::error_code ec, tcp::resolver::iterator it){
            if (!ec) {
                do_read_header();
            }
        });
    }
    
    void do_read_header(){
        boost::asio::async_read(m_socket,
                                boost::asio::buffer(m_read_msg.data(), chat_message::header_length),
                                [this](boost::system::error_code ec, std::size_t lenght){
            if (!ec && m_read_msg.decoder_header()) {
                if (m_read_msg.type() == LOGINFAIL){
                    login_state = false;
                    user_name = "anonymous";
                    // cout << "Login failure: You are using a name that someone else has used" << endl;
                }
                else if (m_read_msg.type() == LOGINOK){
                    // cout << "Login success :)" << endl;
                }
                do_read_body();
            }else{
                m_socket.close();
            }
        });
    }
    
    void do_read_body(){
        boost::asio::async_read(m_socket,
                                boost::asio::buffer(m_read_msg.body(), m_read_msg.body_length()),
                                [this](boost::system::error_code ec, std::size_t length){
            if (!ec) {
                m_read_msg.decoder_header();
                std::cout << m_read_msg.chat_content() << std::endl;
                
                //继续读取下一条信息的头
                do_read_header();
                
            }else{
                m_socket.close();
            }
        });
    }
    
    void do_write(){
        boost::asio::async_write(m_socket,
                                 boost::asio::buffer(m_write_msgs.front().data(), m_write_msgs.front().length()),
                                 [this](boost::system::error_code ec, std::size_t length){
            if (!ec) {
                m_write_msgs.pop_front();
                if (!m_write_msgs.empty()) {
                    do_write();
                }
            }else{
                m_socket.close();
            }
        });
    }
    
};
#endif
