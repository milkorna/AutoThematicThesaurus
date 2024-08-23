#include <TextCorpus.h>
#include <regex>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <unicode/ustream.h>
#include <unicode/utypes.h>

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
        return log(static_cast<double>(totalDocuments) / (1 + documentFrequency.at(lemma)));
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

// This function checks whether a given text contains any unwanted characters.
// The function uses ICU to handle Unicode strings and to check properties of each character.
bool ContainsUnwantedCharacters(const std::string& text)
{
    // Convert the input UTF-8 string to an ICU UnicodeString for processing
    icu::UnicodeString unicodeText = icu::UnicodeString::fromUTF8(text);

    // Iterate through each character in the UnicodeString
    for (int32_t i = 0; i < unicodeText.length(); ++i) {
        UChar32 codepoint = unicodeText.char32At(i);

        // Check if the character is a digit
        if (u_isdigit(codepoint)) {
            return true;
        }

        // Check if the character is an emoji using the extended pictographic property
        if (u_hasBinaryProperty(codepoint, UCHAR_EXTENDED_PICTOGRAPHIC)) {
            return true;
        }

        // Check if the character is a Chinese ideograph (Han script)
        if (u_getIntPropertyValue(codepoint, UCHAR_SCRIPT) == USCRIPT_HAN) {
            return true;
        }

        // Check if the character is part of the Devanagari script
        if (u_getIntPropertyValue(codepoint, UCHAR_SCRIPT) == USCRIPT_DEVANAGARI) {
            return true;
        }

        // Check if the character is part of the Arabic script
        if (u_getIntPropertyValue(codepoint, UCHAR_SCRIPT) == USCRIPT_ARABIC) {
            return true;
        }

        // Check if the character is a mathematical or technical symbol
        if (u_charType(codepoint) == U_MATH_SYMBOL || u_charType(codepoint) == U_OTHER_SYMBOL) {
            return true;
        }
    }

    return false;
}

// This function checks whether a given key should be filtered out based on various conditions
bool ShouldFilterOut(const std::string& key)
{
    // Elements that contain %, *, _, #, or $
    if (key.find('%') != std::string::npos || key.find('*') != std::string::npos ||
        key.find('_') != std::string::npos || key.find('#') != std::string::npos ||
        key.find('$') != std::string::npos) {
        return true;
    }

    // Elements that consist entirely of punctuation or non-alphabetic symbols, excluding Russian letters
    if (std::regex_match(key, std::regex("^[^\\wа-яА-ЯёЁa-zA-Z¨]+$"))) {

        return true;
    }

    // Elements consisting of only English letters, punctuation, and digits and are longer than 25 characters
    if (key.size() > 25 && std::regex_match(key, std::regex("^[a-zA-Z0-9[:punct:]]+$"))) {
        return true;
    }

    // Use the ContainsUnwantedCharacters function to check for other unwanted characters
    if (ContainsUnwantedCharacters(key)) {
        return true;
    }

    // If none of the conditions match, the key is not filtered out
    return false;
}

void TextCorpus::Deserialize(const json& j)
{
    try {
        // Filter and deserialize documentFrequencys
        for (const auto& item : j.at("3_documentFrequency").items()) {
            if (!ShouldFilterOut(item.key())) {
                documentFrequency[item.key()] = item.value();
            }
        }

        // Filter and deserialize wordFrequency
        for (const auto& item : j.at("4_wordFrequency").items()) {
            if (!ShouldFilterOut(item.key())) {
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
    std::ifstream file(filename);
    if (file.is_open()) {
        json j;
        file >> j;
        Deserialize(j);
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

// Constructs the term-document matrix from the TextCorpus.
// Each row of the matrix corresponds to a term, and each column corresponds to a document.
// The values in the matrix represent the frequency of each term in each document.
MatrixXd BuildTermDocumentMatrix(const TextCorpus& corpus)
{
    const auto& texts = corpus.GetTexts();
    const auto& wordFrequency = corpus.GetWordFrequencies();

    std::vector<std::string> terms;
    std::unordered_map<std::string, int> termIndex;
    int termCount = 0;

    // Map each term to a row index in the matrix
    for (const auto& pair : wordFrequency) {
        termIndex[pair.first] = termCount++;
        terms.push_back(pair.first);
    }

    // Initialize the term-document matrix with the appropriate dimensions
    MatrixXd termDocumentMatrix(termCount, corpus.GetTotalDocuments());

    // Fill the matrix with term frequencies for each document
    int docId = 0;
    for (const auto& doc : texts) {
        std::unordered_map<std::string, int> termDocFrequency;

        // Iterate over each paragraph (text) in the document
        for (const auto& text : doc.second) {
            std::vector<std::string> words;
            boost::split(words, text, boost::is_any_of(" ")); // Split text into words

            // Increment the frequency of each term in the current document
            for (const auto& word : words) {
                if (termIndex.find(word) != termIndex.end()) {
                    termDocFrequency[word]++;
                }
            }
        }

        // Fill the term-document matrix with the calculated frequencies
        for (const auto& termFreq : termDocFrequency) {
            int row = termIndex[termFreq.first];
            termDocumentMatrix(row, docId) = termFreq.second;
        }
        docId++;
    }

    return termDocumentMatrix;
}