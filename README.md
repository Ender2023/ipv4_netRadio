## How to use this repository
### 编译
```shell
git clone https://github.com/missFuture/IPv4_streaming_media.git
make
```
### 设置本地媒体库
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
**注**：频道目录channel_x(channel_1, channel_2, channel_3...)位于 **/var** 目录下（如果不存在需要自己创建），每一个频道中包含需要播放的媒体文件（mp3），以及该频道以及该频道的描述信息 **desc.txt** ，否则服务端会报错并将错误信息写入系统日志**syslog**