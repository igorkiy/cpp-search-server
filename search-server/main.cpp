#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <Windows.h>

using namespace std;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

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


class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
      : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for(const auto& word : stop_words_) {
            if (!IsValidWord(word)) {
                throw invalid_argument("some words have unacceptable symbols"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("the document id is negtive value"s);
        }
        if (documents_.count(document_id) > 0) {
            throw invalid_argument("the document has already been added"s);
        }
        documents_id_.push_back(document_id);

        if(!IsValidWord(document)) {
            throw invalid_argument("some words have unacceptable symbols"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        DocumentPredicate document_predicate) const {
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("some words have unacceptable symbols"s);
        }
        if (!IsValidQuery(raw_query)) {
            throw invalid_argument("the query has two serial minus or hasnt word after minus"s);
        }
        const Query query = ParseQuery(raw_query);
        auto matchedDocuments = FindAllDocuments(query, document_predicate);

        sort(matchedDocuments.begin(), matchedDocuments.end(),
            [](const Document& lhs, const Document& rhs) {
                constexpr double EPSILON = 1e-6;
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });

        if (matchedDocuments.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matchedDocuments.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matchedDocuments;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus doc_status, int rating) { return status == doc_status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("the query has unacceptable symbols"s);
        }
        if (!IsValidQuery(raw_query)) {
            throw invalid_argument("the query has two serial minus or hasnt word after minus"s);
        }
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words_) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words_) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return  {matched_words, documents_.at(document_id).status};

    }

    int GetDocumentId(int index) const {
       return documents_id_.at(index);
    }

private:
    vector<int> documents_id_;
    struct DocumentData {
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
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data_;
        bool is_minus_;
        bool is_stop_;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words_;
        set<string> minus_words_;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop_) {
                if (query_word.is_minus_) {
                    query.minus_words_.insert(query_word.data_);
                }
                else {
                    query.plus_words_.insert(query_word.data_);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words_) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const string& word : query.minus_words_) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    bool IsValidQuery(const string& text) const {
        bool is_letter = false;
        for (auto i = 0; i < text.size(); i++) {
            if (text[i] != '-' && text[i] != ' ') {
                is_letter = true;
                continue;
            }
            if (text[i] == '-') {
                is_letter = false;
                if (text[i + 1] == '-') {
                    return false;
                }
            }
        }
        return is_letter;
    }
    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }
};


// -------- Начало модульных тестов поисковой системы ----------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestRelevanceSort() {
    const int doc_id0 = 40;
    const int doc_id1 = 41;
    const int doc_id2 = 42;

    vector<int> rating{ 1,2,3 };
    const string content0 = "белый кот и модный ошейник"s;
    const string content1 = "пушистый кот пушистый хвост"s;
    const string content2 = "ухоженный пёс выразительные глаза"s;

    vector<int> reference_serial_documents_id{ doc_id1, doc_id2, doc_id0 };
    SearchServer server("test"s);
    server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, rating);
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, rating);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, rating);

    // проверяем , что документы отсортированы по релевантности
    const  vector<Document> doc = server.FindTopDocuments("пушистый ухоженный кот");
    for (int i = 0; i < doc.size(); i++) {
        ASSERT_EQUAL(doc[i].id, reference_serial_documents_id[i]);
    }

}

// Тест проверяет, что поисковая система исключет документы содержащие минус слова
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id = 40;
    const int doc_id2 = 41;
    const string content = "black cat in the street"s;
    const string content2 = "black dog the street";
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("test"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    const vector<Document> doc = server.FindTopDocuments("-dog street");
    ASSERT(doc.size() == 1);
    ASSERT_EQUAL(doc[0].id, doc_id);
}

// Тест проверят, что поисковая система корректно возвращает  средний рейтиг документа
void TestAverageRating() {

    SearchServer server("test"s);
    {
        // тест с положительным рейтингом
        const int doc_id = 40;
        const string content = "yellow chicken in the street"s;
        vector<int> ratings = { 1 , 2 , 3 };
        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("yellow"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, average_rating);
    }
    {
        // тест со смешаным рейтингом
        const int doc_id = 41;
        const string content = "black cat in the street"s;
        vector<int> ratings = { -1 ,5, -3 , 0 };
        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("black cat "s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, average_rating);

    }
    {
        // тест с отрицательным рейтингом
        const int doc_id = 42;
        const string content = "white dog in the street"s;
        const vector<int> ratings = { -1 ,-7, -3, -1, -3 };
        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("white dog "s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, average_rating);
    }
}

// Тест проверяет, что поисковая система возвращает документы с указанным статусом
void TestSearchByStatus() {
    const int doc_idACTUAL = 40;
    const int doc_idIRRELEVANT = 41;
    const int doc_idBANNED = 42;
    const int doc_idREMOVED = 43;

    const string content = "black cat in the street"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("test"s);
    server.AddDocument(doc_idACTUAL, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_idIRRELEVANT, content, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_idBANNED, content, DocumentStatus::BANNED, ratings);
    server.AddDocument(doc_idREMOVED, content, DocumentStatus::REMOVED, ratings);
    {
        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::ACTUAL);
        //  проверяем что найден только один документ
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_idACTUAL);
    }
    {
        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::BANNED);
        //  проверяем что найден только один документ
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_idBANNED);
    }
    {
        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::IRRELEVANT);
        //  проверяем что найден только один документ
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_idIRRELEVANT);
    }
    {
        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::REMOVED);
        //  проверяем что найден только один документ
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_idREMOVED);
    }
}

void TestCorrectRelevanceCalculation() {
    const int doc_id0 = 40;
    const int doc_id1 = 41;
    const int doc_id2 = 42;

    vector<int> rating{ 1,2,3 };
    const string content0 = "пушистый кот пушистый хвост"s;
    const string content1 = "ухоженный пёс выразительные глаза"s;
    const string content2 = "белый кот и модный ошейник"s;
    const double IDF_TF_doc0 = 0.650672;

    SearchServer server("test"s);
    server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, rating);
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, rating);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, rating);
    const  vector<Document> doc = server.FindTopDocuments("пушистый ухоженный кот");
    constexpr double EPSILON = 1e-6;
    ASSERT(fabs(doc[0].relevance) - fabs(IDF_TF_doc0) < EPSILON);
}

// Тест проверяет,что возвращаются все слова документа и что, документы в которых есть минус слова исключены
void TestMatchingDocuments() {

    const int doc_id = 40;
    const string content = "black cat in the street"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("test"s);
    // проверяем что возвращаются все слова запроса
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const vector<string> contentWords = { "black"s, "cat"s, "in"s, "the"s, "street"s };
    const vector<string> matchedWords = get<vector<string>>(server.MatchDocument(content, doc_id));
    ASSERT_EQUAL(matchedWords.size(), contentWords.size());

    // проверяем что при наличии минус слов в запросе возвращается пустой список слов
    const string queryWithMinusWords = "black cat in -the street"s;
    const vector<string> emptyListMatchedWords = get<vector<string>>(server.MatchDocument(queryWithMinusWords, doc_id));
    ASSERT(emptyListMatchedWords.empty());

    //проверяем что MatchDocuments правильно возврящает статус
    const int doc_idACTUAL = 40;
    const int doc_idIRRELEVANT = 41;
    const int doc_idBANNED = 42;
    const int doc_idREMOVED = 43;

    server.AddDocument(doc_idACTUAL, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_idIRRELEVANT, content, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_idBANNED, content, DocumentStatus::BANNED, ratings);
    server.AddDocument(doc_idREMOVED, content, DocumentStatus::REMOVED, ratings);

    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idACTUAL)) == DocumentStatus::ACTUAL);
    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idIRRELEVANT)) == DocumentStatus::IRRELEVANT);
    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idBANNED)) == DocumentStatus::BANNED);
    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idREMOVED)) == DocumentStatus::REMOVED);
}

// --------- Окончание модульных тестов поисковой системы -----------

template <typename TFunc>
void RunTestImpl(TFunc& testFunc, const string& func_name) {
    testFunc();
    cerr << func_name << " OK" << endl;
}
#define RUN_TEST(func)  RunTestImpl((func), (#func))

void TestSearchServer() {
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestSearchByStatus);
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestRelevanceSort);
    RUN_TEST(TestCorrectRelevanceCalculation);
}


// ==================== для примера =========================

    void PrintDocument(const Document& document) {
        cout << "{ "s
            << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s << endl;
    }


    int main() {
        setlocale(LC_ALL, "rus");
        SearchServer search_server("и в на"s);
        // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
        // о неиспользуемом результате его вызова
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        try {
            search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        }
        catch(invalid_argument){
            cerr << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
        }

        try {
            search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        }
        catch(invalid_argument) {
            cerr << "Документ не был добавлен, так как его id отрицательный"s << endl;
        }

        try {
            search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        }
        catch(invalid_argument) {
            cerr << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
        }

        try {
            const auto documents = search_server.FindTopDocuments("--пушистый"s);
            for (const Document& document : documents) {
                PrintDocument(document);
            }
        }
        catch(invalid_argument) {
            cerr << "Ошибка в поисковом запросе"s << endl;
        }
        try {
            search_server.GetDocumentId(20);
        }
        catch (out_of_range) {
            cerr << "документ с данным id не найден"s << endl;
        }
    }
// ===============================================================================
