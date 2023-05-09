#pragma once
#include <vector>
#include <string>
#include <set>
#include <execution>


std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    using namespace std;
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(string(str));
        }
    }
    return non_empty_strings;
}
