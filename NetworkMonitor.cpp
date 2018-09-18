#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>

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

// Utilities Functions
vector<string> split(const string& s, char delimiter) {
   vector<string> tokens;
   string token;
   stringstream tokenStream(s);
   while (getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
   }
   return tokens;
}

// this will be our parent class for both masters and slaves
class Machine {
    public:
    Machine() {};
    ~Machine() { cout << "Die Machine!" << endl; };
    int parseStartup(vector<string>* msg) {
        if(msg->size() != 3) {
            cout<<"Machine startup message includes "<<msg->size()<<" parameters, must include exactly 3 to start a machine.";
            return -1;
        }
        machineID = (*msg)[1];
        version = "not yet reported.";
        fps = "not yet reported.";
        return 1; 
    }

    int parseHeartbeat(vector<string>* msg) {
        if(msg->size() != 4) {
            cout<<"Machine heartbeat message includes "<<msg->size()<<" parameters, must include exactly 4 to register as a machine heartbeat.";
            return -1;
        }
        if(machineID == (*msg)[1]) {
            fps = (*msg)[3];
            version = (*msg)[2];
        }
        return 1; 
    }

    string getVersion() { return version; };
    string getMachineID() { return machineID; };
    string getFps() { return fps; };

    protected:
    string machineID;
    string version;
    string fps;
    string sessionToJoin;
};

// This represents a session
class Session {
    public:
    Session() {};
    ~Session() {};
    void parseStartup(vector<string>* msg);
    private:
    string name;
    Machine* creator;
    vector<string > permittedMachineIDs;
    vector<Machine* > slaves;
};

vector<Machine* > machines;
vector<Session* > sessions;

int onStartupMessageRecieved(char * buffer) {
    // First we'll split our string on ur expected delimiter:
    vector<string> message = split(buffer, '|');
    // for(int i = 0; i < message.size(); i++) {
    //     cout<<message[i]<<endl;
    // }
    if(message.size() > 0) {
        string messageType = message[0];
        if(messageType == "SESSION2") {
            // it's a master startup moment.
            // Session* session = new Session();
            // session->parseStartup(&message);
            // sessions.push_back(session);
            // master
        } else if(messageType == "MACHINE") {
            // it's a machine startup moment.
            Machine* machine = new Machine();
            machine->parseStartup(&message);
            for(int i = 0; i < machines.size(); i++) {
                if(machines[i]->getMachineID() == machine->getMachineID()) {
                    // We this machine has already started up!
                    delete machine;
                    return 1;
                }
            }
            machines.push_back(machine);
        } else {
            // It's an unknown message that we can ignore.
            cout<<"Recieved Message: "<<buffer<<" has a malformed type. Valid types are: MACHINE or SESSION2"<<endl;
            return -1;
        }
    } else {
        cout<<"Recieved message: " << buffer << " is malformed and contains no delimiters"<<endl;
        return -1;
    }
    return 1;
}

int onHeartbeatMessageRecieved(char * buffer) {
    vector<string> message = split(buffer, '|');
    if(message.size() > 0) {
        string messageType = message[0];
        if(messageType == "MACHINESTATUS") {
            // it's a machine heartbeat moment.
            for(int i = 0; i < machines.size(); i++) {
                machines[i]->parseHeartbeat(&message);
            }
        } else {
            // It's an unknown message that we can ignore.
            cout<<"Recieved message: "<< buffer << " has a malformed type. Valid type is: MACHINESTATUS"<<endl;
            return -1;
        }
    } else {
        cout<<"Recieved message: " << buffer << " is malformed and contains no delimiters"<<endl;
        return -1;
    }
    return 1;
}

int main() {

    char b1[1024];
    char b2[1024];

    SocketMonitor m4, m6;
    m4.connectToLocalServerAtPort(7104);
    m6.connectToLocalServerAtPort(7106);

    while(1) {
        if(m4.getMessageRecieved(b1) > 0) {
            // cout<<b1<<endl;            
            onHeartbeatMessageRecieved(b1);
            memset(b1, 0, sizeof(b1));
        }
        if(m6.getMessageRecieved(b2) > 0) {
            // cout<<b2<<endl;
            onStartupMessageRecieved(b2);
            memset(b2, 0, sizeof(b2));
            cout<<"Total Machines: "<<machines.size()<<endl;
            cout<<"-----------------------------"<<endl;
            for(int i = 0; i < machines.size(); i++) {
                cout<<i<<" Name: "<<machines[i]->getMachineID()<<" version: "<<machines[i]->getVersion()<<" fps: "<<machines[i]->getFps()<<endl;
            }
            cout<<endl;
        }
    }
    return 0;
}