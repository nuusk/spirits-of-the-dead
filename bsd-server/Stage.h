#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

#include <iostream>       // std::cout
#include <iomanip>        // std::put_time
#include <thread>         // std::this_thread::sleep_until
#include <chrono>         // std::chrono::system_clock
#include <ctime>
#include <pthread.h>
#include <unistd.h>

#define PIPE_READ 5
#define PIPE_WRITE 6

using namespace std;

void error(string errorMessage) 
{
    perror(errorMessage.c_str());
    exit(0);
}

void *timer(void* duration)
{
    int d = *((int *)duration);
    sleep(d);
        
    string msg = "Time's up!";
    int count = write(PIPE_WRITE, msg.c_str(), msg.size());
    if (count == -1)
        error("write() error!");   
    
    pthread_exit(NULL);
}

struct Stage
{
    string text;
    vector<string> answers;
    vector<int> answersStats;
    int duration;
    bool shown;
    bool completed;

    Stage(string txt, vector<string> &a)
    {
        text = txt;
        answers = a;
        answersStats = vector<int>(a.size(), 0);
        shown = false;
        completed = false;
        duration = 3;
    }

    void show()
    {
        shown = true;

        pthread_t thread;
        int result = pthread_create(&thread, NULL, timer, &duration);
        if (result == -1)
            error("pthread_create() error!");
    }

    string getStageText()
    {
        stringstream ss;
        ss << text << endl;
        for (int i = 0; i < (int)answers.size(); i++)
            ss << i+1 << ": " << answers[i] << endl;

        return ss.str();
    }

    string getMostPopularAnswer()
    {
        int max = 0;
        string answer = "";

        for (int i = 0; i < (int)answersStats.size(); i++)
        {
            if (answersStats[i] > max)
            {
                max = answersStats[i];
                answer = answers[i];
            }
        }

        return answer;
    }

    int getMostPopularAnswerIndex()
    {
        int max = 0;
        int index = 0;

        for (int i = 0; i < (int)answersStats.size(); i++)
        {
            if (answersStats[i] > max)
            {
                max = answersStats[i];
                index = i;
            }
        }

        return index;
    }

    void timesUp(int playersCount)
    {
        int n = getAnswersCount() - playersCount;

        for (int i = 0; i < n; i++)
        {
            int k = rand() % answers.size();
            answersStats[k]++;
        }

        //getMostPopularAnswerIndex();
        
        completed = true;
    }

    int getAnswersCount()
    {
        int sum = 0;
        for (int i = 0; i < (int)answersStats.size(); i++)
            sum += answersStats[i];

        return sum;
    }
};