#include <Embedding.h>
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
            model_path = (repoPath / "my_custom_fasttext_model_finetuned.bin").string();
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
    LoadModel();
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
        throw std::runtime_error("Zero magnitude vector, cannot calculate cosine similarity.");
    }
    return dot / (magA * magB);
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
