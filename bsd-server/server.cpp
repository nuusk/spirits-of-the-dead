#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <vector>
#include <cstring>
#include <string>
#include <sstream>

#define PORT 1252
#define BUF_SIZE 65536

using namespace std;

struct Client
{
    int fd;
    char* address;
    int port;
    string name;
    bool readyToPlay;

    Client(int f, char* ip, int p)
    {
        fd = f;
        address = ip;
        port = p;
        name = "";
        readyToPlay = false;
    }

    bool operator==(const Client &rhs)
    {
        if (this->fd == rhs.fd && 
            this->address == rhs.address &&
            this->port == rhs.port)
            return true;

        return false;
    }
};

struct GameManager 
{
    const int playersToStart = 2;
    const string welcomeText = "Hello! It's 'Spirit of the Dead' text game. Enjoy!\n";
    
    bool gameStarted = false;
    vector<Client> clients;
    int readyPlayersCount = 0;
    
    string getPlayersInfo()
    {
        stringstream ss;
        ss << "Players online: " << clients.size() << endl
           << "Players ready: " << readyPlayersCount << endl;

        return ss.str(); 
    }

    void gameStartCheck()
    {
        if ((int)clients.size() >= playersToStart && (int)clients.size() == readyPlayersCount)
        {
            cout << "GameManager: Enough players to start the game! Let's play!" << endl;
            gameStarted = true;
        }
    }
};


void error(string);
void prepareSocket(int &sock, sockaddr_in &address);
void createSocket(int &sock);
void setSocketOption(int sock);
void setSocketAddress(sockaddr_in &address);
void bindAddress(int sock, sockaddr_in &address);
void startListening(int sock);
void prepareEpoll(int sock, int &fd, epoll_event &event);
void createEpollFd(int &fd);
void addEpollEvent(int sock, int fd, epoll_event &event);
void update(int serverSocket, int epollFd, epoll_event &event, GameManager &gm);
void epollWait(int fd, epoll_event &event);
void addNewClient(int serverSock, int epollFd, epoll_event &event, GameManager &gm);
Client acceptClient(int serverSock);
void rejectClient(int serverSock);
bool isAlreadyConnected(Client client, vector<Client> &clients);
void removeCheater(Client client);
void handleClients(GameManager &gm, epoll_event &event);
void removeClient(GameManager &gm, int index);
bool readMessage(Client &client, char* buf);
bool writeMessage(string message, Client &client);
void sendMessageToAll(string message, GameManager &gm);
void sendMessageToAll(string message, GameManager &gm, Client &sender);
void waitForClients(GameManager &gm, epoll_event &event);
void playGame(GameManager &gm, epoll_event &event);
void processMessage(string message, Client &client, GameManager &gm);
void setClientName(Client &client, string name);
void setClientReady(Client &client, GameManager &gm);

int main(int argc, char** argv)
{
    GameManager gameManager;
    int serverSocket;
    sockaddr_in address;
    int epollFd;
    epoll_event event;

    prepareSocket(serverSocket, address);
    prepareEpoll(serverSocket, epollFd, event);

    while (true)
        update(serverSocket, epollFd, event, gameManager);

    close(serverSocket);
    close(epollFd);

    return 0;
}


void prepareSocket(int &sock, sockaddr_in &address)
{
    createSocket(sock);
    setSocketOption(sock);
    setSocketAddress(address);
    bindAddress(sock, address);
    startListening(sock);
}

void createSocket(int &sock)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error("socket() error!");
}

void setSocketOption(int sock)
{
    const int one = 1;
    int result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (result == -1)   
        error("setsockopt() error!");
}

void setSocketAddress(sockaddr_in &address)
{
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr = { htonl(INADDR_ANY) };
}

void bindAddress(int sock, sockaddr_in &address)
{
    int result = bind(sock, (sockaddr*)&address, sizeof(address));
  	if (result == -1)
        error("bind() error!");
}

void startListening(int sock)
{
	int result = listen(sock, 1);
	if (result == -1)
        error("listen() error!");
}

void prepareEpoll(int sock, int &fd, epoll_event &event)
{
    createEpollFd(fd);
    addEpollEvent(sock, fd, event);
}

void createEpollFd(int &fd)
{
    fd = epoll_create1(0);
    if (fd == -1)
        error("epoll_create() error!");
}

void addEpollEvent(int sock, int fd, epoll_event &event)
{
    event.data.fd = sock;
    event.events = EPOLLIN;

    int result = epoll_ctl(fd, EPOLL_CTL_ADD, sock, &event);
    if (result == -1)
        error("epoll_ctl() error!");
}

void update(int serverSocket, int epollFd, epoll_event &event, GameManager &gm)
{
    epollWait(epollFd, event);

    if (event.events == EPOLLIN && event.data.fd == serverSocket)
    {
        if (gm.gameStarted) 
        {
            rejectClient(serverSocket);
            return;
        }

        addNewClient(serverSocket, epollFd, event, gm);
        sendMessageToAll(gm.getPlayersInfo(),  gm);      
    }
    else
        handleClients(gm, event);
}

void epollWait(int fd, epoll_event &event)
{
    int result = epoll_wait(fd, &event, 1, -1);
    if (result == -1)
        error("epoll_wait() error!");
}

//póki co można się łączyć z tego samego IP kilka razy
void addNewClient(int serverSock, int epollFd, epoll_event &event, GameManager &gm)
{
    Client client = acceptClient(serverSock);
    // if (isAlreadyConnected(client, gm.clients))
    // {
    //     removeCheater(client);
    //     return;
    // }

    addEpollEvent(client.fd, epollFd, event);
    gm.clients.push_back(client);

    cout << "New connection from: " << client.address << ":" << client.port << " fd: " <<  client.fd << endl;
    writeMessage(gm.welcomeText, client);
}

bool isAlreadyConnected(Client client, vector<Client> &clients)
{
    for (Client c : clients)
    {
        if (c.address == client.address)
            return true;
    }

    return false;
}

void removeCheater(Client client)
{
    cout << "Client: " << client.address << ":" << client.port << " fd: " << client.fd << " is trying to cheat! Removing him..." << endl;
    writeMessage("Bye bye, cheater!\n", client);
    close(client.fd);
}

Client acceptClient(int serverSock)
{
    int socket;
    sockaddr_in clientInfo;
    socklen_t clientInfoSize = sizeof(clientInfo);

    socket = accept(serverSock, (sockaddr*)&clientInfo, &clientInfoSize);
    if (socket == -1)
        error("accept() error!");

    return Client(socket, inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
}

void rejectClient(int serverSock)
{
    Client client = acceptClient(serverSock);
    
    cout << "Client: " << client.address << ":" << client.port << " fd: " << client.fd << " wanted to connect while the game is running. Removing him..." << endl;    
    writeMessage("The game has already started! Please try again later!\n", client);
    close(client.fd);
}

void handleClients(GameManager &gm, epoll_event &event)
{
    if (!gm.gameStarted)
        waitForClients(gm, event);
    else
        playGame(gm, event);
}

void removeClient(GameManager &gm, int k)
{
    cout << "Removing client: " << gm.clients[k].address << ":" << gm.clients[k].port << " fd: " << gm.clients[k].fd << endl;
    close(gm.clients[k].fd);
    gm.clients.erase(gm.clients.begin() + k);
}

bool readMessage(Client &client, char* buf)
{
    memset(buf, 0, BUF_SIZE);

    int msgSize = read(client.fd, buf, BUF_SIZE);
    if (msgSize < 1)
        return false;

    return true;
}

void error(string errorMessage) 
{
    perror(errorMessage.c_str());
    exit(0);
}

bool writeMessage(string message, Client &client)
{
    int result = write(client.fd, message.c_str(), message.size());
    if (result == -1)
        return false;

    return true;
}

void sendMessageToAll(string message, GameManager &gm)
{
    for (int i = 0; i < (int)gm.clients.size(); i++)
    {
        if (!writeMessage(message, gm.clients[i]))
            removeClient(gm, i);
    }
}

void sendMessageToAll(string message, GameManager &gm, Client &sender)
{
    for (int i = 0; i < (int)gm.clients.size(); i++)    
    {
        if (gm.clients[i] == sender)
            continue;

        if (!writeMessage(message, gm.clients[i]))
            removeClient(gm, i);
    }
}

void waitForClients(GameManager &gm, epoll_event &event)
{
    char buf[BUF_SIZE];
    
    for (int i = 0; i < (int)gm.clients.size(); i++)
    {
        if (event.events == EPOLLIN && event.data.fd == gm.clients[i].fd)
        {
            if(!readMessage(gm.clients[i], buf))
            {
                removeClient(gm, i);
                break;
            }
                
            processMessage(buf, gm.clients[i], gm);
        }
    }
}

void playGame(GameManager &gm, epoll_event &event)
{
    char buf[BUF_SIZE];
    
    for (int i = 0; i < (int)gm.clients.size(); i++)
    {
        if (event.events == EPOLLIN && event.data.fd == gm.clients[i].fd)
        {
            if(!readMessage(gm.clients[i], buf))
            {
                removeClient(gm, i);
                break;
            }

            // processMessage(buf, gm);
        }
    }
}

void processMessage(string message, Client &client, GameManager &gm)
{
    cout << "Processing message: " << message << endl;

    if (message.find("name") != string::npos)
        setClientName(client, message.substr(message.find("name") +  5));
   
    else if (message.find("ready") != string::npos)
        setClientReady(client, gm);
}

void setClientName(Client &client, string name)
{
    if (client.name != "")
        return;

    client.name = name;
    cout << "Set name of " << client.address << ":" << client.port << " fd: " << client.fd << " to '" << name << "'" << endl; 
}

void setClientReady(Client &client, GameManager &gm)
{
    client.readyToPlay = true;
    gm.readyPlayersCount++;

    sendMessageToAll(gm.getPlayersInfo(), gm);
    cout << "Player: " << client.name << " is ready!" << endl; 

    gm.gameStartCheck();
}