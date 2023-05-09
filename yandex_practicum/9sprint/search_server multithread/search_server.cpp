#include "search_server.h"
#include <cmath>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(
        SplitIntoWords((stop_words_text)))  // Invoke delegating constructor from string container
{
}
SearchServer::SearchServer(string_view stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, string_view text, DocumentStatus status,
    const vector<int>& ratings) {
    using namespace std;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    document_text_.push_back(string(text));
    auto words = SplitIntoWordsNoStop(document_text_.back());

    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_freq_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}
std::set<int>::const_iterator  SearchServer::begin() const {
    return document_ids_.begin();
}
std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}
std::set<int>::iterator  SearchServer::begin() {
    return document_ids_.begin();
}
std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

tuple< vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query,
    int document_id) const {
    if (raw_query.empty()) {
        throw std::invalid_argument("incorrect value");
    }
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("incorrect id");
    }

    const auto query = ParseQuery(raw_query, true);
    vector<string_view> matched_words;

    for (string_view word : query.minus_words) {
        //если минус слова нет в словаре пропускаем итерацию проверяем дальше
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        //если минус слово найдено очищаем результаты возвращаем пустой вектор и статус 
        if (word_to_document_freqs_.at(word).count(document_id)) {
            //matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (const string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

// обертака над матч документ
tuple< vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, string_view raw_query,
    int document_id) const {
    return MatchDocument(raw_query, document_id);
}


tuple< std::vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, string_view raw_query,
    int document_id) const {
    if (raw_query.empty()) {
        throw std::invalid_argument("incorrect value");
    }
    if (document_ids_.count(document_id) <= 0) {
        throw std::out_of_range("incorrect id");
    }
    const auto query = ParseQuery(raw_query, false);

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&](auto word) {
        return word_to_document_freqs_.count(word) &&
            word_to_document_freqs_.at(word).count(document_id); })) {
        return { vector<string_view>{}, documents_.at(document_id).status };
    }
    vector<string_view> matched_words(query.plus_words.size());
    auto end_words = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](auto word) {
        return word_to_document_freqs_.count(word) &&
            word_to_document_freqs_.at(word).count(document_id);
        });
    //удаляем лишнее место после последнего слова
    matched_words.resize(std::distance(matched_words.begin(), end_words));
    std::sort(matched_words.begin(), matched_words.end());

    auto last = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    using namespace std;
    vector<string_view> words;
    for (auto& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    using namespace std;
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(word) + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(string_view text, bool use_sort) const {
    Query result;
    for (auto& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    //если не пралельная версия то сортируем после получаем умный итератор и удаляем дупликаты
    if (use_sort) {
        std::sort(result.plus_words.begin(), result.plus_words.end(), [](const auto& lhs, const auto& rhs) { return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
            });
        auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last, result.plus_words.end());
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

//============================ new method ================================
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    assert(word_freq_.count(document_id) != 0);
    return { word_freq_.at(document_id) };
}

// удаляет элементы с данным ид
void SearchServer::RemoveDocument(int document_id) {
    if (!document_ids_.count(document_id)) { return; }
    word_freq_.erase(document_id);
    documents_.erase(document_id);

    for (auto& element : word_to_document_freqs_) {
        if (element.second.count(document_id)) {
            element.second.erase(document_id);
        }
    }
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    // проверяем существует ли документ с данным id 
    if (!document_ids_.count(document_id)) { return; }
    // создаем вектор с ключевыми словами док для удаления
    std::vector<std::string_view> keywords_for_remove(word_freq_.at(document_id).size());

    std::transform(std::execution::par, word_freq_.at(document_id).begin(), word_freq_.at(document_id).end(), keywords_for_remove.begin(), [&keywords_for_remove, &document_id](auto document) {
        return document.first;
        });

    // двигаемся по ключевм словам удаляем документы с данным id 
    std::for_each(std::execution::par, keywords_for_remove.begin(), keywords_for_remove.end(), [this, &document_id](auto word) {
        this->word_to_document_freqs_.at(word).erase(document_id);
        });

    //находим и удаляем данный id из  вектора id документов
    document_ids_.erase(document_id);
    word_freq_.erase(document_id);
    documents_.erase(document_id);
}



