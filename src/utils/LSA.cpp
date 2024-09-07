#include <LSA.h>
#include <PatternPhrasesStorage.h>
#include <StringFilters.h>

// Method to create a term-document frequency matrix, excluding rare words
std::pair<MatrixXd, std::vector<std::string>> LSA::CreateTermDocumentMatrix(bool useSentences)
{
    std::unordered_map<std::string, int> wordFrequency; // Word frequency count
    std::unordered_map<std::string, int> wordIndex;     // Word indices for the matrix
    std::vector<std::string> words;                     // List of unique words
    int index = 0;
    const auto& stopWords = GetStopWords();

    // Container for texts: can contain either documents or sentences depending on the useSentences flag
    std::unordered_map<size_t, std::string> texts;

    if (useSentences) {
        // Use sentences as documents
        for (const auto& [docNum, sentencesMap] : corpus.sentenceMap) {
            for (const auto& [sentNum, sentence] : sentencesMap) {
                size_t uniqueSentId = docNum * 100000 + sentNum; // Unique identifier for each sentence
                texts[uniqueSentId] = sentence.normalizedStr;    // Add each sentence as a separate "document"
            }
        }
    } else {
        // Use entire documents
        for (const auto& [docNum, sentencesMap] : corpus.sentenceMap) {
            std::string combinedText;
            for (const auto& [sentNum, sentence] : sentencesMap) {
                combinedText += sentence.normalizedStr + " "; // Combine sentences into one text
            }
            texts[docNum] = combinedText; // Save the combined text for each document
        }
    }

    // Count word frequency in each text
    for (const auto& [textNum, text] : texts) {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, text, boost::is_any_of(" "));
        for (const auto& word : tokens) {
            // Filter the word based on stop words and other conditions
            if (!word.empty() && word.size() > 5 && LSAStopWords.find(word) == LSAStopWords.end() &&
                stopWords.find(word) == stopWords.end() && !StringFilters::ContainsUnwantedCharacters(word) &&
                !StringFilters::ShouldFilterOut(word)) {
                wordFrequency[word]++;
            }
        }
    }

    // Create an index of unique words, excluding rare words
    for (const auto& [word, freq] : wordFrequency) {
        // Ignore words that appear only once
        if (freq > 1 && word.size() > 5 && !StringFilters::ContainsUnwantedCharacters(word) &&
            !StringFilters::ShouldFilterOut(word)) {
            wordIndex[word] = index++;
            words.push_back(word); // Add the word to the list of words
        }
    }

    // Initialize the frequency matrix with zeros: rows - words, columns - texts (documents or sentences)
    MatrixXd termDocumentMatrix(wordIndex.size(), texts.size());
    termDocumentMatrix.setZero();

    // Fill the matrix with word frequencies at the text level
    int textIndex = 0;
    for (const auto& [textNum, text] : texts) {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, text, boost::is_any_of(" "));
        for (const auto& word : tokens) {
            // Check if the word is in the index and is not a stop word or contains unwanted characters
            if (!word.empty() && word.size() > 5 && LSAStopWords.find(word) == LSAStopWords.end() &&
                stopWords.find(word) == stopWords.end() && wordIndex.find(word) != wordIndex.end() &&
                !StringFilters::ContainsUnwantedCharacters(word) && !StringFilters::ShouldFilterOut(word)) {
                int wordIdx = wordIndex[word];
                termDocumentMatrix(wordIdx, textIndex) += 1; // Increase the frequency of the word in the text
            }
        }
        ++textIndex;
    }

    return {termDocumentMatrix, words}; // Return the matrix and the list of words
}

// Method to perform SVD
void LSA::ComputeSVD(const MatrixXd& termDocumentMatrix)
{
    // Measure computation time
    auto start = std::chrono::high_resolution_clock::now();
    try {
        std::cout << "Starting SVD computation..." << std::endl;

        // Perform SVD with tracking
        Eigen::JacobiSVD<MatrixXd> svd(termDocumentMatrix, Eigen::ComputeThinU | Eigen::ComputeThinV);

        U = svd.matrixU();
        Sigma = svd.singularValues().asDiagonal();
        V = svd.matrixV();

        // Output the sizes of matrices after SVD
        std::cout << "Matrix U size: " << U.rows() << "x" << U.cols() << std::endl;
        std::cout << "Matrix Sigma size: " << Sigma.rows() << "x" << Sigma.cols() << std::endl;
        std::cout << "Matrix V size: " << V.rows() << "x" << V.cols() << std::endl;

        // Determine the number of components, not exceeding available sizes
        int k = std::min(100, (int)Sigma.diagonal().size());
        if (k > U.cols()) {
            std::cerr
                << "Warning: Requested number of components exceeds available columns in U. Adjusting k to U.cols()."
                << std::endl;
            k = U.cols();
        }
        if (k > V.cols()) {
            std::cerr
                << "Warning: Requested number of components exceeds available columns in V. Adjusting k to V.cols()."
                << std::endl;
            k = V.cols();
        }

        // Avoid direct assignment with leftCols, use temporary matrices
        MatrixXd U_trimmed = U.leftCols(k);
        MatrixXd Sigma_trimmed = Sigma.topLeftCorner(k, k);
        MatrixXd V_trimmed = V.leftCols(k);

        // Assign trimmed matrices
        U = U_trimmed;
        Sigma = Sigma_trimmed;
        V = V_trimmed;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "SVD computation completed in " << elapsed.count() << " seconds." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during SVD computation: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error during SVD computation." << std::endl;
    }
}

// Main method to perform the analysis
void LSA::PerformAnalysis(bool useSentences)
{
    // Create the term-document frequency matrix and get the words list
    auto [termDocumentMatrix, words] = CreateTermDocumentMatrix(useSentences);
    std::cout << "Term-document frequency matrix created with size: " << termDocumentMatrix.rows() << "x"
              << termDocumentMatrix.cols() << std::endl;

    // Perform SVD
    ComputeSVD(termDocumentMatrix);
    std::cout << "SVD performed with components: " << 100 << std::endl;

    // Save the list of words in the class for further use
    this->words = words;
}

void LSA::AnalyzeTopics(const MatrixXd& U, const std::vector<std::string>& words, int numTopics, int topWords)
{
    // Check that the number of words matches the number of rows in matrix U
    if (U.rows() != words.size()) {
        std::cerr << "Error: The number of rows in U (" << U.rows() << ") does not match the number of words ("
                  << words.size() << ")." << std::endl;
        return;
    }

    // Analyze topics
    for (int topic = 0; topic < numTopics; ++topic) {
        std::vector<std::pair<double, std::string>> topicWords;

        // Collect words and their values for the current topic
        for (int i = 0; i < U.rows(); ++i) {
            // Check index validity
            if (i < words.size()) {
                topicWords.push_back({U(i, topic), words[i]});
            } else {
                std::cerr << "Warning: Index " << i << " is out of bounds for words array." << std::endl;
            }
        }

        // Sort words by importance for the current topic
        std::sort(topicWords.begin(), topicWords.end(), std::greater<>());

        // Output the most significant words for the current topic
        std::cout << "Topic " << topic + 1 << ": ";
        for (int i = 0; i < topWords && i < topicWords.size(); ++i) {
            std::cout << topicWords[i].second << " (" << topicWords[i].first << "), ";
        }
        std::cout << std::endl;
    }
}

double LSA::CosineSimilarity(const VectorXd& vec1, const VectorXd& vec2)
{
    return vec1.dot(vec2) / (vec1.norm() * vec2.norm());
}

void LSA::CompareDocuments(const MatrixXd& V)
{
    // Open file to write results
    std::ofstream outFile("document_similarity.txt");

    // Check if file was opened successfully
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file 'document_similarity.txt' for writing." << std::endl;
        return;
    }

    // Calculate similarity between each pair of documents
    for (int i = 0; i < V.cols(); ++i) {
        for (int j = i + 1; j < V.cols(); ++j) {
            double similarity = CosineSimilarity(V.col(i), V.col(j));
            // Write results to file
            outFile << "Similarity between document " << i + 1 << " and document " << j + 1 << ": " << similarity
                    << std::endl;
        }
    }

    // Close file after writing is done
    outFile.close();

    std::cout << "Document similarities have been written to 'document_similarity.txt'." << std::endl;
}

void LSA::FindSimilarWords(const MatrixXd& U, const std::vector<std::string>& words, const std::string& targetWord)
{
    int targetIndex = -1;
    for (size_t i = 0; i < words.size(); ++i) {
        if (words[i] == targetWord) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex == -1) {
        std::cout << "Word not found in the list." << std::endl;
        return;
    }

    VectorXd targetVector = U.row(targetIndex);
    std::vector<std::pair<double, std::string>> similarities;
    for (int i = 0; i < U.rows(); ++i) {
        if (i == targetIndex)
            continue;
        double similarity = CosineSimilarity(targetVector, U.row(i));
        similarities.push_back({similarity, words[i]});
    }

    std::sort(similarities.begin(), similarities.end(), std::greater<>());
    std::cout << "Words similar to " << targetWord << ":" << std::endl;
    for (size_t i = 0; i < 10 && i < similarities.size(); ++i) {
        std::cout << similarities[i].second << " (" << similarities[i].first << ")" << std::endl;
    }
}
