#include <TextCorpus.h>

void TextCorpus::AddDocument(const std::string& document)
{
    documents.push_back(document);
    totalDocuments++;
}

void TextCorpus::UpdateWordFrequency(const std::string& lemma)
{
    wordFrequency[lemma]++;
    totalWords++;
}

void TextCorpus::UpdateDocumentFrequency(const std::string& lemma)
{
    documentFrequency[lemma]++;
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

int TextCorpus::GetWordFrequency(const std::string& lemma) const
{
    auto it = wordFrequency.find(lemma);
    if (it != wordFrequency.end()) {
        return it->second;
    }
    return 0;
}

int TextCorpus::GetDocumentFrequency(const std::string& lemma) const
{
    auto it = documentFrequency.find(lemma);
    if (it != documentFrequency.end()) {
        return it->second;
    }
    return 0;
}

int TextCorpus::GetTotalWords() const
{
    return totalWords;
}

double TextCorpus::CalculateTF(const std::string& lemma) const
{
    if (wordFrequency.find(lemma) != wordFrequency.end()) {
        return static_cast<double>(wordFrequency.at(lemma)) / totalWords;
    }
    return 0.0;
}

double TextCorpus::CalculateIDF(const std::string& lemma) const
{
    if (documentFrequency.find(lemma) != documentFrequency.end()) {
        return log(static_cast<double>(totalDocuments) / (1 + documentFrequency.at(lemma)));
    }
    return 0.0;
}

double TextCorpus::CalculateTFIDF(const std::string& lemma) const
{
    return CalculateTF(lemma) * CalculateIDF(lemma);
}

json TextCorpus::Serialize() const
{
    json j;
    j["documents"] = documents;
    j["wordFrequency"] = wordFrequency;
    j["documentFrequency"] = documentFrequency;
    j["totalWords"] = totalWords;
    j["totalDocuments"] = totalDocuments;
    return j;
}

void TextCorpus::Deserialize(const json& j)
{
    documents = j.at("documents").get<std::vector<std::string>>();
    wordFrequency = j.at("wordFrequency").get<std::unordered_map<std::string, int>>();
    documentFrequency = j.at("documentFrequency").get<std::unordered_map<std::string, int>>();
    totalWords = j.at("totalWords").get<int>();
    totalDocuments = j.at("totalDocuments").get<int>();
}

void TextCorpus::SaveCorpusToFile(const std::string& filename)
{
    std::ofstream file(filename);
    if (file.is_open()) {
        file << Serialize().dump(4);
        file.close();
    }
}