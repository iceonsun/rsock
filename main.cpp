#include <iostream>
#include "cap/cap_util.h"

using namespace std;

int main() {
    const std::string dev = "en0";
    const std::string dstIp = "192.168.3.1";
    const std::string srcIp = "127.0.0.1";
    PortLists src = {10012, 10022};
    PortLists dst = {80};
    auto filter = BuildFilterStr(srcIp, dstIp, src, dst);
    std::cout << "Hello, World!" << std::endl;
    cout << "filter: " << filter << endl;
    return 0;
}