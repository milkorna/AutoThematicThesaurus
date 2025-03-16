#include "Logger.h"
#include <TextCorpus.h>

std::string TextCorpus::ExtractTitleFromFilename(const std::string& filename) const
{
    std::string titleFilename = filename;
    size_t pos = titleFilename.find("_text.txt");
    if (pos != std::string::npos) {
        titleFilename.replace(pos, 10, "_title.txt");
    } else {
        throw std::runtime_error("Unexpected filename format: " + filename);
    }

    std::ifstream titleFile(titleFilename);
    if (!titleFile.is_open()) {
        throw std::runtime_error("Failed to open title file: " + titleFilename);
    }

    std::string title;
    std::getline(titleFile, title);
    titleFile.close();

    return title;
}

// Adds a text (paragraph) to the corpus under the associated document (filename).
// Updates the total document count if this is the first text from the document.
// Also updates the total text count.
void TextCorpus::AddText(const std::string& filename, const std::string& text)
{
    std::string title = ExtractTitleFromFilename(filename);

    if (texts.find(title) == texts.end()) {
        totalDocuments++;
    }

    texts[title].push_back(text);
    totalTexts++;
}

// Updates the frequency count of a specific word (lemma) in the corpus.
// Increments the count of the word in the `wordFrequency` map and the total word count.
void TextCorpus::UpdateWordFrequency(const std::string& lemma)
{
    wordFrequency[lemma]++;
    totalWords++; // Increment the total number of words in the corpus.
}

// Updates the document frequency of a specific word (lemma).
// This function increments the count of documents that contain the given word.
void TextCorpus::UpdateDocumentFrequency(const std::string& lemma)
{
    documentFrequency[lemma]++;
}

// Loads texts (paragraphs) from a file, where each paragraph is extracted and associated with the filename.
void TextCorpus::LoadTextsFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        AddText(filename, line); // Add each paragraph as a text under the given filename.
    }
    file.close();
}

// Returns the total number of documents in the corpus (unique filenames).
int TextCorpus::GetTotalDocuments() const
{
    return totalDocuments;
}

// Returns the total number of texts (paragraphs) in the corpus.
int TextCorpus::GetTotalTexts() const
{
    return totalTexts;
}

// Returns the total number of words (lemmas) in the corpus.
int TextCorpus::GetTotalWords() const
{
    return totalWords;
}

// Returns the frequency of a specific word (lemma) in the corpus.
// If the word is not found, it returns 0.
int TextCorpus::GetWordFrequency(const std::string& lemma) const
{
    auto it = wordFrequency.find(lemma);
    if (it != wordFrequency.end()) {
        return it->second;
    }
    return 0;
}

// Returns the document frequency of a specific word (lemma).
// Document frequency refers to the number of documents (filenames) in which the word appears.
int TextCorpus::GetDocumentFrequency(const std::string& lemma) const
{
    auto it = documentFrequency.find(lemma);
    if (it != documentFrequency.end()) {
        return it->second;
    }
    return 0;
}

// Returns the list of all texts (paragraphs) in the corpus.
const std::unordered_map<std::string, std::vector<std::string>>& TextCorpus::GetTexts() const
{
    return texts;
}

// Returns the frequency map of all words (lemmas) in the corpus.
const std::unordered_map<std::string, int>& TextCorpus::GetWordFrequencies() const
{
    return wordFrequency;
}

// Calculates the Term Frequency (TF) for a specific word (lemma) in the corpus.
double TextCorpus::CalculateTF(const std::string& lemma) const
{
    if (wordFrequency.find(lemma) != wordFrequency.end()) {
        return static_cast<double>(wordFrequency.at(lemma)) / totalWords;
    }
    return 0.0;
}

// Calculates the Inverse Document Frequency (IDF) for a specific word (lemma) in the corpus.
double TextCorpus::CalculateIDF(const std::string& lemma) const
{
    if (documentFrequency.find(lemma) != documentFrequency.end()) {
        return log(static_cast<double>(totalDocuments) / (1.0 + documentFrequency.at(lemma)));
    }
    return 0.0;
}

// Calculates the TF-IDF for a specific word (lemma) in the corpus.
double TextCorpus::CalculateTFIDF(const std::string& lemma) const
{
    return CalculateTF(lemma) * CalculateIDF(lemma);
}

// Serializes the corpus data to JSON format for storage or transmission.
json TextCorpus::Serialize() const
{
    json j;

    // Serialize overall corpus information
    j["0_totalDocuments"] = totalDocuments;
    j["1_totalTexts"] = totalTexts;
    j["2_totalWords"] = totalWords;
    j["3_documentFrequency"] = documentFrequency;
    j["4_wordFrequency"] = wordFrequency;

    // Serialize the documents and their corresponding texts
    json documentsJson = json::array();
    for (const auto& doc : texts) {
        json docJson;
        docJson["filename"] = doc.first; // Document name (filename)
        docJson["texts"] = doc.second;   // Vector of texts (paragraphs) in this document
        documentsJson.push_back(docJson);
    }

    j["5_documents"] = documentsJson;

    return j;
}

void TextCorpus::Deserialize(const json& j)
{
    try {
        // Filter and deserialize documentFrequencys
        for (const auto& item : j.at("3_documentFrequency").items()) {
            if (!StringFilters::ShouldFilterOut(item.key())) {
                documentFrequency[item.key()] = item.value();
            }
        }

        // Filter and deserialize wordFrequency
        for (const auto& item : j.at("4_wordFrequency").items()) {
            if (!StringFilters::ShouldFilterOut(item.key())) {
                wordFrequency[item.key()] = item.value();
            }
        }

        totalWords = j.at("2_totalWords").get<int>();
        totalDocuments = j.at("0_totalDocuments").get<int>();
        totalTexts = j.at("1_totalTexts").get<int>();

        // Deserialize the documents and their corresponding texts with additional filtering
        texts.clear(); // Clear the existing data
        for (const auto& docJson : j.at("5_documents")) {
            std::string filename = docJson.at("filename").get<std::string>();
            std::vector<std::string> docTexts;
            for (const auto& text : docJson.at("texts").get<std::vector<std::string>>()) {
                // Filter out texts that do not contain spaces or are shorter than 30 characters
                if (text.find(' ') != std::string::npos && text.length() >= 40) {
                    docTexts.push_back(text);
                }
            }
            if (!docTexts.empty()) {
                texts[filename] = docTexts; // Store the filtered texts under the document name (filename)
            }
        }
    } catch (json::exception& e) {
        // Handle parsing errors
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        throw;
    }
}

// Saves the serialized corpus data to a file.
void TextCorpus::SaveCorpusToFile(const std::string& filename)
{
    std::ofstream file(filename);
    if (file.is_open()) {
        file << Serialize().dump(4);
        file.close();
    }
}

// Loads the corpus data from a file and deserializes it into the singleton instance.
void TextCorpus::LoadCorpusFromFile(const std::string& filename)
{
    Logger::log("TextCorpus", LogLevel::Info, "Loading corpus from file: " + filename);

    std::ifstream file(filename);
    if (file.is_open()) {
        json j;
        file >> j;
        Deserialize(j);
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}
