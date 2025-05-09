import json

# Load existing relations
with open('relations.json', 'r', encoding='utf-8') as f:
    relations = json.load(f)

# Load terms from JSON
with open('/home/milkorna/Documents/AutoThematicThesaurus/one_word_terms/terms_augmented.json', 'r', encoding='utf-8') as f:
    terms_data = json.load(f)
# Ensure we have a list of term entries
terms = terms_data if isinstance(terms_data, list) else [terms_data]

# Augment relations with new terms
for term in terms:
    noun = term.get('noun', '').strip()
    # Collect phrases with their relation types
    phrases = []
    for hyp in term.get('hypernyms', []):
        phrases.append({'phrase': hyp, 'relation': 'hypernym'})
    for hypo in term.get('hyponyms', []):
        phrases.append({'phrase': hypo, 'relation': 'hyponym'})
    for syn in term.get('synonyms', []):
        phrases.append({'phrase': syn, 'relation': 'synonym'})
    for rel in term.get('related_terms', []):
        phrases.append({'phrase': rel, 'relation': 'related'})

    # Only add entry if there are any phrases
    if phrases:
        relations.append({'key': noun, 'phrases': phrases})

# Sort all entries by the 'key' field alphabetically
relations_sorted = sorted(relations, key=lambda x: x['key'])

# Save the augmented relations
output_file = 'relations.json'
with open(output_file, 'w', encoding='utf-8') as f:
    json.dump(relations_sorted, f, ensure_ascii=False, indent=4)

print(f"Augmented relations saved to '{output_file}' with {len(relations_sorted)} entries.")
