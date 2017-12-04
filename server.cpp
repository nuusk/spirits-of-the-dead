#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <deque>
#include <cstring>
#include <string>
#include <sstream>

#include "sha1.hpp"

#define PORT 1252
#define BUF_SIZE 65536
#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

using namespace std;

struct Client
{
    int fd;
    char* address;
    int port;
    bool handshake;

    Client(int f, char* ip, int p)
    {
        fd = f;
        address = ip;
        port = p;
        handshake = false;
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
void update(int serverSocket, int epollFd, epoll_event &event, deque<Client> &clients);
void epollWait(int fd, epoll_event &event);
void addNewClient(int serverSock, int epollFd, epoll_event &event, deque<Client> &clients);
Client acceptClient(int serverSock);
bool isAlreadyConnected(Client client, deque<Client> clients);
void removeCheater(Client client);
void manageClients(int serverSocket, epoll_event &event, deque<Client> &clients);
void removeClient(deque<Client>::iterator &client, deque<Client> &clients);
bool readMessage(Client &client, char* buf);
bool doHandshake(Client &client, string text);
bool writeMessage(char* message, Client &client);

int main(int argc, char** argv)
{
    int serverSocket;
    sockaddr_in address;
    int epollFd;
    epoll_event event;
    deque<Client> clients;

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

void update(int serverSocket, int epollFd, epoll_event &event, deque<Client> &clients)
{
    epollWait(epollFd, event);

    if (event.events == EPOLLIN && event.data.fd == serverSocket)
        addNewClient(serverSocket, epollFd, event, clients);

    else
        manageClients(serverSocket, event, clients);

}

void epollWait(int fd, epoll_event &event)
{
    int result = epoll_wait(fd, &event, 1, -1);
    if (result == -1)
        error("epoll_wait() error!");
}

//póki co można się łączyć z tego samego IP kilka razy
void addNewClient(int serverSock, int epollFd, epoll_event &event, deque<Client> &clients)
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

bool isAlreadyConnected(Client client, deque<Client> clients)
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

void manageClients(int serverSocket, epoll_event &event, deque<Client> &clients)
{
    char buf[BUF_SIZE];

    for (deque<Client>::iterator client = clients.begin(); client != clients.end(); client++)
    {
        if (event.events == EPOLLIN && event.data.fd == (*client).fd)
        {
            if(!readMessage(*client, buf))
            {
                removeClient(client, clients);
                break;
            }
            
            printf("\nI've got new message:\n%s", buf);
            
            if (!(*client).handshake)
            {
                if (!doHandshake(*client, buf))
                {
                    removeClient(client, clients);
                    break;
                }
            }
            else
                writeMessage(buf, *client);
        }
    }
}

void removeClient(deque<Client>::iterator &client, deque<Client> &clients)
{
    printf("\nRemoving client: %s:%hu (fd: %d)\n", (*client).address, (*client).port, (*client).fd);
    close((*client).fd);
    clients.erase(client);
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

bool doHandshake(Client &client, string text)
{
    int index = text.find("Sec-WebSocket-Key: "); 
    string key = text.substr(index + 19, 24) + MAGIC_STRING;
    
    char base64[SHA1_BASE64_SIZE];
    sha1(key.c_str()).finalize().print_base64(base64);
    
    stringstream ss;
    ss << "HTTP/1.1 101 Switching Protocols\r\n" <<
    "Upgrade: websocket\r\n" <<
    "Connection: Upgrade\r\n" <<
    "Content-Type: text/plain\r\n" <<
    "Sec-WebSocket-Accept: " <<
    base64 << "\r\n" <<
    // "Accept: text/plain\r\n" <<
    // "Accept-Encoding: gzip, deflate\r\n" <<
    // "Accept-Charset: utf-8\r\n" <<
    "\r\n";

    if (!writeMessage(const_cast<char*>(ss.str().c_str()), client))
        return false;

    client.handshake = true;
    return true;
}
#include <iostream>
#include <fstream>
bool writeMessage(char* message, Client &client)
{
    string msg = message;
    printf("\nMessage to client:\n%s", message);            
    
    ofstream f;
    f.open("a.txt");
    f << msg << endl;
    f.close();
    int result = write(client.fd, message, strlen(message));
    if (result == -1)
        return false;

    return true;
}