[![Build Status](https://travis-ci.org/iceonsun/rsock.svg?branch=master)](https://travis-ci.org/iceonsun/rsock) [![Build status](https://ci.appveyor.com/api/projects/status/cqtcqeg2n2k0eivl?svg=true)](https://ci.appveyor.com/project/iceonsun/rsock)  [![Downloads][1]][2]

---

### Introduction

中文请点击[这里](doc/README.zh-cn.md).

rsock is neither accelerator, nor vpn. It merely turns a udp connection into multiple fake tcp connections, or multiple normal udp connections or both. It's very similar with udp because it's not reliable and has no flow control, timeout retransmission algorithms, etc. It's **cross platform**. It's supposed to used together with kcptun or other udp client with ARQ. The purpose of rsock is that prevent qos to udp from ISP if any. It supports **Windows, Mac and Linux**. To see introduction and usage of kcptun click [here](https://github.com/xtaci/kcptun) . And shadowsocks, click [here](https://github.com/shadowsocks/shadowsocks-go) .

**REPEAT**: Data transfer of rsock is **NOT** reliable. Reliable data tranfer should be taked care of by app level(kcptun).

The following picture brifely shows principles

![](doc/img/principle.png)

### Installation

There are precompiled binaries for 64bit Linux, 64bit Mac and x86/x64 windows. They can be downloaded from [here](https://github.com/iceonsun/rsock/releases).

For **Windows** users, you have to install winpcap first. [winpcap](https://www.winpcap.org/install/default.htm)

For other platforms, you can download source code and compile it by yourself. Compilation guide is [here](https://github.com/iceonsun/rsock/wiki/Manual-compilation-Guide)



### Quick start

#### Server

Remember to add firewall rule if firewall enabled.

Take Linux as an exmaple:

```
# port=10000
# while [ $port -le 10010 ]
do
sudo ufw allow $port
port=$[ $port + 1]
done
```

It means allow client connects to server from port 10000 to 10010.
(**rsock use port range 10001-10010 by default. If you want to change the default value, please check Parameter Explanation section.**)

`sudo ./server_rsock_Linux -t 127.0.0.1:9999`

Parameter explanation:

127.0.0.1:9999, target address，aka address of kcptun server working on.

#### Client

Take mac as an example:

`sudo ./client_rsock_Darwin --taddr=x.x.x.x -l 127.0.0.1:30000`

Parameter explanation:

-t x.x.x.x , Address of rsock server。Attention. This is different from server. It only contains ip.

-l , local listened udp address, aka target address of kcptun client(the address specified by -t).

#### NOTE

1. If rsock doesn't work, you have to check whether your NIC supoorts winpcap. And routers may also filter packets, especially when you use Windows.

2. For Windows users, speed is far slow than rsock on mac/Linux. （500-800KB/s  during my test) e.g. On Mac/Linux, rsock can support to watch 1080P youtube video smoothly. For windows users, rsock can only support 720P youtube video.

### Exit

`ps axu|grep rsock`

![](doc/img/pid.png)

`sudo kill -SIGUSR1 pid # pid is id of rsock. It's 72294 in image above.`

### Parameters in detail

```

	-t, --taddr=[addr]		target address. e.g. 8.8.8.8:88,7.7.7.7. Required.
	-l, --ludp=[addr]		local listened udp address. Only valid for client. Required by client.
	-d, --dev=[device]		name of network interface card of Internet.e.g,eth0, en0. rsock can auto detect right device to work on. Use this when the default can't work.
	-h, --help			Display help menu. Not available now.
	-f				json config file
	--lcapIp=[ip]			Internet IP. Can omit the -d if this parameter sepcified. rsock can auto detect right device to work on. Use this when the default can't work.
	--unPath			Local unix domain socket. Not available now.
	-p, --ports=[...]		tcp/udp port list for rsock server. e.g.10001,10010(2 ports); 10001-10010(11 ports); 80,443,10001-10010(12 ports). **NO** white spaces allowed. Default value: 10001-10010
	--duration=[timeSec]		Time for app connection to persist if no data is transfered in the app connection. unit: seconds. defalt 30s
	--hash=[hashKey]		Not for encryption. Only for judgement if data belong to rsock. REPEAT: rsock don't encrypt data. Encryption is done by kcptun.
	--type=[tcp|udp|all]		type of communication. One of tcp, udp or all. Default is tcp.
	--daemon=[1|0]			Run as daemon. 1 yes. 0 no. default 1.
	-v				verbose mode. (Better not change default value. There is an unsolved bug that will cause slow speed right now)
	--log=[path/to/log]		Directory of log. Will create if not exist. Default: /var/log/rsock
	--cap_timeout			timeout of libpcap. Don't change this value if know what it really means.

```

### Principle

1. Server listens on some ports. (tcp 10001-10010, 10 ports by default)
2. Client connects to all of server ports.
3. For each of communications, client send data by libnet to one of server ports, which should have been connected.
4. Server receive data by libpcap. It's same for server to send data to client. Now they make communication. 
5. For application, local_ip:app_udp_port and server_ip:app_udp_port make a connection。If no data flows thourgh this connection for a period of time(default 30s), it will be closed.
6. When client receive rst or fin, it will close that real network connection and reconnect to server.

#### Disadvantage

Under tcp mode, since we don't send/recv data from socket, it will send an ack with 0 length, telling peer next seq it expects. This due to standard. And that will waste bandwith.

### Comparison

Comparing objects：rsock, [kcptun](https://github.com/xtaci/kcptun)

##### Server test environment

digitalocean NY vps. 1G RAM


##### Client test environment 1. digitalocean Singaport vps.

rsock(tcp only). It's around 700KB

![](doc/img/rsock_do_sg.png)

rsock(udp only). It's around 1MB

![](doc/img/rsock_do_sg_udp.png)

rsock(tcp and udp). It's around 900KB

![](doc/img/rsock_do_sg_udp_tcp.png)

rsock(tcp only, 11 ports). 1.25M

![](doc/img/rsock_do_sg_11tcp.png)

rsock(udp only, 11 ports).  1.5M

![](doc/img/rsock_do_sg_11udp.png)

rsock(doc/tcp and udp, 11 ports each. 1.1M

![](doc/img/rsock_do_sg_11udp_tcp.png)

kcptun. The fastest, around 1.5MB.

![](doc/img/kcptun_do_sg.png)


##### Client test environment 1. China telecom with 100Mb downloading and 10Mb upload speed.

rsock(tcp only, 2 ports). Around 630KB.

![](doc/img/rsock_telecom.png)

rsock(udp only, 2 ports). Around 1MB

![](doc/img/rsock_udp_telcom.png)

rsock(udp and tcp, 2 ports each). Around 700k

![](doc/img/rsock_udp_tcp_telcom.png)

rsock(tcp only, 11 ports). 1.4M

![](doc/img/rsock_11tcp_telcom.png)

rsock(udp only, 11 ports. 1.7M

![](doc/img/rsock_11udp_telcom.png)

rsock(udp and tcp, 11 ports each）900K. I've tested twice. The speed is slower.

![](doc/img/rsock_11udp_tcp_telcom.png)

kcptun. extremely fast. Around 2MB.

![](doc/img/kcptun_telecom.png)

note: It's **not** more ports rsock use, the faster it is. It's mainly determined by your bandwith. There is no major difference between 10 ports and 5 in test.

#### Conclusion

rsock only has 70%-90% speed of kcptun.

### Note

If you find no network connection, please check if rsock and kcptun still running.

You can run flowing commands to check:

`ps axu|egrep 'kcptun|rsock'`

![](doc/img/running.png)

It is strongly recommended that kcptun server and rsock server run in background. For kcptun server, run

`nohup sudo -u nobody ./server_linux_amd64 -r ":port1" -l ":port2" -mode fast2 -key aKey >/dev/null 2>&1 &`

For rsock server, only need to specify parameter `--daemon=1`

If servers run normally, try to restart kcptun client(turn shadowsocks on/off, this will restart kcptun).

**rsock DOES NOT encrypt data**. Encrption happens in app level(kcptun).

### Other projects

[udp2raw-tunnel](https://github.com/wangyu-/udp2raw-tunnel)

[kcptun-raw](https://github.com/Chion82/kcptun-raw)

[icmptunnel](https://github.com/DhavalKapil/icmptunnel)


### TODO

1. Refactor code.

1. Add randomize port listening.

1. Add idle mode. Don't repeatedly connect to server if no data for a period.

1. Add other fake conn. e.g. ICMP, DNS.


### Donation

Donation is very welcome.

Ethereum

11451A1Y4e8vtK3Jb7DoW8BTqj1afuWSn8

or qrcode

![](doc/img/btdonation.jpeg)

bitcoin

0x648419aE3D49271BB7cC31F2a61bC4c517Ea6578

or qrcode

![](doc/img/ethdonation.jpeg)

[1]: https://img.shields.io/github/downloads/iceonsun/rsock/total.svg?maxAge=1800
[2]: https://github.com/iceonsun/rsock/releases

