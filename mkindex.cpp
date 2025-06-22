/**
 * @file mkindex.cpp
 * @author Marc S. Ressl
 * @brief Makes a database index
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */
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

using namespace std;

// Callback para mostrar entradas de la base de datos (opcional, para depuración)
static int onDatabaseEntry(void* userdata, int argc, char** argv, char** azColName) {
    cout << "--- Entry" << endl;
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    }
    return 0;
}

// Clase simulada para CommandLineParser (ajustá según tu implementación real)
class CommandLineParser {
    int argc;
    const char** argv;
public:
    CommandLineParser(int c, const char* v[]) : argc(c), argv(v) {}
    bool hasOption(const string& opt) {
        for (int i = 1; i < argc; ++i) {
            if (string(argv[i]) == opt) return true;
        }
        return false;
    }
    string getOption(const string& opt) {
        for (int i = 1; i < argc; ++i) {
            if (string(argv[i]) == opt && i + 1 < argc) {
                return string(argv[i + 1]);
            }
        }
        return "";
    }
};

// Extraer texto limpio de contenido HTML (modificado para FTS5)

std::string extractCleanText(const std::string& content) {
    // Quitar etiquetas HTML, comentarios y scripts
    std::string clean = std::regex_replace(content, std::regex("<!--.*?-->|<script.*?</script>|</?[^>]+>", std::regex::icase), "");

    // Convertir a minúsculas
    std::transform(clean.begin(), clean.end(), clean.begin(), ::tolower);

    // Quitar puntuación y normalizar espacios
    clean = std::regex_replace(clean, std::regex("[^a-z\\s]+"), "");
    clean = std::regex_replace(clean, std::regex("\\s+"), " ");

    // Quitar espacios iniciales o finales
    clean = std::regex_replace(clean, std::regex("^\\s+|\\s+$"), "");

    return clean.empty() ? "" : clean;
}

int main(int argc, const char* argv[]) {
    // Step 2: Parsear argumentos de línea de comandos
    CommandLineParser parser(argc, argv);
    if (!parser.hasOption("-h")) {
        cout << "Error: debe especificar la ruta con -h" << endl;
        return 1;
    }
    string wwwPath = parser.getOption("-h");
    if (wwwPath.empty() || !filesystem::exists(wwwPath) || !filesystem::is_directory(wwwPath)) {
        cout << "Error: ruta inválida: " << wwwPath << endl;
        return 1;
    }
    cout << "Ruta válida: " << wwwPath << endl;

    // Step 3: Abrir base de datos
    sqlite3* db;
    char* errMsg = nullptr;
    cout << "Abriendo base de datos..." << endl;
    if (sqlite3_open("index.db", &db) != SQLITE_OK) {
        cout << "Error: no se puede abrir la base de datos: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }
    cout << "Base de datos abierta correctamente" << endl;


    // Step 4: Crear tabla virtual FTS5
    cout << "Creando tabla FTS5..." << endl;
    if (sqlite3_exec(db, "CREATE VIRTUAL TABLE IF NOT EXISTS search_index USING fts5(path, content, tokenize='unicode61');",
        nullptr, 0, &errMsg) != SQLITE_OK) {
        cout << "Error: no se pudo crear la tabla FTS5: " << errMsg << endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }
    cout << "Tabla FTS5 creada correctamente" << endl;

    // Step 5: Procesar archivos HTML
    cout << "Procesando archivos HTML..." << endl;
    string wikiPath = (filesystem::path(wwwPath) / "wiki").string();
    if (!filesystem::exists(wikiPath)) {
        cout << "Error: el directorio wiki no existe: " << wikiPath << endl;
        sqlite3_close(db);
        return 1;
    }

    // Step 6: Guardar en la base de datos
    cout << "Indexando archivos..." << endl;
    sqlite3_stmt* insertDoc;
    if (sqlite3_prepare_v2(db, "INSERT INTO search_index (path, content) VALUES (?, ?);", -1, &insertDoc, nullptr) != SQLITE_OK) {
        cout << "Error: no se pudo preparar la consulta: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }

    for (const auto& entry : filesystem::recursive_directory_iterator(wikiPath)) {
        if (entry.path().extension() != ".html") continue;

        ifstream file(entry.path());
        if (!file.is_open()) {
            cout << "Advertencia: no se puede abrir el archivo: " << entry.path().string() << endl;
            continue;
        }
        stringstream buffer;
        buffer << file.rdbuf();
        string content = buffer.str();
        file.close();

        string cleanText = extractCleanText(content);
        if (cleanText.empty()) {
            cout << "Advertencia: no hay texto válido en: " << entry.path().string() << endl;
            continue;
        }

        if (sqlite3_exec(db, "BEGIN;", nullptr, 0, &errMsg) != SQLITE_OK) {
            cout << "Error: no se pudo iniciar la transacción: " << errMsg << endl;
            sqlite3_free(errMsg);
            continue;
        }

        string relPath = filesystem::relative(entry.path(), wwwPath).string();
        if (sqlite3_bind_text(insertDoc, 1, relPath.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
            sqlite3_bind_text(insertDoc, 2, cleanText.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
            sqlite3_step(insertDoc) != SQLITE_DONE) {
            cout << "Error: no se pudo insertar el documento: " << sqlite3_errmsg(db) << endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
            sqlite3_reset(insertDoc);
            continue;
        }
        sqlite3_reset(insertDoc);

        if (sqlite3_exec(db, "COMMIT;", nullptr, 0, &errMsg) != SQLITE_OK) {
            cout << "Error: no se pudo confirmar la transacción: " << errMsg << endl;
            sqlite3_free(errMsg);
            sqlite3_exec(db, "ROLLBACK;", nullptr, 0, nullptr);
            continue;
        }
    }

    // Step 7: Finalizar y cerrar base de datos
    cout << "Finalizando consultas..." << endl;
    sqlite3_finalize(insertDoc);

    cout << "Cerrando base de datos..." << endl;
    if (sqlite3_close(db) != SQLITE_OK) {
        cout << "Error: no se pudo cerrar la base de datos: " << sqlite3_errmsg(db) << endl;
        return 1;
    }
    cout << "Base de datos cerrada correctamente" << endl;

    return 0;
}
