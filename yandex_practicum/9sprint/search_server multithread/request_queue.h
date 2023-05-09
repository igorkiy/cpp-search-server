#pragma once
#include <vector>
#include <string>
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string & raw_query);

    size_t GetNoResultRequests() const;
private:
    struct QueryResult {
        size_t timestapm;
        size_t result;
    };

    void AddRequest(size_t result_number);
    void check_and_update_deque();

    std::deque<QueryResult> requests_;
    const SearchServer& server_;
    const static int min_in_day_ = 1440;
    size_t no_result_cnt_;
    size_t current_time_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const auto result = server_.FindTopDocuments(raw_query, document_predicate);
    AddRequest(result.size());
    check_and_update_deque();
    return result;
}