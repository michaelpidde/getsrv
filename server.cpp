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
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>

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
    std::string host = "localhost:6660";

    out("---------------------------------------------------");
    out("- GETsrv | Because Even Useless Things Have Names -");
    out("---------------------------------------------------\n");
    out("Starting server at port " + std::to_string(port));

    addrinfo *hostList;
    getAddressInfo(port, &hostList);
    int socketId = socketBind(hostList);
    socketListen(socketId, backlog);

    return 0;
}


void out(std::string message) {
    std::cout << message << std::endl;
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

    int status = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, hostList);

    if(status != 0) {
        std::string error = gai_strerror(status);
        out("getaddrinfo error: " + error);
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
            out("Socket error: " + (std::string)std::strerror(errno));
            continue;
        }
        if(bind(socketId, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }
        close(socketId);
    }
    freeaddrinfo(hostList);
    
    if(addr == NULL) {
        out("Error binding to socket: " + (std::string)strerror(errno));
        exit(EXIT_FAILURE);
    }

    return socketId;
}


void socketListen(int socketId, int backlog) {
    int status = listen(socketId, backlog);
    if(status == -1) {
        out("Error listening to socket: " + (std::string)strerror(errno));
        exit(EXIT_FAILURE);
    }

    while(true) {
        sockaddr_storage address = {};
        socklen_t addressSize = sizeof(address);
        int newSocketId = accept(socketId, (struct sockaddr *)&address, &addressSize);
        if(newSocketId == -1) {
            out("Error listening to socket: " + (std::string)strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Fork returns twice - once in the child process which will return 0 (so we'll handle that process and
        // terminate after it's complete), and once in the parent process which will return the ID of the child process.
        // We really only care about the child process since we're forking in order to handle it and keep the main
        // process running indefinitely.
        pid_t pid = fork();
        if(pid == -1) {
            out("Error forking child process: " + (std::string)strerror(errno));
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
        out("HTTP method not supported.");
        exit(EXIT_FAILURE);
    }

    if(!isValidHttpVersion(buffer)) {
        out("HTTP request type not supported.");
        exit(EXIT_FAILURE);
    }

    const char* resource = getResourceFromRequest(buffer);
    if(strlen(resource) == 0) {
        resource = "index.htm";
    }

    Http_Response *response = (Http_Response*)malloc(sizeof(Http_Response));
    if(findResourceOnDisk(resource)) {
        loadResource(resource, response);
        response200(response);
    } else {
        response404(response);
    }

    char space[4] = "\n\n";
    char outputBuffer[strlen(response->headers) + strlen(response->body) + strlen(space) + 1]{};
    strncpy(outputBuffer, response->headers, strlen(response->headers));
    strncat(outputBuffer, space, strlen(space));
    strncat(outputBuffer, response->body, strlen(response->body));

    send(socketId, outputBuffer, strlen(outputBuffer), 0);

    free(response->headers);
    free(response->body);
    free(response);
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


Find_Result* negatindFindResult() {
    Find_Result* result = (Find_Result*)malloc(sizeof(Find_Result));
    result->found = 0;
    result->string = (char*)malloc(1 * sizeof(char));
    result->string[0] = '\0';
    return result;
}


Find_Result* getStringBetween(const char* buffer, const char start, const char end) {
    if(strlen(buffer) < 2) {
        return negatindFindResult();
    }

    const char* index1 = strchr(buffer, start);
    if(index1 == NULL) {
        return negatindFindResult();
    }
    
    const char* index2 = strchr(index1, end);
    if(index2 == NULL) {
        return negatindFindResult();
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


const char* getResourceFromRequest(const char* buffer) {
    Find_Result* find = getStringBetween(buffer, '/', ' ');
    if(!find->found) {
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


bool findResourceOnDisk(const char* resource) {
    return false;
}


void loadResource(const char* resource, Http_Response* response) {

}


void response200(Http_Response* response) {
    char contentLength[40]{};
    snprintf(contentLength, 40, "Content-Length: %d", (int)strlen(response->body));

    const char* headers = "\
HTTP/1.1 200 OK\n\
Content-Type: text/html\n";
    size_t length = strlen(headers);
    response->headers = (char*)malloc((length + 1) * sizeof(char));
    strncpy(response->headers, headers, length);
    response->headers[length] = '\0';
}


void response404(Http_Response* response) {
    const char* headers = "\
HTTP/1.1 404 Not Found\n\
Content-Type: text/html";
    size_t headerLength = strlen(headers);
    response->headers = (char*)malloc((headerLength + 1) * sizeof(char));
    strncpy(response->headers, headers, headerLength);
    response->headers[headerLength] = '\0';

    response->binary = 0;

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
}