#include "GrammarPatternManager.h"
#include "PatternParser.h"
#include "GrammarComponent.h" // Include this if GrammarComponent definitions are required

// Assuming Model is defined elsewhere
// Include necessary headers for Model and any other used classes

GrammarPatternManager *GrammarPatternManager::instance = nullptr;

GrammarPatternManager *GrammarPatternManager::getInstance()
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
    std::cout << "printPatterns: " << patterns.size() << std::endl;
    for (const auto &[key, model] : patterns)
    {
        std::cout << "model form: " << model->getForm() << ", comps: " << std::endl;
        model->printWords();
        std::cout << std::endl;
    }
}

size_t GrammarPatternManager::size() const
{
    return patterns.size();
}

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