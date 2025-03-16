#ifndef LSA_H
#define LSA_H

#include <Eigen/Dense>
#include <TokenizedSentenceCorpus.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Eigen;

// Configuration structure for LSA metric calculations
struct LSA_MetricsConfig {
    // Determines whether cosine similarity is used for centrality calculation.
    // - If true, cosine similarity is applied.
    // - If false, Euclidean distance is used and converted into a score.
    bool useCosineForCentrality = true;

    // Defines the method for calculating topic relevance.
    // - If true, considers the dominant peak in the coordinate vector
    //   (computed as max^2 / sum^2 ratio).
    // - If false, alternative methods such as lemma occurrences in top words can be used.
    bool useVectorRatioForTopicRelevance = true;

    // Specifies the number of latent components to consider.
    // - If set to std::nullopt, the full matrix width U.cols() is used.
    // - Otherwise, the matrix is truncated to the specified number of components.
    std::optional<int> maxComponents = std::nullopt;

    // Determines whether vectors should be scaled by singular values (Sigma).
    // - If true, scaling is applied to account for the actual "importance" of each topic.
    // - If false, no scaling is applied.
    bool applySigmaScaling = false;
};

class LSA {
public:
    // Constructor
    LSA(const TokenizedSentenceCorpus& corpus) : corpus(corpus)
    {
    }

    // Main method to perform LSA analysis
    void PerformAnalysis(bool useSentences);

    // Method to create the term-document frequency matrix
    std::pair<MatrixXd, std::vector<std::string>> CreateTermDocumentMatrix(bool useSentences);

    // Method to compute Singular Value Decomposition (SVD) of the matrix
    void ComputeSVD(const MatrixXd& termDocumentMatrix);

    // Methods to get the SVD results
    MatrixXd GetU() const
    {
        return U;
    }
    MatrixXd GetSigma() const
    {
        return Sigma;
    }
    MatrixXd GetV() const
    {
        return V;
    }

    // Method to get the list of words
    std::vector<std::string> GetWords() const
    {
        return words;
    }

    const std::unordered_map<int, std::vector<std::string>>& GetTopics() const;

    void AnalyzeTopics(int numTopics = 5, int topWords = 10);

    static double CosineSimilarity(const VectorXd& vec1, const VectorXd& vec2);

    static void CompareDocuments(const MatrixXd& V);

    void FindSimilarWords(const std::string& targetWord);

private:
    const TokenizedSentenceCorpus& corpus; // Reference to the TokenizedSentenceCorpus object containing data
    std::unordered_map<int, std::vector<std::string>> topics;
    MatrixXd U;                     // Left singular matrix
    MatrixXd Sigma;                 // Singular value matrix
    MatrixXd V;                     // Right singular matrix
    std::vector<std::string> words; // List of words used in the term-document matrix
    std::unordered_set<std::string> LSAStopWords = {"мочь", "для", "или", "при", "стр"};
};

#endif // LSA_H
