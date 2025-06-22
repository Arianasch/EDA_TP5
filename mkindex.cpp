/**
 * @file mkindex.cpp
 * @author Marc S. Ressl
 * @brief Makes a database index
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <sqlite3.h>
#include "CommandLineParser.h"

using namespace std;

// Callback for debugging database entries
static int onDatabaseEntry(void *userdata, int argc, char **argv, char **azColName) {
    cout << "--- Entry" << endl;
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    }
    return 0;
}

// Extract words from HTML content
set<string> extractWords(const string& content) {
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

int main(int argc, const char *argv[]) {
    // Step 2: Parse command-line arguments
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

    // Step 3: Open database
    sqlite3 *db;
    char *errMsg = nullptr;
    cout << "Opening database..." << endl;
    if (sqlite3_open("index.db", &db) != SQLITE_OK) {
        cout << "Error: cannot open database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }
    cout << "Database opened successfully" << endl;

    // Step 4: Create tables
    cout << "Creating tables..." << endl;
    if (sqlite3_exec(db, "BEGIN; "
                         "DROP TABLE IF EXISTS word_document; "
                         "DROP TABLE IF EXISTS words; "
                         "DROP TABLE IF EXISTS documents; "
                         "CREATE TABLE documents (doc_id INTEGER PRIMARY KEY, path VARCHAR NOT NULL); "
                         "CREATE TABLE words (word_id INTEGER PRIMARY KEY, word VARCHAR NOT NULL UNIQUE); "
                         "CREATE TABLE word_document (word_id INTEGER, doc_id INTEGER, "
                         "PRIMARY KEY (word_id, doc_id), FOREIGN KEY (word_id) REFERENCES words(word_id), "
                         "FOREIGN KEY (doc_id) REFERENCES documents(doc_id)); "
                         "CREATE INDEX idx_word ON words(word); "
                         "COMMIT;", nullptr, 0, &errMsg) != SQLITE_OK) {
        cout << "Error: failed to create tables: " << errMsg << endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }
    cout << "Tables created successfully" << endl;

    // Step 5: Process HTML files
    cout << "Processing HTML files..." << endl;
    string wikiPath = (filesystem::path(wwwPath) / "wiki").string();
    if (!filesystem::exists(wikiPath)) {
        cout << "Error: wiki directory does not exist: " << wikiPath << endl;
        sqlite3_close(db);
        return 1;
    }

    // Step 6: Save to database
    cout << "Indexing files..." << endl;
    sqlite3_stmt *insertDoc, *insertWord, *selectWord, *insertWordDoc;
    if (sqlite3_prepare_v2(db, "INSERT INTO documents (path) VALUES (?);", -1, &insertDoc, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO words (word) VALUES (?);", -1, &insertWord, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "SELECT word_id FROM words WHERE word = ?;", -1, &selectWord, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "INSERT INTO word_document (word_id, doc_id) VALUES (?, ?);", -1, &insertWordDoc, nullptr) != SQLITE_OK) {
        cout << "Error: failed to prepare statements: " << sqlite3_errmsg(db) << endl;
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

        set<string> words = extractWords(content);
        if (words.empty()) {
            cout << "Warning: no valid words in: " << entry.path().string() << endl;
            continue;
        }

        if (sqlite3_exec(db, "BEGIN;", nullptr, 0, &errMsg) != SQLITE_OK) {
            cout << "Error: failed to begin transaction: " << errMsg << endl;
            sqlite3_free(errMsg);
            continue;
        }

        string relPath = filesystem::relative(entry.path(), wwwPath).string();
        if (sqlite3_bind_text(insertDoc, 1, relPath.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
            sqlite3_step(insertDoc) != SQLITE_DONE) {
            cout << "Error: failed to insert document: " << sqlite3_errmsg(db) << endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
            sqlite3_reset(insertDoc);
            continue;
        }
        sqlite_int64 docId = sqlite3_last_insert_rowid(db);
        sqlite3_reset(insertDoc);

        for (const string& word : words) {
            if (sqlite3_bind_text(insertWord, 1, word.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
                sqlite3_step(insertWord) != SQLITE_DONE ||
                sqlite3_bind_text(selectWord, 1, word.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
                cout << "Error: failed to process word: " << sqlite3_errmsg(db) << endl;
                sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
                break;
            }
            sqlite3_reset(insertWord);

            sqlite_int64 wordId = -1;
            if (sqlite3_step(selectWord) == SQLITE_ROW) {
                wordId = sqlite3_column_int64(selectWord, 0);
            }
            sqlite3_reset(selectWord);
            if (wordId == -1) {
                cout << "Error: cannot retrieve word_id for: " << word << endl;
                sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
                break;
            }

            if (sqlite3_bind_int64(insertWordDoc, 1, wordId) != SQLITE_OK ||
                sqlite3_bind_int64(insertWordDoc, 2, docId) != SQLITE_OK ||
                sqlite3_step(insertWordDoc) != SQLITE_DONE) {
                cout << "Error: failed to insert word_document: " << sqlite3_errmsg(db) << endl;
                sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
                break;
            }
            sqlite3_reset(insertWordDoc);
        }

        if (sqlite3_exec(db, "COMMIT;", nullptr, 0, &errMsg) != SQLITE_OK) {
            cout << "Error: failed to commit transaction: " << errMsg << endl;
            sqlite3_free(errMsg);
            sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
            continue;
        }
    }

    // Step 7: Finalize and close database
    cout << "Finalizing statements..." << endl;
    sqlite3_finalize(insertDoc);
    sqlite3_finalize(insertWord);
    sqlite3_finalize(selectWord);
    sqlite3_finalize(insertWordDoc);

    cout << "Closing database..." << endl;
    if (sqlite3_close(db) != SQLITE_OK) {
        cout << "Error: failed to close database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }
    cout << "Database closed successfully" << endl;

    return 0;
}
