#include "test_examp_functions.h"
#include <numeric>

// -------- ������ ��������� ������ ��������� ������� ----------
using namespace std;
//
//// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
//void TestExcludeStopWordsFromAddedDocumentContent() {
//    const int doc_id = 42;
//    const string content = "cat in the city"s;
//    const vector<int> ratings = { 1, 2, 3 };
//    {
//        SearchServer server("test"s);
//        AddDocument(server, doc_id, content, DocumentStatus::ACTUAL, ratings);
//        const auto found_docs = server.FindTopDocuments("in"s);
//        ASSERT_EQUAL(found_docs.size(), 1u);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.id, doc_id);
//    }
//
//    {
//        SearchServer server("in the"s);
//        AddDocument(server, doc_id, content, DocumentStatus::ACTUAL, ratings);
//        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
//            "Stop words must be excluded from documents"s);
//    }
//}
//
//void TestRelevanceSort() {
//    const int doc_id0 = 40;
//    const int doc_id1 = 41;
//    const int doc_id2 = 42;
//
//    vector<int> rating{ 1,2,3 };
//    const string content0 = "����� ��� � ������ �������"s;
//    const string content1 = "�������� ��� �������� �����"s;
//    const string content2 = "��������� �� ������������� �����"s;
//
//    vector<int> reference_serial_documents_id{ doc_id1, doc_id2, doc_id0 };
//    SearchServer server("test"s);
//    AddDocument(server,doc_id0, content0, DocumentStatus::ACTUAL, rating);
//    AddDocument(server,doc_id1, content1, DocumentStatus::ACTUAL, rating);
//    AddDocument(server,doc_id2, content2, DocumentStatus::ACTUAL, rating);
//
//    // ��������� , ��� ��������� ������������� �� �������������
//    const  vector<Document> doc = server.FindTopDocuments("�������� ��������� ���");
//    for (int i = 0; i < doc.size(); i++) {
//        ASSERT_EQUAL(doc[i].id, reference_serial_documents_id[i]);
//    }
//
//}
//
//// ���� ���������, ��� ��������� ������� �������� ��������� ���������� ����� �����
//void TestExcludeMinusWordsFromAddedDocumentContent() {
//    const int doc_id = 40;
//    const int doc_id2 = 41;
//    const string content = "black cat in the street"s;
//    const string content2 = "black dog the street";
//    const vector<int> ratings = { 1, 2, 3 };
//
//    SearchServer server("test"s);
//    AddDocument(server,doc_id, content, DocumentStatus::ACTUAL, ratings);
//    AddDocument(server,doc_id2, content2, DocumentStatus::ACTUAL, ratings);
//    const vector<Document> doc = server.FindTopDocuments("-dog street");
//    ASSERT(doc.size() == 1);
//    ASSERT_EQUAL(doc[0].id, doc_id);
//}
//
//// ���� ��������, ��� ��������� ������� ��������� ����������  ������� ������ ���������
//void TestAverageRating() {
//
//    SearchServer server("test"s);
//    {
//        // ���� � ������������� ���������
//        const int doc_id = 40;
//        const string content = "yellow chicken in the street"s;
//        vector<int> ratings = { 1 , 2 , 3 };
//        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
//        AddDocument(server, doc_id, content, DocumentStatus::ACTUAL, ratings);
//        const auto found_docs = server.FindTopDocuments("yellow"s);
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.rating, average_rating);
//    }
//    {
//        // ���� �� �������� ���������
//        const int doc_id = 41;
//        const string content = "black cat in the street"s;
//        vector<int> ratings = { -1 ,5, -3 , 0 };
//        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
//        AddDocument(server, doc_id, content, DocumentStatus::ACTUAL, ratings);
//        const auto found_docs = server.FindTopDocuments("black cat "s);
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.rating, average_rating);
//
//    }
//    {
//        // ���� � ������������� ���������
//        const int doc_id = 42;
//        const string content = "white dog in the street"s;
//        const vector<int> ratings = { -1 ,-7, -3, -1, -3 };
//        const int average_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
//        AddDocument(server, doc_id, content, DocumentStatus::ACTUAL, ratings);
//        const auto found_docs = server.FindTopDocuments("white dog "s);
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.rating, average_rating);
//    }
//}
//
//// ���� ���������, ��� ��������� ������� ���������� ��������� � ��������� ��������
//void TestSearchByStatus() {
//    const int doc_idACTUAL = 40;
//    const int doc_idIRRELEVANT = 41;
//    const int doc_idBANNED = 42;
//    const int doc_idREMOVED = 43;
//
//    const string content = "black cat in the street"s;
//    const vector<int> ratings = { 1, 2, 3 };
//
//    SearchServer server("test"s);
//    AddDocument(server, doc_idACTUAL, content, DocumentStatus::ACTUAL, ratings);
//    AddDocument(server, doc_idIRRELEVANT, content, DocumentStatus::IRRELEVANT, ratings);
//    AddDocument(server, doc_idBANNED, content, DocumentStatus::BANNED, ratings);
//    AddDocument(server, doc_idREMOVED, content, DocumentStatus::REMOVED, ratings);
//    {
//        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::ACTUAL);
//        //  ��������� ��� ������ ������ ���� ��������
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.id, doc_idACTUAL);
//    }
//    {
//        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::BANNED);
//        //  ��������� ��� ������ ������ ���� ��������
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.id, doc_idBANNED);
//    }
//    {
//        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::IRRELEVANT);
//        //  ��������� ��� ������ ������ ���� ��������
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.id, doc_idIRRELEVANT);
//    }
//    {
//        const auto found_docs = server.FindTopDocuments("black cat "s, DocumentStatus::REMOVED);
//        //  ��������� ��� ������ ������ ���� ��������
//        ASSERT(found_docs.size() == 1);
//        const Document& doc0 = found_docs[0];
//        ASSERT_EQUAL(doc0.id, doc_idREMOVED);
//    }
//}
//
//void TestCorrectRelevanceCalculation() {
//    const int doc_id0 = 40;
//    const int doc_id1 = 41;
//    const int doc_id2 = 42;
//
//    vector<int> rating{ 1,2,3 };
//    const string content0 = "�������� ��� �������� �����"s;
//    const string content1 = "��������� �� ������������� �����"s;
//    const string content2 = "����� ��� � ������ �������"s;
//
//
//    const double IDF_TF_doc0 = 0.650672;
//
//    SearchServer server("test"s);
//    AddDocument(server,doc_id0, content0, DocumentStatus::ACTUAL, rating);
//    AddDocument(server,doc_id1, content1, DocumentStatus::ACTUAL, rating);
//    AddDocument(server,doc_id2, content2, DocumentStatus::ACTUAL, rating);
//    const  vector<Document> doc = server.FindTopDocuments("�������� ��������� ���");
//    constexpr double EPSILON = 1e-6;
//    ASSERT(fabs(doc[0].relevance) - fabs(IDF_TF_doc0) < EPSILON);
//}
//
//// ���� ���������,��� ������������ ��� ����� ��������� � ���, ��������� � ������� ���� ����� ����� ���������
//void TestMatchingDocuments() {
//
//    const int doc_id = 40;
//    const string content = "black cat in the street"s;
//    const vector<int> ratings = { 1, 2, 3 };
//
//    SearchServer server("test"s);
//    // ��������� ��� ������������ ��� ����� �������
//    AddDocument(server, doc_id, content, DocumentStatus::ACTUAL, ratings);
//    const vector<string> contentWords = { "black"s, "cat"s, "in"s, "the"s, "street"s };
//    const vector<string> matchedWords = get<vector<string>>(server.MatchDocument(content, doc_id));
//    ASSERT_EQUAL(matchedWords.size(), contentWords.size());
//
//    // ��������� ��� ��� ������� ����� ���� � ������� ������������ ������ ������ ����
//    const string queryWithMinusWords = "black cat in -the street"s;
//    const vector<string> emptyListMatchedWords = get<vector<string>>(server.MatchDocument(queryWithMinusWords, doc_id));
//    ASSERT(emptyListMatchedWords.empty());
//
//    //��������� ��� MatchDocuments ��������� ���������� ������
//    const int doc_idACTUAL = 40;
//    const int doc_idIRRELEVANT = 41;
//    const int doc_idBANNED = 42;
//    const int doc_idREMOVED = 43;
//
//    AddDocument(server, doc_idACTUAL, content, DocumentStatus::ACTUAL, ratings);
//    AddDocument(server, doc_idIRRELEVANT, content, DocumentStatus::IRRELEVANT, ratings);
//    AddDocument(server, doc_idBANNED, content, DocumentStatus::BANNED, ratings);
//    AddDocument(server, doc_idREMOVED, content, DocumentStatus::REMOVED, ratings);
//
//    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idACTUAL)) == DocumentStatus::ACTUAL);
//    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idIRRELEVANT)) == DocumentStatus::IRRELEVANT);
//    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idBANNED)) == DocumentStatus::BANNED);
//    ASSERT(get<DocumentStatus>(server.MatchDocument("black cat", doc_idREMOVED)) == DocumentStatus::REMOVED);
//}
//
//
//// --------- ��������� ��������� ������ ��������� ������� -----------
//
//void TestSearchServer() {
//    RUN_TEST(TestMatchingDocuments);
//    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
//    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
//    RUN_TEST(TestSearchByStatus);
//    RUN_TEST(TestAverageRating);
//    RUN_TEST(TestRelevanceSort);
//    RUN_TEST(TestCorrectRelevanceCalculation);
//}

void AddDocument(SearchServer& search_server, int document_id, const std::string &document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}
