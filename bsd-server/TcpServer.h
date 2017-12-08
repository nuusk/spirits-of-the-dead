#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define PORT 1252

using namespace std;

class TcpServer
{
public:
    int serverSocket;
    sockaddr_in address;
    int epollFd;
    epoll_event event;

    void prepareSocket()
    {
        createSocket();
        setSocketOption();
        setSocketAddress();
        bindAddress();
        startListening();
    }

    void prepareEpoll()
    {
        createEpollFd();
        addEpollEvent(serverSocket);
    }

    void addEpollEvent(int sock)
    {
        event.data.fd = sock;
        event.events = EPOLLIN;

        int result = epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &event);
        if (result == -1)
            error("epoll_ctl() error!");
    }

    void closeServer()
    {
        close(serverSocket);
        close(epollFd);
    }

private:

    void createSocket()
    {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1)
            error("socket() error!");
    }

    void setSocketOption()
    {
        const int one = 1;
        int result = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (result == -1)   
            error("setsockopt() error!");
    }

    void setSocketAddress()
    {
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        address.sin_addr = { htonl(INADDR_ANY) };
    }

    void bindAddress()
    {
        int result = bind(serverSocket, (sockaddr*)&address, sizeof(address));
        if (result == -1)
            error("bind() error!");
    }

    void startListening()
    {
        int result = listen(serverSocket, 1);
        if (result == -1)
            error("listen() error!");
    }

    void createEpollFd()
    {
        epollFd = epoll_create1(0);
        if (epollFd == -1)
            error("epoll_create() error!");
    }

    void error(string errorMessage) 
    {
        perror(errorMessage.c_str());
        exit(0);
    }
};