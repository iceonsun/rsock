
### 简介

本程序仅仅是加速，不是vpn，目前须搭配kcptun使用。程序的目的是，防止isp对udp流量的qos。目前仅支持mac（包括其他Unix）和Linux。kcptun的简介和使用见[这里](https://github.com/xtaci/kcptun).

### 安装指南

64位Linux和64位Mac已经预编译好了。可以直接下载二进制。点击[这里](https://github.com/iceonsun/rsock/releases).

其他平台可以自己下载源码进行编译。rsock依赖的第三方库有：libuv, libnet, libpcap。

以ubuntu为例：
`sudo apt-get install g++ libuv libnet libpcap
git clone https://www.xxxx.git rsock
cd rsock
mkdir build && cd build
cmake .. -DRSOCK_RELEASE=1 && make`

为了加快编译速度，make可以指定-j，后跟cpu核数。如：make -j2

### 使用说明

#### 服务器

以64位linux为例：

`sudo ./server_rsock_Linux --dev=eth0 --taddr=127.0.0.1:9999 --ports=10000-10005 --daemon=1`


参数解释:

eth0，外网网卡名称。

127.0.0.1:9999 ，目标地址，即kcptun服务端工作的ip和端口。

10000-10005 ，是rsock服务器端工作的端口**范围**。 形式可以这样, 
10000-10005，表示会占用从端口10000到10005(总共6个）。也可以分别指出试用哪个几个端口，80,443,8080（总共3个）, 表示分别使用这几个端口。也可以同时使用两种表示方法: 80,443,10000-10005（总共8个）.

daemon 等于1表示以守护进程运行程序。为0表示前台。推荐指定为1.

#### 客户端

以mac为例：

`sudo ./client_rsock_Darwin --dev=en0 --taddr=x.x.x.x --ports=10000-10005 --ludp=127.0.0.1:30000 --daemon=1`

参数解释：

en0，外网网卡名称。对于mac，如果连的是wifi，一般是en0。如果是有线，一般是eth1。

taddr，rsock服务器端地址。注意，这里和服务器端不一样：只需指定ip地址。

ports 表示服务端rsock工作的端口**范围**。同上。

ludp 是本地监听的udp端口。即kcptun客户端的目标地址(kcptun中-t 参数对应的地址）。

daemon 同上。

### 退出运行

`ps axu|grep rsock`

![](img/pid.png)

`sudo kill -SIGUSR1 pid # 其中pid是rsock运行的进程id, 图中是72294`

### 注意事项

如果有的时候发现不能上网了，请检查是rsock挂掉了还是kcptun挂掉了.可以运行下面的命令来检查：

`ps axu|egrep 'kcptun|rsock'`

![](img/running.png)

强烈建议服务端kcptun和服务端rsock在后台运行。对于kcptun来说，运行：

`nohup sudo -u nobody ./server_linux_amd64 -t ":port1" -l ":port2" -mode fast2 -key aKey >/dev/null 2>&1 &`

对于rsock，只需在参数中添加 `--daemon=1`

