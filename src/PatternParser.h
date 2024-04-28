#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "GrammarComponent.h"
#include "GrammarPatternManager.h"

namespace ParserUtils
{

    void RemoveSpaces(std::string &str);

    void RemoveSpaces(X::UniString &str);

    std::pair<UniString, UniString> ParseData(const UniString &str);

    std::pair<std::string, std::string> ParseData(const std::string &str);

    std::string ExtractSubstringInQuotes(const std::string &str);

    std::string ExtractSubstringInSq(const std::string &str);
}

class Parser
{
public:
    Parser(const std::string &filePath) : fileStream(filePath)
    {
        if (!fileStream)
        {
            throw std::runtime_error("Error opening file: " + filePath);
        }
    }

    void Parse();

private:
    bool logs = false;
    std::ifstream fileStream;
    std::unordered_map<std::string, std::shared_ptr<Model>> models;

    std::string ParseName(const std::string &line) const;
    SyntaxRole ParseRoleAndCut(std::string &line) const;
    std::shared_ptr<WordComp> ParseWordComp(std::string &line);
    std::shared_ptr<ModelComp> ParseModelComp(std::string &line);
    Additional ParseTags(const std::string &line);

    UniMorphTag ParseUniMorphTag(const std::string &line) const;
    std::pair<UniSPTag, UniMorphTag> ProcessWord(const X::UniString &line);
    std::pair<std::string, UniMorphTag> ProcessModel(const X::UniString &line);

    const std::map<std::string, UniMorphTag> tagMap = {
        {"Gender=Masc", UniMorphTag::Masc}, {"Gender=Fem", UniMorphTag::Fem}, {"Gender=Neut", UniMorphTag::Neut}, {"Animacy=Anim", UniMorphTag::Anim}, {"Animacy=Inan", UniMorphTag::Inan}, {"Number=Sing", UniMorphTag::Sing}, {"Number=Plur", UniMorphTag::Plur}, {"Case=Ins", UniMorphTag::Ins}, {"Case=Acc", UniMorphTag::Acc}, {"Case=Nom", UniMorphTag::Nom}, {"Case=Dat", UniMorphTag::Dat}, {"Case=Gen", UniMorphTag::Gen}, {"Case=Loc", UniMorphTag::Loc}, {"Case=Voc", UniMorphTag::Voc}, {"Degree=Cmp", UniMorphTag::Cmp}, {"Degree=Sup", UniMorphTag::Sup}, {"Degree=Pos", UniMorphTag::Pos}, {"VerbForm=Fin", UniMorphTag::Fin}, {"VerbForm=Inf", UniMorphTag::Inf}, {"VerbForm=Conv", UniMorphTag::Conv}, {"VerbForm=Part", UniMorphTag::Part}, {"Mood=Imp", UniMorphTag::Imp}, {"Mood=Ind", UniMorphTag::Ind}, {"Person=1", UniMorphTag::_1}, {"Person=2", UniMorphTag::_2}, {"Person=3", UniMorphTag::_3}, {"Tense=Fut", UniMorphTag::Fut}, {"Tense=Past", UniMorphTag::Past}, {"Tense=Pres", UniMorphTag::Pres}, {"Tense=Notpast", UniMorphTag::Notpast}, {"Variant=Short", UniMorphTag::Short}, {"Voice=Act", UniMorphTag::Act}, {"Voice=Pass", UniMorphTag::Pass}, {"Voice=Mid", UniMorphTag::Mid}, {"NumForm=Digit", UniMorphTag::Digit}, {"Aspect=Perf", UniMorphTag::Perf}, {"Aspect=Imp", UniMorphTag::Imp}};
};