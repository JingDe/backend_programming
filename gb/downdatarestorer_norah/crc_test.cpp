#include<iostream>
#include<stdio.h>
#include<string>

#include"redis_client_util.h"

#define REDIS_SLOT_NUM 16384

uint16_t GetSlot(const std::string& key)
{
    uint16_t crcValue = crc16(key.c_str(), key.length());
    crcValue %= REDIS_SLOT_NUM;
    return crcValue;
}

int main()
{
    std::string key1="gbproxy.router.gbdownlinker";
    std::cout<<"gbproxy.router.gbdownlinker\t"<<GetSlot(key1)<<std::endl;

    key1="gbproxy.router.gbuplinker";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="gbproxy.router.nvrchannel";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="gbproxy.router.routingtable";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="gbproxy.agent.gbdownlinker";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="gbproxy.agent.gbuplinker";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="downlinker.device";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="downlinker.channel";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    key1="downlinker.invite";
    printf("%s\t%d\n", key1.c_str(), GetSlot(key1));

    return 0; 
}
