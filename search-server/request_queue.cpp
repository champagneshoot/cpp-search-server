#include "request_queue.h"
using namespace std;
RequestQueue ::RequestQueue(const SearchServer& search_server): search_server_(search_server){}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) 
{

    QueryResult result;
    result.query_result_stat = search_server_.FindTopDocuments(raw_query, status);
    if (result.query_result_stat.empty()) {
        result.is_no_result = true;
    }
    if (requests_.size() < min_in_day_) {
        requests_.push_back(result);
    }
    else {
        requests_.pop_front();
        requests_.push_back(result);
    }
    return result.query_result_stat;
}
vector<Document> RequestQueue:: AddFindRequest(const string& raw_query) 
{
    QueryResult result;
    result.query_result = search_server_.FindTopDocuments(raw_query);
    if (result.query_result.empty()) 
    {
        result.is_no_result = true;
    }
    if (requests_.size() < min_in_day_) 
    {
        requests_.push_back(result);
    }
    else 
    {
        requests_.pop_front();
        requests_.push_back(result);
    }
    return result.query_result;
}
int RequestQueue::GetNoResultRequests() const 
{
    int cnt = 0;
    for (auto it = requests_.begin(); it < requests_.end(); ++it) {
        if ((*it).is_no_result) 
        {
            ++cnt;
        }
    }
    return cnt;
}
bool RequestQueue::QueryResult::empty() 
{
    return query_result_pred.empty() && query_result_stat.empty() && query_result.empty();
}
