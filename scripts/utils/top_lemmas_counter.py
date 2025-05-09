import json
from collections import Counter
from core.paths import PATH_TOTAL_RESULTS

# Load data from the JSON file
with open(PATH_TOTAL_RESULTS, 'r', encoding='utf-8') as f:
    phrases_data = json.load(f)

# Counter for storing lemma usage frequencies
lemmas_counter = Counter()

# Extract lemmas from all phrases
for phrase, data in phrases_data.items():
    for lemma in data.get("6_lemmas", []):
        lemma_name = lemma.get("0_lemma", "").split('_', 1)[1]
        lemmas_counter[lemma_name] += 1

# Get the top 10 or 20 most frequent lemmas
top_lemmas = lemmas_counter.most_common(50)

# Print results
print("Top most frequent lemmas:")
for lemma, count in top_lemmas:
    print(f"{lemma}: {count}")
