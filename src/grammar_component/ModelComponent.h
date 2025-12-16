#pragma once

#include "Component.h"
#include "WordComponent.h"

#include <optional>

class ModelComp;

// Derived class representing a grammatical model.
class Model : public Component {
    std::string m_form;
    Components m_comps;

  public:
    explicit Model(const std::string& form = "", const Components& comps = {});
    ~Model() = default;

    [[nodiscard]] const X::UniSPTag getSPTag() const override;

    [[nodiscard]] const std::string getForm() const override;

    [[nodiscard]] const Components getComponents() const override;

    [[nodiscard]] const std::shared_ptr<Component> getComponent(const size_t ind) const;

    [[nodiscard]] const std::shared_ptr<WordComp> getWordComponent(const size_t ind) const;

    [[nodiscard]] const std::shared_ptr<ModelComp> getModelComponent(const size_t ind) const;

    [[nodiscard]] const bool isWord() const override;

    [[nodiscard]] const bool isModel() const override;

    [[nodiscard]] const std::optional<bool> isHead() const override;

    void addComponent(const std::shared_ptr<Component>& component);

    [[nodiscard]] std::optional<size_t> getModelCompIndByForm(const std::string& form) const;

    [[nodiscard]] std::shared_ptr<WordComp> getHead() const;

    [[nodiscard]] std::optional<size_t> getHeadPos() const;

    [[nodiscard]] size_t size() const;

    void printWords() const;
};

// Derived class representing a grammatical model with specific conditions.
class ModelComp : public Model {
    Condition m_cond;

  public:
    ModelComp(const std::string& form = "", const Components& comps = {}, const Condition& cond = {});

    [[nodiscard]] const Condition getCondition() const;

    [[nodiscard]] const std::optional<bool> isHead() const;
};
