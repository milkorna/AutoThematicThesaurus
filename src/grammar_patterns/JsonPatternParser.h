#pragma once

#include "GrammarPatternManager.h"
#include "ModelComponent.h"

#include <memory>
#include <string>
#include <unordered_map>

#include <filesystem>
#include <nlohmann/json.hpp>

/**
 * @class JsonPatternParser
 * @brief Parses grammar patterns from JSON format and builds model objects.
 * @details Supports JSON file and direct JSON array input. Handles pattern
 *          references, cycle detection, and comprehensive validation.
 */
class JsonPatternParser {
  public:
    /**
     * @brief Constructs parser from JSON file.
     *
     * @param filePath Path to JSON file containing pattern array.
     * @throws std::runtime_error If file cannot be opened or is invalid.
     */
    explicit JsonPatternParser(const fs::path& filePath);

    /**
     * @brief Constructs parser from JSON array.
     *
     * @param arr JSON array of pattern objects.
     * @throws std::runtime_error If input is not an array.
     */
    explicit JsonPatternParser(const nlohmann::json& arr);

    /**
     * @brief Parses and registers all enabled patterns in manager.
     * @details Builds models with cycle detection and memoization.
     *          On failure, clears manager and rethrows exception.
     *
     * @throws std::runtime_error If parsing fails for any pattern.
     */
    void parseAll();

  private:
    std::unordered_map<std::string, nlohmann::json> rawPatterns_; ///< Raw JSON patterns by name.
    PatternMap built_;                                            ///< Built models cache (memoization).
    StringSet visiting_;                                          ///< Currently visiting patterns (cycle detection).

    /**
     * @brief Builds a model by name with memoization and cycle detection.
     *
     * @param name The pattern name to build.
     * @return Shared pointer to the built model.
     * @throws std::runtime_error On unknown pattern, cyclic dependency, or invalid structure.
     */
    std::shared_ptr<Model> buildModel(const std::string& name);

    /**
     * @brief Builds components from JSON body array.
     *
     * @param body JSON array of component objects.
     * @param ownerName Pattern name for error context.
     * @return Vector of built components.
     * @throws std::runtime_error On invalid structure or unknown type.
     */
    Components buildComponents(const nlohmann::json& body, const std::string& ownerName);

    /**
     * @brief Builds a word component from JSON.
     *
     * @param item JSON object with type="word".
     * @return Shared pointer to the word component.
     * @throws std::runtime_error On missing or invalid fields.
     */
    std::shared_ptr<WordComp> buildWordComp(const nlohmann::json& item);

    /**
     * @brief Builds a pattern (model) component from JSON.
     *
     * @param item JSON object with type="pattern".
     * @return Shared pointer to the pattern component.
     * @throws std::runtime_error On missing or invalid fields.
     */
    std::shared_ptr<ModelComp> buildPatternComp(const nlohmann::json& item);
};
