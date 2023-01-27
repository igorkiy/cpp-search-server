#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "document.h"

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(const Iterator& begin, const Iterator& end) : page_(begin, end) {}
    auto begin() const {
        return page_.first;
    }
    auto end() const {
        return page_.second;
    }
    size_t size() const {
        return (distance(begin, end));
    }
private:
    std::pair<Iterator, Iterator> page_; //���� ���������� �� ������ � ����� �������� 
};

template<class Iterator>
class Paginator {
public:
    Paginator(const Iterator begin, const Iterator end, size_t size) : page_size_(size) {
        Iterator page_begin = begin;
        Iterator page_end = begin;

        while (distance(page_end, end) > size) {
            advance(page_end, size);
            pages_.push_back(IteratorRange<Iterator>(page_begin, page_end)); //��������� ��������
            page_begin = page_end;
        }
        if (page_begin != end) { // ���� begin �� ����� �� ����� ������ ��������� �������� �������� �� begin �� end
            pages_.push_back(IteratorRange<Iterator>(page_end, end));
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    size_t size() const {  // ���������� ���������� �������
        return pages_.size();
    }

private:
    const size_t page_size_;  //������ ����� ��������
    std::vector<IteratorRange<Iterator> > pages_;  // ������ �������
};

std::ostream& operator <<(std::ostream& os, Document page) {
    using namespace std;
    os << "{ "s
        << "document_id = "s << page.id << ", "s
        << "relevance = "s << page.relevance << ", "s
        << "rating = "s << page.rating << " }"s;
    return os;
}

template<typename Iterator>
std::ostream& operator <<(std::ostream& os, const  IteratorRange<Iterator> page) {
    for (auto it = page.begin(); it != page.end(); it++) {
        os << *it;
    }
    return os;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
