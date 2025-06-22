/**
 * @file HttpRequestHandler.h
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <filesystem>
#include <fstream>

#include <iostream>
#include <sqlite3.h>
#include "HttpRequestHandler.h"
#include <string>
#include <chrono>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

set<string> extractWords(const string& content) { //copied from mkindex.cpp
    set<string> words;
    string cleanText;
    bool inTag = false;

    // Remove HTML tags
    for (char c : content) {
        if (c == '<') inTag = true;
        else if (c == '>') inTag = false;
        else if (!inTag) cleanText += c;
    }

    // Extract words
    stringstream ss(cleanText);
    string word;
    while (ss >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        word.erase(remove_if(word.begin(), word.end(), ::ispunct), word.end());
        if (word.length() >= 3) words.insert(word);
    }
    return words;
}

HttpRequestHandler::HttpRequestHandler(string homePath)
{
    this->homePath = homePath;
}

/**
 * @brief Serves a webpage from file
 *
 * @param url The URL
 * @param response The HTTP response
 * @return true URL valid
 * @return false URL invalid
 */
bool HttpRequestHandler::serve(string url, vector<char> &response)
{
    // Blocks directory traversal
    // e.g. https://www.example.com/show_file.php?file=../../MyFile
    // * Builds absolute local path from url
    // * Checks if absolute local path is within home path
    auto homeAbsolutePath = filesystem::absolute(homePath);
    auto relativePath = homeAbsolutePath / url.substr(1);
    string path = filesystem::absolute(relativePath.make_preferred()).string();

    if (path.substr(0, homeAbsolutePath.string().size()) != homeAbsolutePath)
        return false;

    // Serves file
    ifstream file(path);
    if (file.fail())
        return false;

    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    response.resize(fileSize);
    file.read(response.data(), fileSize);

    return true;
}

bool HttpRequestHandler::handleRequest(string url,
                                               HttpArguments arguments,
                                               vector<char> &response)
{
    string searchPage = "/search";
    if (url.substr(0, searchPage.size()) == searchPage)
    {
        string searchString;
        if (arguments.find("q") != arguments.end())
            searchString = arguments["q"];

        // Header
        string responseString = string("<!DOCTYPE html>\
<html>\
\
<head>\
    <meta charset=\"utf-8\" />\
    <title>EDAoogle</title>\
    <link rel=\"preload\" href=\"https://fonts.googleapis.com\" />\
    <link rel=\"preload\" href=\"https://fonts.gstatic.com\" crossorigin />\
    <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;800&display=swap\" rel=\"stylesheet\" />\
    <link rel=\"preload\" href=\"../css/style.css\" />\
    <link rel=\"stylesheet\" href=\"../css/style.css\" />\
</head>\
\
<body>\
    <article class=\"edaoogle\">\
        <div class=\"title\"><a href=\"/\">EDAoogle</a></div>\
        <div class=\"search\">\
            <form action=\"/search\" method=\"get\">\
                <input type=\"text\" name=\"q\" value=\"" +
                                       searchString + "\" autofocus>\
            </form>\
        </div>\
        ");
		        float searchTime = 0.1F;
        		vector<string> results;
                auto start = chrono::steady_clock::now();

        // Open the SQLite database
        sqlite3 *db;
        if (sqlite3_open("index.db", &db) != SQLITE_OK) {
            cerr << "Error opening database: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        // Split search string into lowercase words
        set<string> searchWords = extractWords(searchString);  // assuming this function exists as in main()

        if (!searchWords.empty()) {
            string sql =
                "SELECT d.path FROM documents d "
                "JOIN word_document wd ON d.doc_id = wd.doc_id "
                "JOIN words w ON wd.word_id = w.word_id "
                "WHERE w.word = ?";

            // If there is more than one word, we want docs that match *all* words
            // Use INTERSECT to get common results
            vector<string> wordVec(searchWords.begin(), searchWords.end());
            for (size_t i = 1; i < wordVec.size(); ++i)
                sql += " INTERSECT " + sql;

            // Prepare final query
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                // Bind each word
                for (size_t i = 0; i < wordVec.size(); ++i)
                    sqlite3_bind_text(stmt, i + 1, wordVec[i].c_str(), -1, SQLITE_STATIC);

                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const unsigned char *path = sqlite3_column_text(stmt, 0);
                    results.push_back(string(reinterpret_cast<const char *>(path)));
                }
            } else {
                cerr << "Failed to prepare search statement: " << sqlite3_errmsg(db) << endl;
            }

            sqlite3_finalize(stmt);
        }

        sqlite3_close(db);
        auto end = chrono::steady_clock::now();
        searchTime = chrono::duration<float>(end - start).count();



        // Print search results
        responseString += "<div class=\"results\">" + to_string(results.size()) +
                          " results (" + to_string(searchTime) + " seconds):</div>";
        for (auto &result : results)
            responseString += "<div class=\"result\"><a href=\"" +
                              result + "\">" + result + "</a></div>";

        // Trailer
        responseString += "    </article>\
</body>\
</html>";

        response.assign(responseString.begin(), responseString.end());

        return true;
    }
    else
        return serve(url, response);

    return false;
}
