#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>

#include "Stage.h"
#include "Client.h"
#include "TcpServer.h"

#define BUF_SIZE 65536

using namespace std;

//póki co struktura - wszystko publiczne
struct GameManager 
{
    const int playersToStart = 1;

    TcpServer server;
    bool gameCanBeStarted = false;
    bool gameStarted = false;
    vector<Client> clients;
    int readyPlayersCount = 0;
    int stageNumber = 0;
    vector<Stage> stages;

    GameManager(TcpServer &serv)
    {
        server = serv;
        loadStages();
        createPipe();
    }

    void createPipe()
    {
        int fd[2];
        int result = pipe(fd);
        if (result == -1)
            error("pipe() error!");

        result = dup2(fd[0], PIPE_READ);
        if (result == -1)
            error("dup2() error!");

        result = dup2(fd[1], PIPE_WRITE);
        if (result == -1)
            error("dup2() error!");

        server.addEpollEvent(PIPE_READ);
    }

    string getLobbyPlayersInfo()
    {
        stringstream ss;
        ss << "Players online: " << clients.size() << endl
           << "Players ready: " << readyPlayersCount << endl;

        return ss.str(); 
    }

    string getStageAnswersInfo()
    {
        stringstream ss;
        ss << "Answers: " << getPlayersWhoAnsweredNumber() << "/" << clients.size() << endl;
        ss << "Most popular answer: " << getStage().getMostPopularAnswer() << endl;
        
        return ss.str();
    }

    int getPlayersWhoAnsweredNumber()
    {
        int count = 0;
        for (int i = 0; i < (int)clients.size(); i++)
        {
            if (clients[i].answered)
                count++;
        }

        return count;
    }

    void gameStartCheck()
    {
        if ((int)clients.size() >= playersToStart && (int)clients.size() == readyPlayersCount)
        {
            cout << "GameManager: Enough players to start the game! Let's play!" << endl;
            gameCanBeStarted = true;
        }
    }

    //testowo
    void loadStages()
    {
        vector<string> ans = {"Yes", "No" };
        Stage s = Stage("Example text\nExample text 2\n", ans);
        stages.push_back(s);

        vector<string> ans2 = {"Pro", "Noob" };
        Stage s2 = Stage("Example question\nExample question 2\n", ans2);
        stages.push_back(s2);    

        vector<string> ans3 = {"Win", "Lose" };
        Stage s3 = Stage("Example question\nExample question 2\n", ans3);
        stages.push_back(s3);        
    }

    bool canLoadNextStage()
    {
         if (!getStage().shown) 
            return true;

        return false;
    }

    Stage& getStage()
    {
        return stages[stageNumber];
    }

    void launchStage()
    {
        getStage().show();
        sendMessageToAll(getStage().getStageText());
        letClientsAnswer();
    }

    void letClientsAnswer()
    {   
        for (int i = 0; i < (int)clients.size(); i++)
            clients[i].answered = false;
    } 

    void startGame()
    {
        gameStarted = true;
        gameCanBeStarted = false;
        sendMessageToAll("The game has started!\nGood luck\n");
    }

    void checkIfAllAnswered()
    {
        if (getStage().getAnswersCount() == (int)clients.size())
            stageNumber++;
    }


//==========================================================================
    
    void update()   //nazwać tego elseifa
    {
        if (gameCanBeStarted)
            startGame();
        if (gameStarted && canLoadNextStage())
            launchStage();

        epollWait();

        if (server.event.events == EPOLLIN && server.event.data.fd == server.serverSocket)
            handleNewClients();
        else if (server.event.events == EPOLLIN && server.event.data.fd == PIPE_READ)
            handlePipe();
        else
            handleClients();
    }

    void handleNewClients()
    {
        if (gameStarted) 
        {
            rejectClient();
            return;
        }

        addNewClient();
        sendMessageToAll(getLobbyPlayersInfo());      
    }

    void epollWait()
    {
        int result = epoll_wait(server.epollFd, &server.event, 1, -1);
        if (result == -1)
            error("epoll_wait() error!");
    }

    //póki co można się łączyć z tego samego IP kilka razy
    //zrobić możliwość reconnectowania z tego samego ip
    void addNewClient()
    {
        Client client = acceptClient();
        // if (isAlreadyConnected(client))
        // {
        //     removeCheater(client);
        //     return;
        // }

        server.addEpollEvent(client.fd);
        clients.push_back(client);

        cout << "New connection from: " << client.address << ":" << client.port << " fd: " <<  client.fd << endl;
        writeMessage("Hello! It's 'Spirit of the Dead' text game. Enjoy!\n", client);
    }

    bool isAlreadyConnected(Client client)
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

    Client acceptClient()
    {
        int socket;
        sockaddr_in clientInfo;
        socklen_t clientInfoSize = sizeof(clientInfo);

        socket = accept(server.serverSocket, (sockaddr*)&clientInfo, &clientInfoSize);
        if (socket == -1)
            error("accept() error!");

        return Client(socket, inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
    }

    void rejectClient()
    {
        Client client = acceptClient();
        
        cout << "Client: " << client.address << ":" << client.port << " fd: " << client.fd << " wanted to connect while the game is running. Removing him..." << endl;    
        writeMessage("The game has already started! Please try again later!\n", client);
        close(client.fd);
    }

    void handlePipe()
    {
        char buf[BUF_SIZE];
        
        readPipe(PIPE_READ, buf);
        cout << buf << endl;

        if (getStage().completed)
            return;

        getStage().timesUp(clients.size());
        stageNumber++;
    }

    void handleClients()
    {
        if (!gameStarted)
            waitForClients();
        else
            playGame();
    }

    void removeClient(int n)
    {
        cout << "Removing client: " << clients[n].address << ":" << clients[n].port << " fd: " << clients[n].fd << endl;
        close(clients[n].fd);
        clients.erase(clients.begin() + n);
    }

    bool readMessage(Client &client, char* buf)
    {
        memset(buf, 0, BUF_SIZE);

        int msgSize = read(client.fd, buf, BUF_SIZE);
        if (msgSize < 1)
            return false;

        return true;
    }

    void readPipe(int fd, char *buf)
    {
        memset(buf, 0, BUF_SIZE);
        read(fd, buf, BUF_SIZE);
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

    void sendMessageToAll(string message)
    {
        for (int i = 0; i < (int)clients.size(); i++)
        {
            if (!writeMessage(message, clients[i]))
                removeClient(i);
        }
    }

    void sendMessageToAll(string message, Client &sender)
    {
        for (int i = 0; i < (int)clients.size(); i++)    
        {
            if (clients[i] == sender)
                continue;

            if (!writeMessage(message, clients[i]))
                removeClient(i);
        }
    }

    void waitForClients()
    {
        char buf[BUF_SIZE];
        
        for (int i = 0; i < (int)clients.size(); i++)
        {
            if (server.event.events == EPOLLIN && server.event.data.fd == clients[i].fd)
            {
                if(!readMessage(clients[i], buf))
                {
                    removeClient(i);
                    break;
                }
                    
                processMessageOnWaiting(buf, clients[i]);
            }
        }
    }

    void processMessageOnWaiting(string message, Client &client)
    {
        cout << "Processing message: " << message;

        if (message.find("name") != string::npos)
            setClientName(client, message.substr(message.find("name")+5));
    
        else if (message.find("notready") != string::npos)
            setClientNotReady(client);
        
        else if (message.find("ready") != string::npos)
            setClientReady(client);

        else if (message.find("chat") != string::npos)
            sendMessageToAll(client.name + ": " + message.substr(message.find("chat")+5), client);
    }

    void setClientName(Client &client, string name)
    {
        client.name = name;
        cout << "Set name of " << client.address << ":" << client.port << " fd: " << client.fd << " to '" << name << "'" << endl; 
    }

    void setClientReady(Client &client)
    {
        if (client.readyToPlay)
            return;

        client.readyToPlay = true;
        readyPlayersCount++;

        sendMessageToAll(getLobbyPlayersInfo());
        cout << "Player: " << client.name << " is ready!" << endl; 

        gameStartCheck();
    }

    void setClientNotReady(Client &client)
    {
        if (!client.readyToPlay)
            return;

        client.readyToPlay = false;
        readyPlayersCount--;

        sendMessageToAll(getLobbyPlayersInfo());
        cout << "Player: " << client.name << " is busy now!" << endl;     
    }

    void playGame()
    {
        char buf[BUF_SIZE];
        
        for (int i = 0; i < (int)clients.size(); i++)
        {
            if (server.event.events == EPOLLIN && server.event.data.fd == clients[i].fd)
            {
                if(!readMessage(clients[i], buf))
                {
                    removeClient(i);
                    break;
                }

                processMessage(buf, clients[i]);
            }
        }
    }

    void processMessage(string message, Client &client)
    {
        cout << "Processing message: " << message;

        if (client.answered)
            return;        

        for (int i = 0; i < (int)getStage().answers.size(); i++)
        {
            if (message.find(getStage().answers[i]) != string::npos)
            {
                cout << client.name << " choice is: " << getStage().answers[i] << endl;
                client.answered = true;
                getStage().answersStats[i]++;
                sendMessageToAll(getStageAnswersInfo());
                checkIfAllAnswered();
                return;
            }
        }

        writeMessage("Your answer is not correct!\n", client);
    }
};