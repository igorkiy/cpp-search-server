#pragma once
#include <string>
#include <string_view>
#include<deque>
using namespace std;

class Translator {
public:
    Translator() = default;
    void Add(string_view source, string_view target);
    string_view TranslateForward(string_view source) const;
    string_view TranslateBackward(string_view target) const;

private:
    deque<string> wordsStore_{};
    map<string_view, string_view> forwardMap_; 
    map<string_view, string_view> backwardMap_;

};
