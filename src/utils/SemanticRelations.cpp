#include <SemanticRelations.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

SemanticRelationsDB::SemanticRelationsDB(const std::string& db_name)
{
    if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Could not open database");
    }
}

sqlite3* SemanticRelationsDB::getDB() const
{
    return db;
}

SemanticRelationsDB::~SemanticRelationsDB()
{
    sqlite3_close(db);
}

std::set<std::string> SemanticRelationsDB::GetRelations(const std::string& word, const std::string& relation_type) const
{
    std::set<std::string> relations;
    std::string sql;

    if (relation_type == "hypernym") {
        sql = "SELECT s2.lemma FROM synsets s1 "
              "JOIN hypernyms h ON s1.synset_id = h.sid "
              "JOIN synsets s2 ON h.hypersid = s2.synset_id "
              "WHERE s1.lemma = ?";
    } else if (relation_type == "hyponym") {
        sql = "SELECT s2.lemma FROM synsets s1 "
              "JOIN hypernyms h ON s1.synset_id = h.hypersid "
              "JOIN synsets s2 ON h.sid = s2.synset_id "
              "WHERE s1.lemma = ?";
    } else {
        throw std::invalid_argument("Unknown relation type: " + relation_type);
    }

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Could not prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Could not bind parameters: " + std::string(sqlite3_errmsg(db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* related_word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (related_word) {
            relations.insert(related_word);
        }
    }

    sqlite3_finalize(stmt);
    return relations;
}

void SemanticRelationsDB::PrintAllTables()
{
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table';";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Could not prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* table_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::cout << (table_name ? table_name : "NULL") << std::endl;
    }

    sqlite3_finalize(stmt);
}

void SemanticRelationsDB::PrintFirstFiveRows(const std::string& table_name)
{
    std::string sql = "SELECT * FROM " + table_name + " LIMIT 5;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Could not prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    int col_count = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < col_count; ++col) {
            const char* col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            std::cout << (col_text ? col_text : "NULL") << " ";
        }
        std::cout << std::endl;
    }

    sqlite3_finalize(stmt);
}

void SemanticRelationsDB::PrintTableSchema(const std::string& table_name)
{
    std::string sql = "PRAGMA table_info(" + table_name + ");";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Could not prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < sqlite3_column_count(stmt); ++col) {
            const char* col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            std::cout << (col_text ? col_text : "NULL") << " ";
        }
        std::cout << std::endl;
    }

    sqlite3_finalize(stmt);
}

void DB::RunTest()
{
    try {
        fs::path repoPath = fs::current_path();
        std::string semantic_data = (repoPath / "wikiwordnet.db").string();
        SemanticRelationsDB db(semantic_data);

        std::cout << "Tables in the database:" << std::endl;
        db.PrintAllTables();
        std::cout << std::endl;

        std::cout << "Schema of table hypernyms:" << std::endl;
        db.PrintTableSchema("hypernyms");
        std::cout << std::endl;

        std::cout << "Schema of table synsets:" << std::endl;
        db.PrintTableSchema("synsets");
        std::cout << std::endl;

        std::string word = "медведь";
        auto hypernyms = db.GetRelations(word, "hypernym");
        auto hyponyms = db.GetRelations(word, "hyponym");

        std::cout << "Hypernyms of " << word << ": ";
        for (const auto& syn : hypernyms) {
            std::cout << syn << " ";
        }
        std::cout << std::endl;

        std::cout << "Hyponyms of " << word << ": ";
        for (const auto& ant : hyponyms) {
            std::cout << ant << " ";
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}