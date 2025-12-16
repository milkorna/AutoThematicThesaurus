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

/**
 * @class GrammarPatternManager
 * @brief Manages grammar patterns and their metadata.
 * @details Implements Singleton pattern. Stores simple and complex patterns,
 *          categorizes them, and tracks used parts of speech.
 */
class GrammarPatternManager {
  public:
    /**
     * @brief Gets the singleton instance.
     *
     * @return Reference to the static GrammarPatternManager instance.
     */
    static GrammarPatternManager& GetManager() {
        static GrammarPatternManager manager;
        return manager;
    }

    /**
     * @brief Deleted copy constructor.
     */
    GrammarPatternManager(const GrammarPatternManager&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     */
    GrammarPatternManager& operator=(const GrammarPatternManager&) = delete;

    /**
     * @brief Checks if a pattern exists by key.
     *
     * @param key The pattern key.
     * @return True if pattern exists, false otherwise.
     */
    [[nodiscard]] bool has(const std::string& key) const noexcept;

    /**
     * @brief Adds a pattern to the manager.
     *
     * @param key The pattern key.
     * @param model The model to associate with the key.
     */
    void add(const std::string& key, const std::shared_ptr<Model>& model) noexcept;

    /**
     * @brief Retrieves a pattern by key.
     *
     * @param key The pattern key.
     * @return Shared pointer to the model or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<Model> get(const std::string& key) const noexcept;

    /**
     * @brief Gets all simple patterns.
     *
     * @return Constant reference to the simple patterns map.
     */
    [[nodiscard]] const PatternMap& getSimplePatterns() const noexcept;

    /**
     * @brief Gets all complex patterns.
     *
     * @return Constant reference to the complex patterns map.
     */
    [[nodiscard]] const PatternMap& getComplexPatterns() const noexcept;

    /**
     * @brief Parses patterns from a JSON file.
     *
     * @param filename Path to the patterns file.
     */
    void readPatterns(const fs::path& filename);

    /**
     * @brief Logs all patterns to logger.
     */
    void printPatterns() const;

    /**
     * @brief Records a used part of speech.
     *
     * @param sp The part of speech string.
     * @param isHead Whether this is a head part of speech.
     */
    void addUsedSp(const std::string sp, const bool isHead);

    /**
     * @brief Gets all used head parts of speech.
     *
     * @return Constant reference to the set of head parts of speech.
     */
    [[nodiscard]] const StringSet& getUsedHeadSp() const noexcept;

    /**
     * @brief Gets all used parts of speech.
     *
     * @return Constant reference to the set of parts of speech.
     */
    [[nodiscard]] const StringSet& getUsedSp() const noexcept;

    /**
     * @brief Gets total pattern count.
     *
     * @return Number of patterns.
     */
    [[nodiscard]] const size_t patternsSize() const noexcept;

    /**
     * @brief Gets simple pattern count.
     *
     * @return Number of simple patterns.
     */
    [[nodiscard]] const size_t simplePatternsSize() const noexcept;

    /**
     * @brief Gets complex pattern count.
     *
     * @return Number of complex patterns.
     */
    [[nodiscard]] const size_t complexPatternsSize() const noexcept;

    /**
     * @brief Divides patterns into simple and complex categories.
     * @details Categorizes based on whether patterns contain nested models.
     */
    void divide();

    /**
     * @brief Clears all stored patterns and metadata.
     */
    void clear();

  private:
    PatternMap simplePatterns;  ///< Patterns without nested models.
    PatternMap complexPatterns; ///< Patterns containing nested models.
    PatternMap patterns;        ///< All patterns combined.

    StringSet usedHeadSpVars; ///< Used head parts of speech.
    StringSet usedSpVars;     ///< Used parts of speech.

    /**
     * @brief Private constructor for Singleton pattern.
     */
    GrammarPatternManager() = default;
};
