#ifndef TEXT_CORPUS_H
#define TEXT_CORPUS_H

#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fasttext.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <Eigen/Dense>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace Eigen;
using VectorPtr = std::shared_ptr<fasttext::Vector>;

// \class TextCorpus
// \brief This class handles the processing and storage of a text corpus,
// where each document is represented by a filename, and the texts are paragraphs extracted from these documents.
class TextCorpus {
public:
    // \brief Default constructor for the TextCorpus class.
    TextCorpus() = default;

    // \brief Singleton instance getter for the TextCorpus class.
    static TextCorpus& GetCorpus()
    {
        static TextCorpus corpus;
        return corpus;
    }

    // Adds a text (paragraph) to the corpus under the associated document (filename).
    // Updates the total document count if this is the first text from the document.
    // \param filename The name of the document (file) from which the text is extracted.
    // \param text The paragraph text to be added.
    void AddText(const std::string& filename, const std::string& text);

    std::string ExtractTitleFromFilename(const std::string& filename) const;

    // Updates the frequency count of a specific word (lemma) in the corpus.
    // Increments the count of the word in the `wordFrequency` map and the total word count.
    // \param lemma The word (lemma) to be updated.
    void UpdateWordFrequency(const std::string& lemma);

    // Updates the document frequency of a specific word (lemma).
    // This function increments the count of documents (filenames) that contain the given word.
    // \param lemma The word (lemma) to update document frequency for.
    void UpdateDocumentFrequency(const std::string& lemma);

    // Loads texts (paragraphs) from a file, where each paragraph is extracted and associated with the filename.
    // \param filename The path to the file containing the paragraphs.
    void LoadTextsFromFile(const std::string& filename);

    // Returns the total number of documents in the corpus (unique filenames).
    int GetTotalDocuments() const;

    // Returns the total number of texts (paragraphs) in the corpus.
    int GetTotalTexts() const;

    // Returns the list of all texts (paragraphs) in the corpus.
    const std::unordered_map<std::string, std::vector<std::string>>& GetTexts() const;

    // Returns the frequency of a specific word (lemma) in the corpus.
    // If the word is not found, it returns 0.
    int GetWordFrequency(const std::string& lemma) const;

    // Returns the document frequency of a specific word (lemma).
    // Document frequency refers to the number of documents (filenames) in which the word appears.
    int GetDocumentFrequency(const std::string& word) const;

    // Calculates the Term Frequency (TF) for a specific word (lemma) in the corpus.
    // TF is calculated as the frequency of the word divided by the total number of words in the corpus.
    double CalculateTF(const std::string& lemma) const;

    // Calculates the Inverse Document Frequency (IDF) for a specific word (lemma) in the corpus.
    // IDF is calculated using the formula: log(total_documents / (1 + document_frequency_of_word)).
    double CalculateIDF(const std::string& lemma) const;

    // Calculates the TF-IDF for a specific word (lemma) in the corpus.
    // TF-IDF is the product of Term Frequency (TF) and Inverse Document Frequency (IDF).
    double CalculateTFIDF(const std::string& lemma) const;

    // Serializes the corpus data to JSON format for storage or transmission.
    json Serialize() const;

    // Returns the total number of words (lemmas) in the corpus.
    void Deserialize(const json& j);

    // Returns the total number of words (lemmas) in the corpus.
    int GetTotalWords() const;

    // Returns the list of all texts (paragraphs) in the corpus.
    // const std::vector<std::string>& GetTexts() const;

    // Returns the frequency map of all words (lemmas) in the corpus.
    const std::unordered_map<std::string, int>& GetWordFrequencies() const;

    // Saves the serialized corpus data to a file.
    void SaveCorpusToFile(const std::string& filename);

    // Loads the corpus data from a file, deserializes it, and returns the restored TextCorpus object.
    static TextCorpus LoadCorpusFromFile(const std::string& filename);

private:
    std::unordered_map<std::string, std::vector<std::string>>
        texts; ///< Map to store paragraphs associated with each document (filename).
    std::unordered_map<std::string, int> wordFrequency;     ///< Map to store the frequency of words in the corpus.
    std::unordered_map<std::string, int> documentFrequency; ///< Map to store the document frequency of words.
    int totalWords = 0;                                     ///< Total number of words (lemmas) in the corpus.
    int totalTexts = 0;
    int totalDocuments = 0; ///< Total number of documents (filenames) in the corpus.
};

class LSA {
public:
    // Constructor to perform Singular Value Decomposition (SVD) on the term-document matrix.
    LSA(const MatrixXd& termDocumentMatrix)
    {
        JacobiSVD<MatrixXd> svd(termDocumentMatrix, ComputeThinU | ComputeThinV);
        U = svd.matrixU();
        V = svd.matrixV();
        S = svd.singularValues();
    }

    // Constructor to perform Singular Value Decomposition (SVD) on the term-document matrix.
    MatrixXd getTermMatrix(int k) const
    {
        return U.leftCols(k) * S.head(k).asDiagonal();
    }

    // Returns the document matrix after dimensionality reduction.
    MatrixXd getDocumentMatrix(int k) const
    {
        return S.head(k).asDiagonal() * V.leftCols(k).transpose();
    }

private:
    MatrixXd U; // Matrix of terms
    MatrixXd V; // Matrix of documents
    VectorXd S; // Vector of singular values
};

// Builds a term-document matrix from the TextCorpus.
// Each row of the matrix corresponds to a term, and each column corresponds to a document (filename).
// The values in the matrix represent the frequency of each term in the respective document.
MatrixXd BuildTermDocumentMatrix(const TextCorpus& corpus);

#endif // TEXT_CORPUS_H
