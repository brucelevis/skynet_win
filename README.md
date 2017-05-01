## Skynet
Skynet is a lightweight online game framework, and it can be used in many other fields.  
官方仓库 https://github.com/cloudwu/skynet  

## Windows版介绍
* 此版本使用VS_Clang工具集编译所以推荐使用vs2017(vs2015没试过),优点是支持C99只需修改极少量代码即可完成Win版重构  
* 增加了几个模块cjson protobuf ltask  

## 修改原版的位置
[socket_server.c] struct socket 结构将成员type类型改为uint32_t(兼容原子操作函数);open_socket()函数将 connect 与 sp_nonblocking 处位置调换.  

## 运行测试

## 参考代码
https://github.com/dpull/skynet-mingw  
https://github.com/sanikoyes/skynet/tree/vs2013  

## 帮助文档
* 在线帮助:https://github.com/cloudwu/skynet/wiki  
* FAQ: https://github.com/cloudwu/skynet/wiki/FAQ  
