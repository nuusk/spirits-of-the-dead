#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define PORT 1252

using namespace std;

class TcpServer
{
    int serverSocket;
    sockaddr_in address;
    int epollFd;
    epoll_event event;

public:
    int getSocket()
    {
        return serverSocket;
    }

    epoll_event &getEvent()
    {
        return event;
    }

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
        addEpollEvent(0);
    }

    void addEpollEvent(int sock)
    {
        event.data.fd = sock;
        event.events = EPOLLIN;

        int result = epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &event);
        if (result == -1)
            error("epoll_ctl() error!");
    }

    int acceptClient(sockaddr* clientInfo, socklen_t* clientInfoSize)
    {
        int result = accept(serverSocket, clientInfo, clientInfoSize);

        return result;
    }

    int epollWait()
    {
        int result = epoll_wait(epollFd, &event, 1, -1);

        return result;
    }

    void closeServer()
    {
        close(serverSocket);
        close(epollFd);
    }
 
    string toString()
    {
        stringstream ss;
        ss << "Socket: " << serverSocket << endl;
        ss << "Address:" << endl;
        ss << "Address.family: " << address.sin_family << endl;
        ss << "Address.address: " << address.sin_addr.s_addr << endl;
        ss << "Address.port: " << address.sin_port << endl;
        ss << "Epoll fd: " << epollFd << endl;

        return ss.str();
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