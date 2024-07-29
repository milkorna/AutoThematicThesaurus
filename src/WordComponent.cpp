#include <WordComponent.h>

using namespace X;

void WordComp::print() const
{
    Logger::log("GrammarComponent", LogLevel::Info, "\t\tword sp: " + this->getSPTag().toString());

    if (const auto& cond = this->getCondition(); !cond.empty()) {
        Logger::log("GrammarComponent", LogLevel::Info, ", mt: " + cond.getMorphTag().toString());

        if (const auto& addCond = cond.getAdditional(); !addCond.empty()) {
            Logger::log("GrammarComponent", LogLevel::Info, ", lex: " + addCond.m_exLex);
        }
    }
    std::cout << std::endl;
}
