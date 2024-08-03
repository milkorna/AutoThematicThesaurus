#include <WordComponent.h>

using namespace X;

Word::Word(UniSPTag sp) : m_sp(sp)
{
}

const X::UniSPTag Word::getSPTag() const
{
    return m_sp;
}

const std::string Word::getForm() const
{
    return "";
}

const Components Word::getComponents() const
{
    return {};
}

const bool Word::isWord() const
{
    return true;
}

const bool Word::isModel() const
{
    return false;
}

void WordComp::print() const
{
    Logger::log("\t\tsp", LogLevel::Info, this->getSPTag().toString());

    if (const auto& cond = this->getCondition(); !cond.empty()) {
        Logger::log("\t\t\t\tmt", LogLevel::Info, cond.getMorphTag().toString());

        if (const auto& addCond = cond.getAdditional(); !addCond.empty()) {
            Logger::log("\t\t\t\tlex", LogLevel::Info, addCond.m_exLex);
        }
    }
}

WordComp::WordComp(const UniSPTag& sp, const Condition& cond) : Word(sp), m_cond(cond)
{
}

const Condition WordComp::getCondition() const
{
    return m_cond;
}

const bool WordComp::isRec() const
{
    return m_cond.getAdditional().m_rec;
}

const std::optional<bool> WordComp::isHead() const
{
    auto role = m_cond.getSyntaxRole();
    if (role == SyntaxRole::Head) {
        return true;
    }
    if (role == SyntaxRole::Dependent || role == SyntaxRole::Independent) {
        return false;
    }
    return std::nullopt;
}