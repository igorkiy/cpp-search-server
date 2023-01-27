#pragma once
#include <string>
#include <iostream>
#include <assert.h>

// -------- ������ ��������� ������ ��������� ������� ----------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    using namespace std;
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


void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    using namespace std;
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


// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent();

void TestRelevanceSort();
// ���� ���������, ��� ��������� ������� �������� ��������� ���������� ����� �����
void TestExcludeMinusWordsFromAddedDocumentContent();
// ���� ��������, ��� ��������� ������� ��������� ����������  ������� ������ ���������
void TestAverageRating();
// ���� ���������, ��� ��������� ������� ���������� ��������� � ��������� ��������
void TestSearchByStatus();

void TestCorrectRelevanceCalculation();
// ���� ���������,��� ������������ ��� ����� ��������� � ���, ��������� � ������� ���� ����� ����� ���������
void TestMatchingDocuments();
// --------- ��������� ��������� ������ ��������� ������� -----------

template <typename TFunc>
void RunTestImpl(TFunc& testFunc, const std::string& func_name) {
    testFunc();
    std::cerr << func_name << " OK" << std::endl;
}
#define RUN_TEST(func)  RunTestImpl((func), (#func))

void TestSearchServer();


