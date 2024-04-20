#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include<numeric>
#include <optional>
#include <stdexcept>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ALMOST_ZERO = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

static bool IsValidWord(const string& word)
{
    bool valid = none_of(word.begin(), word.end(), [](char c) {return c >= '\0' && c < ' '; });
    if (!valid)
        throw invalid_argument("Error! invalid argument");
    return valid;
}

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) { for (auto word : stop_words) { IsValidWord(word); } }

    explicit SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {} 


    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings)
    {
        if (document_id < 0)
            throw invalid_argument("Error in function AddDocument! document_id must be greater than 0");
        if (documents_.count(document_id) != 0)
            throw invalid_argument("Error in function AddDocument! this id has been added already");
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate)const
    {
        if(raw_query.empty())
            throw invalid_argument("Error in function FindTopDocuments! The text is missing");
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < ALMOST_ZERO) {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const
    {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const
    {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words)
        {
            
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };

    }
    int GetDocumentId(int index) const
    {
        if (index >= 0 and index <= GetDocumentCount())
        {
            auto it = documents_.begin();
            advance(it, index);
            return it->first;
        }
        throw out_of_range("Error in function GetDocumentId! index<0 or index>document_count");
    }
private:
    
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;

    map<string, map<int, double>> word_to_document_freqs_;

    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word) and IsValidWord(word)) 
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(),0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text.empty())
        {
            throw invalid_argument("Error! The text is missing");
        }
        IsValidWord(text);
        if (text[0] == '-') 
        {
            is_minus = true;
            text = text.substr(1);
            if (text.empty())
            {
                throw invalid_argument("Error! The text is missing");
            }
            if(text[0]=='-')
                throw invalid_argument("Error! word[0]=='-' ");
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }


    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    try {
        SearchServer search_server("and with for"s);

        
        search_server.AddDocument(6, "funny pet and funny frog"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(5, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        search_server.AddDocument(4, "funny pet with funny sweet dog"s, DocumentStatus::ACTUAL, { 1, 3, 1 });
        search_server.AddDocument(10, "pet snake"s, DocumentStatus::BANNED, { 7, 1, 7 });
        search_server.AddDocument(2, "funny frog and funny hamster"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
        search_server.AddDocument(1, "funny pet and not very funny parrot"s, DocumentStatus::ACTUAL, { 1, 1, 2 });

        
        cout << "Search \"funny pet\""s << endl;
        for (const auto& document : search_server.FindTopDocuments("funny pet"s)) {
            PrintDocument(document);
        }
        cout << endl;

        cout << "Search \"not very funny parrot\""s << endl;
        for (const auto& document : search_server.FindTopDocuments("not very funny parrot"s)) 
        {
            PrintDocument(document);
        }
        cout << endl;

        cout << "Search \"funny pet -curly\""s << endl;
        for (const auto& document : search_server.FindTopDocuments("funny pet -curly"s)) 
        {
            PrintDocument(document);
        }
        cout << endl;

        cout << "Search \"funny pet -snake\""s << endl;
        for (const auto& document : search_server.FindTopDocuments("funny pet -snake"s)) 
        {
            PrintDocument(document);
        }
        cout << endl;

        cout << "Document ID at index 0: "s << search_server.GetDocumentId(0) << endl;
        cout << "Document ID at index 1: "s << search_server.GetDocumentId(1) << endl;
        cout << "Document ID at index 2: "s << search_server.GetDocumentId(2) << endl;
        cout << "Document ID at index 3: "s << search_server.GetDocumentId(3) << endl;
        cout << "Document ID at index 4: "s << search_server.GetDocumentId(4) << endl;
        cout << "Document ID at index 5: "s << search_server.GetDocumentId(5) << endl;

        
        cout << "Matched words in document 2 for query \"funny pet with curly hair\""s << endl;
        auto [words, status] = search_server.MatchDocument("funny pet with curly hair"s, 2);
        cout << "Document status: "s;
        switch (status) {
        case DocumentStatus::ACTUAL:
            cout << "ACTUAL"s;
            break;
        case DocumentStatus::IRRELEVANT:
            cout << "IRRELEVANT"s;
            break;
        case DocumentStatus::BANNED:
            cout << "BANNED"s;
            break;
        case DocumentStatus::REMOVED:
            cout << "REMOVED"s;
            break;
        }
        cout << endl << "Matched words: "s;
        for (const auto& word : words) {
            cout << word << " "s;
        }
        cout << endl;

        cout << "Trying to get document with invalid index..."s << endl;
        try {
            int invalid_index = 10;
            cout << "Document ID at index "s << invalid_index << ": "s << search_server.GetDocumentId(invalid_index) << endl;
        }
        catch (const out_of_range& e) {
            cout << "Caught out_of_range exception: "s << e.what() << endl;
        }

        
        cout << "Trying to add document with invalid ID..."s << endl;
        try {
            search_server.AddDocument(-1, "invalid document"s, DocumentStatus::ACTUAL, {});
        }
        catch (const invalid_argument& e) {
            cout << "Caught invalid_argument exception: "s << e.what() << endl;
        }

        
        cout << "Trying to search with empty query..."s << endl;
        try {
            search_server.FindTopDocuments("");
        }
        catch (const invalid_argument& e) {
            cout << "Caught invalid_argument exception: "s << e.what() << endl;
        }
    }
    catch (const exception& e) {
        cerr << "Caught exception: "s << e.what() << endl;
    }

    return 0;
}
