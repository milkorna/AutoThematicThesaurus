#include <GrammarPatternManager.h>
#include <GrammarComponent.h>
#include <Logger.h>

GrammarPatternManager *GrammarPatternManager::instance = nullptr;

GrammarPatternManager *GrammarPatternManager::GetManager()
{
    if (!instance)
    {
        instance = new GrammarPatternManager();
    }
    return instance;
}

void GrammarPatternManager::addPattern(const std::string &key, const std::shared_ptr<Model> &model)
{
    patterns[key] = model;
}

std::shared_ptr<Model> GrammarPatternManager::getPattern(const std::string &key) const
{
    auto it = patterns.find(key);
    if (it != patterns.end())
    {
        return it->second;
    }
    return nullptr;
}

const std::unordered_map<std::string, std::shared_ptr<Model>> GrammarPatternManager::getSimplePatterns() const
{
    return simplePatterns;
}

const std::unordered_map<std::string, std::shared_ptr<Model>> GrammarPatternManager::getComplexPatterns() const
{
    return complexPatterns;
}

void GrammarPatternManager::readPatterns(const std::string &filePath)
{
    Parser parser(filePath);
    parser.Parse();
}

void GrammarPatternManager::printPatterns() const
{
    Logger::log("GrammarPatternManager", LogLevel::Info, "printPatterns: " + patterns.size());

    for (const auto &[key, model] : patterns)
    {
        std::cout << "model form: " << model->getForm() << ", comps: " << std::endl;
        model->printWords();
        std::cout << std::endl;
    }
}

void GrammarPatternManager::addUsedSp(const std::string sp, const bool isHead)
{
    if (const auto &res = isHead ? usedHeadSpVars.insert(sp) : usedSpVars.insert(sp); res.second)
        Logger::log("GrammarPatternManager", LogLevel::Info, "Addded new part of speach: " + sp);
}

std::unordered_set<std::string> GrammarPatternManager::getUsedHeadSp() const { return usedHeadSpVars; }
std::unordered_set<std::string> GrammarPatternManager::getUsedSp() const { return usedSpVars; }

size_t GrammarPatternManager::patternsAmount() const { return patterns.size(); }
size_t GrammarPatternManager::simplePatternsAmount() const { return simplePatterns.size(); }
size_t GrammarPatternManager::complexPatternsAmount() const { return complexPatterns.size(); }

void GrammarPatternManager::divide()
{
    for (auto &pattern : patterns)
    {
        bool isComplex = false;
        for (const auto &comp : pattern.second->getComponents())
        {
            if (comp->isModel())
            {
                isComplex = true;
                break;
            }
        }

        if (isComplex)
        {
            complexPatterns[pattern.first] = pattern.second;
        }
        else
        {
            simplePatterns[pattern.first] = pattern.second;
        }
    }

    // Optionally clear patterns if they should not be retained after division
    // patterns.clear();
}