#include "search_server.h"
#include "remove_duplicates.h"
#include "document.h"
#include "string_processing.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <numeric>


void RemoveDuplicates(SearchServer& search_server) {
	std::set<std::set<std::string>> all_document; // структура слов для всех документов по id
	std::set<int> to_erase;
	for (const int document_id : search_server) {
		std::set<std::string> key_words; // только слова документа без id
		std::map<std::string, double> id_freq = search_server.GetWordFrequencies(document_id); // получаю частоту слов для id
		// записываю только слова для id В вектор
		for (auto&& [first, second] : id_freq) {
			key_words.insert(first);
		}
		// добавляю set (слов) в итогов document_struct при услови и что аналогичного набора там нет 
		if (all_document_struct.count(key_words) == 0) {
			all_document_struct.insert(key_words);
		}
		else {
			std::cout << "Found duplicate document id " << document_id << std::endl;
			to_erase.insert(document_id);
		}
	}

	for (auto document_id : to_erase) {
		search_server.RemoveDocument(document_id);  // удаляю документ)
	}

}

