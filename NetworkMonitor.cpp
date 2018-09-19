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

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// This is a class for monitoring sockets using basic C++ networking. It is based on the pages found here: http://beej.us/guide/bgnet/html/multi/getaddrinfoman.html
// It has 2 relevant functions.
// 1. It connects to a port on the local network and sets itself up as a passive, non-blocking listener
// 2. It Recieved a message on the specified port and fills a buffer with that message.
class SocketMonitor {
	public:
    SocketMonitor() {};
    ~SocketMonitor() {
        freeaddrinfo(res);
    };

	int connectToLocalServerAtPort(int port) {
        // Set my socket parameters
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;

        // create my socket
        string p = to_string(port);

        // get local addr info for local port
        int status;
        status = getaddrinfo(NULL, p.c_str(), &hints, &res);
        if(status != 0) {
            cout<<"SocketMonitor::connectToLocalServerAtPort: Cannot create connection due to getaddrinfo() Error: "<<gai_strerror(status)<<endl;
            return -1;
        }

        // initialize my socket
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(sockfd == -1) {
            cout<<"SocketMonitor::connectToLocalServerAtPort: Cannot create connection due to socket() Error : "<<strerror(errno)<<endl;
            return -1;
        }

        int yes=1;
        // lose the "Address already in use" error message
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            cout<<"SocketMonitor::connectToLocalServerAtPort: Cannot create connection due to setsockopt() Error : "<<strerror(errno)<<endl;
            return -1;
        }

        // bind my socket to my port
        if(::bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
            cout<<"SocketMonitor::connectToLocalServerAtPort: Cannot create connection due to bind() Error : "<<strerror(errno)<<endl;
            return -1;
        }

        // set my socket to be non-blocking
        if(fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
            cout<<"SocketMonitor::connectToLocalServerAtPort: Cannot set connection to non-blocking due to fcntl Error : "<<strerror(errno)<<endl;
            return -1;
        }

        fromlen = sizeof addr;

		cout<<"Connected Successfully To Local Server at port: "<<port<<endl;
		return 1;
	}

    // If a mesage is recieved on my port then 
	int getMessageRecieved(char * buff) {
		byte_count = recvfrom(sockfd, buff, sizeof buf, 0, &addr, &fromlen);
        return byte_count;
	}

    // char* getBuffer() { return buf; };

	private:
	struct addrinfo hints,  // Hints is our description of our address we want to create
                    *res;   // res is the address we actually create using the hints
	int sockfd;             // This is our socket descriptor
	int byte_count;         // This is the number of bytes that our message included if it was recieved.
	socklen_t fromlen;      // This is the length of the socket addr that we're recieving messages from.
	struct sockaddr addr;   // Address struct to tell us about where we recieved our message
	char buf[1024];         // dummy buffer with a sized size.
};

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// This class represents both masters and slaves. Slaves have no extra functionality bseides this but masters will.
// It has 4 main functions:
// 1. Parse a startup message like a slave machine would and set its parameters based on that.
// 2. Parse a hearbeat message and update its parameters.
// 3. Check if it has timed out, a timeout counts as not sending a heartbeat for more than 1 second.
// 4. Print a description of itself to the console.
class Machine {
    public:
    Machine() { 
        minimumStartupMessageSize = 3; 
        minimumHeartbeatMessageSize = 4;
    };
    ~Machine() {};

    // parse a startup message
    virtual int parseStartup(vector<string>* msg) {
        enum StartupMessageIndex {
            TYPE,
            MACHINE_ID,
            SESSION_TO_JOIN_NAME,
        };
        if(msg->size() != minimumStartupMessageSize) {
            cout<<"Machine startup message includes "<<msg->size()<<" parameters, must include exactly 3 to start a machine.";
            return -1;
        }
        machineID = (*msg)[MACHINE_ID];
        version = "";
        fps = "";
        sessionToJoinName = (*msg)[SESSION_TO_JOIN_NAME];
        timeOfLastHeartbeat = clock();
        return 1; 
    }
    
    // parse a heartbeat message. Update your parameters accordingly.
    int parseHeartbeat(vector<string>* msg) {
        enum HeartbeatMessageIndex {
            TYPE,
            MACHINE_ID,
            VERSION,
            FPS
        };
        if(msg->size() != minimumHeartbeatMessageSize) {
            cout<<"Machine heartbeat message includes "<<msg->size()<<" parameters, must include exactly 4 to register as a machine heartbeat.";
            return -1;
        }
        if(machineID == (*msg)[MACHINE_ID]) {
            fps = (*msg)[FPS];
            version = (*msg)[VERSION];
        }
        timeOfLastHeartbeat = clock();
        return 1; 
    }

    // Determine if you have times out by not sending a heartbeat. The protocol demands that machines send a heartbeat once a second and so that is ourtime at which we will say a machine has timed out.
    bool isTimedOut() {
        clock_t t = clock() - timeOfLastHeartbeat;
        float heartbeatGapInSeconds = ((float)t)/CLOCKS_PER_SEC;
        if(heartbeatGapInSeconds > 1.0) {
            return true;
        }
        return false;
    }

    // Print a description of yourself.
    virtual void printDescription() {
        cout<<"ID: "<<machineID<<" version: "<<version<<" fps: "<<fps<<endl;
    }

    // Getters
    string getVersion() { return version; };
    string getMachineID() { return machineID; };
    string getFps() { return fps; };
    string getSessionToJoinName() { return sessionToJoinName; };

    protected:
    clock_t timeOfLastHeartbeat;    // Time of last heartbeat, also set to the time of machine startup initially.
    string machineID;               // machine ID which identifies it. We are assuming that these are unique on the network.
    string version;                 // machine version.
    string fps;                     // fps that the machine is running at. - reported by Heartbeat
    string sessionToJoinName;       // name of the session that you wish to join.
    int minimumStartupMessageSize;
    int minimumHeartbeatMessageSize;
};

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// This represents a Machine that controls a session. It has 3 main functions.
// 1. It parses a session startup message in such a way that it can create it's own session. While doing this it will add any machines that already exist on the network that want to join its session to its session.
// 2. It adds new machines as slaves when they start up if they are eligible to join the session and they want to.
// 3. It prints a description of itself to the console.
class Master : public Machine {
    public:
    Master() {
        Machine();
        minimumStartupMessageSize = 3;
    };

    // This parses a startup string that starts with the message type SESSION2.
    virtual int parseStartup(vector<string>* msg) {
        enum StartupMessageIndex {
            TYPE,
            SESSION_NAME,
            MACHINE_ID,
            FIRST_PERMITTED_SLAVE_ID
        };
        if(msg->size() < minimumStartupMessageSize) {
            cout<<"To parse a startup message for a Master machine we need at least "<<minimumStartupMessageSize<<" parameters. but we only have: "<<msg->size()<<endl;
            return -1;
        }
        // We are assuming here that our session is well formed, we should maybe check this.
        sessionName = (*msg)[SESSION_NAME];
        machineID = (*msg)[MACHINE_ID];
        if(msg->size() == minimumStartupMessageSize) {
            // This is a private session that no slaves are allowed to join.
        } else {
            // Add the permitted machine IDs
            for(int i = FIRST_PERMITTED_SLAVE_ID; i < msg->size(); i++) {
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

    // Try to add a single machine to this session if any want to join.
    void tryToAddMachineAsSlave(Machine* machine) {
        if(machine->getSessionToJoinName() == sessionName) {
            for(vector<string>::iterator permittedMachineID = permittedMachineIDs.begin(); permittedMachineID != permittedMachineIDs.end(); permittedMachineID++) {
                if(machine->getMachineID() == *permittedMachineID) {
                    addSlave(machine);
                }
            }
        }
    }

    // Descirbe yourself, your statistics, and your session
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

    // Add a slave machine to your officially connected slaves.
    void addSlave(Machine* machine) {
        slaveMachines.push_back(machine);
    }

    // remove a slave machine from your officially connected slaves.
    void removeSlave(Machine* machine) {
        vector<Machine*>::iterator it = find(slaveMachines.begin(), slaveMachines.end(), machine);
        if(it != slaveMachines.end()) {
            slaveMachines.erase(it);
        }
    }

    // Getters
    string getSessionName() { return sessionName; };
    vector<string> getPermittedMachineIDs() { return permittedMachineIDs; };
    vector<Machine *> getSlaveMachines() { return slaveMachines; };

    private:
    string sessionName;     // This is the name of your session, used to determine if slaves want to join it.
    vector<string> permittedMachineIDs; // This is a list of all the machine IDs that are permitted to join our session
    vector<Machine* > slaveMachines;    // This is a list of pointers to slave machines that are active on the network and have successfully joined our session.
    int minimumStartupMessageSize;
};

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------------//
// This is our main workhorse clss which utilises the others to monitor the network and keep track of everything. It has 3 main functions:
// 1. Watch sockets for incoming data and parse them to see if they are well formed and can lead to the creation of a machine.
// 2. Keep track of which machines are timing out.
// 3. Print the status of all it's machines to the console.
// TODO: This could be more general if instead of having 2 named monitors I had a vector of pairs of Socket Monitors and pointers to handler functions and iterated over those.
 class NetworkMonitor {
    private:
    vector< Machine* > machines;    // This is a list of all the machines on the system, both masters and slaves.
    vector< Master* > masters;      // This is a list of only the masters which we keep around for convenience

    SocketMonitor smHeartbeat, smStartup;   // Two socket monitors for monitoring our two sockets.
    
    char heartbeatMessageBuffer[1024];  // Persistent message buffers could be helpful for debugging.
    char startupMessageBuffer[1024];

    public:

    NetworkMonitor() {};
    ~NetworkMonitor() {
        // We only need to clear the machines as the masters list is a subset of the machines list.
        for(int i = 0; i < machines.size(); i++) {
            delete machines[i];
        }
    };

    // Monitor the Network
    void monitorNetwork() {
        checkForTimeouts(&machines, &masters);  // Check all the machiens for timeouts and remove those that have timed out.
        if(smHeartbeat.getMessageRecieved(heartbeatMessageBuffer) > 0) {    // Check if there is a heartbeat message
            if(onHeartbeatMessageRecieved(heartbeatMessageBuffer) > 0) {
                // We have a malformed heartbear message but we don't really care.
            }
            // printStatus();
            memset(heartbeatMessageBuffer, 0, sizeof(heartbeatMessageBuffer));
        }
        if(smStartup.getMessageRecieved(startupMessageBuffer) > 0) {    // Check if there is a startup message
            if(onStartupMessageRecieved(startupMessageBuffer) > 0) {
                // We have a malformed startup message, but we don't really care.
            }
            memset(startupMessageBuffer, 0, sizeof(startupMessageBuffer));
        }
    }

    // Setup my heartbeat socket monitor
    int setupHeartbeatSocketMonitor(int port) {
        return smHeartbeat.connectToLocalServerAtPort(port);
    }

    // Setup my Startup socket monitor
    int setupStartupSocketMonitor(int port) {
        return smStartup.connectToLocalServerAtPort(port);
    }

    // Handle Startup messages by trying to create machines that can be masters.
    int onStartupMessageRecieved(char * buffer) {
        // First we'll split our string on ur expected delimiter:
        vector<string> message = split(buffer, '|');
        if(message.size() > 0) {
            string messageType = message[0];
            if(messageType == "SESSION2") {
                Master* master = new Master();
                if(master->parseStartup(&message) > 0) {
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
                } else {
                    cout<<"Startup Message: "<<buffer<<" was malformed and could not be parsed."<<endl;
                    delete master;
                    return -1;
                }
            } else if(messageType == "MACHINE") {
                // it's a machine startup moment.
                Machine* machine = new Machine();
                if(machine->parseStartup(&message) > 0) {
                    for(int i = 0; i < machines.size(); i++) {
                        if(machines[i]->getMachineID() == machine->getMachineID()) {
                            // This machine has already started up! so we don't need to add it to the list but we should try to see if it wants to join a new master.
                            for(int i = 0; i < masters.size(); i++) {
                                masters[i]->tryToAddMachinesAsSlaves(machines); // This could be optimised.
                            }
                            delete machine;
                            return 1;
                        }
                    }
                    // It's a new machine! How exciting. We can now try to add it to all the masters as a slave.
                    machines.push_back(machine);
                    for(int i = 0; i < masters.size(); i++) {
                        masters[i]->tryToAddMachineAsSlave(machine); // This could be optimised.
                    }
                } else {
                    cout<<"Startup Message: "<<buffer<<" was malformed and could not be parsed."<<endl;
                    delete machine;
                    return -1;
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

    // Handle Heartbear messages by updating the parameters of the machine that sent the heartbeat.
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

    // Check if any machines have timed out. If they have remove them from the machines list. Also remove them from any sessions they a slave of.
    void checkForTimeouts(vector<Machine *>* machines, vector<Master *>* masters) {
        // Look through the machines.
        for(vector<Machine*>::iterator machineIt = machines->begin(); machineIt != machines->end(); ) {
            if((*machineIt)->isTimedOut()) { // Check if any machines have timed out
                cout<<"Machine: "<< (*machineIt)->getMachineID()<<" Timed Out"<<endl; // Tell me.
                for(vector<Master *>::iterator masterIt = masters->begin(); masterIt != masters->end(); masterIt++) {
                    vector<Machine*> slaves = (*masterIt)->getSlaveMachines();
                    for(int i = 0; i < slaves.size(); i++) {
                        if(slaves[i] == (*machineIt)) {
                            (*masterIt)->removeSlave(slaves[i]);
                        }
                    }
                }
                vector<Master*>::iterator it = find(masters->begin(), masters->end(), (*machineIt));
                if(it != masters->end()) {
                    masters->erase(it);
                }
                delete (*machineIt);
                machineIt = machines->erase(machineIt);
            } else {
                machineIt++;
            }
        }
    }

    // Print the current status of all the machines in the order in which they connected to the network.
    void printStatus() {
        if(machines.size() > 0) {
            cout<<"Machines"<<endl;
            cout<<"-----------------------------"<<endl;
            for(int i = 0; i < machines.size(); i++) {
                machines[i]->printDescription();
            }
            cout<<"-----------------------------"<<endl;
            cout<<endl;
        }
    }

    // Utility Function for splitting a string.
    vector<string> split(const string& s, char delimiter) {
        vector<string> tokens;
        string token;
        stringstream tokenStream(s);
        while (getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

int main() {

    NetworkMonitor nm;

    clock_t timeOfLastPrint = clock();
    clock_t t = clock();

    if(nm.setupHeartbeatSocketMonitor(7104) < 0) {
        cout<<"Failed To Connect to Port "<<7104<<endl;
        exit(1);
    }
    if(nm.setupStartupSocketMonitor(7106) < 0) {
        cout<<"Failed To Connect to Port "<<7106<<endl;
        exit(1);
    }
    while(1) {
        nm.monitorNetwork();

        t = clock() - timeOfLastPrint;
        float printGapInSeconds = ((float)t)/CLOCKS_PER_SEC;
        if(printGapInSeconds > 2.0) {
            nm.printStatus();
            timeOfLastPrint = clock();
        }
    }

    return 0;
}