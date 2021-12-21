#pragma once

#include <netdb.h>

struct Find_Result {
    int found;
    char* string;
};

struct Page {
    const char* key;
    const char* value;
};

struct Dictionary {
    Page* pages;
    int entries;
};

struct Http_Response {
    Dictionary headers;
    int code;
    char* body;
    bool binary;
};

void out(const char* message);
void logStandardError(const char* message);
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
bool dictionaryAddPage(Dictionary* dict, Page page);
bool dictionaryAddKeyValue(Dictionary* dict, const char* key, const char* value);
const char* dictionaryFind(Dictionary* dict, const char* key);
char* dictionaryToString(Dictionary* dict);
bool loadResource(const char* resource, Http_Response* response);
void setResponseContentType(const char* extension, Http_Response* response);
const char* httpCodeToText(int code);
void response200(Http_Response* response);
void response404(Http_Response* response);