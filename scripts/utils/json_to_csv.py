import json
import csv
import os

from scripts.core.paths import PATH_TOTAL_RESULTS, PATH_MNLI_CLASSIFIED_PHRASES, PATH_TERM_CANDIDATES, PATH_SENTENCES

def create_csv_from_json():
    csv_path="data.csv"

    print(f"Reading data from '{PATH_TOTAL_RESULTS}'...")
    with open(PATH_TOTAL_RESULTS, "r", encoding="utf-8") as f:
        data_total = json.load(f)
    print(f"Successfully loaded data from '{PATH_TOTAL_RESULTS}'. Total items: {len(data_total)}")

    labels_dict = {}
    if os.path.exists(PATH_MNLI_CLASSIFIED_PHRASES):
        print(f"'{PATH_MNLI_CLASSIFIED_PHRASES}' exists. Reading classified phrases...")
        with open(PATH_MNLI_CLASSIFIED_PHRASES, "r", encoding="utf-8") as f:
            data_classified = json.load(f)
        print(f"Successfully loaded data from '{PATH_MNLI_CLASSIFIED_PHRASES}'. Items: {len(data_classified)}")

        for item in data_classified:
            phrase = item.get("phrase")
            label = item.get("label")
            if phrase is not None:
                labels_dict[phrase] = label
        print("Classified phrases dictionary has been created.")
    else:
        print(f"File '{PATH_MNLI_CLASSIFIED_PHRASES}' does not exist. Skipping classified phrases step.")

    terms_set = set()
    if os.path.exists(PATH_TERM_CANDIDATES):
        print(f"'{PATH_TERM_CANDIDATES}' exists. Reading terms...")
        with open(PATH_TERM_CANDIDATES, "r", encoding="utf-8") as f:
            data_terms = json.load(f)
        print(f"Successfully loaded data from '{PATH_TERM_CANDIDATES}'. Items: {len(data_terms)}")

        terms_set = set(data_terms.keys())
        print("Terms set has been created.")
    else:
        print(f"File '{PATH_TERM_CANDIDATES}' does not exist. Skipping terms step.")

    context_dict = {}
    if os.path.exists(PATH_SENTENCES):
        print(f"'{PATH_SENTENCES}' exists. Reading sentences for context...")
        with open(PATH_SENTENCES, "r", encoding="utf-8") as f:
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
        print(f"File '{PATH_SENTENCES}' does not exist. Skipping context step.")

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
                is_term = "1"
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
    create_csv_from_json(csv_path="data_auto.csv")
