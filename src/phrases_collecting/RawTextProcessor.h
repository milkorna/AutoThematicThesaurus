#pragma once

#include "Embedding.h"
#include "LSA.h"
#include "PhrasesCollectorUtils.h"
#include "ThreadController.h"

#include <nlohmann/json.hpp>

class RawTextProcessor {
  public:
    static RawTextProcessor& GetProcessor() {
        static RawTextProcessor processor;
        return processor;
    }

    void processRawData();

  private:
    void processFile(const fs::path& inputFile, const fs::path& outputDir);

    void collect(const std::vector<X::WordFormPtr>& forms, Process& process);
    void finalizeDocumentProcessing();

    // \brief Default constructor.
    RawTextProcessor() {
    }

    // \brief Default destructor.
    ~RawTextProcessor() {
    }

    int lastDocumentId = -1;
    std::unordered_set<std::string> uniqueLemmasInDoc;

    // \brief Deleted copy constructor to enforce singleton pattern.
    RawTextProcessor(const RawTextProcessor&) = delete;

    // \brief Deleted assignment operator to enforce singleton pattern.
    RawTextProcessor& operator=(const RawTextProcessor&) = delete;

    Options& options = Options::getOptions();
};
