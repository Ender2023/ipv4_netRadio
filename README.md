### 概要
server在创建完socket和更新完媒体库信息后，会调用Thread_List_Create()函数创建节目单线程，从而每秒向多播地址发送当前节目单信息，之后继续调用Thread_Channel_Create()函数创建n个频道线程，从而向多播地址发送各个频道的音乐信息，server端的流量控制采用令牌桶方式。

client在创建完socket后先接收节目单信息并打印，待用户选择好需要收听的频道后从诸多数据报中筛选所需报文，并将得到的数据送子进程(mp3解码器)进行播放，默认解码器使用mpg123(若未安装客户端会提示出错)。

### 设置本地媒体库（如果找不到本地媒体库，服务端会退出）
/var/NetRadio/<br/>
└── medialib<br/>
    ├── channel_1<br/>
    │   ├── desc.txt<br/>
    │   └── Episode 1 - Smitty Mamba.mp3<br/>
    ├── channel_3<br/>
    │   ├── desc.txt<br/>
    │   └── Shooting Stars (Instrumental) - Bag Raiders.mp3<br/>
    ├── channel_4<br/>
    │   ├── desc.txt<br/>
    │   └── Yellow wagon - 出羽良彰.mp3<br/>
    └── channel_5<br/>
        ├── desc.txt<br/>
        └── ひやむぎ、そーめん、时々うどん - ハム,Foxtail-Grass Studio.mp3<br/>

**注**：频道目录channel_x(channel_1, channel_2, channel_3...)需要事先位于 **/var/NetRadio/medialib** 目录下（如果不存在需要创建），并给予可执行权限，每一个频道中包含需要播放的媒体文件（mp3），以及该频道以及该频道的描述信息 **desc.txt** ，否则服务端会报错并将错误信息写入系统日志**syslog**

## 编译
在源码顶层目录下执行
`make && make install`
在install目录中可以找到server.elf和client.elf

## 运行

运行服务端：
`./install/server.elf -f`

运行客户端：
`./install/client.elf`

收到节目单后选择频道即可。

        

