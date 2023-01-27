#pragma once
#include <vector>
#include <string>
#include <deque>
#include "search_server.h"


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        auto result = server_.FindTopDocuments(raw_query, document_predicate);
        requests_.push_back({ result, !result.empty() });
        total_request_cnt_++;
        if (result.empty()) {
            no_result_cnt_++;
        }
        check_and_update_deque();
        return result;
    }
   
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {        
        auto result = server_.FindTopDocuments(raw_query, status);
        requests_.push_back({ result, !result.empty() });
        total_request_cnt_++;
        if (result.empty()) {
            no_result_cnt_++;
        }
        check_and_update_deque();
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        auto result = server_.FindTopDocuments(raw_query);
        requests_.push_back({ result, !result.empty() });
        total_request_cnt_++;
        if (result.empty()) {
            no_result_cnt_++;
        }
        check_and_update_deque();
        return result;
    }

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::vector<Document> result;
        bool is_no_result_;
        
    };

    void check_and_update_deque() {
        if (total_request_cnt_ > min_in_day_) {
            if (!requests_.front().is_no_result_) {
                no_result_cnt_--;
            }
            else {
                total_request_cnt_--;
            }
            requests_.pop_front();
        }
    }

    std::deque<QueryResult> requests_;
    const SearchServer& server_;
    const static int min_in_day_ = 1440;
    int no_result_cnt_;
    int total_request_cnt_;
};

