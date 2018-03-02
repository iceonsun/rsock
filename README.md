
### Introduction

中文请点击[这里](doc/README.zh-cn.md).

rsock is merely for accelerating. It's not vpn. It must be used together with kcptun. The purpose of this program is that prevent qos of ips if any. It supports Mac(and other unices) and Linux. To see introduction and usage of kcptun click [here](https://github.com/xtaci/kcptun).

### Installation

There are precompiled binaries for 64bit Linux and 64bit Mac. Download from [here](https://github.com/iceonsun/rsock/releases).

For other platforms, you can download source code and compile it by yourself. Library dependency list: libuv, libnet, libpcap.

Take Ubuntu as an example:

`sudo apt-get install g++ libuv1-dev libnet libpcap #note!It's libuv1-dev
git clone https://github.com/iceonsun/rsock.git rsock
cd rsock
mkdir build && cd build
cmake .. -DRSOCK_RELEASE=1 && make`

To accelerate compilation, you can specify -jNumOfCpuCores. e.g make -j2


### Usage

#### Server

Take Linux as an exmaple:

`sudo ./server_rsock_Linux --dev=eth1 --taddr=127.0.0.1:9999 --ports=10000-10005 --daemon=1`

Parameter explanation:

eth0，name of network interface card of Internet, not LAN, e.g. eth1。

127.0.0.1:9999, target address，aka address of kcptun server working on.

10000-10005 , **RANGE** of ports that rsock works on. It has following formats: 10000-10005(6 ports in total). 80,443(2 ports). 80,443,10000-10005(8 ports).

daemon Run program as daemon if equals 1. Otherwise 0。Recommend 1.

#### Client

Take mac as an example：

`sudo ./client_rsock_Darwin --dev=en0 --taddr=x.x.x.x --ports=10000-10005 --ludp=127.0.0.1:30000 --daemon=1`

Parameter explanation:

en0. name of network interface card of Internet, not LAN, e.g. eth0。For mac, it is typically en0 for wifi, eth1 for ethernet.

taddr. Address of rsock server。Attention. This is different from server. It only contains ip with ports.

ports **RANGE** of ports that rsock server working on.

ludp. local listened udp address, aka target address of kcptun client(the address specified by -t).

daemon. see above.

### Exit

`ps axu|grep rsock`

![](doc/img/pid.png)

`sudo kill -SIGUSR1 pid # pid is id of rsock. It's 72294 in image above.`

### Note

If you find no network connection, please check if rsock and kcptun still running.

可以运行下面的命令来检查：

`ps axu|egrep 'kcptun|rsock'`

![](doc/img/running.png)

It is strongly recommended that kcptun server and rsock server run in background. For kcptun server, run

`nohup sudo -u nobody ./server_linux_amd64 -t ":port1" -l ":port2" -mode fast2 -key aKey >/dev/null 2>&1 &`

For rsock server, only need to specify parameter `--daemon=1`

### Donation

Donation is very welcome.

Wechat pay

![](doc/img/wxdonation.jpg)

Ethereum

11451A1Y4e8vtK3Jb7DoW8BTqj1afuWSn8

or qrcode

![](doc/img/btdonation.jpeg)

bitcoin

0x648419aE3D49271BB7cC31F2a61bC4c517Ea6578

or qrcode

![](doc/img/ethdonation.jpeg)




