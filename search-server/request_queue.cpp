#include "request_queue.h"


    RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server) {
        // напишите реализацию
        no_result_cnt_ = 0;
        current_time_ = 0;
    }

    int RequestQueue::GetNoResultRequests() const {
        // напишите реализацию
        return no_result_cnt_;
    }

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        const auto result = server_.FindTopDocuments(raw_query, status);
        AddRequest(result.size());
        check_and_update_deque();
        return result;
    }

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        const auto result = server_.FindTopDocuments(raw_query);
        AddRequest(result.size());
        check_and_update_deque();
        return result;
    }

    void RequestQueue::AddRequest(int result_number) {
        current_time_++;
        requests_.push_back({ current_time_, result_number });
        if (0 == result_number) {
            no_result_cnt_++;
        }
    }

    void RequestQueue::check_and_update_deque() {
        if (min_in_day_ < (current_time_ - requests_.front().timestapm)) {
            if (!requests_.front().result) {
                no_result_cnt_--;
            }
            requests_.pop_front();
        }
    }

