#ifndef EMBEDDING_H
#define EMBEDDING_H

#include <cmath>
#include <fasttext.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class Embedding {
private:
    static std::unique_ptr<fasttext::FastText> ft;
    static void LoadModel(std::string model_path);

public:
    Embedding();

    static std::vector<float> GetWordVector(const std::string& word);

    static void RunTest();
};

class WordEmbedding {
private:
    std::vector<float> vector;

public:
    WordEmbedding(const std::string& word);

    const std::vector<float>& GetVector() const
    {
        return vector;
    }

    float CosineSimilarity(const WordEmbedding& other) const;

    float EuclideanDistance(const WordEmbedding& other) const;

    float ManhattanDistance(const WordEmbedding& other) const;

    float JaccardSimilarity(const WordEmbedding& other) const;

    float Magnitude() const;

    float DotProduct(const WordEmbedding& other) const;

    friend std::ostream& operator<<(std::ostream& os, const WordEmbedding& we);
};

using WordEmbeddingPtr = std::shared_ptr<WordEmbedding>;

#endif // EMBEDDING_H
