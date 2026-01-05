#pragma once

#include <string>
#include <unordered_set>

/**
 * @class StopWordsManager
 * @brief Manages a collection of stop words used for text filtering.
 * @details Implements Singleton pattern to ensure single instance throughout application.
 *          Stop words are loaded from file and used to filter out common words.
 */
class StopWordsManager {
  public:
    /**
     * @brief Deleted copy constructor.
     */
    StopWordsManager(const StopWordsManager&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     */
    StopWordsManager& operator=(const StopWordsManager&) = delete;

    /**
     * @brief Gets the singleton instance of StopWordsManager.
     *
     * @return Reference to the static StopWordsManager instance.
     */
    static StopWordsManager& getInstance() {
        static StopWordsManager instance;
        return instance;
    }

    /**
     * @brief Gets the set of stop words.
     *
     * @return Constant reference to the unordered set of stop words.
     */
    [[nodiscard]] static const std::unordered_set<std::string>& getStopWords();

    /**
     * @brief Checks if a word is a stop word.
     *
     * @param word The word to check.
     * @return True if the word is a stop word, false otherwise.
     */
    [[nodiscard]] static bool isStopWord(const std::string& word);

  private:
    /**
     * @brief Private constructor for Singleton pattern.
     */
    StopWordsManager() = default;

    /**
     * @brief Loads stop words from file.
     */
    void loadStopWords();

  private:
    std::unordered_set<std::string> stopWords;
};
