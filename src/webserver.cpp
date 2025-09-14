#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include "sqlite3.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

bool initDatabase() {
    sqlite3* DB;
    int exit = 0;
    exit = sqlite3_open("test.db", &DB);
    if (exit) {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        return false;
    }
    else std::cout << "Opened Database Successfully!" << std::endl;
    
    std::string sql = "CREATE TABLE IF NOT EXISTS USER("
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "USERNAME TEXT NOT NULL, "
                      "PASSWORD TEXT NOT NULL);";
    char* messaggeError;
    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messaggeError);
    if (exit != SQLITE_OK) {
        std::cerr << "Error Create Table" << std::endl;
        sqlite3_free(messaggeError);
        return false;
    }
    else
        std::cout << "Table created Successfully" << std::endl;
    sqlite3_close(DB);
	return true;
}

bool createUser(const std::string& username, const std::string& password) {
    sqlite3* DB;
    int exit = 0;
    exit = sqlite3_open("test.db", &DB);
    if (exit) {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        return false;
    }
    std::string sql = "INSERT INTO USER (USERNAME, PASSWORD) VALUES ('" + username + "', '" + password + "');";
    char* messaggeError;
    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messaggeError);
    if (exit != SQLITE_OK) {
        std::cerr << "Error Insert" << std::endl;
        sqlite3_free(messaggeError);
        return false;
    }
    else
        std::cout << "Records created Successfully!" << std::endl;
    sqlite3_close(DB);
    return true;
}

std::string loadHTML(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
	std::string content = ss.str();
    
	//std::cout << "Loaded content from " << path << ":\n" << content << std::endl;

	// implement server side includes; <!--#include "dashboard.html"-->
	const std::string includeTag = "<!--#include ";
    const std::string endTag = "-->";
	size_t pos = content.find(includeTag);
    if (pos != std::string::npos) {
		//std::cout << "Processing server-side include in " << path << std::endl;
        size_t start = pos + includeTag.length() + 1; // length of <!--#include "
        size_t end = content.find(endTag);
        if (end != std::string::npos) {
			//std::cout << "Found end tag at position: " << end << std::endl;
            std::string includeFile = content.substr(start, end - start - 1);
            //std::cout << "file to be included: " << includeFile << std::endl;
            std::string includeContent = loadHTML("view/" + includeFile); // recursive call
            content.replace(pos, end + endTag.length(), includeContent); // replace entire tag with content
            //std::cout << "new content: " + content << std::endl;
        }
	}
    
    return content;
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

	// Initialize database
	if (!initDatabase()) {
		std::cerr << "Failed to initialize database. Exiting." << std::endl;
		return -1;
	}

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
        if (bytes <= 0) {
            std::cout << "Could not read client request" << WSAGetLastError() << std::endl;
        }

        // generate server message
        std::string serverMessage = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: ";
        /*std::string response = loadHTML("index.html");*/
        std::istringstream reqStream(buff);
        std::string method, path, version;
        reqStream >> method >> path >> version;

		//std::cout << "Received request:\n" << buff << std::endl;
        //std::cout << "Path is: " << path << std::endl;
		//std::cout << "Method is: " << method << std::endl;

        if (method == "POST") {
            // Handle POST request (not implemented)
            std::cout << "POST request handling not implemented.\n";
		    
			std::string username, password;
			// Very basic parsing, assumes well-formed input
			size_t userPos = std::string(buff).find("username=");
			size_t passPos = std::string(buff).find("password=");
			if (userPos != std::string::npos && passPos != std::string::npos) {
				username = std::string(buff).substr(userPos + 9, std::string(buff).find('&', userPos) - (userPos + 9));
				password = std::string(buff).substr(passPos + 9, std::string(buff).find('\n', passPos) - (passPos + 9));
				std::cout << "Parsed username: " << username << ", password: " << password << std::endl;
                if (createUser(username, password)) {
                    std::cout << "User created successfully.\n";
                }
                else {
                    std::cout << "Failed to create user.\n";
				}
			}
            else {
                std::cout << "Could not parse username and password from POST data.\n";
            }
        }
        
		std::string viewPath = "view/";
        // Remove leading '/'
        if (path == "/") path = viewPath + "home.html";
        else if (path == "/home") path = viewPath + "home.html";
        else if (path == "/testpage") path = viewPath + "testpage.html";
        else if (path == "/extra") path = viewPath + "extra.html";
        else {
            //std::cout << path << " requested\n";
            path = path.substr(1);
        };

        std::string response;
        try {
			//std::cout << "Requested file path: " << path << std::endl;
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

