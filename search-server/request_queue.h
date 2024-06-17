#pragma once
#include "search_server.h"
#include "document.h"
#include <deque>
#include <string>
#include <vector>
class RequestQueue 
{
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) 
    {
        QueryResult result;
        result.query_result_pred = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (result.query_result_pred.empty()) {
            result.is_no_result = true;
        }
        if (requests_.size() < min_in_day_) {
            requests_.push_back(result);
        }
        else {
            requests_.pop_front();
            requests_.push_back(result);
        }
        return result.query_result_pred;

    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult 
    {
        std::vector<Document> query_result_pred, query_result_stat, query_result;
        bool is_no_result = false;
        bool empty();

    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
};
