#ifndef GRAMMARCOMPONENTS_H
#define GRAMMARCOMPONENTS_H

#include <vector>
#include <memory>
#include <string>
#include <xmorphy/morph/WordForm.h>
#include <xmorphy/tag/UniMorphTag.h>
#include <xmorphy/tag/UniSPTag.h>
#include <Logger.h>

using namespace X;

enum class SyntaxRole
{
    Head,
    Dependent,
    Independent
};

struct Additional
{
    bool m_rec = false;
    std::string m_exLex = "";
    std::vector<std::string> m_themes = {};

    bool isEmpty() const { return m_themes.empty() && m_exLex.empty(); }
    bool exLexCheck(const X::MorphInfo &morphForm) const
    {
        if (const X::UniString exLex(m_exLex); !exLex.isEmpty())
        {
            if (exLex == morphForm.normalForm)
            {
                // exLex matched
            }
            else
            {
                return false;
            }
        }
        return true;
    }
    bool themesCheck() const
    {
        if (const auto &theme = m_themes; !theme.empty())
        {
            // TODO: Add logic to compare themes
        }
        return true;
    }
};

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

    bool isEmpty() const { return m_tag == UniMorphTag::UNKN && m_addcond.isEmpty(); }

    const bool matches() const;

    bool morphTagCheck(const X::MorphInfo &morphForm) const;
};

class Component;

using Components = std::vector<std::shared_ptr<Component>>;

class Component
{
public:
    virtual ~Component() = default;
    virtual const UniSPTag getSPTag() const = 0;
    virtual const std::string getForm() const = 0;
    virtual const Components getComponents() const = 0;

    virtual const bool isWord() const = 0;
    virtual const bool isModel() const = 0;
};

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

    const bool matches(const std::vector<WordFormPtr> &lexemes, size_t &position) const;
    const bool matches(const WordFormPtr &lexeme, size_t &position) const;
};

class WordComp : public Word
{
    Condition m_cond;

public:
    WordComp(const UniSPTag &sp = UniSPTag::X, const Condition &cond = Condition()) : Word(sp), m_cond(cond) {}

    const Condition getCondition() const { return m_cond; }
    const bool isRec() { return m_cond.getAdditional().m_rec; }

    ~WordComp() override {}

    void print() const;

    const bool matches(const std::vector<WordFormPtr> &lexemes, size_t &position) const;
    const bool matches(const WordFormPtr &wordForm, size_t &position) const;
};

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

    void addCondition(const std::shared_ptr<Condition> &Condition);
    bool checkComponentsMatch(const WordFormPtr &wordForm) const;

    void addComponent(const std::shared_ptr<Component> &component);

    bool matches(const std::vector<WordFormPtr> &lexemes, size_t &position) const;

    std::shared_ptr<WordComp> getHead() const;
    size_t getHeadPos() const;
    size_t getSize() const;

    void printWords() const;
};

class ModelComp : public Model
{
    Condition m_cond;

public:
    ModelComp(const std::string &form = "", const Components &comps = {}, const Condition &cond = {}) : Model(form, comps), m_cond(cond){};

    const Condition getCondition() const { return m_cond; }

    void addComponent(const std::shared_ptr<Component> &component);
    const bool matches(const std::vector<WordFormPtr> &lexemes, size_t position) const;
};

#endif // GRAMMARCOMPONENTS_H
