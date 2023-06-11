#ifndef chat_server_hpp                                                                                                      
#define chat_server_hpp   
#include <cstdlib>                                                                                                            
#include <deque>                                                                                                              
#include <iostream>                                                                                                           
#include <list>                                                                                                               
#include <memory>                                                                                                             
#include <set>                                                                                                                
#include <utility>       
#include <string>
#include <vector>                                                                                                     
#include <boost/asio.hpp>                                                                                                     
                                                                                                                              
#include "message.hpp"                                                                                                   
                                                                                                                              
using boost::asio::ip::tcp;                                                                                                   
                                                                                                                                                                                 
using chat_message_queue = std::deque<chat_message>;    
using std::string;    
using std::pair;
using std::vector;
using std::cout;
using std::endl;                                                                 
                                                                                                                              
//聊天成员                                                                                                                    
class chat_participant {                                                                                                      
public:
    string name;                                                                                                    
    virtual ~chat_participant(){}                                                                                             
    virtual void deliver(const chat_message &msg) = 0;                                                                        
};                                                                                                                            
                                                                                                                              
typedef std::shared_ptr<chat_participant> chat_participant_ptr;

class Chat_group {
private:
    string name;
    std::map<string, chat_participant_ptr> participants;
    std::deque<string> chat_history;
    int member_number;
    enum{max_recent_msgs = 100, max_member_number = 100};
public:
    string get_name(){
        return name;
    }
    void init(string name){
        this->name = name;
        this->member_number = 0;
    }
    bool member_number_limit(){
        return member_number < max_member_number;
    }
    vector<string> get_participants(){
        vector<string> participant_name;
        for (auto par_pair: participants){
            participant_name.push_back(par_pair.first);
        }
        return participant_name;
    }
    std::deque<string> get_chat_history(){
        return chat_history;
    }
    bool participant_existence(string name){
        return participants.count(name) != 0;
    }
    bool add_participant(string name, chat_participant_ptr ptr){
        if (participants.count(name)) return false;
        if (!member_number_limit()) return false;
        participants[name] = ptr;
        member_number++;
        return true;
    }
    bool remove_participant(string name){
        if (!participants.count(name)) return false;
        auto it = participants.find(name);
        participants.erase(it);
        member_number--;
        return true;
    }
    void add_chat_history(string message){
        if (chat_history.size() == max_recent_msgs)
            chat_history.pop_front();
        chat_history.push_back(message);
    }
};
                                                                                                                              
//聊天室                                                                                                                      
class chat_room {                                                                                                             
    //聊天室中所有成员                                                                                                        
    std::set<chat_participant_ptr> participants;
    std::map<pair<string,string>, std::deque<string>> chat_history;                                                                       
    //历史消息 每个聊天记录最多100条                                                                                                     
    enum{max_recent_msgs = 100};                                                                                       
    std::map<string, chat_participant_ptr> online_users;
    std::map<string, Chat_group> groups;

public:
    enum{ok = 1, already_login = 2, no_accout_login = 3, max_list = 100}; 
    string server_name = "__server__";      
                                                                                     
                                                                                                                              
public:                                                                                                                       
    //成员进入聊天室                                                                                                          
    void join(chat_participant_ptr participant){                                                                              
        participants.insert(participant);                                                                                                                                                                                                   
    }
    int login(string name, chat_participant_ptr participant){
        if(online_users.count(name))return already_login;
        online_users[name] = participant;
        participant->name = name;
        return ok;
    }
    int logout(string name){
        if(online_users.count(name) == 0) return no_accout_login;
        return ok;
    }
    void leave(chat_participant_ptr participant){
        participants.erase(participant);
        string name = participant->name;
        auto it = online_users.find(name);
        if (it != online_users.end()) online_users.erase(it);
    }
    vector<string> online_user_list(string name){
        vector<string> user_list;
        user_list.push_back("online user list:");

        for(auto user_pair : online_users){
            string cur = user_pair.first;
            if(cur == name)continue;
            if (user_list.size() > max_list){
                user_list.push_back("...");
                break;
            }
            user_list.push_back(cur);
        }
        return user_list;
    }
    vector<string> group_list(){
        vector<string> group_list;
        group_list.push_back("group list:");

        for(auto group_pair : groups){
            string cur = group_pair.first;
            if (group_list.size() > max_list){
                group_list.push_back("...");
                break;
            }
            group_list.push_back(cur);
        }
        return group_list;
    }
    vector<string> group_member_list(string name){
        vector<string> group_list;
        Chat_group group = groups[name];
        group_list = group.get_participants();
        if (group_list.size() > max_list)
            group_list.resize(max_list),group_list.push_back("...");
        return group_list;
    }
    bool find_online_user(string name){
        return online_users.count(name) != 0;
    }
    bool find_history_user(string sender, string receiver){
        return chat_history.count(make_pair(sender, receiver)) != 0;
    }
    //传递消息
    void deliver(chat_message msg){
        msg.decoder_header();
        string sender = msg.sender_name();
        string receiver = msg.receiver_name();
        string content = msg.chat_content();
        if (msg.type() == SENDGROUP){
            Chat_group group = groups[receiver];
            group.add_chat_history(sender + ": " + content);
            vector<string> members = group.get_participants();
            for (string member: members){
                if (find_online_user(member)){
                    string group_content = "[" + receiver + "] " + sender + ": " + content;
                    msg.encode_header(sender, member, group_content, SEND);
                    online_users[member]->deliver(msg);
                }
            }
            groups[receiver] = group;
        }
        else{
            if (sender != server_name){
                string content = sender + ": " + msg.chat_content();
                msg.encode_header(sender, receiver, content, msg.type());
                pair<string,string> sender_his = make_pair(sender, receiver);
                pair<string,string> receiver_his = make_pair(receiver, sender);
                if(chat_history[sender_his].size() == max_recent_msgs)
                    chat_history[sender_his].pop_front();
                chat_history[sender_his].push_back(content);
                    
                if(chat_history[receiver_his].size() == max_recent_msgs)
                    chat_history[receiver_his].pop_front();
                chat_history[receiver_his].push_back(content);
            }
            online_users[receiver]->deliver(msg);
        }
        if (msg.type() == LOGOUTOK){
            auto it = online_users.find(receiver);
            online_users.erase(it);
        }
    }
    std::deque<string> history(string sender, string receiver){
        return chat_history[make_pair(sender, receiver)];
    }
    bool group_existence(string name){
        return groups.count(name) != 0;
    }
    void add_new_group(string name){
        Chat_group new_group;
        new_group.init(name);
        groups[name] = new_group;
    }
    bool add_new_group_member(string group_name, string member_name){
        Chat_group group = groups[group_name];
        if (!group.add_participant(member_name, online_users[member_name])) return false;
        groups[group_name] = group;
        return true;
    }
    bool remove_group_member(string group_name, string member_name){
        Chat_group group = groups[group_name];
        if (!group.remove_participant(member_name)) return false;
        groups[group_name] = group;
        return true;
    }
    bool in_group(string group_name, string member_name){
        Chat_group group = groups[group_name];
        return group.participant_existence(member_name);
    }
    std::deque<string> history_group(string group_name){
        return groups[group_name].get_chat_history();
    }
};
 
//每个 client 一个 chat_session 
class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session>{
 
    tcp::socket m_socket;//与客户端建立连接的socket
    chat_room &m_room;  //所属聊天室
    chat_message read_msg;//当前消息
    chat_message_queue m_write_msgs;//写给客户端的消息队列
    int session_id;
public:
    chat_session(boost::asio::io_service &io_service, chat_room &room):
        m_socket(io_service), m_room(room){
        
    }
    
    tcp::socket& socket(){
        return m_socket;
    }
                    
    void start(){
        m_room.join(shared_from_this());//聊天室增加成员
        do_read_header();//读取成员传递过来的消息头
    }
    
    virtual void deliver(const chat_message &msg){
        //第一次调用时为false
        bool write_in_progress = !m_write_msgs.empty();
        
        //存放写给客户端的消息队列
        m_write_msgs.push_back(msg);
        
        //防止同时多次调用
        if (!write_in_progress) {
            do_write();//将历史消息写给客户端
        }
    }
    
    //读取成员传递过来的消息头
    void do_read_header(){
        auto sharedFromThis = shared_from_this();
        //异步读取4个字节
        boost::asio::async_read(m_socket,
                                boost::asio::buffer(read_msg.data(), chat_message::header_length),
                                [this, sharedFromThis](boost::system::error_code ec, std::size_t lenght){
            if (!ec && read_msg.decoder_header()) {
                do_read_body();
            }else{
                m_room.leave(sharedFromThis);
            }
            
        });
    }
    
    void parse_message(){
        if (read_msg.type() == LOGIN){
            int ret = m_room.login(read_msg.sender_name(), shared_from_this());

            if(ret == m_room.already_login)
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "Login failure: You are using a name that someone else has used",
                                       LOGINFAIL);
            else{
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "Login success",
                                       LOGINOK);
            }
            m_room.deliver(read_msg);
            
        }
        else if (read_msg.type() == LOGOUT){
            int ret = m_room.logout(read_msg.sender_name());
            if(ret == m_room.no_accout_login)
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "Error: No existing logged-in account",
                                       SEND);
            else{
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "Logout Success",
                                       LOGOUTOK);
            }
            m_room.deliver(read_msg);
            
        }
        else if (read_msg.type() == LIST){
            vector<string> online_user_list = m_room.online_user_list(read_msg.sender_name());
            string receiver = read_msg.sender_name();
            for (string str: online_user_list){
                // cout << str << endl;
                read_msg.encode_header(m_room.server_name,
                                       receiver,
                                       str,
                                       SEND);
                m_room.deliver(read_msg);
            }
        }
        else if (read_msg.type() == SEND){
            if(!m_room.find_online_user(read_msg.receiver_name()))
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "Error: Chat partner does not exist",
                                       SEND);
            
            else
                read_msg.encode_header(read_msg.sender_name(),
                                       read_msg.receiver_name(), 
                                       read_msg.chat_content(), 
                                       SEND);    
            m_room.deliver(read_msg);
        }
        else if (read_msg.type() == HISTORY){
            string sender = read_msg.sender_name();
            string receiver = read_msg.receiver_name();
            if(!m_room.find_history_user(sender, receiver)){
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "You don't seem to have talked with him or her",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else{
                std::deque<string> que = m_room.history(sender, receiver);
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "Chat history between " + sender + " and " + receiver + " is:",
                                       SEND);
                for (auto str:que){
                    read_msg.encode_header(m_room.server_name,
                                           sender,
                                           str,
                                           SEND);
                    m_room.deliver(read_msg);
                }
            }
        }
        else if (read_msg.type() == ADDGROUP){
            string sender = read_msg.sender_name();
            string name = read_msg.receiver_name();
            if (m_room.group_existence(name)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "The group chat already exists",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else{
                m_room.add_new_group(name);
                m_room.add_new_group_member(name, sender);
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "Add group successfully",
                                       SEND);
                m_room.deliver(read_msg);
            }
        }
        else if (read_msg.type() == ENTERGROUP){
            string sender = read_msg.sender_name();
            string name = read_msg.receiver_name();
            if (!m_room.group_existence(name)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "The group chat does not exist",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else if (m_room.in_group(name, sender)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "You have already in the group",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else{
                m_room.add_new_group_member(name, sender);
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "Enter group successfully",
                                       SEND);
                m_room.deliver(read_msg);
            }
        }
        else if (read_msg.type() == SENDGROUP){
            string sender = read_msg.sender_name();
            string name = read_msg.receiver_name();
            if (!m_room.group_existence(name)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "The group chat does not exist",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else
                m_room.deliver(read_msg);
        }
        else if (read_msg.type() == HISTORYGROUP){
            string sender = read_msg.sender_name();
            string name = read_msg.receiver_name();
            if (!m_room.group_existence(name)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "The group chat does not exist",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else if (!m_room.in_group(name, sender)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "You are not in the group",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else{
                std::deque<string> que = m_room.history_group(name);
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "Chat history in group " + name + " is:",
                                       SEND);
                for (auto str:que){
                    read_msg.encode_header(m_room.server_name,
                                           sender,
                                           str,
                                           SEND);
                    m_room.deliver(read_msg);
                }
            }
        }
        else if (read_msg.type() == LEAVEGROUP){
            string sender = read_msg.sender_name();
            string name = read_msg.receiver_name();
            if (!m_room.group_existence(name)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "The group chat does not exist",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else if (!m_room.in_group(name, sender)){
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "You are not in the group",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else{
                m_room.remove_group_member(name, sender);
                read_msg.encode_header(m_room.server_name,
                                       sender,
                                       "Leave group successfully",
                                       SEND);
                m_room.deliver(read_msg);
            }
        }
        else if (read_msg.type() == LISTGROUP){
            vector<string> group_list = m_room.group_list();
            string receiver = read_msg.sender_name();
            for (string str: group_list){
                read_msg.encode_header(m_room.server_name,
                                       receiver,
                                       str,
                                       SEND);
                m_room.deliver(read_msg);
            }
        }
        else if (read_msg.type() == LISTGROUPMEMBER){
            string name = read_msg.receiver_name();
            if (!m_room.group_existence(name)){
                read_msg.encode_header(m_room.server_name,
                                       read_msg.sender_name(),
                                       "The group chat does not exist",
                                       SEND);
                m_room.deliver(read_msg);
            }
            else{
                vector<string> group_list = m_room.group_member_list(name);
                string receiver = read_msg.sender_name();
                for (string str: group_list){
                    read_msg.encode_header(m_room.server_name,
                                           receiver,
                                           str,
                                           SEND);
                    m_room.deliver(read_msg);
                }
            }
        }
    }

    //读取成员传递过来的消息体
    void do_read_body(){
        auto sharedFromThis = shared_from_this();
        boost::asio::async_read(m_socket,
                                boost::asio::buffer(read_msg.body(), read_msg.body_length()),
                                [this, sharedFromThis](boost::system::error_code ec, std::size_t lenght){
            if (!ec) {
                read_msg.decoder_header();
                parse_message();
                do_read_header();
            }else{
                m_room.leave(sharedFromThis);
            }
        });
    }
    
    //将m_write_msgs中的第一条写给客户端
    void do_write(){
        auto sharedFromThis = shared_from_this();
        boost::asio::async_write(m_socket,
                                 boost::asio::buffer(m_write_msgs.front().data(), m_write_msgs.front().length()),
                                 [this, sharedFromThis](boost::system::error_code ec, std::size_t length){
            if (!ec) {
                m_write_msgs.pop_front();
                if (!m_write_msgs.empty()) {
                    do_write();
                }
            }else{
                m_room.leave(sharedFromThis);
            }
        });
    }
};
 
//聊天服务器，当接受客户端发送过来的请求后，就会创建一个chat_session，用它读写消息
class Chat_server {
    
    boost::asio::io_service &m_io_service;
    tcp::acceptor m_acceptor;
    //tcp::socket m_socket;//为客户端提供的,便于建立socket连接(服务端socket<---->客户端socket)
    chat_room m_room;//每个chat_server都有一个room
    
public:
    Chat_server(boost::asio::io_service &io_service,
                const tcp::endpoint &endpoint):m_io_service(io_service),
    m_acceptor(io_service, endpoint){
        do_accept();
    }
    
    //每加入一个客户端，就创建一个 session 对象
    void do_accept(){
        auto session = std::make_shared<chat_session>(m_io_service, m_room);
        
        m_acceptor.async_accept(session->socket(), [this, session](boost::system::error_code ec){
            if(!ec){
                session->start();
            }
            
            do_accept();
        });
    }
    
};
 
#endif
 
