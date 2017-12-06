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
#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

using namespace std;

struct Client
{
    int fd;
    char* address;
    int port;

    Client(int f, char* ip, int p)
    {
        fd = f;
        address = ip;
        port = p;
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
void update(int serverSocket, int epollFd, epoll_event &event, vector<Client> &clients);
void epollWait(int fd, epoll_event &event);
void addNewClient(int serverSock, int epollFd, epoll_event &event, vector<Client> &clients);
Client acceptClient(int serverSock);
bool isAlreadyConnected(Client client, vector<Client> clients);
void removeCheater(Client client);
void handleClients(int serverSocket, epoll_event &event, vector<Client> &clients);
void removeClient(vector<Client> &clients, int index);
bool readMessage(Client &client, char* buf);
bool writeMessage(char* message, Client &client);
void processMessage(char* message, Client &sender, vector<Client> &clients);
void sendMessageToAll(char* message, vector<Client> &clients);
void sendMessageToAll(char* message, vector<Client> &clients, Client &sender);

int main(int argc, char** argv)
{
    int serverSocket;
    sockaddr_in address;
    int epollFd;
    epoll_event event;
    vector<Client> clients;

    prepareSocket(serverSocket, address);
    prepareEpoll(serverSocket, epollFd, event);

    while (true)
        update(serverSocket, epollFd, event, clients);

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

void update(int serverSocket, int epollFd, epoll_event &event, vector<Client> &clients)
{
    epollWait(epollFd, event);

    if (event.events == EPOLLIN && event.data.fd == serverSocket)
        addNewClient(serverSocket, epollFd, event, clients);

    else
        handleClients(serverSocket, event, clients);

}

void epollWait(int fd, epoll_event &event)
{
    int result = epoll_wait(fd, &event, 1, -1);
    if (result == -1)
        error("epoll_wait() error!");
}

//póki co można się łączyć z tego samego IP kilka razy
void addNewClient(int serverSock, int epollFd, epoll_event &event, vector<Client> &clients)
{
    Client client = acceptClient(serverSock);
    // if (isAlreadyConnected(client, clients))
    // {
    //     removeCheater(client);
    //     return;
    // }

    addEpollEvent(client.fd, epollFd, event);
    clients.push_back(client);
    printf("\nNew connection from: %s:%hu (fd: %d)\n", client.address, client.port, client.fd);
}

bool isAlreadyConnected(Client client, vector<Client> clients)
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
    printf("\nClient: %s:%hu (fd : %d) is trying to cheat! Removing him...\n", client.address, client.port, client.fd);
    close(client.fd);
}

Client acceptClient(int serverSock)
{
    int clientSocket;
    sockaddr_in clientInfo;
    socklen_t clientInfoSize = sizeof(clientInfo);

    clientSocket = accept(serverSock, (sockaddr*)&clientInfo, &clientInfoSize);
    if (clientSocket == -1)
        error("accept() error!");

    return Client(clientSocket, inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
}

void handleClients(int serverSocket, epoll_event &event, vector<Client> &clients)
{
    char buf[BUF_SIZE];

    for (int i = 0; i < (int)clients.size(); i++)
    {
        if (event.events == EPOLLIN && event.data.fd == clients[i].fd)
        {
            if(!readMessage(clients[i], buf))
            {
                removeClient(clients, i);
                break;
            }
            
            printf("\nI've got new message:\n%s", buf);
            
            sendMessageToAll(buf, clients, clients[i]);
            sendMessageToAll(buf, clients);
        }
    }
}

void removeClient(vector<Client> &clients, int k)
{
    printf("\nRemoving client: %s:%hu (fd: %d)\n", clients[k].address, clients[k].port, clients[k].fd);
    close(clients[k].fd);
    clients.erase(clients.begin() + k);
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

bool writeMessage(char* message, Client &client)
{
    string msg = message;
    printf("\nMessage to client:\n%s", message);            
    
    int result = write(client.fd, message, strlen(message));
    if (result == -1)
        return false;

    return true;
}

void processMessage(char* message, Client &sender, vector<Client> &clients)
{
    //TODO
}

void sendMessageToAll(char* message, vector<Client> &clients)
{
    for (int i = 0; i < (int)clients.size(); i++)
    {
        if (!writeMessage(message, clients[i]))
            removeClient(clients, i);
    }
}

void sendMessageToAll(char* message, vector<Client> &clients, Client &sender)
{
    for (int i = 0; i < (int)clients.size(); i++)    
    {
        if (clients[i] == sender)
            continue;

        if (!writeMessage(message, clients[i]))
            removeClient(clients, i);
    }
}