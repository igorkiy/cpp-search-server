#include "remove_duplicates.h";
#include <iostream>;
#include<map>;
#include "log_duration.h"
using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	std::vector<pair <int, std::map<string, double>>> documents;
	for (auto doc : search_server) {
		documents.push_back({ doc, search_server.GetWordFrequencies(doc) });
	}
		
	for ( auto it = documents.begin(); it != documents.end(); it = next(it)) { 
		for (auto next_it = next(it); next_it != documents.end();) {
			if (next_it->first > it->first && next_it->second == it->second) {
				cout << "Found duplicate document id " << next_it->first << endl;
				search_server.RemoveDocument(next_it->first);
				next_it = documents.erase(next_it);
			}
			else next_it = next(next_it);
		}

	}
}