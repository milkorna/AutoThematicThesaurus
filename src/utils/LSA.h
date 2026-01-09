#pragma once

#include "Eigen/Dense"
#include "SentenceCorpus.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Eigen;

/**
 * @brief Configuration parameters for LSA metric calculations
 * @details Controls behavior of topic relevance and centrality computations
 */
struct LSA_MetricsConfig {
    /**
     * @brief Selects centrality calculation method
     * @details
     * - true: Use cosine similarity between word vectors
     * - false: Use Euclidean distance converted to centrality score
     */
    bool useCosineForCentrality = true;

    /**
     * @brief Selects topic relevance calculation method
     * @details
     * - true: Use vector concentration ratio (max^2 / sum^2)
     *         measures how much a word clusters into single topics
     * - false: Use alternative methods (e.g., lemma occurrence in top words)
     */
    bool useVectorRatioForTopicRelevance = true;

    /**
     * @brief Maximum latent components to retain from SVD
     * @details
     * - std::nullopt: Use full matrix width (U.cols())
     * - Otherwise: Truncate to specified number of components
     */
    std::optional<int> maxComponents = std::nullopt;

    /**
     * @brief Applies sigma scaling to topic vectors
     * @details
     * - true: Scale vectors by singular values to weight by topic importance
     * - false: Use unscaled vectors
     */
    bool applySigmaScaling = false;
};

/**
 * @brief Latent Semantic Analysis (LSA) for text corpus decomposition
 * @details Performs SVD on term-document matrix to extract latent semantic topics.
 * Supports analysis at document or sentence level. Computes topic assignments,
 * word similarities, and document comparisons.
 */
class LSA {
  public:
    /**
     * @brief Constructs LSA analyzer for given corpus
     *
     * @param corpus Reference to SentenceCorpus containing source data
     */
    LSA(const SentenceCorpus& corpus) : corpus(corpus) {
    }

    /**
     * @brief Executes complete LSA analysis pipeline
     * @details Creates term-document matrix, computes SVD, and prepares matrices for metrics
     *
     * @param useSentences If true, treats sentences as documents;
     *                     if false, treats entire documents as units
     */
    void PerformAnalysis(bool useSentences);

    /**
     * @brief Builds term-document frequency matrix
     * @details Tokenizes texts, filters stop words and rare words (freq > 1, length > 5).
     * Excludes punctuation, non-Cyrillic characters, and specified stop words.
     *
     * @param useSentences If true, treats each sentence as separate document;
     *                     if false, combines sentences per document
     * @return Pair of (frequency matrix, list of included words)
     */
    [[nodiscard]] std::pair<MatrixXd, std::vector<std::string>> CreateTermDocumentMatrix(bool useSentences);

    /**
     * @brief Computes Singular Value Decomposition of term-document matrix
     * @details Performs thin SVD, retains top 100 components (or available).
     * Stores U (term vectors), Sigma (singular values), V (document vectors).
     * Includes error handling and progress logging.
     *
     * @param termDocumentMatrix Input matrix (terms × documents)
     */
    void ComputeSVD(const MatrixXd& termDocumentMatrix);

    /**
     * @brief Retrieves left singular vectors (term vectors)
     *
     * @return U matrix (terms × components)
     */
    [[nodiscard]] MatrixXd GetU() const {
        return U;
    }

    /**
     * @brief Retrieves singular values matrix
     *
     * @return Sigma matrix (diagonal)
     */
    [[nodiscard]] MatrixXd GetSigma() const {
        return Sigma;
    }

    /**
     * @brief Retrieves right singular vectors (document vectors)
     *
     * @return V matrix (documents × components)
     */
    [[nodiscard]] MatrixXd GetV() const {
        return V;
    }

    /**
     * @brief Retrieves vocabulary used in term-document matrix
     *
     * @return Vector of word strings indexed by SVD row index
     */
    [[nodiscard]] std::vector<std::string> GetWords() const {
        return words;
    }

    /**
     * @brief Retrieves extracted topics and their constituent words
     * @details Maps topic index to vector of top words for that topic
     *
     * @return Const reference to topics map
     */
    [[nodiscard]] const std::unordered_map<int, std::vector<std::string>>& GetTopics() const;

    /**
     * @brief Identifies top words for each latent topic
     * @details Analyzes U matrix columns to find words with highest loadings.
     * Stores results in topics map.
     *
     * @param numTopics Number of topics to extract (default: 5)
     * @param topWords Number of top words to extract per topic (default: 10)
     */
    void AnalyzeTopics(int numTopics = 5, int topWords = 10);

    /**
     * @brief Computes cosine similarity between two vectors
     * @details Normalized dot product: v1·v2 / (||v1|| * ||v2||)
     *
     * @param vec1 First vector
     * @param vec2 Second vector
     * @return Similarity score [-1, 1]
     */
    [[nodiscard]] static double CosineSimilarity(const VectorXd& vec1, const VectorXd& vec2);

    /**
     * @brief Computes pairwise document similarities and writes to file
     * @details Calculates cosine similarity between all document pairs.
     * Output file: "document_similarity.txt"
     *
     * @param V Right singular vectors (document vectors from SVD)
     */
    static void CompareDocuments(const MatrixXd& V);

    /**
     * @brief Finds words semantically similar to target word
     * @details Computes cosine similarity in LSA space between target word vector
     * and all other word vectors. Outputs top 10 similar words to stdout.
     *
     * @param targetWord Word to find similar words for
     */
    void FindSimilarWords(const std::string& targetWord);

  private:
    /// @brief Reference to source corpus data
    const SentenceCorpus& corpus;

    /// @brief Topic assignments: topic ID → vector of top words
    std::unordered_map<int, std::vector<std::string>> topics;

    /// @brief Left singular vectors (term × component)
    MatrixXd U;

    /// @brief Singular values matrix (diagonal)
    MatrixXd Sigma;

    /// @brief Right singular vectors (document × component)
    MatrixXd V;

    /// @brief Vocabulary: words indexed by SVD row
    std::vector<std::string> words;

    /// @brief LSA-specific stop words (Russian)
    std::unordered_set<std::string> LSAStopWords = {"мочь", "для", "или", "при", "стр"};
};