#pragma once

#include <string>
#include <netdb.h>

struct Find_Result {
    int found;
    char* string;
};

void out(std::string message);
void getAddressInfo(int port, addrinfo** hostList);
int socketBind(addrinfo* hostList);
void socketListen(int socketId, int backlog);
void handleRequest(int socketId);
bool isValidRequestType(const char* buffer);
const char* getResource(const char* buffer);
void sendResource(const char* resource);
Find_Result* negatindFindResult();
Find_Result* getStringBetween(const char* buffer, const char start, const char end);