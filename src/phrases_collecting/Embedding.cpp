#include <Embedding.h>
#include <PhrasesCollectorUtils.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::unique_ptr<fasttext::FastText> Embedding::ft = nullptr;

void Embedding::LoadModel(std::string model_path = "")
{
    if (!ft) {
        ft = std::make_unique<fasttext::FastText>();
        fs::path repoPath = fs::current_path();
        if (model_path.empty()) {
            model_path = PhrasesCollectorUtils::g_options.embeddingModelFile.string();
        }
        try {
            ft->loadModel(model_path);
        } catch (const std::exception& e) {
            std::cerr << "Exception occurred: " << e.what() << std::endl;
        }
    }
}

Embedding::Embedding()
{
    Logger::log("Embedding", LogLevel::Info, "Initializing embedding model...");
    LoadModel();
    Logger::log("Embedding", LogLevel::Info, "Model loaded successfully...");
}

void Embedding::RunTest()
{
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

std::vector<float> Embedding::GetWordVector(const std::string& word)
{
    fasttext::Vector vec(ft->getDimension());
    ft->getWordVector(vec, word);
    return std::vector<float>(vec.data(), vec.data() + vec.size());
}

WordEmbedding::WordEmbedding(const std::string& word)
{
    vector = Embedding::GetWordVector(word);
}

float WordEmbedding::CosineSimilarity(const WordEmbedding& other) const
{
    float dot = DotProduct(other);
    float magA = Magnitude();
    float magB = other.Magnitude();
    if (magA == 0.0f || magB == 0.0f) {
        return 0;
    }
    return dot / (magA * magB);
}

float NormalizedLevenshteinDistance(const std::string& s1, const std::string& s2)
{
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

    return static_cast<float>(levenshteinDistance) / maxLength;
}

float WordEmbedding::EuclideanDistance(const WordEmbedding& other) const
{
    float sum = 0.0f;
    for (size_t i = 0; i < vector.size(); ++i) {
        float diff = vector[i] - other.vector[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float WordEmbedding::ManhattanDistance(const WordEmbedding& other) const
{
    float sum = 0.0f;
    for (size_t i = 0; i < vector.size(); ++i) {
        sum += std::abs(vector[i] - other.vector[i]);
    }
    return sum;
}

float WordEmbedding::JaccardSimilarity(const WordEmbedding& other) const
{
    float intersection = 0.0f;
    float union_set = 0.0f;

    for (size_t i = 0; i < vector.size(); ++i) {
        intersection += std::min(vector[i], other.vector[i]);
        union_set += std::max(vector[i], other.vector[i]);
    }

    if (union_set == 0.0f) {
        return 0.0f;
    }

    return intersection / union_set;
}

float WordEmbedding::Magnitude() const
{
    float sum = 0.0f;
    for (float val : vector) {
        sum += val * val;
    }
    return std::sqrt(sum);
}

float WordEmbedding::DotProduct(const WordEmbedding& other) const
{
    float dot = 0.0f;
    for (size_t i = 0; i < vector.size(); ++i) {
        dot += vector[i] * other.vector[i];
    }
    return dot;
}

std::ostream& operator<<(std::ostream& os, const WordEmbedding& we)
{
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
