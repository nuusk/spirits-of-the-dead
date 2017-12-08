#include "GameManager.h"

int main(int argc, char** argv)
{
    TcpServer server;

    srand(time(0));    

    server.prepareSocket();
    server.prepareEpoll();
    GameManager gameManager = GameManager(server);

    while (true)
        gameManager.update();

    server.closeServer();

    return 0;
}