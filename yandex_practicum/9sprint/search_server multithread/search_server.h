#pragma once
#include "search_server.h"
#include <vector>
#include <cassert>
#include <string>
#include <algorithm>
#include <numeric>
#include <set>
#include <deque>
#include <map>
#include <stdexcept>
#include "document.h"
#include <execution>
#include <future>
#include <type_traits>
#include <thread>
#include "string_processing.h"
#include "concurrent_map.h"


const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;


    // версии с выборм политики
    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy policy, std::string_view raw_query) const;
    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    //============================ new method ================================
    size_t GetDocumentCount() const;
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    std::set<int>::iterator begin();
    std::set<int>::iterator end();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    std::deque<std::string> document_text_; // здесь храним текст документа
    const std::set<std::string, std::less<>> stop_words_; // храним уникальные стоп слова

    //в отальных контейнерах храним стрингвью
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> word_freq_;


    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(std::string_view word) const;
    static bool IsValidWord(std::string_view word);
    static int ComputeAverageRating(const std::vector<int>& ratings);
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    QueryWord ParseQueryWord(std::string_view text) const;
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool use_sort = true) const;
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    //версия с выбором политики
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy policy, const Query& query,
        DocumentPredicate document_predicate) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    using namespace std;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);
    constexpr double EPSILON = 1e-6;
    sort(matched_documents.begin(), matched_documents.end(),
        [EPSILON](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}




template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    using namespace std;
    map<int, double> document_to_relevance;
    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}



// версия метода поиска документов с выбором политики
template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy policy, const Query& query,
    DocumentPredicate document_predicate) const {
    // если выбрана последовательная политика , вызываем обычный метод поиска документов
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        FindAllDocuments(query, document_predicate);
    }
    else {
        using namespace std;
        ConcurrentMap<int, double> document_to_relevance(100);

        for_each(execution::par, query.plus_words.begin(), query.plus_words.end(), [this, &document_to_relevance, &document_predicate](string_view word) {
            if (word_to_document_freqs_.count(word) > 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
            });

        for_each(execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_to_relevance](string_view word) {
            if (word_to_document_freqs_.count(word) > 0) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.Erase(document_id);
                }
            }
            });

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;

    }

}


// 1 обертка над FindTopdocuments  только с запросом 
template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy policy, std::string_view raw_query) const {
    //если передана последовательная политика вызываем обертку над обычным методом FindTopDocuments
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    // иначе вызываем обертку над пралеллельным методом FindTopDocements
    else {
        return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
    }
}

// 2 обертка над FindTopdocuments с запросом и статусом 
template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy policy, std::string_view raw_query, DocumentStatus status) const {
    //если передана последовательная политика вызываем обычный метод FindTopDocuments с запросом и предикатом
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    // иначе вызываем пралеллельный метод FindTopDocements с запросом и предикатом
    else {
        return FindTopDocuments(std::execution::par,
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
}
//
// 3 метод FindTopdocuments с запросом и предикатом
template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy policy, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    //если передана последовательная политика вызываем обычный метод FindTopDocuments с запросом и предикатом
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    }
    else {
        const auto query = ParseQuery(raw_query);
        // вызываем параллельную версию поиска документов
        auto matched_documents = FindAllDocuments(policy, query, document_predicate);
        constexpr double EPSILON = 1e-6;
        // используем параллельную сортировку
        sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
            [EPSILON](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
}

