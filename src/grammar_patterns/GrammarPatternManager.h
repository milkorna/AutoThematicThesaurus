#ifndef GPAMMAR_PATTERN_MANAGER_H
#define GPAMMAR_PATTERN_MANAGER_H

#include <PatternParser.h>

#include <unordered_map>
#include <unordered_set>

// Forward declaration
class Model;

class GrammarPatternManager {
private:
    static GrammarPatternManager* instance;

    std::unordered_map<std::string, std::shared_ptr<Model>> simplePatterns;
    std::unordered_map<std::string, std::shared_ptr<Model>> complexPatterns;

    std::unordered_map<std::string, std::shared_ptr<Model>> patterns;

    std::unordered_set<std::string> usedHeadSpVars;
    std::unordered_set<std::string> usedSpVars;

    // Private constructors for Singleton pattern
    GrammarPatternManager() {};

public:
    // Singleton access method
    static GrammarPatternManager* GetManager();

    // Deleting copy constructor and assignment operator
    GrammarPatternManager(const GrammarPatternManager&) = delete;
    GrammarPatternManager& operator=(const GrammarPatternManager&) = delete;

    // Method to add a pattern to the manager
    void addPattern(const std::string& key, const std::shared_ptr<Model>& model);

    // Method to retrieve a pattern by key
    std::shared_ptr<Model> getPattern(const std::string& key) const;

    const std::unordered_map<std::string, std::shared_ptr<Model>> getSimplePatterns() const;
    const std::unordered_map<std::string, std::shared_ptr<Model>> getComplexPatterns() const;

    // Method to parse document strings and create/fill models
    void readPatterns(const std::string& filename);

    void printPatterns() const;

    void addUsedSp(const std::string sp, const bool isHead);

    std::unordered_set<std::string> getUsedHeadSp() const;
    std::unordered_set<std::string> getUsedSp() const;

    size_t patternsAmount() const;
    size_t simplePatternsAmount() const;
    size_t complexPatternsAmount() const;

    void divide();
};

#endif // GPAMMAR_PATTERN_MANAGER_H
