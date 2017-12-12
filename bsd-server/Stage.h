#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

#define PIPE_READ 5
#define PIPE_WRITE 6
#define END_GAME_ID 32767

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
    vector<int> nextStages;
    int duration;
    bool shown;

    Stage()
    {
        shown = false;
        duration = 20;
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
        return answers[getMostPopularAnswerIndex()];
    }

    int getNextStageId()
    {
        return nextStages[getMostPopularAnswerIndex()];
    }

    int getMostPopularAnswerIndex()
    {
        int max = getMax();
        int index = handleTie(max);

        return index;
    }

    int getMax()
    {
        int max = 0;

        for (int i = 0; i < (int)answersStats.size(); i++)
        {
            if (answersStats[i] > max)
                max = answersStats[i];
        }

        return max;
    }

    int handleTie(int max)
    {
        vector<int> maxes;

        for (int i = 0; i < (int)answersStats.size(); i++)
        {
            if (answersStats[i] == max)
                maxes.push_back(i);
        }

        return maxes[rand() % maxes.size()];
    }

    void timesUp(int playersCount)
    {
        int n = getAnswersCount() - playersCount;

        for (int i = 0; i < n; i++)
        {
            int k = rand() % answers.size();
            answersStats[k]++;
        }
    }

    int getAnswersCount()
    {
        int sum = 0;
        for (int i = 0; i < (int)answersStats.size(); i++)
            sum += answersStats[i];

        return sum;
    }

    void reset()
    {
        shown = false;
        
        for (int i = 0; i < (int)answersStats.size(); i++)
            answersStats[i] = 0;
    }

    string toString()
    {
        stringstream ss;
        ss << "Text: " << endl;
        ss << "Answers: " << endl;
        for (int i = 0; i < (int)answers.size(); i++)
            ss << i+1 << ": " << answers[i] << endl;
        ss << "Answers stats:" << endl;
        for (int i = 0; i < (int)answersStats.size(); i++)
            ss << i+1 << ": " << answersStats[i] << endl;
        ss << "Answers destinations: " << endl;
        for (int i = 0; i < (int)nextStages.size(); i++)
            ss << i+1 << " --> " << nextStages[i] << endl;
        ss << "Duration: " << duration << endl;
        ss << "Shown: " << shown << endl;

        return ss.str();
    }
};