#include <iostream>
#include <fstream>
#include <vector>
#include <string>


class Entry {
public:
  std::string type;
  std::string storyID;
  std::string npc;
  std::string text;
  std::vector<std::string> answers;
};

int searchFor(std::string word, std::string line) {
  std::size_t wordPosition = line.find(word);
  if (wordPosition!=std::string::npos) {
    return wordPosition;
  }
  return -1;
}

int main() {

  std::string line;
  std::ifstream input("../lore/story.txt");
  int entries = 0;

  //this object contains the whole 'story' written in text.
  //type (where it can happen)
  //id (used to determine where the encounter can happen)
  //npc (who will say the encounter text)
  //text (what will be actually said)
  //answers (list of possible answers to this question)
  std::vector<Entry> story;

  bool readingAnswers = false;

  while (std::getline(input, line)) {
    if (readingAnswers) {
      if (searchFor("]", line) != -1) {
        readingAnswers = false;
        continue;
      }
      // std::cout<<line<<std::endl;
      std::string currentAnswer = line.substr(searchFor(":", line)+3);
      currentAnswer.erase(currentAnswer.end()-2, currentAnswer.end());
      currentAnswer.erase(currentAnswer.begin(), currentAnswer.begin()+3);
      story[entries].answers.push_back(currentAnswer);
      continue;
    }
    if (searchFor("{", line) != -1) {
      story.push_back(*(new Entry()));
      // std::cout << searchFor("{", line) << std::endl;
    }
    if (searchFor("}", line) != -1) {
      entries++;
      // std::cout << searchFor("{", line) << std::endl;
    }
    if (searchFor("type", line) != -1) {
      story[entries].type = line.substr(searchFor(":", line)+3);
      story[entries].type.erase(story[entries].type.end()-2, story[entries].type.end());
      //std::cout << searchFor("text", line) << std::endl;
    }
    if (searchFor("story-id", line) != -1) {
      story[entries].storyID = line.substr(searchFor(":", line)+3);
      story[entries].storyID.erase(story[entries].storyID.end()-2, story[entries].type.end());
      //std::cout << searchFor("text", line) << std::endl;
    }
    if (searchFor("npc", line) != -1) {
      story[entries].npc = line.substr(searchFor(":", line)+3);
      story[entries].npc.erase(story[entries].npc.end()-2, story[entries].npc.end());
      //std::cout << searchFor("text", line) << std::endl;
    }
    if (searchFor("text", line) != -1) {
      story[entries].text = line.substr(searchFor(":", line)+3);
      story[entries].text.erase(story[entries].text.end()-2, story[entries].text.end());
      //std::cout << searchFor("text", line) << std::endl;
    }
    if (searchFor("answers", line) != -1) {
      readingAnswers = true;
    }
  }

  std::cout<<story[entries].answers[1]<<std::endl;
}
