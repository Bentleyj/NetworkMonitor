// This is a test application that utilizes the Network Monitor class directly.

#include "NetworkMonitor.cpp"
#include <iostream>
#include <time.h>

int main() 
{

    NetworkMonitor nm;

    if(nm.setupHeartbeatSocketMonitor(7104) < 0) 
    {
        cout<<"Fatal: Failed To Connect to Port "<<7104<<endl;
        exit(1);
    }
    if(nm.setupStartupSocketMonitor(7106) < 0) 
    {
        cout<<"Fatal: Failed To Connect to Port "<<7106<<endl;
        exit(1);
    }

    clock_t timeOfLastPrint = clock();
    clock_t t = clock();

    while(1) 
    {
        nm.monitorNetwork();

        t = clock() - timeOfLastPrint;
        float printGapInSeconds = ((float)t)/CLOCKS_PER_SEC;
        if(printGapInSeconds > 2.0) 
        {
            // cout<<(int)((float)clock())/CLOCKS_PER_SEC<<endl; // optioally report the time
            nm.printStatus();
            timeOfLastPrint = clock();
        }
    }

    return 0;
}