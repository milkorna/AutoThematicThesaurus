#ifndef GRAMMARCOMPONENTS_H
#define GRAMMARCOMPONENTS_H

#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>
#include <Logger.h>

using namespace X;

// Enum to describe syntactic roles of components within a sentence.
enum class SyntaxRole
{
    Head,       // The central word of a phrase.
    Dependent,  // Dependent on the head.
    Independent // Neither dependent nor a head.
};

// Struct to manage additional grammatical conditions.
struct Additional
{
    bool m_rec = false;                     // Flag for recursion, if needed.
    std::string m_exLex = "";               // Example lexicon (specific words to match).
    std::vector<std::string> m_themes = {}; // Themes associated with this condition.

    // Checks if the Additional instance has no data.
    bool isEmpty() const { return m_themes.empty() && m_exLex.empty(); }

    // Checks if a specific word form matches the example lexicon.
    bool exLexCheck(const X::MorphInfo &morphForm) const
    {
        if (const X::UniString exLex(m_exLex); !exLex.isEmpty())
        {
            if (exLex == morphForm.normalForm)
            {
                // exLex matched return true;
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    // Placeholder for logic to compare themes.
    bool themesCheck() const
    {
        if (const auto &theme = m_themes; !theme.empty())
        {
            // TODO: Add logic to compare themes
        }
        return true;
    }
};

// Class to define conditions for matching grammatical components.
class Condition
{
private:
    SyntaxRole m_role;
    UniMorphTag m_tag;
    Additional m_addcond;

public:
    Condition(SyntaxRole role = SyntaxRole::Independent, UniMorphTag morphTag = UniMorphTag::UNKN, Additional cond = Additional())
        : m_role(role), m_tag(morphTag), m_addcond(cond){};

    const UniMorphTag getMorphTag() const { return m_tag; };
    const Additional getAdditional() const { return m_addcond; };
    const SyntaxRole getSyntaxRole() const { return m_role; };

    // Checks if the Condition instance contains default or empty values.
    bool isEmpty() const { return m_tag == UniMorphTag::UNKN && m_addcond.isEmpty(); }

    const bool matches() const;

    // Method to determine if a given morphological form matches the condition.
    bool morphTagCheck(const X::MorphInfo &morphForm) const;
};

class Component;

// Type definition for a vector of shared pointers to Components.
using Components = std::vector<std::shared_ptr<Component>>;

// Abstract base class for grammatical components.
class Component
{
public:
    virtual ~Component() = default;
    virtual const UniSPTag getSPTag() const = 0;
    virtual const std::string getForm() const = 0;
    virtual const Components getComponents() const = 0;

    virtual const bool isWord() const = 0;
    virtual const bool isModel() const = 0;
    virtual const std::optional<bool> isHead() const = 0;
};

// Derived class representing a Word in the grammar system.
class Word : public Component
{
    UniSPTag m_sp;

public:
    Word(UniSPTag sp = UniSPTag::X) : m_sp(sp) {}

    const UniSPTag getSPTag() const override { return m_sp; }
    const std::string getForm() const override { return ""; }
    const Components getComponents() const override { return {}; }

    const bool isWord() const override
    {
        return true;
    }
    const bool isModel() const override { return false; }
    const std::optional<bool> isHead() const { return std::nullopt; }
};

// Derived class representing a Word with specific conditions.
class WordComp : public Word
{
    Condition m_cond;

public:
    WordComp(const UniSPTag &sp = UniSPTag::X, const Condition &cond = Condition()) : Word(sp), m_cond(cond) {}

    const Condition getCondition() const { return m_cond; }
    const bool isRec() { return m_cond.getAdditional().m_rec; }
    const std::optional<bool> isHead() const
    {
        auto role = m_cond.getSyntaxRole();
        if (role == SyntaxRole::Head)
        {
            return true;
        }
        if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent)
        {
            return false;
        }
        return std::nullopt;
    }

    ~WordComp() override {}

    void print() const;
};

// Derived class representing a grammatical model.
class Model : public Component
{
    std::string m_form;
    Components m_comps;

public:
    Model(const std::string &form = "", const Components &comps = {}) : m_form(form), m_comps(comps){};

    const UniSPTag getSPTag() const override { return UniSPTag::X; }
    const std::string getForm() const override { return m_form; }
    const Components getComponents() const override { return m_comps; }

    const bool isWord() const override { return false; }
    const bool isModel() const override { return true; }
    const std::optional<bool> isHead() const { return std::nullopt; }

    void addCondition(const std::shared_ptr<Condition> &Condition);
    bool checkComponentsMatch(const WordFormPtr &wordForm) const;

    void addComponent(const std::shared_ptr<Component> &component);

    std::optional<size_t> getModelCompIndByForm(const std::string &form) const;

    std::shared_ptr<WordComp> getHead() const;
    std::optional<size_t> getHeadPos() const;
    size_t getSize() const;

    void printWords() const;
};

// Derived class representing a grammatical model with specific conditions.
class ModelComp : public Model
{
    Condition m_cond;

public:
    ModelComp(const std::string &form = "", const Components &comps = {}, const Condition &cond = {}) : Model(form, comps), m_cond(cond){};

    const Condition getCondition() const { return m_cond; }

    const std::optional<bool> isHead() const
    {
        auto role = m_cond.getSyntaxRole();
        if (role == SyntaxRole::Head)
        {
            return true;
        }
        if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent)
        {
            return false;
        }
        return std::nullopt;
    }

    void addComponent(const std::shared_ptr<Component> &component);
};

#endif // GRAMMARCOMPONENTS_H
