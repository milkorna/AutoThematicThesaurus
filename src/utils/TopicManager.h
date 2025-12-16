#include <cctype>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

using WordEmbeddingPtr = std::shared_ptr<class WordEmbedding>;

/**
 * @class TopicManager
 * @brief Manages topics and their corresponding word embeddings.
 * @details Implements Singleton pattern to ensure single instance throughout application.
 *          Loads topics from file and provides access to topic vectors for analysis.
 */
class TopicManager {

  public:
    /**
     * @brief Deleted copy constructor.
     */
    TopicManager(const TopicManager&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     */
    TopicManager& operator=(const TopicManager&) = delete;

    /**
     * @brief Gets the singleton instance of TopicManager.
     *
     * @return Reference to the static TopicManager instance.
     */
    static TopicManager& getInstance() {
        static TopicManager instance;
        return instance;
    }

    /**
     * @brief Gets the set of available topics.
     *
     * @return Constant reference to the unordered set of topic strings.
     */
    [[nodiscard]] static const std::unordered_set<std::string>& getTopics();

    /**
     * @brief Gets the map of topic vectors (embeddings).
     *
     * @return Constant reference to the unordered map of topic names to word embeddings.
     */
    [[nodiscard]] static const std::unordered_map<std::string, WordEmbeddingPtr>& getTopicVectors();

  private:
    /**
     * @brief Private constructor for Singleton pattern.
     */
    TopicManager() = default;

    /**
     * @brief Trims trailing digits and spaces from a line.
     *
     * @param line The input line to trim.
     * @return The trimmed line string.
     */
    [[nodiscard]] static std::string trimTrailingDigitsAndSpaces(std::string line);

    /**
     * @brief Validates if a line represents a valid topic.
     *
     * @param line The line to validate.
     * @return True if the line is a valid topic, false otherwise.
     */
    [[nodiscard]] static bool isValidTopic(const std::string& line);

    /**
     * @brief Loads topics from file.
     */
    void loadTopics();

  private:
    std::unordered_set<std::string> topics;
    std::unordered_map<std::string, WordEmbeddingPtr> topicVectors;
};
