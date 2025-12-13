#include "TextCorpus.h"
#include "Logger.h"

std::string TextCorpus::ExtractTitleFromFilename(const std::string& filename) const {
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
void TextCorpus::addText(const std::string& filename, const std::string& text) {
    std::string title = ExtractTitleFromFilename(filename);

    if (texts.find(title) == texts.end()) {
        documentCount++;
    }

    texts[title].push_back(text);
    textCount++;
}

// Updates the frequency count of a specific word (lemma) in the corpus.
// Increments the count of the word in the `wordFrequency` map and the total word count.
void TextCorpus::UpdateWordFrequency(const std::string& lemma) {
    wordFrequency[lemma]++;
    wordCount++; // Increment the total number of words in the corpus.
}

// Updates the document frequency of a specific word (lemma).
// This function increments the count of documents that contain the given word.
void TextCorpus::UpdateDocumentFrequency(const std::string& lemma) {
    documentFrequency[lemma]++;
}

// Loads texts (paragraphs) from a file, where each paragraph is extracted and associated with the filename.
void TextCorpus::LoadTextsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        addText(filename, line); // Add each paragraph as a text under the given filename.
    }
    file.close();
}

// Returns the total number of documents in the corpus (unique filenames).
int TextCorpus::getDocumentCount() const {
    return documentCount;
}

// Returns the total number of texts (paragraphs) in the corpus.
int TextCorpus::getTextCount() const {
    return textCount;
}

// Returns the total number of words (lemmas) in the corpus.
int TextCorpus::getWordCount() const {
    return wordCount;
}

// Returns the frequency of a specific word (lemma) in the corpus.
// If the word is not found, it returns 0.
int TextCorpus::getWordFrequency(const std::string& lemma) const {
    auto it = wordFrequency.find(lemma);
    if (it != wordFrequency.end()) {
        return it->second;
    }
    return 0;
}

// Returns the document frequency of a specific word (lemma).
// Document frequency refers to the number of documents (filenames) in which the word appears.
int TextCorpus::getDocumentFrequency(const std::string& lemma) const {
    auto it = documentFrequency.find(lemma);
    if (it != documentFrequency.end()) {
        return it->second;
    }
    return 0;
}

// Returns the list of all texts (paragraphs) in the corpus.
const std::unordered_map<std::string, std::vector<std::string>>& TextCorpus::getTexts() const {
    return texts;
}

// Returns the frequency map of all words (lemmas) in the corpus.
const std::unordered_map<std::string, int>& TextCorpus::getWordFrequencies() const {
    return wordFrequency;
}

const std::unordered_map<std::string, int>& TextCorpus::getDocumentFrequencies() const {
    return documentFrequency;
}

// Calculates the Term Frequency (TF) for a specific word (lemma) in the corpus.
double TextCorpus::CalculateTF(const std::string& lemma) const {
    if (wordFrequency.find(lemma) != wordFrequency.end()) {
        return static_cast<double>(wordFrequency.at(lemma)) / wordCount;
    }
    return 0.0;
}

// Calculates the Inverse Document Frequency (IDF) for a specific word (lemma) in the corpus.
double TextCorpus::CalculateIDF(const std::string& lemma) const {
    if (documentFrequency.find(lemma) != documentFrequency.end()) {
        return log(static_cast<double>(documentCount) / (1.0 + documentFrequency.at(lemma)));
    }
    return 0.0;
}

// Calculates the TF-IDF for a specific word (lemma) in the corpus.
double TextCorpus::CalculateTFIDF(const std::string& lemma) const {
    return CalculateTF(lemma) * CalculateIDF(lemma);
}

std::unordered_map<std::string, std::vector<std::string>>& TextCorpus::getTextsForModification() {
    return texts;
}

std::unordered_map<std::string, int>& TextCorpus::getWordFrequenciesForModification() {
    return wordFrequency;
}

std::unordered_map<std::string, int>& TextCorpus::getDocumentFrequenciesForModification() {
    return documentFrequency;
}

void TextCorpus::clearAllData() {
    texts.clear();
    wordFrequency.clear();
    documentFrequency.clear();
    wordCount = 0;
    textCount = 0;
    documentCount = 0;
}

void TextCorpus::recalculateStatistics() {
    wordCount = 0;
    textCount = 0;
    documentCount = texts.size();

    for (const auto& [_, freq] : wordFrequency) {
        wordCount += freq;
    }

    for (const auto& [_, textList] : texts) {
        textCount += textList.size();
    }
}
