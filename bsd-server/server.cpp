#include "GameManager.h"

int main(int argc, char** argv)
{
    TcpServer server;

    server.prepareSocket();
    server.prepareEpoll();
    GameManager gameManager = GameManager(server);

    while (true)
        gameManager.update();

    return 0;
}