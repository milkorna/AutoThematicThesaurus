#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "GrammarComponent.h"
#include "GrammarPatternManager.h"
#include <Logger.h>

// Namespace containing utilities for parsing operations.
namespace ParserUtils
{
    // Removes spaces from a std::string.
    void RemoveSpaces(std::string &str);

    // Removes spaces from a UniString type.
    void RemoveSpaces(X::UniString &str);

    // Parses a UniString and returns two separated parts.
    std::pair<UniString, UniString> ParseData(const UniString &str);

    // Parses a std::string and returns two separated parts.
    std::pair<std::string, std::string> ParseData(const std::string &str);

    // Extracts a substring enclosed in double quotes.
    std::string ExtractSubstringInQuotes(const std::string &str);

    // Extracts a substring enclosed in single quotes.
    std::string ExtractSubstringInSq(const std::string &str);
}

// Parser class responsible for parsing linguistic data from a file.
class Parser
{
public:
    // Constructs a parser object for the given file path and throws if the file cannot be opened.
    Parser(const std::string &filePath) : fileStream(filePath)
    {
        if (!fileStream)
        {
            throw std::runtime_error("Error opening file: " + filePath);
        }
    }
    // Parses the data from the associated file.
    void Parse();

private:
    bool logs = false;                                              // Flag to control logging
    std::ifstream fileStream;                                       // Input file stream
    std::unordered_map<std::string, std::shared_ptr<Model>> models; // Map to store parsed models

    // Parses a name from the line.
    std::string ParseName(const std::string &line) const;

    // Parses a syntactic role from the line and modifies the line to remove the parsed part.
    SyntaxRole ParseRoleAndCut(std::string &line) const;

    // Parses a word component from the line.
    std::shared_ptr<WordComp> ParseWordComp(std::string &line);

    // Parses a model component from the line.
    std::shared_ptr<ModelComp> ParseModelComp(std::string &line);

    // Parses additional tags from the line.
    Additional ParseTags(const std::string &line);

    // Parses a UniMorph tag from the line.
    UniMorphTag ParseUniMorphTag(const std::string &line) const;

    // Processes a word and returns the syntactic tag and morphological tag.
    std::pair<UniSPTag, UniMorphTag> ProcessWord(const X::UniString &line, const bool isHead);

    // Processes a model and returns the model name and morphological tag.
    std::pair<std::string, UniMorphTag> ProcessModel(const X::UniString &line);

    // Map of string representations to UniMorphTag enums.
    const std::map<std::string, UniMorphTag> tagMap = {
        {"Gender=Masc", UniMorphTag::Masc}, {"Gender=Fem", UniMorphTag::Fem}, {"Gender=Neut", UniMorphTag::Neut}, {"Animacy=Anim", UniMorphTag::Anim}, {"Animacy=Inan", UniMorphTag::Inan}, {"Number=Sing", UniMorphTag::Sing}, {"Number=Plur", UniMorphTag::Plur}, {"Case=Ins", UniMorphTag::Ins}, {"Case=Acc", UniMorphTag::Acc}, {"Case=Nom", UniMorphTag::Nom}, {"Case=Dat", UniMorphTag::Dat}, {"Case=Gen", UniMorphTag::Gen}, {"Case=Loc", UniMorphTag::Loc}, {"Case=Voc", UniMorphTag::Voc}, {"Degree=Cmp", UniMorphTag::Cmp}, {"Degree=Sup", UniMorphTag::Sup}, {"Degree=Pos", UniMorphTag::Pos}, {"VerbForm=Fin", UniMorphTag::Fin}, {"VerbForm=Inf", UniMorphTag::Inf}, {"VerbForm=Conv", UniMorphTag::Conv}, {"VerbForm=Part", UniMorphTag::Part}, {"Mood=Imp", UniMorphTag::Imp}, {"Mood=Ind", UniMorphTag::Ind}, {"Person=1", UniMorphTag::_1}, {"Person=2", UniMorphTag::_2}, {"Person=3", UniMorphTag::_3}, {"Tense=Fut", UniMorphTag::Fut}, {"Tense=Past", UniMorphTag::Past}, {"Tense=Pres", UniMorphTag::Pres}, {"Tense=Notpast", UniMorphTag::Notpast}, {"Variant=Short", UniMorphTag::Short}, {"Voice=Act", UniMorphTag::Act}, {"Voice=Pass", UniMorphTag::Pass}, {"Voice=Mid", UniMorphTag::Mid}, {"NumForm=Digit", UniMorphTag::Digit}, {"Aspect=Perf", UniMorphTag::Perf}, {"Aspect=Imp", UniMorphTag::Imp}};
};