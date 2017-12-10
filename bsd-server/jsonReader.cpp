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

int findFor(std::string word, std::string line) {
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
  std::vector<Entry> story;
  bool readingAnswers = false;

  while (std::getline(input, line)) {
    if (readingAnswers) {
      std::cout<<line<<std::endl;
      readingAnswers = false;
      continue;
    }
    if (findFor("{", line) != -1) {
      story.push_back(*(new Entry()));
      // std::cout << findFor("{", line) << std::endl;
    }
    if (findFor("type", line) != -1) {
      story[entries].type = line.substr(findFor(":", line)+3);
      story[entries].type.erase(story[entries].type.end()-2, story[entries].type.end());
      //std::cout << findFor("text", line) << std::endl;
    }
    if (findFor("story-id", line) != -1) {
      story[entries].storyID = line.substr(findFor(":", line)+3);
      story[entries].storyID.erase(story[entries].storyID.end()-2, story[entries].type.end());
      //std::cout << findFor("text", line) << std::endl;
    }
    if (findFor("npc", line) != -1) {
      story[entries].npc = line.substr(findFor(":", line)+3);
      story[entries].npc.erase(story[entries].npc.end()-2, story[entries].npc.end());
      //std::cout << findFor("text", line) << std::endl;
    }
    if (findFor("text", line) != -1) {
      story[entries].text = line.substr(findFor(":", line)+3);
      story[entries].text.erase(story[entries].text.end()-2, story[entries].text.end());
      //std::cout << findFor("text", line) << std::endl;
    }
    if (findFor("answers", line) != -1) {
      readingAnswers = true;
    }
  }

  // std::cout<<story[entries].storyID<<std::endl;
}
// "type": "story",
// "storyID": "1",
// "npc": "gm",
// "text": "You are at the tavern with your best friends. What do you intend to do?",
// "answers": [
//   "a": "Go for an adventure.",
//   "b": "Let's stay here and relax."
// ]
