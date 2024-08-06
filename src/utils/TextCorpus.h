#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fasttext.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using VectorPtr = std::shared_ptr<fasttext::Vector>;

// \class TextCorpus
// \brief This class handles the processing and storage of a text corpus,
// including document addition, frequency calculation, and FastText vector retrieval.
class TextCorpus {
public:
    // \brief Default constructor for the TextCorpus class.
    TextCorpus() = default;

    // \brief Parameterized constructor to initialize the FastText model.
    // \param modelPath     The path to the FastText model file.
    TextCorpus(const std::string& modelPath);

    // \brief Adds a document to the corpus.
    // \param document      The document text to be added.
    void AddDocument(const std::string& document);

    // \brief Retrieves the FastText vector for a given word.
    // \param word          The word for which to retrieve the vector.
    // \return              A shared pointer to the FastText vector of the word.
    const std::shared_ptr<fasttext::Vector> GetWordVector(const std::string& word) const;

    // \brief Loads documents from a file and adds them to the corpus.
    // \param filename      The path to the file containing the documents.
    void LoadDocumentsFromFile(const std::string& filename);

    // \brief Gets the total number of documents in the corpus.
    // \return              The total number of documents.
    int GetTotalDocuments() const;

    // \brief Gets the document frequency of a given word.
    // \param word          The word for which to get the document frequency.
    // \return              The document frequency of the word.
    int GetDocumentFrequency(const std::string& word) const;

private:
    std::vector<std::string> documents;                     ///< Vector to store the documents in the corpus.
    std::unordered_map<std::string, int> wordFrequency;     ///< Map to store the frequency of words in the corpus.
    std::unordered_map<std::string, int> documentFrequency; ///< Map to store the document frequency of words.
    std::unordered_map<std::string, VectorPtr> wordVectors; ///< Map to store the FastText vectors of words.
    int totalDocuments = 0;                                 ///< Total number of documents in the corpus.
    fasttext::FastText model;                               ///< FastText model for generating word vectors.
};
