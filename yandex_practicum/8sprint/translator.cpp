#pragma once
#include "Translator.h"



void  Translator::Add(string_view source, string_view target) {
    wordsStore_.emplace_back(source);
    source = wordsStore_.back();
    wordsStore_.emplace_back(target);
    target = wordsStore_.back();

    forwardMap_[source] = target;
    backwardMap_[target] = source;
}
string_view Translator::TranslateForward(string_view source) const {
    if (forwardMap_.count(source) != 0) {
        return forwardMap_.at(source);
    }
    return string_view{};
}
string_view Translator::TranslateBackward(string_view target) const {
    if (backwardMap_.count(target) != 0) {
        return backwardMap_.at(target);
    }
    return string_view{};
}

