#pragma once

#include <string>
#include <netdb.h>

struct Find_Result {
    int found;
    char* string;
};

struct Http_Response {
    char* headers;
    char* body;
    bool binary;
};

struct Page {
    const char* key;
    const char* value;
};

struct Dictionary {
    Page* pages;
    int entries;
};

void out(std::string message);
void getAddressInfo(int port, addrinfo** hostList);
int socketBind(addrinfo* hostList);
void socketListen(int socketId, int backlog);
void handleRequest(int socketId);
bool isValidRequestType(const char* buffer);
bool isValidHttpVersion(const char* buffer);
const char* getResourceFromRequest(const char* buffer);
void sendResource(const char* resource);
Find_Result* negativeFindResult();
Find_Result* getStringBetween(const char* buffer, const char start, const char end);
void dictionaryAdd(Dictionary* dict, const char* key, const char* value);
const char* dictionaryFind(Dictionary* dict, const char* key);
bool loadResource(const char* resource, Http_Response* response);
void response200(Http_Response* response);
void response404(Http_Response* response);