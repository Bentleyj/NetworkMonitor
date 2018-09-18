#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>

using namespace std;

// This is a connection controller class that I had previously built for my Sunrise Kingdom project for both windows and unix systems.
// It is a very basic class which simply monitors a single port.
// It is based on the tutorial ma pages found here: http://beej.us/guide/bgnet/html/multi/getaddrinfoman.html

class SocketMonitor {
	public:
	int connectToLocalServerAtPort(int port) {
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;

        string p = to_string(port);

		getaddrinfo(NULL, p.c_str(), &hints, &res);
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		bind(sockfd, res->ai_addr, res->ai_addrlen);

		cout<<"Connecting To Local Server"<<endl;
		return 1;
	}

	int printNumBytesRecieved() { 		
        fromlen = sizeof addr;
		byte_count = recvfrom(sockfd, buf, sizeof buf, 0, &addr, &fromlen);
		printf("recv()'d %d bytes of data in buf\n", byte_count);
		cout<<buf<<endl;
		return 1;
	}

    char* getBuffer() { return buf; };

	private:
	struct addrinfo hints, *res;
	int sockfd;
	int byte_count;
	socklen_t fromlen;
	struct sockaddr addr;
	char buf[1024];
	char ipstr[INET6_ADDRSTRLEN];
};

// this will be our parent class for both masters and slaves
class Machine {
    public:
    Machine();
    ~Machine();
    private:
    string machineID;
    string version;
    float fps;
};

// This represents a master machine
class Master : public Machine {
    public:
    Master();
    ~Master();
    private:
};

// This represents a slave machine
class Slave : public Machine {
    public:
    Slave();
    ~Slave();
    private:
};

// This represents a session
class Session {
    public:
    private:
    string name;
    Master creator;
    vector<Slave> slaves;
};

int main() {

    char buffer[1024];
    SocketMonitor m4, m6;
    m4.connectToLocalServerAtPort(7104);
    m6.connectToLocalServerAtPort(7106);

    while(1) {
        // cout<<"Buffer: " <<m6.getBuffer()<<endl;
        // m4.printNumBytesRecieved();
        m6.printNumBytesRecieved();
    }
    return 0;
}