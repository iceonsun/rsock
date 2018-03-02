
### Introduction

中文请点击[这里](doc/README.zh-cn.md).

rsock is merely for accelerating. It's not vpn. It must be used together with kcptun. The purpose of this program is that prevent qos of ips if any. It supports Mac(and other unices) and Linux. To see introduction and usage of kcptun click [here](https://github.com/xtaci/kcptun).

The following picture brifely introduces principle

![](doc/img/principle.png)

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

eth0, name of network interface card of Internet, not LAN, e.g. eth1

127.0.0.1:9999, target address，aka address of kcptun server working on.

10000-10005 , **RANGE** of ports that rsock works on. It has following formats: 10000-10005(6 ports in total). 80,443(2 ports). 80,443,10000-10005(8 ports).

daemon Run program as daemon if equals 1. Otherwise 0。Recommend 1.

#### Client

Take mac as an example:

`sudo ./client_rsock_Darwin --dev=en0 --taddr=x.x.x.x --ports=10000-10005 --ludp=127.0.0.1:30000 --daemon=1`

Parameter explanation:

en0. name of network interface card of Internet, not LAN, e.g. eth0. For mac, it is typically en0 for wifi, eth1 for ethernet.

taddr. Address of rsock server。Attention. This is different from server. It only contains ip with ports.

ports **RANGE** of ports that rsock server working on.

ludp. local listened udp address, aka target address of kcptun client(the address specified by -t).

daemon. see above.

### Exit

`ps axu|grep rsock`

![](doc/img/pid.png)

`sudo kill -SIGUSR1 pid # pid is id of rsock. It's 72294 in image above.`

### Principle

1. Server listens on some ports. tcp or udp
2. Client connects to all of server ports.
3. Client send data by libnet to one of server ports. 
4. Server receive data by libpcap. It's same for server to send data to client. Now they make communication.
5. For application, local_ip:app_udp_port and server_ip:app_udp_port make a connection。If no data flows thourgh this connection, it will be closed.
6. When client receive rst or fin, it will close that real network connection and reconnect to server.

#### Disadvantage

Under tcp mode, since we don't send/recv data from socket, it will send an ack with 0 length, telling peer next seq it expects. This due to standard. And that will waste bandwith.

### Comparison

Comparing objects：rsock, [udp2raw-tunnel](https://github.com/wangyu-/udp2raw-tunnel), [kcptun](https://github.com/xtaci/kcptun)

##### Server test environment

digitalocean NY vps. 1G RAM


##### Client test environment 1. digitalocean Singaport vps.

rsock(tcp only). It's around 700KB

![](doc/img/rsock_do_sg.png)

rsock(udp only). It's around 1MB

![](doc/img/rsock_do_sg_udp.png)

rsock(tcp and udp). It's around 900KB

![](doc/img/rsock_do_sg_udp_tcp.png)

udp2raw-tunnel. Very fast, 1360KB.

![](doc/img/udp2raw_do_sg.png)

kcptun. The fastest, around 1.5MB.

![](doc/img/kcptun_do_sg.png)


##### Client test environment 1. China telecom with 100Mb downloading and 10Mb upload speed.

rsock(tcp only). Around 630KB.

![](doc/img/rsock_telecom.png)

rsock(udp only)。Around 1MB

![](doc/img/rsock_udp_telcom.png)

rsock(udp and tcp). Around 700k

![](doc/img/rsock_udp_tcp_telcom.png)

kcptun. extremely fast. Around 2MB.

![](doc/img/kcptun_telecom.png)


I didn't test udp2raw-tunnel on mac.

#### Conclusion

rsock only has 30%-50% speed of kcptun. But it's suffienct to watch youtube 1080 videos. 

### Note

If you find no network connection, please check if rsock and kcptun still running.

You can run flowing commands to check:

`ps axu|egrep 'kcptun|rsock'`

![](doc/img/running.png)

It is strongly recommended that kcptun server and rsock server run in background. For kcptun server, run

`nohup sudo -u nobody ./server_linux_amd64 -r ":port1" -l ":port2" -mode fast2 -key aKey >/dev/null 2>&1 &`

For rsock server, only need to specify parameter `--daemon=1`

### TODO

1. Locate problem of slow speed. 
   - I don't think that kcptun running above rsock is the main reason that why speed of rsock is much slower than kcptun. I don't know intermediate routers/firewalls will drop packets due to wrong seq and ack. I did googled it. The anwsers are they won't drop packets. But I'm still questioning it.
   
   - Probably schedule problem of libuv. Because it may cause balsting recv/send data and it will flood network. Then router will drop packets.
   
2. If low speed is due to 0 length ack, consider use linux rawsocket and BSD bpf.


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




