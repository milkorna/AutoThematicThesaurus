#ifndef TERM_LSA_H
#define TERM_LSA_H

#include <Eigen/Dense>
#include <PatternPhrasesStorage.h> // Include the structure WordComplexCluster
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Eigen;

class TermLSA {
public:
    // Constructor: Initializes TermLSA with a reference to the PatternPhrasesStorage object.
    TermLSA(const PatternPhrasesStorage& storage) : storage(storage)
    {
    }

    // Main method to perform the LSA analysis.
    // numComponents defines the number of components to retain after SVD.
    void PerformAnalysis(int numComponents = 100);

    // Methods to retrieve the matrices after SVD decomposition.
    MatrixXd GetU() const
    {
        return U;
    } // Returns the left singular matrix U.
    MatrixXd GetSigma() const
    {
        return Sigma;
    } // Returns the diagonal matrix of singular values Sigma.
    MatrixXd GetV() const
    {
        return V;
    } // Returns the right singular matrix V.

    // Method to get the list of terms.
    std::vector<std::string> GetTerms() const
    {
        return terms;
    }

    // Method to get the topics extracted from the analysis.
    std::unordered_map<int, std::vector<std::string>> GetTopics() const
    {
        return topics;
    }

    // Method to compute the cosine similarity between two vectors.
    double CosineSimilarity(const VectorXd& vec1, const VectorXd& vec2);

    // Method to find and display terms similar to the specified target term based on cosine similarity.
    void FindSimilarTerms(const std::string& targetTerm);

    // Method to analyze and identify top topics from the term-document matrix.
    // numTopics specifies the number of topics to analyze, topWords specifies the number of top words per topic.
    void AnalyzeTopics(int numTopics = 5, int topWords = 10);

    // Method to calculate the relevance of terms to the identified topics.
    std::unordered_map<std::string, double> CalculateTermRelevanceToTopics();

private:
    const PatternPhrasesStorage& storage; // Reference to the term clusters stored in PatternPhrasesStorage.
    MatrixXd U;                           // Left singular matrix from SVD.
    MatrixXd Sigma;                       // Diagonal matrix of singular values from SVD.
    MatrixXd V;                           // Right singular matrix from SVD.
    std::vector<std::string> terms;       // List of terms used in the analysis.
    std::unordered_map<int, std::vector<std::string>> topics; // Map of topics identified from analysis.
    std::unordered_map<size_t, int> docIndexMap;              // Mapping of document numbers to matrix column indices.

    // Method to create the term-document frequency matrix.
    // Returns a pair consisting of the frequency matrix and the list of terms.
    std::pair<MatrixXd, std::vector<std::string>> CreateTermDocumentMatrix();

    // Method to perform Singular Value Decomposition (SVD) on the term-document matrix.
    // This decomposes the matrix into U, Sigma, and V matrices used for further analysis.
    void ComputeSVD(const MatrixXd& termDocumentMatrix);
};

#endif // TERM_LSA_H
