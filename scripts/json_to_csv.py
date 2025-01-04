import json
import csv
import os

def create_csv_from_json(
    total_results_path="total_results.json",
    classified_phrases_path="classified_phrases.json",
    terms_path="terms.json",
    sentences_path="sentences.json",
    csv_path="data.csv"
):
    print("Starting create_csv_from_json function...")

    print(f"Reading data from '{total_results_path}'...")
    with open(total_results_path, "r", encoding="utf-8") as f:
        data_total = json.load(f)
    print(f"Successfully loaded data from '{total_results_path}'. Total items: {len(data_total)}")

    labels_dict = {}
    if os.path.exists(classified_phrases_path):
        print(f"'{classified_phrases_path}' exists. Reading classified phrases...")
        with open(classified_phrases_path, "r", encoding="utf-8") as f:
            data_classified = json.load(f)
        print(f"Successfully loaded data from '{classified_phrases_path}'. Items: {len(data_classified)}")

        for item in data_classified:
            phrase = item.get("phrase")
            label = item.get("label")
            if phrase is not None:
                labels_dict[phrase] = label
        print("Classified phrases dictionary has been created.")
    else:
        print(f"File '{classified_phrases_path}' does not exist. Skipping classified phrases step.")

    terms_set = set()
    if os.path.exists(terms_path):
        print(f"'{terms_path}' exists. Reading terms...")
        with open(terms_path, "r", encoding="utf-8") as f:
            data_terms = json.load(f)
        print(f"Successfully loaded data from '{terms_path}'. Items: {len(data_terms)}")

        terms_set = set(data_terms.keys())
        print("Terms set has been created.")
    else:
        print(f"File '{terms_path}' does not exist. Skipping terms step.")

    context_dict = {}
    if os.path.exists(sentences_path):
        print(f"'{sentences_path}' exists. Reading sentences for context...")
        with open(sentences_path, "r", encoding="utf-8") as f:
            data_sentences = json.load(f)
        if "sentences" in data_sentences:
            for s in data_sentences["sentences"]:
                doc_num = s.get("docNum")
                sent_num = s.get("sentNum")
                norm_str = s.get("normalizedStr", "")
                if doc_num is not None and sent_num is not None:
                    context_dict[(doc_num, sent_num)] = norm_str
        print("Context dictionary has been created.")
    else:
        print(f"File '{sentences_path}' does not exist. Skipping context step.")

    print(f"Writing CSV output to '{csv_path}'...")
    with open(csv_path, "w", encoding="utf-8", newline="") as csvfile:
        writer = csv.writer(csvfile)

        print("Writing CSV header...")
        writer.writerow([
            "key",
            "phrase_size",
            "frequency",
            "topic_relevance",
            "centrality_score",
            "tag_match",
            "model_name",
            "phrases_count",
            "label",
            "is_term_auto",
            "context"
        ])

        print("Writing CSV rows...")
        for phrase_key, phrase_data in data_total.items():
            phrase_size = phrase_data.get("0_phrase_size")
            frequency = phrase_data.get("1_frequency")
            topic_relevance = phrase_data.get("2_topic_relevance")
            centrality_score = phrase_data.get("3_centrality_score")
            tag_match = phrase_data.get("4_tag_match")
            model_name = phrase_data.get("5_model_name")
            phrases_count = phrase_data.get("7_phrases_count")

            label = labels_dict.get(phrase_key, None)

            if phrase_key in terms_set:
                is_term = ""
            else:
                is_term = "0"

            context_list = []
            eight_phrases = phrase_data.get("8_phrases", [])
            for p in eight_phrases:
                position = p.get("1_position", {})
                doc_num = position.get("2_doc_num")
                sent_num = position.get("3_sent_num")
                if (doc_num, sent_num) in context_dict:
                    context_list.append(context_dict[(doc_num, sent_num)])
                else:
                    pass
            
            context_str = " | ".join(context_list) if context_list else ""

            writer.writerow([
                phrase_key,
                phrase_size,
                frequency,
                topic_relevance,
                centrality_score,
                tag_match,
                model_name,
                phrases_count,
                label,
                is_term,
                context_str
            ])

    print(f"CSV file '{csv_path}' has been created successfully.")

if __name__ == "__main__":
    create_csv_from_json(
        total_results_path="/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results.json",
        classified_phrases_path="/home/milkorna/Documents/AutoThematicThesaurus/my_data/classified_phrases.json",
        terms_path="/home/milkorna/Documents/AutoThematicThesaurus/my_data/terms.json",
        sentences_path="/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/sentences.json",
        csv_path="data_auto.csv"
    )