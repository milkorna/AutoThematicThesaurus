import pandas as pd
import json

# Load existing dataset
input_excel = 'data.xlsx'
df = pd.read_excel(input_excel)

# Load terms from JSON
json_file = '/home/milkorna/Documents/AutoThematicThesaurus/one_word_terms/terms_augmented.json'
with open(json_file, 'r', encoding='utf-8') as f:
    data = json.load(f)
# Ensure we have a list of term entries
terms = data if isinstance(data, list) else [data]

# Prepare new rows
new_rows = []
for term in terms:
    noun = term.get('noun', '').strip()
    # Extract sentences and document IDs
    context_entries = term.get('context', [])
    sentences = [ctx.get('sentence', '').strip() for ctx in context_entries]
    doc_ids = [ctx.get('document') for ctx in context_entries if 'document' in ctx]
    unique_docs = sorted(set(doc_ids))
    # Compute frequency as total number of sentences divided by 300
    frequency = len(sentences) / 300

    new_rows.append({
        'phrase': noun,
        'normalized_form': noun.capitalize(),
        'is_term_manual': 1,
        'phrase_size': 1,
        'frequency': frequency,
        'model_name': 'С',  # Russian letter Es
        'context': '||'.join(sentences),
        'oof_prob_class': 1,
        'documents': ','.join(map(str, unique_docs))
    })

# Create DataFrame for new terms and append to the original
df_new = pd.DataFrame(new_rows)
df_augmented = pd.concat([df, df_new], ignore_index=True)

# Save the augmented dataset to a new Excel file
output_excel = 'data_augmented.xlsx'
df_augmented.to_excel(output_excel, index=False)

print(f"Augmented dataset saved to '{output_excel}' with {len(new_rows)} new entries.")