#pragma once
#include <utility>
#include <iostream>
#include <string>

#include "document.h"
using namespace std;
template <typename It>
class IteratorRange
{
public:
    IteratorRange(It begin, It end, int size)
        : begin_(begin), end_(end), size_(size) {}
    auto begin() {
        return begin_;
    }
    auto end() {
        return end_;
    }
    auto size() {
        return size_;
    }
private:

    It begin_;
    It end_;
    int size_;
};
std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating
        << " }";
    return out;
}
template <typename Iterator>
std::ostream& operator << (std::ostream& out, IteratorRange<Iterator> iterator) {
    for (auto it = iterator.begin(); it < iterator.end(); ++it) {
        out << *it;
    }

    return out;
}

int DivUp(int x, int y)
{
    return x / y + (x % y != 0 ? 1 : 0);
}

template <typename Iterator>
class Paginator {
public:
    Paginator() = default;
    Paginator(Iterator begin, Iterator end, int page_size)
    {
        number_of_page_ = DivUp(end - begin, page_size);
        for (int i = 0; i < number_of_page_; ++i) {
            auto end_range = begin + page_size;
            if (i != number_of_page_ - 1) {
                IteratorRange <Iterator> iterator_range(begin, end_range, page_size);
                pages_.push_back(iterator_range);
                begin += page_size;
            }
            else {
                IteratorRange <Iterator> iterator_range(begin, end, end - begin);
                pages_.push_back(iterator_range);
                begin += page_size;
            }
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
    int number_of_page_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
