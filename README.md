# boost-chat

## 用户类

用户登录：` -login <user_name> `  （名字不能超过32，且其组成只为大小写英文或数字）

用户登出：`-logout`

用户发送消息：`-send <receiver_name> <message>` （message长度不超过512，下面同理）

查询帮助：`-help`

查询所有在线用户：`-list` （当人数过多时，只输出100人，并以...结尾）

查询历史记录：`-history <receiver_name>` （只保留最新的100条历史记录）

## 群聊

创建群聊：`-add_group <group_name>` （名字不能超过32，且其组成只为大小写英文或数字）

加入群聊：`-enter_group <group_name>` （群聊人数有上限，上限为100）

用户在群聊中发送消息：`-send_group <group_name> <message>`

查询群聊中的历史记录：`-history_group <group_name>` （只保留最新的100条历史记录）

退群：`-leave_group <group_name>`

查找所有群聊：`-list_group` （当群数过多时，只输出100群名，并以...结尾）

查找群聊中的所有人：`-list_group_member <group_name>` （当群人数过多时，只输出100人，并以...结尾）

client 下线会将当前用户自动下线

**DDos攻击** 

- server会记录每个client的ip地址，如果某个client在短时间内发送大量的请求，server会把这个client的ip地址加入到黑名单中，不再接受其请求。
- 与 `LIST` 有关的指令都限制了长度，避免造成数量过多而输出过久的情况