#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

// Forward declaration
class Model;

using PatternMap = std::unordered_map<std::string, std::shared_ptr<Model>>;

using StringSet = std::unordered_set<std::string>;

class GrammarPatternManager {
  private:
    static GrammarPatternManager* instance;

    PatternMap simplePatterns;
    PatternMap complexPatterns;

    PatternMap patterns;

    StringSet usedHeadSpVars;
    StringSet usedSpVars;

    // Private constructors for Singleton pattern
    GrammarPatternManager() {};

  public:
    // Singleton access method
    static GrammarPatternManager* GetManager();

    // Deleting copy constructor and assignment operator
    GrammarPatternManager(const GrammarPatternManager&) = delete;
    GrammarPatternManager& operator=(const GrammarPatternManager&) = delete;

    [[nodiscard]] bool has(const std::string& key) const noexcept;

    // Method to add a pattern to the manager
    void add(const std::string& key, const std::shared_ptr<Model>& model) noexcept;

    // Method to retrieve a pattern by key
    [[nodiscard]] std::shared_ptr<Model> get(const std::string& key) const noexcept;

    [[nodiscard]] const PatternMap& getSimplePatterns() const noexcept;
    [[nodiscard]] const PatternMap& getComplexPatterns() const noexcept;

    // Method to parse document strings and create/fill models
    void readPatterns(const fs::path& filename);

    void printPatterns() const;

    void addUsedSp(const std::string sp, const bool isHead);

    [[nodiscard]] const StringSet& getUsedHeadSp() const noexcept;

    [[nodiscard]] const StringSet& getUsedSp() const noexcept;

    [[nodiscard]] size_t patternsSize() const noexcept;

    [[nodiscard]] size_t simplePatternsSize() const noexcept;

    [[nodiscard]] size_t complexPatternsSize() const noexcept;

    void divide();

    void clear();
};
