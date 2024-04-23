#ifndef GPAMMARPATTERNMANAGER_H
#define GPAMMARPATTERNMANAGER_H

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <unordered_map>

// Forward declaration
class Model;

class GrammarPatternManager
{
private:
    static GrammarPatternManager *instance;

    std::unordered_map<std::string, std::shared_ptr<Model>> bases;
    std::unordered_map<std::string, std::shared_ptr<Model>> assemblies;

    std::unordered_map<std::string, std::shared_ptr<Model>> patterns;

    // Private constructors for Singleton pattern
    GrammarPatternManager(){};

public:
    // Singleton access method
    static GrammarPatternManager *getInstance();

    // Deleting copy constructor and assignment operator
    GrammarPatternManager(const GrammarPatternManager &) = delete;
    GrammarPatternManager &operator=(const GrammarPatternManager &) = delete;

    // Method to add a pattern to the manager
    void addPattern(const std::string &key, const std::shared_ptr<Model> &model);

    // Method to retrieve a pattern by key
    std::shared_ptr<Model> getPattern(const std::string &key) const;

    const std::unordered_map<std::string, std::shared_ptr<Model>> getBases() const;
    const std::unordered_map<std::string, std::shared_ptr<Model>> getAssemblies() const;

    // Method to parse document strings and create/fill models
    void readPatterns(const std::string &filename);

    void printPatterns() const;

    size_t size() const;

    void divide();
};

#endif // GPAMMARPATTERNMANAGER_H
