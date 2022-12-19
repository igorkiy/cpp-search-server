

/* Подставьте вашу реализацию класса SearchServer сюда */
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>
#include <tuple>

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
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {

        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus doc_status, int rating) { return status == doc_status; });
    }

    template<typename TPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        TPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }


    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus>  MatchDocument(const string& raw_query,
        int document_id) const {
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

        return { matched_words, documents_.at(document_id).status_ };
    }

private:
    struct DocumentData {
        int rating_;
        DocumentStatus status_;
    };

    set<string> stop_words_;
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
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
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

    template<typename TPredicate>
    vector<Document> FindAllDocuments(const Query& query, TPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words_) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (document_predicate(document_id, documents_.at(document_id).status_, documents_.at(document_id).rating_)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const string& word : query.minus_words_) {
            if (word_to_document_freqs_.count(word) == 0) {
                break;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating_ });
        }
        return matched_documents;
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
            cout << " Hint: "s << hint;
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
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void RelevanceSortTest() {
    const int doc_id = 40;
    const int doc_id2 = 41;
    const int doc_id3 = 43;

    vector<int> rating{ 1,2,3 };
    const string content = "белый кот и модный ошейник"s;
    const string content2 = "пушистый кот пушистый хвост"s;
    const string content3 = "ухоженный пёс выразительные глаза"s;
    const vector<pair<string, double>> IDFTF{{content2, 0.6507}, {content3,0.2746} , {content, 0.1014}};

    SearchServer server;
    server.AddDocument(doc_id,  content,  DocumentStatus::ACTUAL, rating);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, rating);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, rating);

    // проверяем , что документы отсортированы по релевантности
  const  vector<Document> doc = server.FindTopDocuments(content);
   for (int i = 0; i < doc.size(); i++) {
       ASSERT_EQUAL(doc[i].relevance, IDFTF[i].second);
   }

}

// Тест проверяет, что поисковая система исключет документы содержащие минус слова
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id = 40;
    const int doc_id2 = 41;
    const string content = "black cat in the street"s;
    const string content2 = "black dog the street";
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    const vector<Document> doc = server.FindTopDocuments("-dog street");
    ASSERT(doc.size() == 1);
    ASSERT_EQUAL(doc[0].id, doc_id);
}

// Тест проверят, что поисковая система корректно возвращает  средний рейтиг документа
void TestAverageRating() {
    const int doc_id = 40;
    const string content = "black cat in the street"s;
    const vector<int> ratings = { 1, 2, 3 };
    const int average_rating = (1 + 2 + 3) / 3;

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("black cat "s);
    ASSERT(found_docs.size() == 1);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.rating, average_rating);
}

// Тест проверяет, что поисковая система возвращает документы с указанным статусом
void TestSearchByStatus() {
    const int doc_idACTUAL = 40;
    const int doc_idIRRELEVANT = 41;
    const int doc_idBANNED = 42;
    const int doc_idREMOVED = 43;

    const string content = "black cat in the street"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
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

/*void TestCorrectRelevanceCalculation() {
    const int doc_id = 40;
    const int doc_id = 41;
    const int doc_id = 42;
    const int doc_id = 43;

    const string content = "black cat in the street"s;
    const vector<int> ratings = { 1, 2, 3 };
    const
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("black cat "s);
    assert(found_docs.size() == 1);
    const Document& doc0 = found_docs[0];
    assert(doc0.relevance == )


 }*/

// Тест проверяет,что возвращаются все слова документа и что, документы в которых есть минус слова исключены
void TestMatchingDocuments() {

    const int doc_id = 40;
    const string content = "black cat in the street"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
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
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestSearchByStatus);
    RUN_TEST(TestAverageRating);
}

int main() {
    TestSearchServer();
}


/*Задание
+ 1) Добавление документов.Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
+ 2) Поддержка стоп - слов.Стоп - слова исключаются из текста документов.
+ 3) Поддержка минус - слов.Документы, содержащие минус - слова поискового запроса, не должны включаться в результаты поиска.
+ 4) Матчинг документов.При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе.Если есть соответствие хотя бы по одному минус - слову, должен возвращаться пустой список слов.
+ 5) Сортировка найденных документов по релевантности.Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
+ 6) Вычисление рейтинга документов.Рейтинг добавленного документа равен среднему арифметическому оценок документа.
7) Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
+ 8) Поиск документов, имеющих заданный статус.
+ 9) Корректное вычисление релевантности найденны документов.
*/


