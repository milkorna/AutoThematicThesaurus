import json

with open('mask_deep_pavlov.json', 'r', encoding='utf-8') as f:
    mask_data = json.load(f)

with open('synonyms_rut5_base_paraphraser.json', 'r', encoding='utf-8') as f:
    paraphraser_data = json.load(f)

combined = {}

for key, entry in mask_data.items():
    phrases = [p["new_phrase"] for p in entry.get("phrases", [])]
    if key in combined:
        combined[key].update(phrases)
    else:
        combined[key] = set(phrases)

for key, entry in paraphraser_data.items():
    phrases = entry.get("top_paraphrases", [])
    if key in combined:
        combined[key].update(phrases)
    else:
        combined[key] = set(phrases)

final_output = [
    {
        "key": key,
        "synonyms": sorted(list(phrases))
    }
    for key, phrases in combined.items()
    if phrases
]

with open('final_synonyms.json', 'w', encoding='utf-8') as f:
    json.dump(final_output, f, ensure_ascii=False, indent=4)
