#include <cstdlib>                                                                                                            
#include <deque>                                                                                                              
#include <iostream>                                                                                                           
#include <thread>
#include <string>                                                                                                         
#include <boost/asio.hpp>                                                                                                     
                                                                                                                              
#include "message.hpp"  
#include "chat_client.hpp"  

#define max_idle_time 3
                                                                                                                        
using boost::asio::ip::tcp;                                                                                                   
                                                                                                                              
using chat_message_queue = std::deque<chat_message>;      
using std::string;
using std::cout;
using std::endl;

boost::posix_time::ptime last_active_time;

bool check_log_name(char ch){
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= '0' && ch <= '9') return true;
    return false;
}
 
int main(int argc, const char * argv[]) {
    
    try {
        if (argc != 3) {
            std::cerr << "Usage: chat_client <host> <port>\n";
            return 1;
        }
 
        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({argv[1], argv[2]});
        chat_client client(io_service, endpoint_iterator);
 
        std::thread thr([&io_service](){
            io_service.run();//这样，与io_service绑定的事件源的回调均在子线程上执行（这里指的是boost::asio::async_xxx中的lambda函数）。
        });
 
        string line;
        while (std::getline(std::cin, line)) {
            boost::posix_time::time_duration idle_time = boost::posix_time::microsec_clock::local_time() - last_active_time;
            
            // 如果两次消息之间的时间间隔小于指定的阈值，停止向聊天室server发送消息
            if (idle_time.total_seconds() < max_idle_time)  {
                cout<< "Message interval too short" << endl;
                continue;
            }
            last_active_time = boost::posix_time::microsec_clock::local_time();
            string op = "";
            int pos = 0;
            int len = line.length();
            while(pos < len && line[pos] != ' ')
                op += line[pos], pos++;
            if (op == "-login"){
                string name = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && check_log_name(line[pos]))
                    name += line[pos], pos++;
                if (pos != len){
                    cout << "Invalid name: The name can contain only uppercase and lowercase letters or numbers" << endl;
                    continue;
                }
                chat_message msg;
                if (name.length() == 0){
                    cout << "Illegal name: You did not enter a name" << endl;
                    continue;
                }
                if (name.length() >= msg.sender_name_length){
                    cout << "Illegal name: The length of the name must not exceed " 
                         << msg.sender_name_length - 1 << endl;
                    continue;
                }
                msg.encode_header(name, "", "", LOGIN);
                user_name = name;
                login_state = true;
                client.write(msg);
            }
            else if (op == "-logout"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string name = user_name;
                login_state = false;
                user_name = "anonymous";
                chat_message msg;
                msg.encode_header(name, "", "", LOGOUT);
                client.write(msg);
            }
            else if (op == "-send"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string receiver = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    receiver += line[pos], pos++;
                string content = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len) content += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-send' is incorrect. The format is as follows: -send <receiver_name> <message>" << endl;
                    continue;
                }
                chat_message msg;
                if (receiver.length() == 0 || receiver.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The recipient's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                if (content.length() == 0 || content.length() >= msg.max_body_length){
                    cout << "Illegal message: The message is the wrong length, which is need in (0," << msg.max_body_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, receiver, content, SEND);
                client.write(msg);
            }
            else if (op == "-help"){
                cout << "==========================================================================" << endl;
                cout << "If you want to log in, you can use: -login <user_name>" << endl;
                cout << "If you want to log out, you can use: -logout" << endl;
                cout << "If you want to send message, you can use: -send <receiver_name> <message>" << endl;
                cout << "If you query all online user members, you can use: -list" << endl;
                cout << "If you look up the history, you can use: -history <receiver_name>" << endl;
                cout << "==========================================================================" << endl;
            }
            else if (op == "-list"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                chat_message msg;
                msg.encode_header(user_name, "", "", LIST);
                client.write(msg);
            }
            else if (op == "-history"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string receiver = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    receiver += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-history' is incorrect. The format is as follows: -history <receiver_name>" << endl;
                    continue;
                }
                chat_message msg;
                if (receiver.length() == 0 || receiver.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The recipient's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, receiver, "", HISTORY);
                client.write(msg);
            }
            else if (op == "-add_group"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string name = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    name += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-add_group' is incorrect. The format is as follows: -add_group <group_name>" << endl;
                    continue;
                }
                chat_message msg;
                if (name.length() == 0 || name.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The group's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, name, "", ADDGROUP);
                client.write(msg);
            }
            else if (op == "-enter_group"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string name = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    name += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-enter_group' is incorrect. The format is as follows: -enter_group <group_name>" << endl;
                    continue;
                }
                chat_message msg;
                if (name.length() == 0 || name.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The group's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, name, "", ENTERGROUP);
                client.write(msg);
            }
            else if (op == "-send_group"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string receiver = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    receiver += line[pos], pos++;
                string content = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len) content += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-send_group' is incorrect. The format is as follows: -send_group <group_name> <message>" << endl;
                    continue;
                }
                chat_message msg;
                if (receiver.length() == 0 || receiver.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The group's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                if (content.length() == 0 || content.length() >= msg.max_body_length){
                    cout << "Illegal message: The message is the wrong length, which is need in (0," << msg.max_body_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, receiver, content, SENDGROUP);
                client.write(msg);
            }
            else if (op == "-history_group"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string name = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    name += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-history_group' is incorrect. The format is as follows: -history_group <group_name>" << endl;
                    continue;
                }
                chat_message msg;
                if (name.length() == 0 || name.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The group's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, name, "", HISTORYGROUP);
                client.write(msg);
            }
            else if (op == "-leave_group"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string name = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    name += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-leave_group' is incorrect. The format is as follows: -leave_group <group_name>" << endl;
                    continue;
                }
                chat_message msg;
                if (name.length() == 0 || name.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The group's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, name, "", LEAVEGROUP);
                client.write(msg);
            }
            else if (op == "-list_group"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                chat_message msg;
                msg.encode_header(user_name, "", "", LISTGROUP);
                client.write(msg);
            }
            else if (op == "-list_group_member"){
                if (!login_state){
                    cout << "You're not logged in" << endl;
                    continue;
                }
                string sender = user_name;
                string name = "";
                while (pos < len && line[pos] == ' ') pos++;
                while (pos < len && line[pos] != ' ')
                    name += line[pos], pos++;
                if (pos != len){
                    cout << "The format of '-list_group_member' is incorrect. The format is as follows: -list_group_member <group_name>" << endl;
                    continue;
                }
                chat_message msg;
                if (name.length() == 0 || name.length() >= msg.receiver_name_length){
                    cout << "Illegal name: The group's name is the wrong length, which is need in (0," << msg.receiver_name_length << ")" << endl;
                    continue;
                }
                msg.encode_header(sender, name, "", LISTGROUPMEMBER);
                client.write(msg);
            }
            else{
                cout << "Irregular operation: you can use '-help' for more information" << endl;
                continue;
            }
        }
 
        client.close();//这里必须close，否则子线程中run不会退出，因为boost::asio::async_read事件源一直注册在io_service中.
        thr.join();
 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}

 