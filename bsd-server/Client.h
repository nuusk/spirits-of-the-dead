#include <string>

using namespace std;

struct Client
{
    int fd;
    char* address;
    int port;
    string name;
    bool readyToPlay;
    bool answered;

    Client(int f, char* ip, int p)
    {
        fd = f;
        address = ip;
        port = p;
        name = getRandomName();
        readyToPlay = false;
        answered = false;
    }

    bool operator==(const Client &rhs)
    {
        if (this->fd == rhs.fd && 
            this->address == rhs.address &&
            this->port == rhs.port)
            return true;

        return false;
    }

    string getRandomName()
    {
        string value = "";

        for (int i = 0; i < 10; i++)
            value += rand() % 26 + 97; 

        return value;
    }

    string toString()
    {
        stringstream ss;
        ss << "Address: " << address << endl;
        ss << "Port: " << port << endl;
        ss << "Fd: " << fd << endl;
        ss << "Name: " << name << endl;
        ss << "Ready: " << readyToPlay << endl;
        ss << "Answered: " << answered << endl;

        return ss.str();
    }
};