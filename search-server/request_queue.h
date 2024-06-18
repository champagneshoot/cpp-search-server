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
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult 
    {
        bool is_no_result = false;
    };
    std::deque<QueryResult> requests_;
    template <typename Query_Result>
    void function_for_AddFindRequest(QueryResult& result, Query_Result& query_result);
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
};

template <typename Query_Result>
void RequestQueue::function_for_AddFindRequest(QueryResult& result, Query_Result& query_result)
{
    if (query_result.empty()) {
        result.is_no_result = true;
    }
    if (requests_.size() < min_in_day_) {
        requests_.push_back(result);
    }
    else
    {
        requests_.pop_front();
        requests_.push_back(result);
    }
}

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
{
    QueryResult result;
    auto query_result = search_server_.FindTopDocuments(raw_query, document_predicate);
    function_for_AddFindRequest(result, query_result);
    return query_result;
}
