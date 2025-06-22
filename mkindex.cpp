/**
 * @file mkindex.cpp
 * @author Marc S. Ressl
 * @brief Makes a database index
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */
#define SQLITE_ENABLE_FTS5  // Must be defined before including sqlite3.h
#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <sqlite3.h>
#include <cctype>
#include <algorithm>
#include <regex>
#include "CommandLineParser.h"

using namespace std;

static int onDatabaseEntry(void* userdata, int argc, char** argv, char** azColName) {
    cout << "--- Entry" << endl;
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    }
    return 0;
}

// Extract clean text from HTML content
string extractCleanText(const string& content)
{
    string clean = regex_replace(content, regex("<!--.*?-->|<script.*?</script>|</?[^>]+>", regex::icase), "");
    transform(clean.begin(), clean.end(), clean.begin(), ::tolower);

    // Remove punctuation and normalize spaces
    clean = regex_replace(clean, regex("[^a-z\\s]+"), "");
    clean = regex_replace(clean, regex("\\s+"), " ");

    // Trim leading/trailing spaces
    clean = regex_replace(clean, regex("^\\s+|\\s+$"), "");

    return clean;
}

int main(int argc, const char* argv[]) 
{
    // Step 1: Parse command-line arguments
    CommandLineParser parser(argc, argv);
    if (!parser.hasOption("-h")) {
        cout << "Error: must specify path with -h" << endl;
        return 1;
    }
    string wwwPath = parser.getOption("-h");
    if (wwwPath.empty() || !filesystem::exists(wwwPath) || !filesystem::is_directory(wwwPath)) {
        cout << "Error: invalid path: " << wwwPath << endl;
        return 1;
    }
    cout << "Valid path: " << wwwPath << endl;

    // Step 2: Open database
    sqlite3* db;
    char* errMsg = nullptr;
    cout << "Opening database..." << endl;
    if (sqlite3_open("index.db", &db) != SQLITE_OK) {
        cout << "Error: cannot open database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }
    cout << "Database opened successfully" << endl;

    // Step 3: Create FTS5 virtual table
    cout << "Creating FTS5 table..." << endl;
    if (sqlite3_exec(db, "CREATE VIRTUAL TABLE IF NOT EXISTS search_index USING fts5(path, content, tokenize='unicode61');",
                     nullptr, 0, &errMsg) != SQLITE_OK) {
        cout << "Error: failed to create FTS5 table: " << errMsg << endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }
    cout << "FTS5 table created successfully" << endl;

    // Step 4: Process HTML files
    cout << "Processing HTML files..." << endl;
    string wikiPath = (filesystem::path(wwwPath) / "wiki").string();
    if (!filesystem::exists(wikiPath)) {
        cout << "Error: wiki directory does not exist: " << wikiPath << endl;
        sqlite3_close(db);
        return 1;
    }

    // Step 5: Index files into database
    cout << "Indexing files..." << endl;
    sqlite3_stmt* insertDoc;
    if (sqlite3_prepare_v2(db, "INSERT INTO search_index (path, content) VALUES (?, ?);", -1, &insertDoc, nullptr) != SQLITE_OK) {
        cout << "Error: failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }

    for (const auto& entry : filesystem::recursive_directory_iterator(wikiPath)) {
        if (entry.path().extension() != ".html") continue;

        ifstream file(entry.path());
        if (!file.is_open()) {
            cout << "Warning: cannot open file: " << entry.path().string() << endl;
            continue;
        }
        stringstream buffer;
        buffer << file.rdbuf();
        string content = buffer.str();
        file.close();

        string cleanText = extractCleanText(content);
        if (cleanText.empty()) {
            cout << "Warning: no valid text in: " << entry.path().string() << endl;
            continue;
        }

        if (sqlite3_exec(db, "BEGIN;", nullptr, 0, &errMsg) != SQLITE_OK) {
            cout << "Error: failed to begin transaction: " << errMsg << endl;
            sqlite3_free(errMsg);
            continue;
        }

        string relPath = filesystem::relative(entry.path(), wwwPath).string();
        if (sqlite3_bind_text(insertDoc, 1, relPath.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
            sqlite3_bind_text(insertDoc, 2, cleanText.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
            sqlite3_step(insertDoc) != SQLITE_DONE) {
            cout << "Error: failed to insert document: " << sqlite3_errmsg(db) << endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
            sqlite3_reset(insertDoc);
            continue;
        }
        sqlite3_reset(insertDoc);

        if (sqlite3_exec(db, "COMMIT;", nullptr, 0, &errMsg) != SQLITE_OK) {
            cout << "Error: failed to commit transaction: " << errMsg << endl;
            sqlite3_free(errMsg);
            sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
            continue;
        }
    }

    // Step 6: Search in FTS5 table (example query)
    cout << "Performing example search..." << endl;
    string searchTerm = "evolucion"; // Example term for testing
    string query = "SELECT path FROM search_index WHERE content MATCH ?;";
    sqlite3_stmt* searchStmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &searchStmt, nullptr) != SQLITE_OK) {
        cout << "Error: failed to prepare search statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(insertDoc);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(searchStmt, 1, searchTerm.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
        sqlite3_exec(db, "BEGIN;", nullptr, 0, &errMsg) != SQLITE_OK) {
        cout << "Error: failed to initiate search: " << (errMsg ? errMsg : sqlite3_errmsg(db)) << endl;
        sqlite3_free(errMsg);
        sqlite3_finalize(searchStmt);
        sqlite3_finalize(insertDoc);
        sqlite3_close(db);
        return 1;
    }

    cout << "Search results for '" << searchTerm << "':" << endl;
    while (sqlite3_step(searchStmt) == SQLITE_ROW) {
        const char* path = reinterpret_cast<const char*>(sqlite3_column_text(searchStmt, 0));
        cout << "Found in: " << (path ? path : "NULL") << endl;
    }
    sqlite3_finalize(searchStmt);

    if (sqlite3_exec(db, "COMMIT;", nullptr, 0, &errMsg) != SQLITE_OK) {
        cout << "Error: failed to commit search transaction: " << errMsg << endl;
        sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
    }

    // Step 7: Finalize and close database
    cout << "Finalizing statements..." << endl;
    sqlite3_finalize(insertDoc);

    cout << "Closing database..." << endl;
    if (sqlite3_close(db) != SQLITE_OK) {
        cout << "Error: failed to close database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }
    cout << "Database closed successfully" << endl;

    return 0;
}
