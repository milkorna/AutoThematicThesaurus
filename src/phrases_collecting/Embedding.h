#pragma once

#include <cmath>
#include <fasttext.h>
#include <memory>
#include <string>
#include <vector>

class Embedding {
  private:
    static std::unique_ptr<fasttext::FastText> ft;
    static void LoadModel(std::string model_path);

  public:
    Embedding();

    static std::vector<double> GetWordVector(const std::string& word);

    static void RunTest();
};

class WordEmbedding {
  private:
    std::vector<double> vector;

  public:
    WordEmbedding(const std::string& word);

    const std::vector<double>& GetVector() const {
        return vector;
    }

    double CosineSimilarity(const WordEmbedding& other) const;

    double EuclideanDistance(const WordEmbedding& other) const;

    double ManhattanDistance(const WordEmbedding& other) const;

    double JaccardSimilarity(const WordEmbedding& other) const;

    double Magnitude() const;

    double DotProduct(const WordEmbedding& other) const;

    friend std::ostream& operator<<(std::ostream& os, const WordEmbedding& we);
};

using WordEmbeddingPtr = std::shared_ptr<WordEmbedding>;
