#include "GrammarPatternManager.h"

#include "GrammarComponent.h"
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

const std::unordered_map<std::string, std::shared_ptr<Model>> GrammarPatternManager::getBases() const
{
    return bases;
}

const std::unordered_map<std::string, std::shared_ptr<Model>> GrammarPatternManager::getAssemblies() const
{
    return assemblies;
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
    {
        Logger::log("GrammarPatternManager", LogLevel::Info, "Addde new part of speach: " + sp);
    }
    else
    {
        Logger::log("GrammarPatternManager", LogLevel::Info, "Part of speach: " + sp + " was already in the set.");
    }
}

std::unordered_set<std::string> GrammarPatternManager::getUsedHeadSp() const { return usedHeadSpVars; }
std::unordered_set<std::string> GrammarPatternManager::getUsedSp() const { return usedSpVars; }

size_t GrammarPatternManager::patternsAmount() const { return patterns.size(); }
size_t GrammarPatternManager::basesAmount() const { return bases.size(); }
size_t GrammarPatternManager::assemsAmount() const { return assemblies.size(); }

void GrammarPatternManager::divide()
{
    for (auto &pattern : patterns)
    {
        bool isAssembly = false;
        for (const auto &comp : pattern.second->getComponents())
        {
            if (comp->isModel())
            {
                isAssembly = true;
                break;
            }
        }

        if (isAssembly)
        {
            assemblies[pattern.first] = pattern.second;
        }
        else
        {
            bases[pattern.first] = pattern.second;
        }
    }

    // Optionally clear patterns if they should not be retained after division
    // patterns.clear();
}