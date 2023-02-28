
// в качестве заготовки кода используйте последнюю версию своей поисковой системы
// в качестве заготовки кода используйте последнюю версию своей поисковой системы
#include "search_server.h"
#include "remove_duplicates.h"
#include "document.h"
#include "string_processing.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <numeric>


//4. Вне класса сервера разработайте функцию поиска и удаления дубликатов :
/*void RemoveDuplicates(SearchServer& search_server) {
    search_server.RemoveDuplicates();
}*/

void RemoveDuplicates(SearchServer& search_server) {
  std::map<std::string, double> m; // то что приходит из GetWordFrequencies(document_id)
  //pair<int, vector<string>> document_struct; // структура слов для 1 документа по id 
  std::vector <std::pair<int, std::vector<std::string>>>  all_document_struct; // структура слов для всех документов по id 

  for (const int document_id: search_server) {
    std::vector<std::string> key_words;  // только слова документа без id 
    std::map<std::string, double> id_freq = search_server.GetWordFrequencies(document_id); // получаю частоту слов для id
    // записываю только слова для id В вектор
    for (std::map<std::string, double>::iterator it = id_freq.begin(); it != id_freq.end(); ++it) {
      key_words.push_back(it->first);
    }
    // добавляю id : vector (слов) в итоговую пару document_struct
    all_document_struct.push_back(std::make_pair(document_id, key_words));  // добавляю в вектора для каждого id его струткуру документов 

  }
  // иду по вектору all_document_struct и нахожу равные key_words
  for (auto i = 0; i < (int) all_document_struct.size() - 1; ++i) {
    for (auto j = i + 1; j < (int) all_document_struct.size(); ++j) {
      if (all_document_struct[i].second == all_document_struct[j].second) {
        std::cout << "Found duplicate document id " << all_document_struct[j].first << std::endl;
        search_server.RemoveDocument(all_document_struct[j].first);
        all_document_struct.erase(all_document_struct.begin() + j);
        j--;
      }
    }
  }


}
