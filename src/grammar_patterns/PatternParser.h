#ifndef PATTERN_PARSER_H
#define PATTERN_PARSER_H

#include <ModelComponent.h>

#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Namespace containing utilities for parsing operations.
namespace ParserUtils {

    size_t extractNumberFromPath(const std::string& filePath);

    // Removes spaces from a std::string.
    void RemoveSpaces(std::string& str);

    // Removes spaces from a UniString type.
    void RemoveSpaces(X::UniString& str);

    // Parses a UniString and returns two separated parts.
    std::pair<UniString, UniString> ParseData(const UniString& str);

    // Parses a std::string and returns two separated parts.
    std::pair<std::string, std::string> ParseData(const std::string& str);

    // Extracts a substring enclosed in double quotes.
    std::string ExtractSubstringInQuotes(const std::string& str);

    // Extracts a substring enclosed in single quotes.
    std::string ExtractSubstringInSq(const std::string& str);
}

struct Process {
    fs::path inputFile;
    fs::path outputFile;
    json jsonData;
    size_t docNum;
    size_t sentNum;

    explicit Process(const fs::path& inputFile, const fs::path& outputFile, size_t sentNum = 0)
        : inputFile(inputFile), outputFile(outputFile), docNum(ParserUtils::extractNumberFromPath(inputFile)),
          sentNum(sentNum)
    {

        // Открываем существующий JSON или создаем новый
        if (fs::exists(outputFile)) {
            std::ifstream inFile(outputFile);
            if (inFile) {
                try {
                    inFile >> jsonData; // Читаем JSON, если он есть
                    Logger::log("Process", LogLevel::Info, "Loaded existing JSON file: " + outputFile.string());
                } catch (...) {
                    Logger::log("Process", LogLevel::Error, "Failed to parse JSON file: " + outputFile.string());
                    jsonData = json::array(); // Если ошибка, создаём новый массив
                }
            }
        } else {
            jsonData = json::array(); // Если файла нет, создаём пустой JSON
            Logger::log("Process", LogLevel::Debug, "Created new JSON file: " + outputFile.string());
        }
    }

    void addJsonObject(const json& newObj)
    {
        jsonData.push_back(newObj);
    }

    ~Process()
    {
        std::ofstream outFile(outputFile, std::ios::trunc | std::ios::binary);
        if (!outFile) {
            Logger::log("Process", LogLevel::Error, "Failed to open JSON file for writing: " + outputFile.string());
            return;
        }
        outFile << jsonData.dump(4) << std::endl;
        Logger::log("Process", LogLevel::Info, "Successfully saved JSON file: " + outputFile.string());
    }
};

// Parser class responsible for parsing linguistic data from a file.
class Parser {
public:
    // Constructs a parser object for the given file path and throws if the file cannot be opened.
    explicit Parser(const std::string& filePath) : fileStream(filePath)
    {
        if (!fileStream) {
            throw std::runtime_error("Error opening file: " + filePath);
        }
    }
    // Parses the data from the associated file.
    void Parse();

    ~Parser() = default;

private:
    bool logs = false;                                              // Flag to control logging
    std::ifstream fileStream;                                       // Input file stream
    std::unordered_map<std::string, std::shared_ptr<Model>> models; // Map to store parsed models

    // Parses a name from the line.
    std::string ParseName(const std::string& line) const;

    // Parses a syntactic role from the line and modifies the line to remove the parsed part.
    SyntaxRole ParseRoleAndCut(std::string& line) const;

    // Parses a word component from the line.
    std::shared_ptr<WordComp> ParseWordComp(std::string& line);

    // Parses a model component from the line.
    std::shared_ptr<ModelComp> ParseModelComp(std::string& line);

    // Parses additional tags from the line.
    Additional ParseTags(const std::string& line);

    // Parses a UniMorph tag from the line.
    UniMorphTag ParseUniMorphTag(const std::string& line) const;

    // Processes a word and returns the syntactic tag and morphological tag.
    std::pair<UniSPTag, UniMorphTag> ProcessWord(const X::UniString& line, const bool isHead);

    // Processes a model and returns the model name and morphological tag.
    std::pair<std::string, UniMorphTag> ProcessModel(const X::UniString& line);

    // Map of string representations to UniMorphTag enums.
    const std::map<std::string, UniMorphTag> tagMap = {
        {"Gender=Masc", UniMorphTag::Masc},    {"Gender=Fem", UniMorphTag::Fem},
        {"Gender=Neut", UniMorphTag::Neut},    {"Animacy=Anim", UniMorphTag::Anim},
        {"Animacy=Inan", UniMorphTag::Inan},   {"Number=Sing", UniMorphTag::Sing},
        {"Number=Plur", UniMorphTag::Plur},    {"Case=Ins", UniMorphTag::Ins},
        {"Case=Acc", UniMorphTag::Acc},        {"Case=Nom", UniMorphTag::Nom},
        {"Case=Dat", UniMorphTag::Dat},        {"Case=Gen", UniMorphTag::Gen},
        {"Case=Loc", UniMorphTag::Loc},        {"Case=Voc", UniMorphTag::Voc},
        {"Degree=Cmp", UniMorphTag::Cmp},      {"Degree=Sup", UniMorphTag::Sup},
        {"Degree=Pos", UniMorphTag::Pos},      {"VerbForm=Fin", UniMorphTag::Fin},
        {"VerbForm=Inf", UniMorphTag::Inf},    {"VerbForm=Conv", UniMorphTag::Conv},
        {"VerbForm=Part", UniMorphTag::Part},  {"Mood=Imp", UniMorphTag::Imp},
        {"Mood=Ind", UniMorphTag::Ind},        {"Person=1", UniMorphTag::_1},
        {"Person=2", UniMorphTag::_2},         {"Person=3", UniMorphTag::_3},
        {"Tense=Fut", UniMorphTag::Fut},       {"Tense=Past", UniMorphTag::Past},
        {"Tense=Pres", UniMorphTag::Pres},     {"Tense=Notpast", UniMorphTag::Notpast},
        {"Variant=Short", UniMorphTag::Short}, {"Voice=Act", UniMorphTag::Act},
        {"Voice=Pass", UniMorphTag::Pass},     {"Voice=Mid", UniMorphTag::Mid},
        {"NumForm=Digit", UniMorphTag::Digit}, {"Aspect=Perf", UniMorphTag::Perf},
        {"Aspect=Imp", UniMorphTag::Imp}};
};

#endif // PATTERN_PARSER_H