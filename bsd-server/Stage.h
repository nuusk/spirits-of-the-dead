#include <string>
#include <vector>
#include <sstream>

using namespace std;

struct Stage
{
    string text;
    vector<string> answers;
    vector<int> answersStats;
    int timer;
    bool shown;

    Stage(string txt, vector<string> &a, int t = 10)
    {
        text = txt;
        answers = a;
        answersStats = vector<int>(a.size(), 0);
        timer = t;
        shown = false;
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
};