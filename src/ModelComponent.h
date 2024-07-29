#ifndef MODEL_COMPONENT_H
#define MODEL_COMPONENT_H

#include <WordComponent.h>

class ModelComp;

// Derived class representing a grammatical model.
class Model : public Component {
    std::string m_form;
    Components m_comps;

public:
    explicit Model(const std::string& form = "", const Components& comps = {});
    ~Model() = default;

    const X::UniSPTag getSPTag() const override;

    const std::string getForm() const override;

    const Components getComponents() const override;

    const std::shared_ptr<Component> getComponent(const size_t ind) const;

    const std::shared_ptr<WordComp> getWordComponent(const size_t ind) const;

    const std::shared_ptr<ModelComp> getModelComponent(const size_t ind) const;

    const bool isWord() const override;

    const bool isModel() const override;

    const std::optional<bool> isHead() const;

    void addComponent(const std::shared_ptr<Component>& component);

    std::optional<size_t> getModelCompIndByForm(const std::string& form) const;

    std::shared_ptr<WordComp> getHead() const;

    std::optional<size_t> getHeadPos() const;

    size_t size() const;

    void printWords() const;
};

// Derived class representing a grammatical model with specific conditions.
class ModelComp : public Model {
    Condition m_cond;

public:
    ModelComp(const std::string& form = "", const Components& comps = {}, const Condition& cond = {});

    const Condition getCondition() const;

    const std::optional<bool> isHead() const;
};

#endif // MODEL_COMPONENT_H
