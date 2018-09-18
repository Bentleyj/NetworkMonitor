#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
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
    SocketMonitor() {};
    ~SocketMonitor() {};
	int connectToLocalServerAtPort(int port) {
        // Set my socket parameters
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;

        // create me socket
        string p = to_string(port);

        // get local addr info for local port
		getaddrinfo(NULL, p.c_str(), &hints, &res);

        // initialize my socket
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        // bind my socket to my port
		bind(sockfd, res->ai_addr, res->ai_addrlen);

        // set my socket to be non-clocking
        fcntl(sockfd, F_SETFL, O_NONBLOCK);

		cout<<"Connecting To Local Server"<<endl;
		return 1;
	}

    // If a mesage is recieved on my port then 
	int getMessageRecieved(char * buff) {
		byte_count = recvfrom(sockfd, buff, sizeof buf, 0, &addr, &fromlen);
        return byte_count;
	}

    char* getBuffer() { return buf; };

	private:
    struct timeval tv;
    fd_set readfds;
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
    Master() {};
    ~Master() {};
    private:
};

// This represents a slave machine
class Slave : public Machine {
    public:
    Slave() {};
    ~Slave() {};
    private:
};

// This represents a session
class Session {
    public:
    private:
    string name;
    Master* creator;
    vector<Slave* > slaves;
};

int main() {

    char b1[1024];
    char b2[1024];

    SocketMonitor m4, m6;
    m4.connectToLocalServerAtPort(7104);
    m6.connectToLocalServerAtPort(7106);

    while(1) {
        if(m4.getMessageRecieved(b1) > 0) {
            cout<<b1<<endl;
            memset(b1, 0, sizeof(b1));
        }
        if(m6.getMessageRecieved(b2) > 0) {
            cout<<b2<<endl;
            memset(b2, 0, sizeof(b2));
        }
    }
    return 0;
}