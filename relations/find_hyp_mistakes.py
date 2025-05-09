import json

def find_hypernym_hyponym_mismatches(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    relation_dict = {}
    for entry in data:
        key = entry["key"]
        relation_dict[key] = {}
        for phrase_data in entry["phrases"]:
            phrase = phrase_data["phrase"]
            relation = phrase_data.get("relation")
            if relation:
                relation_dict[key][phrase] = relation

    for key_A, phrases_A in relation_dict.items():
        for key_B, rel_A_B in phrases_A.items():
            if key_B in relation_dict and key_A in relation_dict[key_B]:
                rel_B_A = relation_dict[key_B][key_A]
                if rel_A_B == "hypernym" and rel_B_A != "hyponym":
                    print(f"\n{key_A} -> {key_B}")

if __name__ == "__main__":
    file_path = "marked_relations_manual.json"
    find_hypernym_hyponym_mismatches(file_path)
