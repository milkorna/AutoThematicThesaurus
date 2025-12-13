#include <cctype>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

using WordEmbeddingPtr = std::shared_ptr<class WordEmbedding>;

class TopicManager {

  public:
    // Удаляем копирование
    TopicManager(const TopicManager&) = delete;
    TopicManager& operator=(const TopicManager&) = delete;

    // Статический метод для получения единственного экземпляра
    static TopicManager& getInstance() {
        static TopicManager instance;
        return instance;
    }

    // Инициализация при первом вызове
    static const std::unordered_set<std::string>& getTopics();

    static const std::unordered_map<std::string, WordEmbeddingPtr>& getTopicVectors();

  private:
    // Приватный конструктор для Singleton паттерна
    TopicManager() = default;

    // Вспомогательные статические методы
    static std::string trimTrailingDigitsAndSpaces(std::string line);

    static bool isValidTopic(const std::string& line);

    void loadTopics();

  private:
    std::unordered_set<std::string> topics;
    std::unordered_map<std::string, WordEmbeddingPtr> topicVectors;
};
