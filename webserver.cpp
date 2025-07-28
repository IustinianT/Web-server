#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

std::string loadHTML(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main()
{
    std::cout << "Creating the server\n";

    SOCKET wsocket;
    SOCKET new_wsocket;
    WSADATA wsaData;
    struct sockaddr_in server;
    int server_len;
    int BUFFER_SIZE = 30720;

    // initialise
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Could not initialise server: " << WSAGetLastError() << std::endl;
    }

    // create socket
    wsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (wsocket == INVALID_SOCKET) {
        std::cout << "Invalid socket: " << WSAGetLastError() << std::endl;
    }

    // bind socket to addr
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8080);
    server_len = sizeof(server);

    if (bind(wsocket, (SOCKADDR*)&server, server_len) != 0) {
        std::cout << "Could not bind socket: " << WSAGetLastError() << std::endl;
    }

    // listen on addr
    if (listen(wsocket, 20) != 0) {
        std::cout << "Could not start listening: " << WSAGetLastError() << std::endl;
    }

    std::cout << "Listening on 127.0.0.1::8080\n";

    int bytes = 0;
    while (true) {
        // client request accept
        new_wsocket = accept(wsocket, (SOCKADDR*)&server, &server_len);
        if (new_wsocket == INVALID_SOCKET) {
            std::cout << "Invalid new socket: " << WSAGetLastError() << std::endl;
        }

        // read request
        char buff[30720] = { 0 };
        bytes = recv(new_wsocket, buff, BUFFER_SIZE, 0);
        if (bytes < 0) {
            std::cout << "Could not read client request" << WSAGetLastError() << std::endl;
        }

        // generate server message
        std::string serverMessage = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: ";
        /*std::string response = loadHTML("index.html");*/
        std::istringstream reqStream(buff);
        std::string method, path, version;
        reqStream >> method >> path >> version;

        // Remove leading '/'
        if (path == "/") path = "index.html";
        else path = path.substr(1); // "testpage.html"

        std::string response;
        try {
            response = loadHTML(path);
        }
        catch (...) {
            // Return 404 if file not found
            response = "<html><body><h1>404 Not Found</h1></body></html>";
        }

        serverMessage.append(std::to_string(response.size()));
        serverMessage.append("\n\n");
        serverMessage.append(response);

        int bytesSent = 0;
        int totalBytesSent = 0;
        while (totalBytesSent < serverMessage.size()) {
            bytesSent = send(new_wsocket, serverMessage.c_str(), serverMessage.size(), 0);
            if (bytesSent < 0) {
                std::cout << "Could not send response bytes";
            }
            totalBytesSent += bytesSent;
        }
        std::cout << "Response sent to client\n";

        closesocket(new_wsocket);
    }
    closesocket(wsocket);
    WSACleanup();

    return 0;
}

