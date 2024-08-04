#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

struct TrieNode {
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    std::vector<std::string> wordsEndingHere;
};

void insert(TrieNode& root, const std::string& word)
{
    TrieNode* node = &root;
    for (char ch : word) {
        if (!node->children[ch]) {
            node->children[ch] = std::make_unique<TrieNode>();
        }
        node = node->children[ch].get();
    }
    node->wordsEndingHere.push_back(word);
}

void search(TrieNode& root, const std::string& word, std::unordered_map<std::string, int>& substringCount)
{
    TrieNode* node = &root;
    for (char ch : word) {
        if (!node->children[ch]) {
            return;
        }
        node = node->children[ch].get();
    }

    std::queue<TrieNode*> nodes;
    nodes.push(node);
    while (!nodes.empty()) {
        TrieNode* currentNode = nodes.front();
        nodes.pop();
        for (const auto& w : currentNode->wordsEndingHere) {
            substringCount[w]++;
        }
        for (auto& [_, childNode] : currentNode->children) {
            nodes.push(childNode.get());
        }
    }
}

void updateWeights(std::unordered_map<std::string, double>& umap)
{
    TrieNode root;
    for (const auto& [key, _] : umap) {
        insert(root, key);
    }

    std::unordered_map<std::string, int> substringCount;
    for (const auto& [key, _] : umap) {
        substringCount[key] = 0;
    }

    for (const auto& [key, _] : umap) {
        for (size_t i = 0; i < key.size(); ++i) {
            search(root, key.substr(i), substringCount);
        }
    }

    for (auto& [key, weight] : umap) {
        int count = substringCount[key];
        if (count > 0) {
            weight /= count;
        }
    }
}
