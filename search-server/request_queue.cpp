#include "request_queue.h"


    RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server) {
        // напишите реализацию
        no_result_cnt_ = 0;
        total_request_cnt_ = 0;
    }
    

    int RequestQueue::GetNoResultRequests() const {
        // напишите реализацию
        return no_result_cnt_;
    }

