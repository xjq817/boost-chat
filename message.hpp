 
//下面是消息的头文件
#ifndef chat_message_hpp                                                                                                      
#define chat_message_hpp                                                                                                      
                                                                                                                              
#include <cstdio>                                                                                                             
#include <cstdlib>                                                                                                            
#include <cstring> 
#include <string>     
#define HISTORY         1
#define LOGIN           2
#define LOGOUT          3
#define LIST            4
#define SEND            5
#define LOGINFAIL       6
#define LOGINOK         7
#define LOGOUTOK        8
#define ADDGROUP        9
#define ENTERGROUP      10
#define SENDGROUP       11
#define HISTORYGROUP    12
#define LEAVEGROUP      13
#define LISTGROUP       14
#define LISTGROUPMEMBER 15

using std::size_t;
using std::string;

//承载内容的消息包的类：包含header和body（类似于http协议），header包含了body相关属性，比如这里用固定的4个字节保存了body中字节个数                                                                                                                          
class chat_message {                                                                                                          
                                                                                                                              
public:
    enum {sender_name_length = 32, receiver_name_length = 32, type_length = 4};                                                                                                                       
    enum {header_length = sender_name_length + receiver_name_length + type_length};                                                                               
    enum {max_body_length = 512};//body最大字节个数                                                                        
                                                                                                                              
    chat_message():m_data(""){}
    ~chat_message(){}
    
    //信息字节数组
    char* data(){return m_data;};
    
    //信息字节数组中有效长度，即头+体的总长度
    size_t length(){return header_length + max_body_length;}
    
    //获取指向 body 的指针
    char* body(){return m_data + header_length;}
 
    //获取 body 长度
    size_t body_length(){return max_body_length;}
    
    
    //解码头信息
    bool decoder_header(){
        char *data = m_data;
        this->sender = "";
        for (int i = 0; i < sender_name_length; i++){
            if (data[i] == ' ') break;
            this->sender += data[i];
        }
        data += sender_name_length;
        
        this->receiver = "";
        for (int i = 0; i < receiver_name_length; i++){
            if (data[i] == ' ') break;
            this->receiver += data[i];
        }
        data += receiver_name_length;
        
        this->message_type = data[0];
        data += type_length;
        
        size_t len = strlen(data);
        this->content = "";
        for (int i = 0; i < len; i++){
            this->content += data[i];
        }
        return true;
    }
    
    //编码头信息
    void encode_header(string sender, string receiver, string content, int type){
        char *data = m_data;
        size_t sender_len = sender.length();
        for (int i = 0; i < sender_len; i++) data[i] = sender[i];
        data[sender_len] = ' ';
        this->sender = sender;
        data += sender_name_length;
        
        size_t receiver_len = receiver.length();
        for (int i = 0; i < receiver_len; i++) data[i] = receiver[i];
        data[receiver_len] = ' ';
        this->receiver = receiver;
        data += receiver_name_length;

        data[0] = type;
        this->message_type = type;
        data += type_length;

        size_t content_len = content.length();
        for (int i = 0; i < content_len; i++) data[i] = content[i];
        this->content = content;
        data[content_len] = '\0';
    }

    string receiver_name(){
        return receiver;
    }

    string sender_name(){
        return sender;
    }
    
    string chat_content(){
        return content;
    }

    int type(){
        return message_type;
    }
    
private:
    int message_type;
    string receiver;
    string sender;
    string content;
    char m_data[header_length + max_body_length];//用来存放消息：固定的前4个字节保存了body中字节个数，接着是body信息
};
 
#endif /* chat_message_hpp */
 
 
