#include <string>
#include <map>
#include <tuple>
#include <PatternParser.h>

#include <xmorphy/utils/UniString.h>

#include <algorithm>

#include <regex>

size_t ParserUtils::extractNumberFromPath(const std::string &filePath)
{
    std::regex regexPattern(R"(\d+)");
    std::smatch matches;

    if (std::regex_search(filePath, matches, regexPattern))
    {
        if (!matches.empty())
        {
            return std::stoull(matches[0]);
        }
    }
    return 0;
}

// Remove spaces from a std::string
void ParserUtils::RemoveSpaces(std::string &str)
{
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
}

// Remove spaces from an X::UniString
void ParserUtils::RemoveSpaces(X::UniString &str)
{
    std::string stdStr = str.getRawString();
    RemoveSpaces(stdStr);
    str = X::UniString{stdStr};
}

// Parse data separated by a '|' into a pair of UniStrings
std::pair<UniString, UniString> ParserUtils::ParseData(const UniString &str)
{
    X::UniString compData, tags("");
    size_t pipePos = str.find(UniString{"|"});

    if (pipePos != std::string::npos)
    {
        tags = str.subString(pipePos + 1);
    }

    compData = (pipePos != std::string::npos) ? str.subString(0, pipePos) : str;

    return std::make_pair(compData, tags);
}

// Parse data separated by a '|' into a pair of std::strings
std::pair<std::string, std::string> ParserUtils::ParseData(const std::string &str)
{
    std::string compData, tags("");
    size_t pipePos = str.find(std::string{"|"});

    if (pipePos != std::string::npos)
    {
        tags = str.substr(pipePos + 1);
    }
    compData = (pipePos != std::string::npos) ? str.substr(0, pipePos) : str;

    return std::make_pair(compData, tags);
}

// Extract a substring enclosed in double quotes
std::string ParserUtils::ExtractSubstringInQuotes(const std::string &str)
{
    size_t firstQuotePos = str.find("\"");
    if (firstQuotePos != std::string::npos)
    {
        size_t secondQuotePos = str.find("\"", firstQuotePos + 1);
        if (secondQuotePos != std::string::npos)
        {
            return str.substr(firstQuotePos + 1, secondQuotePos - firstQuotePos - 1);
        }
    }
    return "";
}

std::string ParserUtils::ExtractSubstringInSq(const std::string &str)
{
    size_t firstQuotePos = str.find("[");
    if (firstQuotePos != std::string::npos)
    {
        size_t secondQuotePos = str.find("]", firstQuotePos + 1);
        if (secondQuotePos != std::string::npos)
        {
            return str.substr(firstQuotePos + 1, secondQuotePos - firstQuotePos - 1);
        }
    }
    return "";
}

// Parsing a name from a quoted string
std::string Parser::ParseName(const std::string &line) const
{
    try
    {
        return std::string(ParserUtils::ExtractSubstringInQuotes(line));
    }
    catch (const std::exception &e)
    {
        Logger::log("PatternParser", LogLevel::Error, "Failed to parse name: " + std::string(e.what()));
        return "";
    }
}

SyntaxRole Parser::ParseRoleAndCut(std::string &line) const
{
    try
    {
        size_t pos = line.find(":");
        if (pos != std::string::npos && pos > 0)
        {
            char role = line[pos + 1];
            line = line.substr(pos + 3); // Adjusted to remove correct part of string
            switch (role)
            {
            case 'h':
                return SyntaxRole::Head;
            case 'i':
                return SyntaxRole::Independent;
            case 'd':
                return SyntaxRole::Dependent;
            default:
                Logger::log("PatternParser", LogLevel::Warning, "Unknown role: " + std::string(1, role));

                return SyntaxRole::Independent;
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::log("PatternParser", LogLevel::Error, "Error in ParseRoleAndCut: " + std::string(e.what()));
    }
    return SyntaxRole::Independent;
}

UniMorphTag Parser::ParseUniMorphTag(const std::string &line) const
{
    try
    {
        std::istringstream iss(line.substr(1, line.size() - 2));
        std::string token;
        UniMorphTag result;

        while (std::getline(iss, token, ','))
        {
            auto it = tagMap.find(token);
            if (it != tagMap.end())
            {
                result = result | it->second;
            }
        }

        return result;
    }
    catch (const std::exception &e)
    {

        Logger::log("PatternParser", LogLevel::Error, "Error in ParseUniMorphTag: " + std::string(e.what()));

        return UniMorphTag(); // Return an empty or default tag on failure
    }
}

std::pair<UniSPTag, UniMorphTag> Parser::ProcessWord(const X::UniString &line, const bool isHead)
{
    try
    {
        std::vector<X::UniString> data = line.split(' ');
        UniSPTag spTag(data[0].getRawString());

        GrammarPatternManager::GetManager()->addUsedSp(spTag.toString(), isHead);

        UniMorphTag morphTag;

        if (data.size() > 1 && data[1].length() > 1 && data[1].contains('[') && data[1].contains(']'))
        {
            morphTag = ParseUniMorphTag(data[1].getRawString());
        }

        return std::make_pair(spTag, morphTag);
    }
    catch (const std::exception &e)
    {
        Logger::log("PatternParser", LogLevel::Error, "Error in ProcessWord: " + std::string(e.what()));
        return std::make_pair(UniSPTag(), UniMorphTag()); // Return default tags on failure
    }
}

Additional Parser::ParseTags(const std::string &line)
{
    try
    {
        bool isRec = false;
        std::string exlexWord = "";
        std::vector<std::string> themes;

        if (line.find("rec") != std::string::npos)
        {
            isRec = true;
        }

        auto exlexStart = line.find("exlex:\"");
        if (exlexStart != std::string::npos)
        {
            auto start = exlexStart + 7;
            auto end = line.find("\"", start);
            if (end != std::string::npos)
            {
                exlexWord = line.substr(start, end - start);
            }
        }

        auto themesStart = line.find("themes:\"");
        if (themesStart != std::string::npos)
        {
            auto start = themesStart + 8;
            auto end = line.find("\"", start);
            if (end != std::string::npos)
            {
                std::string themesStr = line.substr(start, end - start);
                std::istringstream themesStream(themesStr);
                std::string theme;
                while (std::getline(themesStream, theme, ','))
                {
                    themes.push_back(theme);
                }
            }
        }

        return Additional{isRec, exlexWord, themes};
    }
    catch (const std::exception &e)
    {
        Logger::log("PatternParser", LogLevel::Error, "Error in ParseTags: " + std::string(e.what()));

        return Additional(); // Return an empty Additional structure on failure
    }
}

// Parse a line of data and build a word composition object
std::shared_ptr<WordComp> Parser::ParseWordComp(std::string &line)
{
    try
    {
        SyntaxRole synRole = this->ParseRoleAndCut(line);
        auto wordData = ParserUtils::ParseData(X::UniString{line});
        auto w = ProcessWord(wordData.first, synRole == SyntaxRole::Head ? true : false);

        Additional addcond;
        if (!wordData.second.isEmpty())
        {
            addcond = ParseTags(wordData.second.getRawString());
        }
        return std::make_shared<WordComp>(WordComp{w.first, Condition{synRole, w.second, addcond}});
    }
    catch (const std::exception &e)
    {
        Logger::log("PatternParser", LogLevel::Error, "Failed to parse word component: " + std::string(e.what()));
        return nullptr;
    }
}

// Similar parsing function for model components
std::shared_ptr<ModelComp> Parser::ParseModelComp(std::string &line)
{
    try
    {
        const SyntaxRole synRole = this->ParseRoleAndCut(line);
        auto modelData = ParserUtils::ParseData(line);
        const auto &patName = ParserUtils::ExtractSubstringInQuotes(modelData.first);
        UniMorphTag morphTag;
        if (line.find("[") != std::string::npos && line.find("]") != std::string::npos)
        {
            std::string patTags = ParserUtils::ExtractSubstringInSq(modelData.first);
            morphTag = ParseUniMorphTag(patTags);
        }

        auto model = GrammarPatternManager::GetManager()->getPattern(patName);
        if (!model)
        {
            Logger::log("PatternParser", LogLevel::Warning, "Failed to find model: " + patName);

            return nullptr;
        }

        Additional addcond;
        if (!modelData.second.empty())
        {
            addcond = ParseTags(modelData.second);
        }
        return std::make_shared<ModelComp>(ModelComp{patName, model->getComponents(), Condition{synRole, morphTag, addcond}});
    }
    catch (const std::exception &e)
    {
        Logger::log("PatternParser", LogLevel::Error, "Failed to parse model component: " + std::string(e.what()));
        return nullptr;
    }
}

// Main parsing function
void Parser::Parse()
{
    try
    {
        GrammarPatternManager *manager = GrammarPatternManager::GetManager();

        bool isInBody = false;
        std::string line;
        std::string name;

        Components comps;

        while (std::getline(fileStream, line))
        {
            std::istringstream iss(line);

            if (line.empty() || line.find_first_not_of(" \t\n\v\f\r") == std::string::npos)
            {
                continue;
            }

            if (line.find("name:") != std::string::npos)
            {
                name = ParserUtils::ExtractSubstringInQuotes(line);
                Logger::log("PatternParser", LogLevel::Debug, "Pattern name parsed successfully!");

                continue;
            }

            if (line.find("body: {") != std::string::npos)
            {
                isInBody = true;
                Logger::log("PatternParser", LogLevel::Debug, "Started parsing body for pattern: " + name);

                continue;
            }

            if (isInBody)
            {
                if (line.find("w:") != std::string::npos)
                {
                    auto word = this->ParseWordComp(line);
                    if (word)
                    {
                        comps.push_back(word);
                        Logger::log("PatternParser", LogLevel::Debug, "Word component parsed successfully: " + line);
                    }
                    else
                    {
                        Logger::log("PatternParser", LogLevel::Error, "Word component parsing failed: " + line);
                    }
                    continue;
                }
                else if (line.find("m:") != std::string::npos)
                {
                    auto model = this->ParseModelComp(line);
                    if (model)
                    {
                        comps.push_back(model);

                        Logger::log("PatternParser", LogLevel::Debug, "Model component parsed successfully: " + line);
                    }
                    else
                    {
                        Logger::log("PatternParser", LogLevel::Error, "Model component parsing failed: " + line);
                    }
                    continue;
                }

                if (line.find("}") != std::string::npos)
                {
                    isInBody = false;
                    Logger::log("PatternParser", LogLevel::Debug, "Adding pattern with key: " + name);

                    manager->addPattern(name, std::make_shared<Model>(Model{name, comps}));

                    Logger::log("PatternParser", LogLevel::Debug, "Current number of patterns: " + manager->patternsAmount());

                    Logger::log("PatternParser", LogLevel::Debug, "Finished parsing body for pattern: " + name);

                    comps.clear();
                }
            }
        }

        manager->divide();

        if (this->logs)
        {
            Logger::log("PatternParser", LogLevel::Info,
                        std::string("Parsing completed successfully.\n") +
                            "Number of patterns after parsing: " + std::to_string(manager->patternsAmount()) +
                            std::string("\nNumber of simple patterns: ") + std::to_string(manager->simplePatternsAmount()) +
                            std::string("\nNumber of complex patterns: ") + std::to_string(manager->complexPatternsAmount()));
        }
    }
    catch (const std::exception &e)
    {
        // Handle parsing exceptions and log them
        Logger::log("PatternParser", LogLevel::Error, "Parsing failed: " + std::string(e.what()));
        throw; // Rethrow exception to handle it further up if necessary
    }
}