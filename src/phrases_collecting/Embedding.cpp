#include "Embedding.h"
#include "PhrasesCollectorUtils.h"
#include "Options.h"
#include "Logger.h"

#include <iostream>

std::unique_ptr<fasttext::FastText> Embedding::ft = nullptr;

void Embedding::loadModel(std::string model_path = "") {
    auto& options = Options::getOptions();
    if (!ft) {
        ft = std::make_unique<fasttext::FastText>();
        if (model_path.empty()) {
            model_path = options.embeddingModelFile.string();
        }
        try {
            ft->loadModel(model_path);
        } catch (const std::exception& e) {
            std::cerr << "Exception occurred: " << e.what() << std::endl;
        }
    }
}

Embedding::Embedding() {
    Logger::log("Embedding", LogLevel::Info, "Initializing embedding model...");
    loadModel();
    Logger::log("Embedding", LogLevel::Info, "Model loaded successfully.");
}

void Embedding::runTest() {
    if (!ft) {
        std::cerr << "Model is not loaded. Please load the model before running the test." << std::endl;
        return;
    }

    std::cout << "Listing all words in the model:" << std::endl;
    const auto dict = ft->getDictionary();
    std::cout << std::to_string(dict->nwords()) << std::endl;
    fasttext::Vector vec(ft->getDimension());
    ft->getWordVector(vec, "передовой");
    for (int i = 0; i < 5; ++i) {
        std::cout << dict->getWord(i) << std::endl;
    }
}

std::vector<double> Embedding::getWordVector(const std::string& word) {
    fasttext::Vector vec(ft->getDimension());
    ft->getWordVector(vec, word);
    return std::vector<double>(vec.data(), vec.data() + vec.size());
}

WordEmbedding::WordEmbedding(const std::string& word) {
    vector = Embedding::getWordVector(word);
}

double WordEmbedding::cosineSimilarity(const WordEmbedding& other) const {
    double dot = dotProduct(other);
    double magA = magnitude();
    double magB = other.magnitude();
    if (magA == 0.0 || magB == 0.0) {
        return 0;
    }
    return dot / (magA * magB);
}

double NormalizedLevenshteinDistance(const std::string& s1, const std::string& s2) {
    int len1 = s1.size();
    int len2 = s2.size();
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    for (int i = 0; i <= len1; ++i) {
        for (int j = 0; j <= len2; ++j) {
            if (i == 0) {
                dp[i][j] = j;
            } else if (j == 0) {
                dp[i][j] = i;
            } else {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
            }
        }
    }

    int levenshteinDistance = dp[len1][len2];
    int maxLength = std::max(len1, len2);

    return static_cast<double>(levenshteinDistance) / maxLength;
}

double WordEmbedding::euclideanDistance(const WordEmbedding& other) const {
    double sum = 0.0;
    for (size_t i = 0; i < vector.size(); ++i) {
        double diff = vector[i] - other.vector[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

double WordEmbedding::manhattanDistance(const WordEmbedding& other) const {
    double sum = 0.0;
    for (size_t i = 0; i < vector.size(); ++i) {
        sum += std::abs(vector[i] - other.vector[i]);
    }
    return sum;
}

double WordEmbedding::jaccardSimilarity(const WordEmbedding& other) const {
    double intersection = 0.0;
    double union_set = 0.0;

    for (size_t i = 0; i < vector.size(); ++i) {
        intersection += std::min(vector[i], other.vector[i]);
        union_set += std::max(vector[i], other.vector[i]);
    }

    if (union_set == 0.0) {
        return 0.0;
    }

    return intersection / union_set;
}

double WordEmbedding::magnitude() const {
    double sum = 0.0;
    for (double val : vector) {
        sum += val * val;
    }
    return std::sqrt(sum);
}

double WordEmbedding::dotProduct(const WordEmbedding& other) const {
    double dot = 0.0;
    for (size_t i = 0; i < vector.size(); ++i) {
        dot += vector[i] * other.vector[i];
    }
    return dot;
}

std::ostream& operator<<(std::ostream& os, const WordEmbedding& we) {
    os << "[";
    for (size_t i = 0; i < we.vector.size(); ++i) {
        os << we.vector[i];
        if (i < we.vector.size() - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}
