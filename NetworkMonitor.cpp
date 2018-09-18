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
    ~Machine() {};
    int parseStartup(vector<string>* msg) {
        if(msg->size() != 3) {
            cout<<"Machine startup message includes "<<msg->size()<<" parameters, must include exactly 3 to start a machine.";
            return -1;
        }
        machineID = (*msg)[1];
        version = "not yet reported.";
        fps = "not yet reported.";
        sessionToJoin = (*msg)[2];
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
    string getSessionToJoin() { return sessionToJoin; };

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

    void parseStartup(vector<string>* msg) {
        name = (*msg)[1];
        creatorID = (*msg)[2];
        if(msg->size() == 3) {
            // This is a private session that no slaves are allowed to join.
        } else {
            // Add the permitted machine IDs
            for(int i = 3; i < msg->size(); i++) {
                cout<<i<<": "<<(*msg)[i]<<endl;
                permittedMachineIDs.push_back((*msg)[i]);
            }
        }
    };

    void addSlave(Machine* machine) {
        slaveMachineIDs.push_back(machine->getMachineID());
        // slaveMachines.push_back(machine);
    }

    string getName() { return name; };
    string getCreatorID() { return creatorID; };
    vector<string> getPermittedMachineIDs() { return permittedMachineIDs; };
    vector<string> getSlaveMachineIDs() { return slaveMachineIDs; };

    private:
    string name;
    string creatorID;
    vector<string> permittedMachineIDs;
    vector<string> slaveMachineIDs;
    // vector<Machine* > slaveMachine;

};

vector< Machine* > machines;
vector< Session* > sessions;

int onStartupMessageRecieved(char * buffer) {
    // First we'll split our string on ur expected delimiter:
    vector<string> message = split(buffer, '|');
    if(message.size() > 0) {
        string messageType = message[0];
        if(messageType == "SESSION2") {
            // it's a session startup moment.
            Session* session = new Session();
            session->parseStartup(&message);
            for(int i = 0; i < sessions.size(); i++) {
                if(sessions[i]->getName() == session->getName()) {
                    if(sessions[i]->getCreatorID() == session->getCreatorID()) {
                        // This session has already started up! so let's get rid of the one we just made.
                        delete session;
                        return 1;
                    }
                }
            }
            sessions.push_back(session);
        } else if(messageType == "MACHINE") {
            // it's a machine startup moment.
            Machine* machine = new Machine();
            machine->parseStartup(&message);
            for(int i = 0; i < machines.size(); i++) {
                if(machines[i]->getMachineID() == machine->getMachineID()) {
                    // This machine has already started up! so let's get rid of the one we just made.
                    delete machine;
                    return 1;
                }
            }
            machines.push_back(machine);
            // Now check if this machine wants to join an existing session, and if it does join them!
            for(int i = 0; i < sessions.size(); i++) {
                if(sessions[i]->getName() == machine->getSessionToJoin()) {
                    for(int j = 0; j < sessions[i]->getPermittedMachineIDs().size(); j++) {
                        if(sessions[i]->getPermittedMachineIDs()[j] == machine->getMachineID()) {
                            sessions[i]->addSlave(machine);
                        }
                    }
                }
            }
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

void printStatus() {
    cout<<"-----------------------------"<<endl;
    cout<<"Machines"<<endl;
    cout<<"-----------------------------"<<endl;
    for(int i = 0; i < machines.size(); i++) {
        cout<<i<<" Name: "<<machines[i]->getMachineID()<<" version: "<<machines[i]->getVersion()<<" fps: "<<machines[i]->getFps()<<endl;
    }
    cout<<"Sessions"<<endl;
    cout<<"-----------------------------"<<endl;
    for(int i = 0; i < sessions.size(); i++) {
        cout<<i<<" Name: "<<sessions[i]->getName()<<" creator: "<<sessions[i]->getCreatorID()<<" permitted Machines: " << endl;
        vector<string> permittedMachineIDs = sessions[i]->getPermittedMachineIDs();
        vector<string> slaveMachineIDs = sessions[i]->getSlaveMachineIDs();
        for(int j = 0; j < permittedMachineIDs.size(); j++) {
            bool present = false;
            for(int k = 0; k < slaveMachineIDs.size(); k++) {
                if(slaveMachineIDs[k] == permittedMachineIDs[j]) {
                    present = true;
                    break;
                }
            }
            cout<<j<<" "<<permittedMachineIDs[j]<< " Present: "<< present <<endl;
        }
    }
    cout<<"-----------------------------"<<endl;
    cout<<endl;
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
            printStatus();
        }
    }
    return 0;
}