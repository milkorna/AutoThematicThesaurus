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

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using VectorPtr = std::shared_ptr<fasttext::Vector>;

// \class TextCorpus
// \brief This class handles the processing and storage of a text corpus,
// including document addition, frequency calculation, and FastText vector retrieval.
class TextCorpus {
public:
    // \brief Default constructor for the TextCorpus class.
    TextCorpus() = default;

    static TextCorpus& GetCorpus()
    {
        static TextCorpus corpus;
        return corpus;
    }

    // \brief Adds a document to the corpus.
    // \param document      The document text to be added.
    void AddDocument(const std::string& document);

    void UpdateWordFrequency(const std::string& lemma);

    void UpdateDocumentFrequency(const std::string& lemma);

    // \brief Loads documents from a file and adds them to the corpus.
    // \param filename      The path to the file containing the documents.
    void LoadDocumentsFromFile(const std::string& filename);

    // \brief Gets the total number of documents in the corpus.
    // \return              The total number of documents.
    int GetTotalDocuments() const;

    int GetWordFrequency(const std::string& lemma) const;

    // \brief Gets the document frequency of a given word.
    // \param word          The word for which to get the document frequency.
    // \return              The document frequency of the word.
    int GetDocumentFrequency(const std::string& word) const;

    double CalculateTF(const std::string& lemma) const;
    double CalculateIDF(const std::string& lemma) const;
    double CalculateTFIDF(const std::string& lemma) const;

    json Serialize() const;
    void Deserialize(const json& j);

    int GetTotalWords() const;

    void SaveCorpusToFile(const std::string& filename);

    static TextCorpus LoadCorpusFromFile(const std::string& filename)
    {
        std::ifstream file(filename);
        if (file.is_open()) {
            json j;
            file >> j;
            TextCorpus corpus;
            corpus.Deserialize(j);
            return corpus;
        }
        return TextCorpus();
    }

private:
    std::vector<std::string> documents;                     ///< Vector to store the documents in the corpus.
    std::unordered_map<std::string, int> wordFrequency;     ///< Map to store the frequency of words in the corpus.
    std::unordered_map<std::string, int> documentFrequency; ///< Map to store the document frequency of words.
    int totalWords = 0;                                     ///< Total number of words (lemmas) in the corpus.
    int totalDocuments = 0;                                 ///< Total number of documents in the corpus.
};

#endif // TEXT_CORPUS_H
