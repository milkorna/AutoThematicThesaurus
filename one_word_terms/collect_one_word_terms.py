import json
import pandas as pd
import pymorphy2
import inspect
import re
import math

# Patch pymorphy2 for compatibility with Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults  # Совместимый формат
    inspect.getargspec = getargspec_patched

# File paths
json_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/filtered_corpus.json"
excel_path = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
sentences_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/sentences.json"
output_path = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_one_word_terms.xlsx"

# Initialize morphological analyzer
morph = pymorphy2.MorphAnalyzer()

def is_noun(word):
    """
    Checks whether the given word is a noun using pymorphy2.
    Returns True if the word is classified as a noun, otherwise False.
    """
    parsed = morph.parse(word)[0]
    return 'NOUN' in parsed.tag

# Load JSON file with error handling
try:
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    print("JSON file loaded successfully")
except json.JSONDecodeError as e:
    print(f"JSON parsing error: {e}")
    exit(1)
except Exception as e:
    print(f"Error loading JSON file: {e}")
    exit(1)

# Load sentences JSON with error handling
try:
    with open(sentences_path, "r", encoding="utf-8") as f:
        sentences_data = json.load(f)
    print("Sentences JSON file loaded successfully")
except json.JSONDecodeError as e:
    print(f"JSON parsing error in sentences file: {e}")
    exit(1)
except Exception as e:
    print(f"Error loading sentences JSON file: {e}")
    exit(1)

# Group sentences by document number
documents = {}
for sentence in sentences_data["sentences"]:
    doc_id = sentence["docNum"]
    normalized_text = sentence["normalizedStr"]
    if doc_id not in documents:
        documents[doc_id] = []
    documents[doc_id].append(normalized_text)

# Merge sentences into full document texts
doc_texts = {doc_id: " ".join(sentences) for doc_id, sentences in documents.items()}
N = len(doc_texts)  # Total number of documents

# Extract words from '4_wordFrequency' with frequency > 2 and filter only nouns
try:
    word_freq = data["4_wordFrequency"]
    nouns = {word for word, freq in word_freq.items() if freq > 2 and is_noun(word) and len(word) > 2}
    print(f"Extracted {len(nouns)} nouns")
except KeyError as e:
    print(f"Error: Missing key {e} in JSON")
    exit(1)

# Load filtered_data.xlsx with error handling
try:
    filtered_data = pd.read_excel(excel_path, usecols=["key", "oof_prob_class"])
    print("Excel file loaded successfully")
except Exception as e:
    print(f"Error loading Excel file: {e}")
    exit(1)

# Extract all normalized words from the "key" column
normalized_words = set()
word_counts = {}
word_prob_stats = {}
try:
    for _, row in filtered_data.dropna().iterrows():
        words = re.findall(r'\b\w+\b', str(row["key"]))
        normalized_words.update(words)
        for word in words:
            word_counts[word] = word_counts.get(word, 0) + 1
            if word not in word_prob_stats:
                word_prob_stats[word] = []
            word_prob_stats[word].append(row["oof_prob_class"])
    print(f"Extracted {len(normalized_words)} normalized words from 'key' column")
except Exception as e:
    print(f"Error processing 'key' column in Excel: {e}")
    exit(1)

# Filter nouns that exist in the key column
valid_nouns = nouns & normalized_words

# Compute TF-IDF for filtered nouns
idf = {word: math.log(N / (1 + sum(1 for doc in doc_texts.values() if word in doc))) for word in valid_nouns}
tf_idf = []
for word in sorted(valid_nouns):
    prob_values = word_prob_stats.get(word, [0])
    prob_mean = sum(prob_values) / len(prob_values)
    prob_max = max(prob_values)
    prob_min = min(prob_values)
    phrase_count = word_counts.get(word, 0)
    if phrase_count > 1:
        tf_idf.append((word, phrase_count, idf[word], prob_mean, prob_max, prob_min))

print(f"Final noun list contains {len(tf_idf)} words")

# Save results to an Excel file with error handling
try:
    df = pd.DataFrame(tf_idf, columns=["noun", "phrase_count", "tf_idf", "prob_mean", "prob_max", "prob_min"])
    df.to_excel(output_path, index=False)
    print(f"Filtered noun list saved to {output_path}")
except Exception as e:
    print(f"Error saving Excel file: {e}")
    exit(1)
