import json

def find_hypernym_hyponym_mismatches(file_path):
    """
    Identifies inconsistencies in hypernym-hyponym relationships in a JSON dataset.

    Specifically, it checks if for any given pair (A, B), where A is marked as a hypernym of B,
    the reverse relation (B as a hyponym of A) is also correctly set. If not, the inconsistent
    pair is printed.
    """
    # Load the JSON file
    with open(file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    key_relations = {}

    # Build a dictionary of relationships
    for entry in data:
        key = entry["key"]
        key_relations[key] = {}
        for phrase_data in entry["phrases"]:
            phrase = phrase_data["phrase"]
            key_relations[key][phrase] = {
                "is_hypernym": phrase_data["is_hypernym"],
                "is_hyponym": phrase_data["is_hyponym"]
            }

    # Identify inconsistencies in hypernym-hyponym relations
    for key_A, relations in key_relations.items():
        for key_B, rel_A_B in relations.items():
            if rel_A_B["is_hypernym"]:
                if key_B in key_relations and key_A in key_relations[key_B]:
                    rel_B_A = key_relations[key_B][key_A]
                    if not rel_B_A["is_hyponym"]:
                        print(f"{key_A} - {key_B}")

if __name__ == "__main__":
    file_path = "/home/milkorna/Documents/AutoThematicThesaurus/relations/marked_relations.json"
    find_hypernym_hyponym_mismatches(file_path)
