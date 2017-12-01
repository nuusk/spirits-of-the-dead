#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h> 
#include <thread>
#include <unordered_set>
#include <signal.h>
#define PORT 1252

void prepareSocket(int &sock, sockaddr_in &address);
void createSocket(int &sock);
void setSocketOption(int &sock);
void setSocketAddress(sockaddr_in &address);
void bindAddress(int &sock, sockaddr_in &address);
void startListening(int &sock);
void prepareEpoll(int &sock, int &fd, epoll_event &event);
void createEpollFd(int &fd);
void addEpollEvent(int &sock, int &fd, epoll_event &event);
void update(int &serverSocket, int &epollFd, epoll_event &event);
void epollWait(int fd, epoll_event &event);
void addNewClient(int &serverSock, int &epollFd, epoll_event &event);
void acceptClient(int &client, int &serverSock, sockaddr_in &clientInfo, socklen_t &clientInfoSize);



int main(int argc, char** argv)
{
    int serverSocket;
    sockaddr_in address;
    int epollFd;
    epoll_event event;


    prepareSocket(serverSocket, address);
    prepareEpoll(serverSocket, epollFd, event);

    while (true)
        update(serverSocket, epollFd, event);


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
    {
        perror("socket() error!");
        exit(0);
    }
}

void setSocketOption(int &sock)
{
    const int one = 1;
    int result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (result == -1 ) 
	{
		perror("setsockopt() error!");
        exit(0);
    }
}

void setSocketAddress(sockaddr_in &address)
{
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr = { htonl(INADDR_ANY) };
}

void bindAddress(int &sock, sockaddr_in &address)
{
    int result = bind(sock, (sockaddr*)&address, sizeof(address));
	if (result == -1)
	{
		perror("bind() error!");
        exit(0);
    } 
}

void startListening(int &sock)
{
	int result = listen(sock, 1);
	if (result == -1)
	{
		perror("listen() error!");
        exit(0);
	}
}

void prepareEpoll(int &sock, int &fd, epoll_event &event)
{
    createEpollFd(fd);
    addEpollEvent(sock, fd, event);
}

void createEpollFd(int &fd)
{
    fd = epoll_create1(0);
    if (fd == -1)
    {
        perror("epoll_create() error!");
        exit(0);
    }
}

void addEpollEvent(int &sock, int &fd, epoll_event &event)
{
    event.data.fd = sock;
    event.events = EPOLLIN;

    int result = epoll_ctl(fd, EPOLL_CTL_ADD, sock, &event);
    if (result == -1)
    {
        perror("epoll_ctl() error!");
        exit(0);
    }
}

void update(int &serverSocket, int &epollFd, epoll_event &event)
{
    epollWait(epollFd, event);

    if (event.events == EPOLLIN && event.data.fd == serverSocket)
    {
        addNewClient(serverSocket, epollFd, event);
    }

}   

void epollWait(int fd, epoll_event &event)
{
    int result = epoll_wait(fd, &event, 1, -1);
    if (result == -1)
    {
        perror("epoll_wait() error!");
        exit(0);
    }
}

void addNewClient(int &serverSock, int &epollFd, epoll_event &event)
{
    int clientSocket;
    sockaddr_in clientInfo;
    socklen_t clientInfoSize = sizeof(clientInfo);
    
    acceptClient(clientSocket, serverSock, clientInfo, clientInfoSize);
    addEpollEvent(clientSocket, epollFd, event);

    printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port), clientSocket);
}

void acceptClient(int &client, int &serverSock, sockaddr_in &clientInfo, socklen_t &clientInfoSize)
{
    client = accept(serverSock, (sockaddr*)&clientInfo, &clientInfoSize);
    if (client == -1)
    {
        perror("accept() error!");
        exit(0);
    }
}

