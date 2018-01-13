#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "Stage.h"

using namespace std;

int searchFor(string word, string line) 
{
    size_t wordPosition = line.find(word);

    if (wordPosition!=string::npos) 
        return wordPosition;

  return -1;
}

vector<Stage> loadStory(int pipeWrite) 
{
    vector<Stage> story;
    int entries = 0;

    ifstream input("../lore/story.txt");
    string line;
    bool readingAnswers = false;
    bool readingRouting = false;

    while (getline(input, line)) 
    {
        if (readingAnswers) 
        {
            if (searchFor("]", line) != -1) 
            {
                readingAnswers = false;
                continue;
            }

            string currentAnswer = line.substr(searchFor(":", line)+3);
            currentAnswer.erase(currentAnswer.end()-2, currentAnswer.end());
            currentAnswer.erase(currentAnswer.begin(), currentAnswer.begin()+3);
            story[entries].answers.push_back(currentAnswer);
            continue;
        }

        if (readingRouting)
        {
            if (searchFor("]", line) != -1) 
            {
                readingRouting = false;
                continue;
            }

            string route = line.substr(searchFor(":", line)+3);
            route.erase(route.end()-1, route.end());
            route.erase(route.begin(), route.begin()+3);
            story[entries].nextStages.push_back(atoi(route.c_str()));
            continue;
        }

        if (searchFor("{", line) != -1) 
            story.push_back(*(new Stage(pipeWrite)));

        if (searchFor("}", line) != -1)
        {
            story[entries].answersStats = vector<int>(story[entries].answers.size(), 0); 
            entries++;
        }

        if (searchFor("text", line) != -1) 
        {
            story[entries].text = line.substr(searchFor(":", line)+3);
            story[entries].text.erase(story[entries].text.end()-2, story[entries].text.end());
        }

        if (searchFor("answers", line) != -1) 
            readingAnswers = true;

        if (searchFor("routing", line) != -1)
            readingRouting = true;
    }

    return story;
}