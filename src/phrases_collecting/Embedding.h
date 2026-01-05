#pragma once

#include <cmath>
#include <fasttext.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Manages FastText embedding model loading and word vector retrieval
 * @details Provides static interface to load pre-trained FastText models and extract word vectors.
 * Uses singleton pattern for model instance to ensure single model in memory.
 */
class Embedding {
  private:
    /// @brief Singleton FastText model instance
    static std::unique_ptr<fasttext::FastText> ft;

    /**
     * @brief Loads FastText model from file
     * @details Initializes the FastText model if not already loaded.
     * Falls back to options configuration if no path provided.
     *
     * @param model_path Path to FastText model file (uses default from options if empty)
     */
    static void loadModel(std::string model_path);

  public:
    /**
     * @brief Initializes the Embedding system
     * @details Loads the FastText model from configured path
     */
    Embedding();

    /**
     * @brief Retrieves word vector from the FastText model
     *
     * @param word Word to get embedding for
     * @return Vector of doubles representing the word embedding
     */
    [[nodiscard]] static std::vector<double> getWordVector(const std::string& word);

    /**
     * @brief Runs diagnostic test of the embedding model
     * @details Outputs model statistics and first few words in dictionary
     */
    static void runTest();
};

/**
 * @brief Represents a single word embedding with distance/similarity metrics
 * @details Encapsulates a FastText word vector and provides various similarity
 * and distance computation methods (Cosine, Euclidean, Manhattan, Jaccard).
 */
class WordEmbedding {
  private:
    /// @brief The embedding vector from FastText model
    std::vector<double> vector;

  public:
    /**
     * @brief Constructs a word embedding from a word string
     * @details Retrieves the word vector from the FastText model
     *
     * @param word The word to create embedding for
     */
    explicit WordEmbedding(const std::string& word);

    /**
     * @brief Gets the embedding vector
     *
     * @return Const reference to the embedding vector
     */
    [[nodiscard]] const std::vector<double>& getVector() const {
        return vector;
    }

    /**
     * @brief Computes cosine similarity with another embedding
     * @details Range: [-1, 1], where 1 means identical direction
     *
     * @param other Another word embedding to compare
     * @return Cosine similarity value
     */
    [[nodiscard]] double cosineSimilarity(const WordEmbedding& other) const;

    /**
     * @brief Computes Euclidean distance to another embedding
     * @details L2 norm distance: sqrt(sum((v1-v2)^2))
     *
     * @param other Another word embedding to compare
     * @return Euclidean distance value (non-negative)
     */
    [[nodiscard]] double euclideanDistance(const WordEmbedding& other) const;

    /**
     * @brief Computes Manhattan distance to another embedding
     * @details L1 norm distance: sum(|v1-v2|)
     *
     * @param other Another word embedding to compare
     * @return Manhattan distance value (non-negative)
     */
    [[nodiscard]] double manhattanDistance(const WordEmbedding& other) const;

    /**
     * @brief Computes Jaccard similarity with another embedding
     * @details Range: [0, 1], based on intersection/union of coordinates
     *
     * @param other Another word embedding to compare
     * @return Jaccard similarity value
     */
    [[nodiscard]] double jaccardSimilarity(const WordEmbedding& other) const;

    /**
     * @brief Computes L2 norm magnitude of the vector
     *
     * @return Magnitude (non-negative)
     */
    [[nodiscard]] double magnitude() const;

    /**
     * @brief Computes dot product with another embedding
     *
     * @param other Another word embedding to compare
     * @return Dot product value
     */
    [[nodiscard]] double dotProduct(const WordEmbedding& other) const;

    /**
     * @brief Outputs the embedding vector to stream
     *
     * @param os Output stream
     * @param we WordEmbedding to output
     * @return Reference to output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const WordEmbedding& we);
};

/// @brief Shared pointer to WordEmbedding for use in collections
using WordEmbeddingPtr = std::shared_ptr<WordEmbedding>;