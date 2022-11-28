

#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        document_count_++;
        double word_count = 1.0;
        for (const auto& word : words) {
            word_to_documents_freqs_[word][document_id] += ((word_count) / words.size()); // сохраням слова добавляем  id и TF
        }
    }

    int GetDocumentCount() const {
        return document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    int document_count_ = 0;
    struct Query {
        set<string> minus_words_;
        set<string> plus_words_;
    };
    map<string, map<int, double>> word_to_documents_freqs_;
    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWords(text)) {
            if (word[0] == '-') {
                word.substr(1);
                if (!IsStopWord(word))
                {
                    query_words.minus_words_.insert(word);
                }
            }
            else {
                if (!IsStopWord(word)) {
                    query_words.plus_words_.insert(word);
                }
            }
        }
        return query_words;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / static_cast<double>(word_to_documents_freqs_.at(word).size()));
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> document_to_relevance;
        for (const auto& word : query.plus_words_) {
            if (word_to_documents_freqs_.count(word)) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [id, termfreq] : word_to_documents_freqs_.at(word)) {
                    document_to_relevance[id] += (double)(inverse_document_freq * termfreq);
                }
            }
        }
        vector<int> documents_to_remove;
        for (const auto& word : query.minus_words_) {
            if (word_to_documents_freqs_.count(word)) {
                for (const auto& [id, tf] : word_to_documents_freqs_.at(word)) {
                    documents_to_remove.push_back(id);
                }
            }
        }
        if (!documents_to_remove.empty()) {
            for (const auto& id : documents_to_remove) {
                document_to_relevance.erase(id);
            }
        }
        vector<Document> matched_documents;
        for (const auto& [id, relevance] : document_to_relevance) {
            matched_documents.push_back({ id,relevance });
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}
