import pandas as pd
import json

# File paths
EXCEL_FILE = 'one_word_terms_cleaned.xlsx'
SENTENCES_FILE = '/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/sentences.json'
OUTPUT_FILE = 'terms.json'

# 1) Read Excel file
df = pd.read_excel(EXCEL_FILE)

# 2) Load all sentences from JSON
with open(SENTENCES_FILE, 'r', encoding='utf-8') as f:
    sentences_data = json.load(f)
all_sentences = sentences_data.get('sentences', [])

# 3) Filter terms and build output list
terms_output = []
for noun in df.loc[df['is_term'] == 1, 'noun']:
    # 1) Define the search key:
    # if the word is shorter than 5 letters — use it as is, otherwise — drop the last character
    if len(noun) < 5:
        key = noun
    else:
        key = noun[:-1]

    # 2) Collect contexts
    context = []
    for sent in all_sentences:
        norm = sent.get('normalizedStr', '')
        tokens = norm.split()

        if len(noun) < 5:
            # for short words — exact token match and length check
            for token in tokens:
                if key == token and abs(len(token) - len(noun)) <= 3:
                    context.append({
                        'sentence': sent.get('originalStr', ''),
                        'document': sent.get('docNum')
                    })
                    break
        else:
            # for others — substring in token + length check
            for token in tokens:
                if key in token and abs(len(token) - len(noun)) <= 3:
                    context.append({
                        'sentence': sent.get('originalStr', ''),
                        'document': sent.get('docNum')
                    })
                    break

    # 3) If more than one context, filter by sentence length (>5 words)
    if len(context) > 1:
        context = [
            c for c in context
            if len(c['sentence'].split()) > 5
        ]

    terms_output.append({
        'noun': noun,
        'context': context
    })

# 4) Save to JSON
with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
    json.dump(terms_output, f, ensure_ascii=False, indent=2)

print(f"Saved {len(terms_output)} terms with contexts to {OUTPUT_FILE}")
