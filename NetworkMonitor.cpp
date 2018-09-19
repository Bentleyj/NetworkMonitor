#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <time.h>


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
    virtual int parseStartup(vector<string>* msg) {
        if(msg->size() != 3) {
            cout<<"Machine startup message includes "<<msg->size()<<" parameters, must include exactly 3 to start a machine.";
            return -1;
        }
        machineID = (*msg)[1];
        version = "not yet reported.";
        fps = "not yet reported.";
        sessionToJoinName = (*msg)[2];
        timeOfLastHeartbeat = clock();
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
        timeOfLastHeartbeat = clock();
        return 1; 
    }

    bool isTimedOut() {
        clock_t t = clock() - timeOfLastHeartbeat;
        float heartbeatGapInSeconds = ((float)t)/CLOCKS_PER_SEC;
        if(heartbeatGapInSeconds > 1.0) {
            return true;
        }
        return false;
    }

    virtual void printDescription() {
        cout<<"ID: "<<machineID<<" version: "<<version<<" fps: "<<fps<<endl;
    }

    string getVersion() { return version; };
    string getMachineID() { return machineID; };
    string getFps() { return fps; };
    string getSessionToJoinName() { return sessionToJoinName; };

    protected:
    clock_t timeOfLastHeartbeat;
    string machineID;
    string version;
    string fps;
    string sessionToJoinName;
    vector<Machine* > masters;
};

// This represents a session
class Master : public Machine {
    public:

    virtual int parseStartup(vector<string>* msg) {
        sessionName = (*msg)[1];
        machineID = (*msg)[2];
        if(msg->size() == 3) {
            // This is a private session that no slaves are allowed to join.
        } else {
            // Add the permitted machine IDs
            for(int i = 3; i < msg->size(); i++) {
                permittedMachineIDs.push_back((*msg)[i]);
            }
        }
        timeOfLastHeartbeat = clock();
        return 1;
    };

    // Try to add any machines to this session if any want to join.
    void tryToAddMachinesAsSlaves(vector<Machine*> machines) {
        for(vector<Machine*>::iterator machine = machines.begin(); machine != machines.end(); machine++) {
            if((*machine)->getSessionToJoinName() == sessionName) {
                for(vector<string>::iterator permittedMachineID = permittedMachineIDs.begin(); permittedMachineID != permittedMachineIDs.end(); permittedMachineID++) {
                    if((*machine)->getMachineID() == *permittedMachineID) {
                        addSlave(*machine);
                    }
                }
            }
        }
    }

    virtual void printDescription() {
        cout<<"Master ID: "<<machineID<<" version: "<<version<<" fps: "<<fps<<endl;
        cout<<"Session Name "<<sessionName<<endl;
        cout<<"Permitted Machines: "<<endl;
        for(int i = 0; i < permittedMachineIDs.size(); i++) {
            bool present = false;
            for(int j = 0; j < slaveMachines.size(); j++) {
                if(slaveMachines[j] != nullptr) {
                    if(slaveMachines[j]->getMachineID() == permittedMachineIDs[i]) {
                        present = true;
                    }
                }
            }
            cout<<permittedMachineIDs[i]<<" Present: "<<present<<endl;
        }
    }

    void addSlave(Machine* machine) {
        slaveMachines.push_back(machine);
    }

    void removeSlave(Machine* machine) {
        vector<Machine*>::iterator it = find(slaveMachines.begin(), slaveMachines.end(), machine);
        if(it != slaveMachines.end()) {
            slaveMachines.erase(it);
        }
    }

    string getSessionName() { return sessionName; };
    vector<string> getPermittedMachineIDs() { return permittedMachineIDs; };
    vector<Machine *> getSlaveMachines() { return slaveMachines; };

    private:
    string sessionName;
    vector<string> permittedMachineIDs;
    vector<Machine* > slaveMachines;

};

vector< Machine* > machines;
vector< Master* > masters;

int onStartupMessageRecieved(char * buffer) {
    // First we'll split our string on ur expected delimiter:
    vector<string> message = split(buffer, '|');
    if(message.size() > 0) {
        string messageType = message[0];
        if(messageType == "SESSION2") {
            Master* master = new Master();
            master->parseStartup(&message);
            for(int i = 0; i < machines.size(); i++) {
                if(machines[i]->getMachineID() == master->getMachineID()) {
                    // This master has already started up! so let's get rid of the one we just made.
                    delete master;
                    return 1;
                }
            }
            master->tryToAddMachinesAsSlaves(machines);
            machines.push_back(master);
            masters.push_back(master);
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
            for(int i = 0; i < masters.size(); i++) {
                masters[i]->tryToAddMachinesAsSlaves(machines); // This could be optimised.
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

void checkForTimeouts(vector<Machine *>* machines, vector<Master *>* masters) {
    for(vector<Machine*>::iterator machineIt = machines->begin(); machineIt != machines->end(); ) {
        if((*machineIt)->isTimedOut()) {
            cout<<"Machine: "<< (*machineIt)->getMachineID()<<" Timed Out"<<endl;
            for(vector<Master *>::iterator masterIt = masters->begin(); masterIt != masters->end(); masterIt++) {
                vector<Machine*> slaves = (*masterIt)->getSlaveMachines();
                for(int i = 0; i < slaves.size(); i++) {
                    if(slaves[i] == (*machineIt)) {
                        (*masterIt)->removeSlave(slaves[i]);
                    }
                }
            }
            delete (*machineIt);
            machineIt = machines->erase(machineIt);
        } else {
            machineIt++;
        }
    }
}

void printStatus() {
    cout<<"Machines"<<endl;
    cout<<"-----------------------------"<<endl;
    for(int i = 0; i < machines.size(); i++) {
        machines[i]->printDescription();
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
        checkForTimeouts(&machines, &masters);

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