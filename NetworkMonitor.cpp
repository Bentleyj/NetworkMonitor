#include <iostream>
#include <vector>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
    vector<Slave> slaves;
};

int main() {
    cout << "Hello World In Network Monitor" << endl;
    return 0;
}