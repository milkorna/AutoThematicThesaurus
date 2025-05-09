import json
import pandas as pd

# File paths
TERMS_FILE       = 'terms.json'
HEAD_FILE        = '/home/milkorna/Documents/AutoThematicThesaurus/relations/hypernym_hyponym_analysis/head_nouns_filtered.json'
DATA_FILE        = '/home/milkorna/Documents/AutoThematicThesaurus/thesaurus/data.xlsx'
OUTPUT_FILE      = 'terms_augmented.json'

# 1) Load initial data
with open(TERMS_FILE, 'r', encoding='utf-8') as f:
    terms = json.load(f)

with open(HEAD_FILE, 'r', encoding='utf-8') as f:
    head_data = json.load(f)

df_data = pd.read_excel(DATA_FILE)
valid_phrases = set(df_data['phrase'].astype(str).tolist())

# List of all nouns to filter hypernyms/hyponyms/synonyms
nouns_set = { entry['noun'] for entry in terms }

# 2) Augment each term
for entry in terms:
    noun = entry['noun']
    info = head_data.get(noun, {})

    # Filter lists of dictionaries by 'word' presence in nouns_set
    def extract_words(key):
        result = []
        for d in info.get(key, []):
            w = d.get('word')
            if w and w in nouns_set and w != noun:
                result.append(w)
        return result

    entry['hypernyms'] = extract_words('hypernyms')
    entry['hyponyms']  = extract_words('hyponyms')
    entry['synonyms']  = extract_words('synonyms')

    # Filter related_terms by presence in data.xlsx
    related = [rel for rel in info.get('related_terms', []) if rel in valid_phrases]
    entry['related_terms'] = related

# 3) Save the result
with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
    json.dump(terms, f, ensure_ascii=False, indent=2)
