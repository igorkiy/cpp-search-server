#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& word) {
        return search_server.FindTopDocuments(word);
        });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    auto documents = ProcessQueries(search_server, queries);
    const int SIZE = std::transform_reduce(std::execution::par, documents.begin(), documents.end(), 0,
        std::plus<>(), [](std::vector<Document>& doc) { return doc.size(); });
    std::vector<Document> result(SIZE);
    
    std::vector<Document>::iterator insert = result.begin();
    for (auto& docs : documents) {
      insert = std::transform(std::execution::par, docs.begin(), docs.end(), insert, [](Document doc) {
            return doc; });
    }
    return result;   
}

