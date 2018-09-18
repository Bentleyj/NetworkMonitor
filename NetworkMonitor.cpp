#include "ConnectionController.cpp"

using namespace std;

// this will be our parent class for both masters and slaves
class Machine {
    public:
    Machine();
    ~Machine();
    private:
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
    Master master;
};

int main() {

    char buffer[1024];
    ConnectionController c;
    c.setIPAndPort("127.0.0.1", 7106);
    c.connectToServer();
    c.getSockStatus();
    // while(1) {
    //     int status = c.recieveMessage(buffer);
    //     if(status > 0) {
    //         cout<<"Recieved: "<<buffer<<endl;
    //     }
    // }
    return 0;
}