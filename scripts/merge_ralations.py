import json
from collections import defaultdict

from scripts.core.paths import PATH_FINAL_SYNONYMS, PATH_HYPERNUM_TRIGGERS, PATH_HYPERNUM_NLI, PATH_HYPERNUM_VOTING

def load_json(file_path):
    """Load a JSON file and return the corresponding Python data structure."""
    with open(file_path, "r", encoding="utf-8") as f:
        return json.load(f)

def main():
    # Initialize a default dictionary to store thesaurus data with default flags for relations
    thesaurus = defaultdict(lambda: defaultdict(lambda: {
        "is_synonym": False,
        "is_usage_variant": False,
        "is_related": False,
        "is_hypernym": False,
        "is_hyponym": False
    }))

    # Step 1: Process final_synonyms.json
    synonyms_data = load_json(PATH_FINAL_SYNONYMS)
    for entry in synonyms_data:
        key = entry["key"]

        # Process synonyms
        for syn_phrase in entry.get("synonyms", []):
            thesaurus[key][syn_phrase]["is_synonym"] = True
            thesaurus[syn_phrase][key]["is_synonym"] = True

        # Process usage variants
        for uv_phrase in entry.get("usage_variants", []):
            thesaurus[key][uv_phrase]["is_usage_variant"] = True
            thesaurus[uv_phrase][key]["is_usage_variant"] = True

        # Process similar phrases
        for sp_phrase in entry.get("similar_phrases", []):
            thesaurus[key][sp_phrase]["is_related"] = True
            thesaurus[sp_phrase][key]["is_related"] = True

    # Step 2: Process hypernym_voting_results.json
    hypernym_data = load_json(PATH_HYPERNUM_VOTING)
    for result in hypernym_data.get("results", []):
        query = result["query"]
        for h in result.get("hypernyms", []):
            hypernym_phrase = h["hypernym"]
            thesaurus[query][hypernym_phrase]["is_hypernym"] = True
            thesaurus[hypernym_phrase][query]["is_hyponym"] = True

    # Step 3: Process nli_hypernym_relations.json
    nli_data = load_json(PATH_HYPERNUM_NLI)
    for rel in nli_data.get("relations", []):
        hyper = rel["hyper"]["phrase"]
        hypo = rel["hypo"]["phrase"]
        thesaurus[hyper][hypo]["is_hyponym"] = True
        thesaurus[hypo][hyper]["is_hypernym"] = True

    # Step 4: Process extract_relations_by_triggers.json
    triggers_data = load_json(PATH_HYPERNUM_TRIGGERS)
    for sentence in triggers_data.get("sentences", []):
        for isa in sentence.get("is-a", []):
            subject = isa["subject"]["phrase"]
            predicate = isa["predicate"]["phrase"]
            thesaurus[subject][predicate]["is_hypernym"] = True
            thesaurus[predicate][subject]["is_hyponym"] = True

    # Finalize and sort the thesaurus data structure
    output = []
    all_keys_sorted = sorted(thesaurus.keys(), key=str.lower)

    for key in all_keys_sorted:
        phrases_list = []
        related_phrases_sorted = sorted(thesaurus[key].keys(), key=str.lower)
        for phrase in related_phrases_sorted:
            phrases_list.append({
                "phrase": phrase,
                "is_synonym": thesaurus[key][phrase]["is_synonym"],
                "is_usage_variant": thesaurus[key][phrase]["is_usage_variant"],
                "is_related": thesaurus[key][phrase]["is_related"],
                "is_hypernym": thesaurus[key][phrase]["is_hypernym"],
                "is_hyponym": thesaurus[key][phrase]["is_hyponym"]
            })
        output.append({
            "key": key,
            "phrases": phrases_list
        })

    # Define the output file path and save the final structure to it
    final_output_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/final_relations.json"
    with open(final_output_path, "w", encoding="utf-8") as f:
        json.dump(output, f, ensure_ascii=False, indent=4)

    print(f"File successfully generated: {final_output_path}")

if __name__ == "__main__":
    main()
