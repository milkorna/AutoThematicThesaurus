#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class TextCorpus {
public:
    TextCorpus() = default;

    void AddDocument(const std::string& document)
    {
        documents.push_back(document);
        std::vector<std::string> words;
        boost::split(words, document, boost::is_any_of(" \t\n.,;:!?'\"()"), boost::token_compress_on);
        std::unordered_map<std::string, bool> seenWords;

        for (auto& word : words) {
            if (!word.empty()) {
                boost::to_lower(word);
                wordFrequency[word]++;
                if (!seenWords[word]) {
                    documentFrequency[word]++;
                    seenWords[word] = true;
                }
            }
        }
        totalDocuments++;
    }

    void LoadDocumentsFromFile(const std::string& filename)
    {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            AddDocument(line);
        }
        file.close();
    }

    int GetTotalDocuments() const
    {
        return totalDocuments;
    }

    int GetDocumentFrequency(const std::string& word) const
    {
        auto it = documentFrequency.find(word);
        return (it != documentFrequency.end()) ? it->second : 0;
    }

    // double ComputeTF(const std::string& word, const std::string& document)
    // {
    //     std::vector<std::string> words;
    //     boost::split(words, document, boost::is_any_of(" \t\n.,;:!?'\"()"), boost::token_compress_on);
    //     int wordCount = 0;
    //     int totalWords = 0;
    //     for (const auto& token : words) {
    //         if (!token.empty()) {
    //             if (token == word) {
    //                 wordCount++;
    //             }
    //             totalWords++;
    //         }
    //     }
    //     return static_cast<double>(wordCount) / totalWords;
    // }

    // double ComputeIDF(const std::string& word)
    // {
    //     int docsWithWord = 0;
    //     for (const auto& doc : documents) {
    //         if (doc.find(word) != std::string::npos) {
    //             docsWithWord++;
    //         }
    //     }
    //     return log(static_cast<double>(totalDocuments) / (1 + docsWithWord));
    // }

    // double ComputeTFIDF(const std::string& word, const std::string& document)
    // {
    //     return ComputeTF(word, document) * ComputeIDF(word);
    // }

private:
    std::vector<std::string> documents;
    std::unordered_map<std::string, int> wordFrequency;
    std::unordered_map<std::string, int> documentFrequency;
    int totalDocuments = 0;
};