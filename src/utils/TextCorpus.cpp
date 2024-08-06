#include <TextCorpus.h>

TextCorpus::TextCorpus(const std::string& modelPath)
{
    model.loadModel(modelPath);
}

void TextCorpus::AddDocument(const std::string& document)
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

            // Retrieve FastText vector for the word
            if (wordVectors.find(word) == wordVectors.end()) {
                auto vec = std::make_shared<fasttext::Vector>(model.getDimension());
                model.getWordVector(*vec, word);
                wordVectors[word] = vec;
            }
        }
    }
    totalDocuments++;
}

const std::shared_ptr<fasttext::Vector> TextCorpus::GetWordVector(const std::string& word) const
{
    return wordVectors.at(word);
}

void TextCorpus::LoadDocumentsFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        AddDocument(line);
    }
    file.close();
}

int TextCorpus::GetTotalDocuments() const
{
    return totalDocuments;
}

int TextCorpus::GetDocumentFrequency(const std::string& word) const
{
    auto it = documentFrequency.find(word);
    return (it != documentFrequency.end()) ? it->second : 0;
}
