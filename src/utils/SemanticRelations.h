#ifndef SEMANTIC_RELATIONS_H
#define SEMANTIC_RELATIONS_H

#include <set>
#include <sqlite3.h>
#include <string>

struct SemanticRelation {
    std::string word;
    std::string relation_type;
    std::string related_word;
};

class SemanticRelationsDB {
public:
    SemanticRelationsDB();
    ~SemanticRelationsDB();

    sqlite3* getDB() const;

    std::set<std::string> GetRelations(const std::string& word, const std::string& relation_type) const;
    void PrintAllTables();
    void PrintFirstFiveRows(const std::string& table_name);
    void PrintTableSchema(const std::string& table_name);

private:
    sqlite3* db;
};

namespace DB {
    void RunTest();
}

#endif // SEMANTIC_RELATIONS_H
