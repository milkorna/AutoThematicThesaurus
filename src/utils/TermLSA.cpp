#include <TermLSA.h>

std::pair<MatrixXd, std::vector<std::string>> TermLSA::CreateTermDocumentMatrix()
{
    // Map to store term frequencies across documents: key is term, value is a map of document IDs to frequencies.
    std::unordered_map<std::string, std::unordered_map<size_t, int>> termFrequency;

    // Map to store indices of terms for the matrix: key is term, value is its index in the matrix rows.
    std::unordered_map<std::string, int> termIndex;

    int index = 0; // Counter to assign indices to each unique term.

    std::cout << "Starting to build the term-document matrix..." << std::endl;

    // Collect term frequencies across documents.
    for (const auto& cluster : PatternPhrasesStorage::GetStorage().GetClusters()) {
        const std::string& term = cluster.first; // The term associated with the current cluster.

        // If the term is not already indexed, assign it a new index and add it to the terms list.
        if (termIndex.find(term) == termIndex.end()) {
            termIndex[term] = index++;
            terms.push_back(term); // Store the term in the terms vector.
        }

        // Count the frequency of the term in each document and create document mapping.
        for (const auto& wordComplex : cluster.second.wordComplexes) {
            size_t docNum = wordComplex->pos.docNum; // Document number where the term appears.
            termFrequency[term][docNum]++;           // Increment the term's frequency in the current document.

            // Add the document index to the map if it is not already present.
            if (docIndexMap.find(docNum) == docIndexMap.end()) {
                int docIndex = docIndexMap.size(); // Assign the next available index for this document.
                docIndexMap[docNum] = docIndex;    // Map the document number to its index in the matrix columns.
            }
        }
    }

    // Check if any terms were indexed; otherwise, the matrix will be empty.
    if (termIndex.empty()) {
        std::cerr << "Warning: No terms were indexed, term-document matrix will be empty." << std::endl;
    } else {
        std::cout << "Indexed " << termIndex.size() << " unique terms." << std::endl;
    }

    // Initialize the term-document frequency matrix: rows represent terms, columns represent documents.
    MatrixXd termDocumentMatrix(termIndex.size(), docIndexMap.size());
    termDocumentMatrix.setZero(); // Set all elements to zero initially.

    // Fill the term-document matrix with frequencies.
    for (const auto& [term, docFreqMap] : termFrequency) {
        int termIdx = termIndex[term]; // Get the row index of the term.

        // Iterate over each document-frequency pair for the current term.
        for (const auto& [docNum, freq] : docFreqMap) {
            // Check if the document number is correctly mapped in docIndexMap.
            if (docIndexMap.find(docNum) == docIndexMap.end()) {
                std::cerr << "Error: Document index " << docNum << " exceeds mapped indices size." << std::endl;
                continue; // Skip this entry if the document index is not found.
            }
            int colIdx = docIndexMap[docNum]; // Get the column index for the document.

            // Check that the column index does not exceed the matrix dimensions.
            if (colIdx >= termDocumentMatrix.cols()) {
                std::cerr << "Error: Mapped column index " << colIdx << " for document " << docNum
                          << " exceeds matrix column size." << std::endl;
                continue; // Skip this entry if the column index is out of bounds.
            }

            // Assign the term frequency to the appropriate position in the matrix.
            termDocumentMatrix(termIdx, colIdx) = freq;
        }
    }

    // Log the size of the created term-document matrix.
    std::cout << "Term-document frequency matrix created with size: " << termDocumentMatrix.rows() << "x"
              << termDocumentMatrix.cols() << std::endl;

    // Return the term-document matrix and the list of terms.
    return {termDocumentMatrix, terms};
}

// Method to perform SVD
void TermLSA::ComputeSVD(const MatrixXd& termDocumentMatrix)
{
    // Measure computation time
    auto start = std::chrono::high_resolution_clock::now();
    try {
        std::cout << "Starting SVD computation for TermLSA..." << std::endl;

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
        int k = std::min(300, static_cast<int>(Sigma.diagonal().size()));
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
        if (k > Sigma.rows() || k > Sigma.cols()) {
            std::cerr << "Warning: Requested number of components exceeds available dimensions in Sigma. Adjusting k "
                         "to min(Sigma.rows(), Sigma.cols())."
                      << std::endl;
            k = std::min(Sigma.rows(), Sigma.cols());
        }

        // Logging to check final value of k
        std::cout << "Final value of k used for matrix trimming: " << k << std::endl;

        // Avoid direct assignment with leftCols, use temporary matrices
        MatrixXd U_trimmed = U.leftCols(k);
        MatrixXd Sigma_trimmed = Sigma.topLeftCorner(k, k);
        MatrixXd V_trimmed = V.leftCols(k);

        // Validate dimensions before assignment
        if (U_trimmed.cols() == k && Sigma_trimmed.rows() == k && Sigma_trimmed.cols() == k && V_trimmed.cols() == k) {
            // Assign trimmed matrices
            U = U_trimmed;
            Sigma = Sigma_trimmed;
            V = V_trimmed;
        } else {
            std::cerr << "Error: Trimmed matrix sizes do not match expected dimensions. Aborting assignment."
                      << std::endl;
            return;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "SVD computation completed in " << elapsed.count() << " seconds." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during SVD computation: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error during SVD computation." << std::endl;
    }
}

std::unordered_map<std::string, double> TermLSA::CalculateTermRelevanceToTopics()
{
    std::unordered_map<std::string, double>
        termRelevance; // Container to store the relevance scores of terms to topics.

    // Check that the number of rows in matrix U matches the number of terms before calculations begin.
    if (U.rows() != terms.size()) {
        std::cerr << "Error: U rows (" << U.rows() << ") do not match the number of terms (" << terms.size() << ")."
                  << std::endl;
        return termRelevance; // Return an empty map if sizes do not match.
    }

    // Check that there are topics available before proceeding with normalization.
    if (topics.empty()) {
        std::cerr << "Warning: No topics found. Cannot calculate topic relevance." << std::endl;
        return termRelevance; // Return an empty map if no topics are found.
    }

    // Calculate the relevance of each term to the topics.
    for (int i = 0; i < U.rows(); ++i) {
        double relevance = 0.0; // Variable to accumulate the relevance score for each term.

        // Check that the number of topics does not exceed the number of columns in matrix U.
        if (topics.size() > U.cols()) {
            std::cerr << "Error: Number of topics (" << topics.size() << ") exceeds U columns (" << U.cols() << ")."
                      << std::endl;
            break; // Stop processing if topics exceed the available columns in U.
        }

        // Iterate over each topic to calculate relevance.
        for (int topic = 0; topic < topics.size(); ++topic) {
            // Check the validity of the indices before accessing the matrix element.
            if (i < 0 || i >= U.rows() || topic < 0 || topic >= U.cols()) {
                std::cerr << "Error: Index out of bounds for U matrix access at (" << i << ", " << topic << ")."
                          << std::endl;
                continue; // Skip the current index pair if it is out of bounds.
            }
            // Add the absolute value of the term's relevance score to the current topic.
            relevance += std::abs(U(i, topic));
        }

        // Normalize the relevance score by the number of topics to prevent scale discrepancies.
        if (topics.size() > 0) {
            termRelevance[terms[i]] = relevance / topics.size(); // Assign the normalized score to the term.
        } else {
            std::cerr << "Warning: Attempting to normalize by zero topics." << std::endl;
        }
    }

    // Return the map containing the relevance scores of each term to the topics.
    return termRelevance;
}

// Function to calculate the cosine similarity between two vectors (terms)
// Cosine similarity measures the cosine of the angle between two vectors,
// providing a value between -1 and 1 that indicates how similar the vectors are.
double TermLSA::CosineSimilarity(const VectorXd& vec1, const VectorXd& vec2)
{
    // Compute the dot product of the two vectors and divide by the product of their norms (magnitudes).
    // This results in a similarity measure, where 1 indicates identical direction and 0 indicates orthogonality.
    return vec1.dot(vec2) / (vec1.norm() * vec2.norm());
}

// Function to find terms similar to the specified target term based on cosine similarity
void TermLSA::FindSimilarTerms(const std::string& targetTerm)
{
    int targetIndex = -1; // Index of the target term in the list of terms.

    // Search for the index of the target term within the list of terms.
    for (size_t i = 0; i < terms.size(); ++i) {
        if (terms[i] == targetTerm) {
            targetIndex = i; // Save the index if the term is found.
            break;
        }
    }

    // If the term is not found, output a message and exit the function.
    if (targetIndex == -1) {
        std::cout << "Term not found in the list." << std::endl;
        return;
    }

    // Retrieve the vector corresponding to the target term from the U matrix.
    VectorXd targetVector = U.row(targetIndex);
    std::vector<std::pair<double, std::string>> similarities; // Vector to store similarity scores and terms.

    // Calculate the similarity of the target term to all other terms in the matrix.
    for (int i = 0; i < U.rows(); ++i) {
        // Skip the comparison with the term itself.
        if (i == targetIndex)
            continue;

        // Compute the cosine similarity between the target term vector and the current term vector.
        double similarity = CosineSimilarity(targetVector, U.row(i));
        // Store the similarity score and the corresponding term.
        similarities.push_back({similarity, terms[i]});
    }

    // Sort the terms by their similarity scores in descending order.
    std::sort(similarities.begin(), similarities.end(), std::greater<>());

    // Output the top 10 most similar terms.
    std::cout << "Terms similar to \"" << targetTerm << "\":" << std::endl;
    for (size_t i = 0; i < 10 && i < similarities.size(); ++i) {
        std::cout << similarities[i].second << " (" << similarities[i].first << ")" << std::endl;
    }
}

void TermLSA::AnalyzeTopics(int numTopics, int topWords)
{
    // Check if the number of rows in matrix U matches the number of terms.
    // This ensures that each row of U corresponds to a term.
    if (U.rows() != terms.size()) {
        std::cerr << "Error: The number of rows in U does not match the number of terms." << std::endl;
        return;
    }

    // Analyze the topics based on the specified number of topics.
    for (int topic = 0; topic < numTopics; ++topic) {
        std::vector<std::pair<double, std::string>> topicTerms;

        // Collect terms and their values for the current topic.
        // Each value in U(i, topic) represents the importance of the term in the current topic.
        for (int i = 0; i < U.rows(); ++i) {
            topicTerms.push_back({U(i, topic), terms[i]});
        }

        // Sort the terms by their importance in the current topic in descending order.
        std::sort(topicTerms.begin(), topicTerms.end(), std::greater<>());

        // Save the most significant terms for the current topic.
        std::vector<std::string> topTermsForTopic;
        std::cout << "Topic " << topic + 1 << ": ";
        for (int i = 0; i < topWords && i < topicTerms.size(); ++i) {
            std::cout << topicTerms[i].second << " (" << topicTerms[i].first << "), ";
            topTermsForTopic.push_back(topicTerms[i].second); // Save the top term for the topic.
        }
        std::cout << std::endl;

        // Store the top terms for each topic in the class variable.
        topics[topic] = topTermsForTopic;
    }
}

void TermLSA::PerformAnalysis(int numComponents)
{
    // Create the term-document matrix, where rows represent terms and columns represent documents.
    auto [termDocumentMatrix, terms] = CreateTermDocumentMatrix();

    // Check if the term-document matrix is empty; if so, terminate the analysis with an error message.
    if (termDocumentMatrix.size() == 0) {
        std::cerr << "Error: Term-document matrix is empty, analysis cannot be performed." << std::endl;
        return;
    }

    // Perform Singular Value Decomposition (SVD) on the term-document matrix.
    // This step decomposes the matrix into U, Sigma, and V matrices which are used for topic analysis.
    ComputeSVD(termDocumentMatrix);
    std::cout << "SVD performed with components: " << numComponents << std::endl;
}
