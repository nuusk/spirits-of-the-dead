#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

#include "Client.h"
#include "TcpServer.h"
#include "StoryReader.h"

#define BUF_SIZE 65536
#define END_OF_MSG "10101010101010101010101010101010"
#define END_OF_MSG_SIZE 32

using namespace std;

enum ReadResult 
{
    Good, NotAll, Bad
};

class GameManager 
{
    const int playersToStart = 1;

    TcpServer server;
    int pipeFd[2];
    bool gameCanBeStarted;
    bool gameStarted;
    vector<Client> clients;
    vector<Client> clientsLobby;
    int readyPlayersCount;
    int stageNumber;
    vector<Stage> stages;
    int stagesBeforeTimer;

public:
    GameManager(TcpServer &serv)
    {
        server = serv;
        gameCanBeStarted = false;
        gameStarted = false;
        readyPlayersCount = 0;
        stageNumber = 0;
        stagesBeforeTimer = 0;
        createPipe();
        stages = loadStory(pipeFd[1]);
        showHelp();
        cout << endl << "<<<<<----- server is running ----->>>>>" << endl;
    }

    void update()
    {
        if (gameCanBeStarted)
            startGame();
        if (gameStarted && canLoadNextStage())
            launchStage();

        epollWait();

        if (server.getEvent().events == EPOLLIN && server.getEvent().data.fd == server.getSocket())
            handleNewClients();
        else if (server.getEvent().events == EPOLLIN && server.getEvent().data.fd == 0)
            handleServerRequests();
        else if (server.getEvent().events == EPOLLIN && server.getEvent().data.fd == pipeFd[0])
            handleTimers();
        else
        {
            handleClients();
            handleClientsLobby();
        }
    }


private:
    void showHelp()
    {
        cout << "server => shows server info" << endl;
        cout << "manager => shows game manager info" << endl;
        cout << "clients => shows clients info" << endl;
        cout << "stages => shows stages info" << endl;
    }

    void createPipe()
    {
        int result = pipe(pipeFd);
        if (result == -1)
            error("pipe() error!");

        server.addEpollEvent(pipeFd[0]);
    }

    string getClientsInfo()
    {
        stringstream ss;
        ss << "{\"type\":\"playersLobbyInfo\", \"online\":\"" << clients.size() << "\", \"ready\":\"" << readyPlayersCount << "\"}";
        
        return ss.str();
    }

    string getClientsInLobbyInfo()
    {
        stringstream ss;
        ss << "{\"type\":\"playersLobbyInfo\", \"inGame\":\"" << clients.size() << "\", \"waiting\":\"" << clientsLobby.size() << "\"}";

        return ss.str();
    }

    string getGameStartedInfo()
    {
        stringstream ss;
        ss << "{\"type\" : \"gameStartedInfo\", \"gameStarted\" : \"" << (gameStarted ? "true" : "false") << "\"}";

        return ss.str();
    }

    string getStageAnswersInfo()
    {
        stringstream ss;
        ss << "{\"type\" : \"answersInfo\",";
        ss << "\"answerNumber\" : \"" << getPlayersWhoAnsweredNumber() << "/" << clients.size() << "\",";
        ss << "\"mostPopularAnswer\" : \"" << getStage().getMostPopularAnswer() << "\"}";
        
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
        sendMessageToAll(getStageAnswersInfo());
    }

    void letClientsAnswer()
    {   
        for (int i = 0; i < (int)clients.size(); i++)
            clients[i].answered = false;
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

        sendMessageToAll(getClientsInfo());

        gameStartCheck();
    }

    void setClientNotReady(Client &client)
    {
        if (!client.readyToPlay)
            return;

        client.readyToPlay = false;
        readyPlayersCount--;

        sendMessageToAll(getClientsInfo());
    }

    void tryToAnswer(string message, Client &client)
    {
        if (isdigit(message[0]))
        {
            int ans = atoi(message.c_str()) - 1;
            
            if (ans >= 0 && ans < (int)getStage().answers.size())
            {
                client.answered = true;
                getStage().answersStats[ans]++;
                sendMessageToAll(getStageAnswersInfo());
                checkIfAllAnswered();
            }
        }
    }

    void startGame()
    {
        gameStarted = true;
        gameCanBeStarted = false;
                
        sendMessageToAll("{\"type\" : \"gameStart\"}");
    }

    void checkIfAllAnswered()
    {
        if (getPlayersWhoAnsweredNumber() == (int)clients.size())
        {
            stageNumber = getStage().getNextStageId();
            stagesBeforeTimer++;
        }

        if (stageNumber == END_GAME_ID)
            endGame();
    }

    void endGame()
    {
        cout << "The game has finished! Reseting server..." << endl;
        gameStarted = false;
        stageNumber = 0;
        readyPlayersCount = 0;
        
        letClientsAnswer();

        for (Client &c : clients)
            c.readyToPlay = false;
        
        for (Stage &s : stages)
            s.reset();

        for (Client &c : clientsLobby)
            clients.push_back(c);

        clientsLobby.clear();

        sendMessageToAll("{ \"type\" : \"gameEnd\"}");
        sendMessageToAll(getClientsInfo());
    }

    void showClients()
    {
        stringstream ss;
        ss << "Clients in game info:" << endl;
        for (Client c : clients)
            ss << c.toString() << endl;
        
        ss << "Clients in lobby info:" << endl;
        for (Client c : clientsLobby)
            ss << c.toString() << endl;

        cout << ss.str();
    }

    void showStages()
    {
        stringstream ss;
        ss << "Stages info:" << endl;
        for (Stage s : stages)
            ss << s.toString() << endl;

        cout << ss.str();
    }

    void showServer()
    {
        stringstream ss;
        ss << "Server info:" << endl;
        ss << server.toString() << endl;

        cout << ss.str();
    }

    void showGameManager()
    {
        stringstream ss;
        ss << "Game manager:" << endl;
        ss << "Players to start: " << playersToStart << endl;
        ss << "Players online: " << clients.size() << endl;
        ss << "Ready players: " << readyPlayersCount << endl;        
        ss << "Can start game: " << gameCanBeStarted << endl;
        ss << "Game started: " << gameStarted << endl;
        ss << "Actual stage: " << stageNumber << endl;
        ss << "Stages before timer: " << stagesBeforeTimer << endl;

        cout << ss.str();
    }


//==========================================================================================================
//==========================================================================================================
//==========================================================================================================


    void handleNewClients()
    {
        addNewClient();

        if (!gameStarted)
            sendMessageToAll(getClientsInfo());      
        else 
            sendMessageToAllLobby(getClientsInLobbyInfo());
    }

    void epollWait()
    {
        int result = server.epollWait();

        if (result == -1)
            error("epoll_wait() error!");
    }

    void addNewClient()
    {
        Client client = acceptClient();
        // if (isAlreadyConnected(client))
        // {
        //     removeCheater(client);
        //     return;
        // }

        fcntl(client.fd, F_SETFL, O_NONBLOCK, 1);
        server.addEpollEvent(client.fd);
        
        if (!gameStarted)
            clients.push_back(client);
        else
            clientsLobby.push_back(client);

        cout << "New connection from: " << client.address << ":" << client.port << " fd: " <<  client.fd << endl;
    }

    //NOT USED
    bool isAlreadyConnected(Client client)
    {
        for (Client c : clients)
        {
            if (c.address == client.address)
                return true;
        }

        for (Client c : clientsLobby)
        {
            if (c.address == client.address)
                return true;
        }

        return false;
    }
    
    //NOT USED
    void removeCheater(Client client)
    {
        cout << "Client: " << client.address << ":" << client.port << " fd: " << client.fd << " is trying to cheat! Removing him..." << endl;
        close(client.fd);
    }

    Client acceptClient()
    {
        int socket;
        sockaddr_in clientInfo;
        socklen_t clientInfoSize = sizeof(clientInfo);
        char address[INET_ADDRSTRLEN];

        socket = server.acceptClient((sockaddr*)&clientInfo, &clientInfoSize);
        if (socket == -1)
            error("accept() error!");

        inet_ntop(AF_INET, &(clientInfo.sin_addr), address, INET_ADDRSTRLEN);

        return Client(socket, address, ntohs(clientInfo.sin_port));
    }

    void handleServerRequests()
    {
        char buf[BUF_SIZE];

        int result = read(0, buf, BUF_SIZE);
        if (result == -1)
            error("read() error!");

        processMessageStdIn(buf);        
    }

    void handleTimers()
    {
        char buf[BUF_SIZE];
        
        readPipe(pipeFd[0], buf);

        if (stagesBeforeTimer > 0)
        {
            stagesBeforeTimer--;
            return;
        }

        cout << buf << endl;
        getStage().timesUp(clients.size());
        stageNumber = getStage().getNextStageId();

        if (stageNumber == END_GAME_ID)
            endGame();
    }

    void removeClient(Client &client)
    {
        for (int i = 0; i < (int)clients.size(); i++)
        {
            if (clients[i].fd == client.fd)
            {
                removeClient(i);
                return;
            }
        }

        for (int i = 0; i < (int)clientsLobby.size(); i++)
        {
            if (clients[i].fd == client.fd)
            {
                removeClient(i);
                return;
            }
        }
    }

    void removeClient(int n)
    {
        cout << "Removing client: " << clients[n].address << ":" << clients[n].port << " fd: " << clients[n].fd << endl;
        
        if (clients[n].readyToPlay)
            readyPlayersCount--;
        
        close(clients[n].fd);
        clients.erase(clients.begin() + n);

        if ((int)clients.size() == 0)
            endGame();
        else 
        {
            sendMessageToAll(getClientsInfo());

            if (gameStarted) 
            {
                sendMessageToAll(getStageAnswersInfo());
                checkIfAllAnswered();
            }
            else 
                gameStartCheck();
        }
    }

    void removeClientFromLobby(int n)
    {
        cout << "Removing client: " << clientsLobby[n].address << ":" << clientsLobby[n].port << " fd: " << clientsLobby[n].fd << endl;
        close(clientsLobby[n].fd);
        clientsLobby.erase(clientsLobby.begin() + n);   
        sendMessageToAllLobby(getClientsInLobbyInfo()); 
    }

    ReadResult readMessage(int fd, char* buf)
    {
        memset(buf, 0, BUF_SIZE);

        
        int msgSize = recv(fd, buf, BUF_SIZE, MSG_PEEK);
        if (msgSize < 1)
            return ReadResult::Bad;

        string msg = buf;

        if (msg.find(END_OF_MSG) != string::npos)
        {
            msgSize = read(fd, buf, (int)msg.find(END_OF_MSG) + END_OF_MSG_SIZE);
            if (msgSize < 1)
                return ReadResult::Bad;

            return ReadResult::Good;
        } 

        return ReadResult::NotAll;
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

        if (result != (int)message.size())
        {
            removeClient(client);
            return false;
        }

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

    void sendMessageToAllLobby(string message)
    {
        for (int i = 0; i < (int)clientsLobby.size(); i++)
        {
            if (!writeMessage(message, clientsLobby[i]))
                removeClientFromLobby(i);
        }
    }

    void sendMessageToAll(string message, Client &sender)
    {
        for (int i = 0; i < (int)clients.size(); i++)    
        {
            if (clients[i] == sender)
                continue;
            
            stringstream ss;
            ss << "{ \"type\" : \"chat\", \"message\" : \"" << message << "\" }";

            if (!writeMessage(ss.str(), clients[i]))
                removeClient(i);
        }
    }

    void sendMessageToAllLobby(string message, Client &sender)
    {
        for (int i = 0; i < (int)clientsLobby.size(); i++)
        {
            if (clientsLobby[i] == sender)
                continue;

            stringstream ss;
            ss << "{ \"type\" : \"chat\", \"message\" : \"" << message << "\" }";

            if (!writeMessage(ss.str(), clientsLobby[i]))
                removeClientFromLobby(i);
        }
    }

    void handleClients()
    {
        char buf[BUF_SIZE];
        bool breakLoop = false;

        for (int i = 0; i < (int)clients.size(); i++)
        {
            if (server.getEvent().events == EPOLLIN && server.getEvent().data.fd == clients[i].fd)
            {
                while (true)
                {
                    ReadResult result = readMessage(clients[i].fd, buf);

                    if(result == ReadResult::Bad)
                    {
                        removeClient(i);
                        breakLoop = true;
                        break;
                    }

                    if (result == ReadResult::Good)
                    {
                        if (!gameStarted)       
                            processMessageBeforeStart(buf, clients[i]);
                        else
                            processMessageInGame(buf, clients[i]);

                        breakLoop = true;
                        break;
                    }
                }
            }

            if (breakLoop)
                break;
        }
    }

    void handleClientsLobby()
    {
        char buf[BUF_SIZE];
        bool breakLoop = false;

        for (int i = 0; i < (int)clientsLobby.size(); i++)
        {
            if (server.getEvent().events == EPOLLIN && server.getEvent().data.fd == clientsLobby[i].fd)            
            {
                while (true)
                {
                    ReadResult result = readMessage(clientsLobby[i].fd, buf);

                    if(result == ReadResult::Bad)
                    {
                        removeClient(i);
                        breakLoop = true;
                        break;
                    }

                    if (result == ReadResult::Good)
                    {
                        processMessageInLobby(buf, clientsLobby[i]);
                        breakLoop = true;                        
                        break;
                    }
                }
            }

            if (breakLoop)
                break;
        }
    }

    void processMessageStdIn(string message)
    {
        if (message == "\n")
            return; 
            
        if (message.find("\n") != string::npos)
            message.erase(message.find("\n", 1));

        if (message == "server")    
            showServer();
        else if (message == "clients")    
            showClients();
        else if (message == "stages")    
            showStages();
        else if (message == "manager")    
            showGameManager();
    }

    void processMessageBeforeStart(string message, Client &client)
    {
        message = message.substr(0, message.find(END_OF_MSG));

        if (message.substr(0, 5) == "name ")
            setClientName(client, message.substr(message.find("name")+5));
    
        else if (message == "notready")
            setClientNotReady(client);
        
        else if (message == "ready")
            setClientReady(client);

        else if (message == "playersLobbyInfo")
            writeMessage(getClientsInfo(), client);

        else if (message == "getGameStarted")
            writeMessage(getGameStartedInfo(), client);

        else if (message.find("chat ") != string::npos)
            sendMessageToAll(client.name + ": " + message.substr(message.find("chat")+5), client);
    }

    void processMessageInGame(string message, Client &client)
    {
        if (client.answered)
            return;        
     
        message = message.substr(0, message.find(END_OF_MSG));

        tryToAnswer(message, client);
    }

    void processMessageInLobby(string message, Client &client)
    {
        message = message.substr(0, message.find(END_OF_MSG));

        if (message == "getGameStarted")
            writeMessage(getGameStartedInfo(), client);

        else if (message == "playersLobbyInfo")
            writeMessage(getClientsInLobbyInfo(), client);

        else if (message.substr(0, 5) == "name ")
            setClientName(client, message.substr(message.find("name")+5));

        else if (message.find("chat") != string::npos)
            sendMessageToAllLobby(client.name + ": " + message.substr(message.find("chat")+5), client);
    }
};