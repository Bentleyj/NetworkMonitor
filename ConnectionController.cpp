#ifdef _WIN32
#include <WinSock2.h>
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib") //Winsock Library
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sstream>

using namespace std;

// This is a connection controller class that I had previously built for my Sunrise Kingdom project for both windows and unix systems.
// It opens up a connection to an IP address on a port and can send and recieve messages from that port.
// It is based on the tutorial which I used here: https://www.geeksforgeeks.org/socket-programming-cc/

class ConnectionController {
public:
	string IPAddress;
	char buffer[1024];
	struct sockaddr_in server;
	int port, valread;

#ifdef _WIN32
	WSADATA wsa;
	SOCKET sock;
#else
	int sock;
#endif

#ifdef _WIN32
	int initializeWinSock() {
		printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			printf("Failed. Error Code: %d", WSAGetLastError());
			return 1;
		}
		printf("Initialised.");

		return 0;
	}
#endif

	void setIPAndPort(string _IPAddress, int _port) {
		IPAddress = _IPAddress;
		port = _port;
	}

	int connectToServer() {
#ifdef _WIN32
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			printf("Could not create socket : %d", WSAGetLastError());
		}
#else
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("\n Socket creation error \n");
			return 1;
		}
		memset(&server, '0', sizeof(server));
#endif

		server.sin_addr.s_addr = inet_addr(IPAddress.c_str()); // REMEMBER THIS! IT MIGHT CAUSE AN ERROR!
		server.sin_family = AF_INET;
		server.sin_port = htons(port);

#ifndef _WIN32
		// Convert IPv4 and IPv6 addresses from text to binary form
		if (inet_pton(AF_INET, IPAddress.c_str(), &server.sin_addr) <= 0) {
			printf("\nInvalid address/ Address not supported \n");
			return 1;
		}
#endif

        int opt=1;

        // lose the "Address already in use" error message
        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof opt) < 0) {
            printf("\nError setting socket options\n");
            exit(1);
        } 

		if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
			printf("\nConnection Failed: %s\n", strerror(errno));
			return 1;
		}

        printf("Connected successfully to port: %i", port);
		return 0;
	}

	string sendMessage(string message) {
		send(sock, message.c_str(), message.length(), 0);
#ifdef _WIN32
		valread = recv(sock, buffer, 1024, 0);
#else
		valread = read(sock, buffer, 1024);
#endif
		return buffer;
	}

    void getSockStatus() {
        int error = 0;
        socklen_t len = sizeof (error);
        int retval = getsockopt (sock, SOL_SOCKET, SO_ERROR, &error, &len);
        if (retval != 0) {
            /* there was a problem getting the error code */
            fprintf(stderr, "error getting socket error code: %s\n", strerror(retval));
            return;
        }

        if (error != 0) {
            /* socket has a non zero error status */
            fprintf(stderr, "socket error: %s\n", strerror(error));
        }
    }

    int recieveMessage(char * buffer) {
        int val = read(sock, buffer, 1024);
        if(val < 0) {
			printf("\nConnection Failed: %s\n", strerror(errno));
        }
        return val;
    }
    
};