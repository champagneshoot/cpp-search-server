#include "request_queue.h"
#include <algorithm>
using namespace std;
RequestQueue ::RequestQueue(const SearchServer& search_server): search_server_(search_server){}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) 
{
    QueryResult result;
    auto query_result = search_server_.FindTopDocuments(raw_query, status);
    function_for_AddFindRequest(result, query_result);
    return query_result;
}
vector<Document> RequestQueue:: AddFindRequest(const string& raw_query) 
{
    QueryResult result;
    auto query_result = search_server_.FindTopDocuments(raw_query);
    function_for_AddFindRequest(result, query_result);
    return query_result;
    
}
int RequestQueue::GetNoResultRequests() const 
{
    return std::count_if(requests_.begin(), requests_.end(), [](const QueryResult& result) {return result.is_no_result; });
}


