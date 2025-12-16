#include "JsonPatternParser.h"
#include <Component.h>
#include <GrammarPatternManager.h>

#include <filesystem>

namespace fs = std::filesystem;

bool GrammarPatternManager::has(const std::string& key) const noexcept {
    return patterns.find(key) != patterns.end();
}

void GrammarPatternManager::add(const std::string& key, const std::shared_ptr<Model>& model) noexcept {
    patterns[key] = model;
}

std::shared_ptr<Model> GrammarPatternManager::get(const std::string& key) const noexcept {
    auto it = patterns.find(key);
    if (it != patterns.end()) {
        return it->second;
    }
    return nullptr;
}

const PatternMap& GrammarPatternManager::getSimplePatterns() const noexcept {
    return simplePatterns;
}

const PatternMap& GrammarPatternManager::getComplexPatterns() const noexcept {
    return complexPatterns;
}

void GrammarPatternManager::readPatterns(const fs::path& filePath) {
    try {
        Logger::log("GrammarPatternManager", LogLevel::Info, "Reading patterns from file: " + filePath.string());

        if (!fs::exists(filePath)) {
            throw std::runtime_error("patterns file not found: " + filePath.string());
        }

        clear();

        JsonPatternParser jp(filePath);
        jp.parseAll();

    } catch (const std::exception& e) {
        Logger::log("GrammarPatternManager", LogLevel::Error, std::string("readPatterns failed: ") + e.what());
    } catch (...) {
        Logger::log("GrammarPatternManager", LogLevel::Error, "readPatterns failed: unknown exception");
    }
}

void GrammarPatternManager::printPatterns() const {
    Logger::log("GrammarPatternManager", LogLevel::Info, "printPatterns: " + std::to_string(patterns.size()));

    for (const auto& [key, model] : patterns) {
        if (!model)
            continue;
        Logger::log("model form", LogLevel::Info, model->getForm());
        model->printWords();
    }
}

void GrammarPatternManager::addUsedSp(const std::string sp, const bool isHead) {
    if (const auto& res = isHead ? usedHeadSpVars.insert(sp) : usedSpVars.insert(sp); res.second)
        Logger::log("GrammarPatternManager", LogLevel::Debug, "Addded new part of speach: " + sp);
}

const StringSet& GrammarPatternManager::getUsedHeadSp() const noexcept {
    return usedHeadSpVars;
}

const StringSet& GrammarPatternManager::getUsedSp() const noexcept {
    return usedSpVars;
}

size_t GrammarPatternManager::patternsSize() const noexcept {
    return patterns.size();
}

size_t GrammarPatternManager::simplePatternsSize() const noexcept {
    return simplePatterns.size();
}

size_t GrammarPatternManager::complexPatternsSize() const noexcept {
    return complexPatterns.size();
}

void GrammarPatternManager::divide() {
    for (auto& pattern : patterns) {
        bool isComplex = false;
        for (const auto& comp : pattern.second->getComponents()) {
            if (comp->isModel()) {
                isComplex = true;
                break;
            }
        }

        if (isComplex) {
            complexPatterns[pattern.first] = pattern.second;
        } else {
            simplePatterns[pattern.first] = pattern.second;
        }
    }
}

void GrammarPatternManager::clear() {
    patterns.clear();
    simplePatterns.clear();
    complexPatterns.clear();
    usedHeadSpVars.clear();
    usedSpVars.clear();
}
