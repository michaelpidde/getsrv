/*******************************************************************************
 * 
 * Note to future code explorers:
 * 
 * This code is probably junk. I'm well aware that I mix null-terminated C
 * strings with probably unterminated C strings (does the recv buffer get
 * null terminated? I don't know.) and C++ standard lib strings. Some of that is
 * convenience. It's just easier to pass a C++ string around and modify it
 * rather than write a bunch of dumb convenience functions to do concatenation
 * of const char*. On the other hand, I do enjoy a puzzle sometimes, and messing
 * with char*s is fun - sometimes. I like to write functions and test them
 * myself, so there's a bit of "wasn't written here" syndrome. I'm ok admitting
 * that. I'm also aware that not all aspects of proper socket programming are
 * being taken into consideration. I neither know nor have the patience to study
 * all of the nuances of socket programming. I just want to handle one kind of
 * rather opaque request that can come through - just to send some static junk
 * content back to the requester. I don't feel the need to be a socket guru in
 * order to do this. Whatever the case, don't read this code and think that it's
 * worthy of being an example. It doesn't matter how much time I've spent
 * dumping myself into C and C++ code; I'm never going to be a C or C++
 * programmer for real. I don't consider myself one. I just appreciate some of
 * the anachronistic qualities of the genre: explicitness, speed, being "close
 * to the machine." I could be closer to the machine just as I could be farther
 * from it. The entire idea of this project is to be closer to the machine in a
 * sort of way by utilizing a unikernel to run this process. In that sense, it's
 * a clear choice to select C or C++ as the language for this endeavor.
 * 
 ******************************************************************************/

#include <cerrno>
#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"

#if INTERNAL
    #include "test.cpp"
#endif


int main(int argc, char** argv) {
    #if INTERNAL
        runTests();
        #if TESTONLY
            exit(EXIT_SUCCESS);
        #endif
    #endif

    int port = 6660;
    int backlog = 10;

    out("---------------------------------------------------");
    out("- GETsrv | Because Even Useless Things Have Names -");
    out("---------------------------------------------------\n");
    char* portString;
    sprintf(portString, "Starting server at port %d", port);
    out(portString);

    addrinfo *hostList;
    getAddressInfo(port, &hostList);
    int socketId = socketBind(hostList);
    socketListen(socketId, backlog);

    return 0;
}


void out(const char* message) {
    printf("%s\n", message);
}


void logStandardError(const char* message) {
    // TODO: Write to error log
    char* error;
    sprintf(error, "%message: %s", strerror(errno));
    out(error);
}


void getAddressInfo(int port, addrinfo** hostList) {
    addrinfo hints = {};

    //
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    //

    // The value AF_UNSPEC indicates that getaddrinfo() should return socket addresses for any address family (either 
    // IPv4 or IPv6, for example) that can be used with node and service.
    hints.ai_family = AF_UNSPEC;
    // If the AI_PASSIVE flag is specified in hints.ai_flags, and node is NULL, then the returned socket addresses will
    // be suitable for bind(2)ing a socket that will accept(2) connections. The returned socket address will contain the
    // "wildcard address" (INADDR_ANY for IPv4 addresses, IN6ADDR_ANY_INIT for IPv6 address). The wildcard address is
    // used by applications (typically servers) that intend to accept connections on any of the host's network addresses.
    hints.ai_flags = AI_PASSIVE;
    // This field specifies the preferred socket type
    hints.ai_socktype = SOCK_STREAM;

    char* portString;
    sprintf(portString, "%d", port);
    int status = getaddrinfo(NULL, portString, &hints, hostList);

    if(status != 0) {
        // TODO: Write to error log
        char* error;
        sprintf(error, "getaddrinfo error: %s", gai_strerror(status));
        out(error);
        exit(EXIT_FAILURE);
    }
}


int socketBind(addrinfo* hostList) {
    int socketId;
    addrinfo* addr;

    // Iterate addresses returned by getaddrinfo until we successfully bind.
    for(addr = hostList; addr != NULL; addr = addr->ai_next) {
        socketId = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if(socketId == -1) {
            logStandardError("Socket error");
            continue;
        }
        if(bind(socketId, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }
        close(socketId);
    }
    freeaddrinfo(hostList);
    
    if(addr == NULL) {
        logStandardError("Error binding to socket");
        exit(EXIT_FAILURE);
    }

    return socketId;
}


void socketListen(int socketId, int backlog) {
    int status = listen(socketId, backlog);
    if(status == -1) {
        logStandardError("Error listening to socket");
        exit(EXIT_FAILURE);
    }

    while(true) {
        sockaddr_storage address = {};
        socklen_t addressSize = sizeof(address);
        int newSocketId = accept(socketId, (struct sockaddr *)&address, &addressSize);
        if(newSocketId == -1) {
            logStandardError("Error listening to socket");
            exit(EXIT_FAILURE);
        }

        // Fork returns twice - once in the child process which will return 0 (so we'll handle that process and
        // terminate after it's complete), and once in the parent process which will return the ID of the child process.
        // We really only care about the child process since we're forking in order to handle it and keep the main
        // process running indefinitely.
        pid_t pid = fork();
        if(pid == -1) {
            logStandardError("Error forking child process");
            exit(EXIT_FAILURE);
        } else if(pid == 0) {
            close(socketId);
            handleRequest(newSocketId);
            close(newSocketId);
            exit(EXIT_SUCCESS);
        } else {
            close(newSocketId);
        }
    }
}


void handleRequest(int socketId) {
    size_t bufferSize = 1024 * 8;
    char buffer[bufferSize];
    recv(socketId, &buffer, bufferSize, 0);
    
    // TODO: Write buffer to request log
    // out(buffer);

    if(!isValidRequestType(buffer)) {
        // TODO: Write to error log
        out("HTTP method not supported.");
        exit(EXIT_FAILURE);
    }

    if(!isValidHttpVersion(buffer)) {
        // TODO: Write to error log
        out("HTTP request type not supported.");
        exit(EXIT_FAILURE);
    }

    const char* resource = getResourceFromRequest(buffer);
    if(strlen(resource) == 0) {
        resource = "index.htm";
    }

    Http_Response *response = (Http_Response*)malloc(sizeof(Http_Response));
    response->headers = {};

    const char* extension = strrchr(resource, '.');
    setResponseContentType(extension, response);

    if(loadResource(resource, response)) {
        response200(response);
    } else {
        response404(response);
    }

    // TODO: Make dictionaryToString function for headers and replace response->headers append below.

    // char line[2] = "\n";
    // char outputBuffer[strlen(response->headers) + strlen(response->body) + strlen(line) + 1]{};
    // strncpy(outputBuffer, response->headers, strlen(response->headers));
    // strncat(outputBuffer, line, strlen(line));
    // strncat(outputBuffer, response->body, strlen(response->body));

    // send(socketId, outputBuffer, strlen(outputBuffer), 0);

    free(response->headers->pages);
    free(response->body);
    free(response);
}


void setResponseContentType(const char* extension, Http_Response* response) {
    response->binary = false;

    Page contentTypeHeader = {};
    contentTypeHeader.key = "Content-Type";

    if(extension == NULL) {
        contentTypeHeader.value = "text/html";
    } else {
        // We're removing the period from the start of the extension, but make the new array the same length so
        // the null terminator can be put onto the end.
        char extensionLower[strlen(extension)];
        // Start at 1 so we can remove the period.
        for(int i = 1; extension[i]; ++i) {
            // Minus 1 to put the character at the right index in the new array.
            extensionLower[i - 1] = tolower(extension[i]);
        }
        extensionLower[strlen(extension) - 1] = '\0';
        out(extensionLower);
        if(strcmp(extension, "png") == 0) {
            response->binary = true;
            contentTypeHeader.value = "image/png";
        } else if(strcmp(extension, "jpg") == 0) {
            response->binary = true;
            contentTypeHeader.value = "image/jpg";
        }
    }

    dictionaryAddPage(response->headers, contentTypeHeader);
}


bool isValidRequestType(const char* buffer) {
    if(strlen(buffer) == 0) {
        return false;
    }

    const char* find = strchr(buffer, ' ');
    int len = (find - buffer);
    char method[len + 1];
    strncpy(method, buffer, sizeof(method));
    method[len] = '\0';
    if(strcmp(method, "GET") == 0) {
        return true;
    }
    return false;
}


bool isValidHttpVersion(const char* buffer) {
    if(strlen(buffer) == 0) {
        return false;
    }

    // Intentionally not caring about "HTTP/1.12" or similar because you can't send that via curl. I realize that this
    // would still pass if such an HTTP version came through.
    const char* find = strstr(buffer, "HTTP/1.1");
    if(find != NULL) {
        return true;
    }
    return false;
}


Find_Result* negativeFindResult() {
    Find_Result* result = (Find_Result*)malloc(sizeof(Find_Result));
    result->found = 0;
    result->string = (char*)malloc(1 * sizeof(char));
    result->string[0] = '\0';
    return result;
}


Find_Result* getStringBetween(const char* buffer, const char start, const char end) {
    if(strlen(buffer) < 2) {
        return negativeFindResult();
    }

    const char* index1 = strchr(buffer, start);
    if(index1 == NULL) {
        return negativeFindResult();
    }
    
    const char* index2 = strchr(index1, end);
    if(index2 == NULL) {
        return negativeFindResult();
    }

    Find_Result* result = (Find_Result*)malloc(sizeof(Find_Result));
    result->found = 1;
    // Add 1 to index1 so we can get the final section length AFTER the start char.
    size_t sectionLength = index2 - (index1 + 1);
    // Add 1 so we can null terminate the string.
    result->string = (char*)malloc((sectionLength + 1) * sizeof(char));
    // Again, add 1 so we start copying from the next char AFTER the start char.
    strncpy(result->string, index1 + 1, sectionLength);
    result->string[sectionLength] = '\0';
    return result;
}


bool dictionaryAddPage(Dictionary* dict, Page page) {
    if(dictionaryFind(dict, page.key) != NULL) {
        return false;
    }

    if(dict->entries == 0) {
        dict->pages = (Page*)malloc(sizeof(Page));
    } else {
        dict->pages = (Page*)realloc(dict->pages, sizeof(Page) * ((dict->entries) + 1));
    }
    int next = dict->entries++;
    dict->pages[next] = page;
    return true;
}


bool dictionaryAddKeyValue(Dictionary* dict, const char* key, const char* value) {
    if(dictionaryFind(dict, key) != NULL) {
        return false;
    }

    if(dict->entries == 0) {
        dict->pages = (Page*)malloc(sizeof(Page));
    } else {
        dict->pages = (Page*)realloc(dict->pages, sizeof(Page) * ((dict->entries) + 1));
    }
    int next = dict->entries++;
    Page page;
    page.key = key;
    page.value = value;
    dict->pages[next] = page;
    return true;
}


const char* dictionaryFind(Dictionary* dict, const char* key) {
    if(dict->entries > 0) {
        for(int i = 0; i < dict->entries; ++i) {
            if(strncmp(dict->pages[i].key, key, strlen(key)) == 0) {
                return dict->pages[i].value;
            }
        }
    }
    return NULL;
}


char* dictionaryToString(Dictionary* dict) {
    char* result = (char*)malloc(1024 * sizeof(char));
    int length = 0;
    for(int i = 0; i < dict->entries; ++i) {
        if(strlen(dict->pages[i].key) == 0) {
            length += sprintf(result + length, "%s\n", dict->pages[i].value);
        } else {
            length += sprintf(result + length, "%s: %s\n", dict->pages[i].key, dict->pages[i].value);
        }
    }
    result[length] = '\0';
    return result;
}


const char* getResourceFromRequest(const char* buffer) {
    Find_Result* find = getStringBetween(buffer, '/', ' ');
    if(!find->found) {
        // TODO: Write to error log
        out("Error getting resource name.");
        exit(EXIT_FAILURE);
    }
    size_t length = strlen(find->string);
    char* resource = (char*)malloc(length + 1);
    strncpy(resource, find->string, length);
    resource[length] = '\0';
    free(find);
    return resource;
}


bool loadResource(const char* resource, Http_Response* response) {
    FILE *file;
    char fullPath[1024] = "./content/";
    strncat(fullPath, resource, 1024 - strlen(fullPath));
    file = fopen(fullPath, "r");
    if(file == NULL) {
        logStandardError("Error reading resource file");
        return false;
    }
    fseek(file, 0, SEEK_END);
    int fileLength = ftell(file);
    response->body = (char*)malloc((fileLength + 1) * sizeof(char));
    fseek(file, 0, SEEK_SET);
    fread(response->body, fileLength, 1, file);
    fclose(file);
    if(response->body == NULL) {
        logStandardError("Error reading resource file");
        return false;
    }
    return true;
}


void response200(Http_Response* response) {
    dictionaryAddKeyValue(response->headers, "", "HTTP/1.1 200 OK");
    dictionaryAddKeyValue(response->headers, "Content-Type", "text/html");
    char* length;
    sprintf(length, "%d", (int)strlen(response->body));
    dictionaryAddKeyValue(response->headers, "Content-Length", length);
}


void response404(Http_Response* response) {
    const char* body = "\
<!doctype html>\
<html>\
<head>\
<title>Page Not Found</title>\
</head>\
<body>404 Page Not Found</body>\
</html>";
    size_t bodyLength = strlen(body);
    response->body = (char*)malloc((bodyLength + 1) * sizeof(char));
    strncpy(response->body, body, bodyLength);
    response->body[bodyLength] = '\0';

    dictionaryAddKeyValue(response->headers, "", "HTTP/1.1 404 Not Found");
    dictionaryAddKeyValue(response->headers, "Content-Type", "text/html");
    char* length;
    sprintf(length, "%d", (int)strlen(response->body));
    dictionaryAddKeyValue(response->headers, "Content-Length", length);

    response->binary = 0;
}