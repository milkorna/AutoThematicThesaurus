import os
import json
import sqlite3
import pandas as pd
import re
import numpy as np
import inspect
from gensim.models import fasttext
from ruwordnet import RuWordNet

# Natasha and dependencies
from natasha import (
    Segmenter,
    NewsEmbedding,
    NewsMorphTagger,
    NewsSyntaxParser,
    Doc
)
import pymorphy2

# Patch pymorphy2 for compatibility with Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults
    inspect.getargspec = getargspec_patched

# File paths and model locations
PATH_FILTERED = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
PATH_WIKIWORDNET = "/home/milkorna/Documents/AutoThematicThesaurus/wikiwordnet.db"
PATH_FASTTEXT = "/home/milkorna/Documents/AutoThematicThesaurus/my_custom_fasttext_model_finetuned.bin"
PATH_HEADS_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/hyponym_hyponym_analysis/head_nouns.json"
CORPUS_PATH = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/filtered_corpus.json"

# Column name for the original phrase
COLUMN_PHRASE = "key"

# Initialize Natasha and pymorphy2
segmenter = Segmenter()
embedding = NewsEmbedding()
morph_tagger = NewsMorphTagger(embedding)
syntax_parser = NewsSyntaxParser(embedding)
morph = pymorphy2.MorphAnalyzer()

# Compute IDF weights based on word frequency in the corpus
import math
with open(CORPUS_PATH, "r", encoding="utf-8") as f:
    corpus_data = json.load(f)
total_words = corpus_data.get("2_totalWords", 1)
word_freq = corpus_data.get("4_wordFrequency", {})
idf_weights = {}
for word, freq in word_freq.items():
    # Compute IDF using the formula: idf = log((total_words + 1) / (freq + 1))
    idf_weights[word.lower()] = math.log((total_words + 1) / (freq + 1))

def find_head_noun_natasha(phrase):
    """
    Performs syntactic parsing of a phrase using Natasha,
    determines the head noun,
    and returns its lemmatized form.
    """
    doc = Doc(phrase)
    doc.segment(segmenter)
    doc.tag_morph(morph_tagger)
    doc.parse_syntax(syntax_parser)

    if not doc.sents:
        return None

    sent = doc.sents[0]
    tokens = sent.tokens

    # Find the token with head_id == 0 (root of the sentence)
    root_token = None
    for t in tokens:
        if t.head_id == 0:
            root_token = t
            break

    if not root_token:
        # If no root is found, try selecting the first noun token
        for t in tokens:
            if t.pos == 'NOUN':
                root_token = t
                break
        if not root_token:
            return None

    # If the root is not a noun, check its dependent tokens
    if root_token.pos != 'NOUN':
        children = [tok for tok in tokens if tok.head_id == root_token.id]
        for c in children:
            if c.pos == 'NOUN':
                root_token = c
                break

    if root_token.pos != 'NOUN':
        for t in tokens:
            if t.pos == 'NOUN':
                root_token = t
                break
        else:
            return None

    # Return the lemma of the identified noun
    normal = morph.parse(root_token.text)[0].normal_form
    return normal


def extract_example(text):
    """
    Extracts usage examples from a string by finding all occurrences of the pattern {{пример|...}},
    concatenates them into a single text, removes nested templates (e.g., {{выдел|...}}),
    converts the result to lowercase, and removes punctuation.
    Returns cleaned example text.
    """
    import re
    # Extract all contents within {{пример|...}}
    matches = re.findall(r'\{\{пример\|(.*?)\}\}', text)
    combined = " ".join(matches) if matches else ""

    replacements = {
        "{{пример|": "",
        "{{выдел|": "",
        "}}": "",
        ".": "",
        "/": "",
        "}": "",
        "{": "",
        "|": "",
        "!": "",
        "?": ""
    }
    cleaned = combined
    for old, new in replacements.items():
        cleaned = cleaned.replace(old, new)

    return cleaned.lower()

import numpy as np
import math

def get_sentence_embedding(text, ft_model, idf_weights=None, total_words=None):
    """
    Computes sentence embedding by performing weighted averaging of individual word embeddings.
    If an IDF weight is available for a word (calculated based on corpus frequencies), its vector is multiplied by this weight.
    Common stopwords are excluded before computing the embeddings.
    If no words are found in the model, returns a zero vector.
    """
    # Basic stopword list for the Russian language
    stopwords = {
        'и', 'в', 'на', 'с', 'что', 'как', 'а', 'но', 'у', 'о', 'по', 'из', 'к', 'до', 'за',
        'то', 'этот', 'так', 'такой', 'это', 'все', 'не', 'если', 'же', 'ли', 'бы', 'от', 'при'
    }
    # Tokenization: split by spaces and remove punctuation
    tokens = [word.strip(".,!?;:()\"'«»") for word in text.lower().split()]
    # Filter out stopwords
    filtered_tokens = [word for word in tokens if word and word not in stopwords]

    weighted_sum = np.zeros(ft_model.vector_size, dtype=np.float32)
    total_weight = 0.0

    for word in filtered_tokens:
        vec = get_word_embedding(word, ft_model)
        if np.linalg.norm(vec) > 0:
            # Use IDF weight if provided, otherwise assign weight = 1
            if idf_weights is not None and word in idf_weights:
                weight = idf_weights[word]
            else:
                weight = 1.0
            weighted_sum += weight * vec
            total_weight += weight

    if total_weight > 0:
        return weighted_sum / total_weight
    else:
        return np.zeros(ft_model.vector_size, dtype=np.float32)

import nltk
nltk.download('stopwords')
from nltk.corpus import stopwords as nltk_stopwords

# Extend stopword list using the precompiled NLTK Russian stopwords
extended_stopwords = set(nltk_stopwords.words('russian'))


def sentence_similarity_alignment(text1, text2, ft_model, stopwords=None):
    """
    Computes similarity between two texts using word alignment.
    For each word in the first text, finds the maximum similarity with any word from the second text,
    then averages the results.

    If no stopword list is provided, a default extended list is used.
    """
    if stopwords is None:
        # Use extended stopword list from NLTK
        stopwords = extended_stopwords

    # Simple tokenization with punctuation removal
    def tokenize(text):
        tokens = [word.strip(".,!?;:()\"'«»").lower() for word in text.split()]
        return [w for w in tokens if w and w not in stopwords]

    tokens1 = tokenize(text1)
    tokens2 = tokenize(text2)

    if not tokens1 or not tokens2:
        return 0.0

    sims = []
    for w1 in tokens1:
        vec1 = get_word_embedding(w1, ft_model)
        if np.linalg.norm(vec1) == 0:
            continue
        max_sim = 0.0
        for w2 in tokens2:
            vec2 = get_word_embedding(w2, ft_model)
            if np.linalg.norm(vec2) == 0:
                continue
            sim = cosine_similarity(vec1, vec2)
            if sim > max_sim:
                max_sim = sim
        sims.append(max_sim)
    return sum(sims) / len(sims) if sims else 0.0

def get_wikiwordnet_relations(db_conn, noun_lemma, phrase_context, ft_model):
    """
    Extracts hypernyms, hyponyms, and synonyms from WikiWordNet.
    For each candidate, retrieves a description (example field) and computes
    cosine similarity between the example's embedding and the embedding of the original phrase's context.
    If similarity is below 0.3, the candidate is excluded.
    The function uses caching to reduce the number of database queries.
    Returns a dictionary with results and a set of excluded candidates.
    """
    c = db_conn.cursor()
    c.execute("SELECT synset_id FROM synsets WHERE lemma = ?", (noun_lemma,))
    synset_ids = [row[0] for row in c.fetchall()]
    if not synset_ids:
        return {"head_noun": noun_lemma, "hypernyms": [], "hyponyms": [], "synonyms": []}, set()

    synonyms = set()
    hypernyms = set()
    hyponyms = set()
    excluded_candidates = set()

    # Cache for candidate_allowed results (to avoid redundant queries)
    candidate_cache = {}

    def candidate_allowed(candidate):
        if candidate in candidate_cache:
            return candidate_cache[candidate]
        c.execute("SELECT * FROM synsets WHERE lemma = ? LIMIT 1", (candidate,))
        row_ex = c.fetchone()
        allowed = True
        if row_ex:
            # Assuming description with examples is stored in field index 2
            description = row_ex[2]
            example_text = extract_example(description)
            if example_text and phrase_context:
                filtered_example = " ".join([w for w in example_text.split() if w.lower() != candidate.lower()])
                filtered_phrase  = " ".join([w for w in phrase_context.split() if w.lower() != candidate.lower()])
                sim_example = sentence_similarity_alignment(filtered_example, filtered_phrase, ft_model)
                if sim_example < 0.9:
                    allowed = False
        candidate_cache[candidate] = allowed
        return allowed

    # Process synonyms
    for sid in synset_ids:
        c.execute("SELECT lemma FROM synsets WHERE synset_id = ?", (sid,))
        for row in c.fetchall():
            candidate = row[0]
            if not candidate_allowed(candidate):
                excluded_candidates.add(candidate)
                continue
            synonyms.add(candidate)

    # Process hypernyms
    for sid in synset_ids:
        c.execute("SELECT hypersid FROM hypernyms WHERE sid = ?", (sid,))
        hyper_ids = [row[0] for row in c.fetchall()]
        for hid in hyper_ids:
            c.execute("SELECT lemma, * FROM synsets WHERE synset_id = ?", (hid,))
            for row in c.fetchall():
                candidate = row[0]
                if not candidate_allowed(candidate):
                    excluded_candidates.add(candidate)
                    continue
                hypernyms.add(candidate)

    # Process hyponyms
    for sid in synset_ids:
        c.execute("SELECT sid FROM hypernyms WHERE hypersid = ?", (sid,))
        hypo_ids = [row[0] for row in c.fetchall()]
        for hid in hypo_ids:
            c.execute("SELECT lemma, * FROM synsets WHERE synset_id = ?", (hid,))
            for row in c.fetchall():
                candidate = row[0]
                if not candidate_allowed(candidate):
                    excluded_candidates.add(candidate)
                    continue
                hyponyms.add(candidate)

    return {
        "head_noun": noun_lemma,
        "hypernyms": list(hypernyms),
        "hyponyms": list(hyponyms),
        "synonyms": list(synonyms)
    }, excluded_candidates

def get_ruwordnet_relations(wn, noun_lemma, excluded_set=None):
    """
    Extracts hypernyms, hyponyms, and synonyms from RuWordNet.
    If a candidate exists in the excluded_set (obtained from WikiWordNet), it is not included.
    """
    senses = wn.get_senses(noun_lemma)
    if not senses:
        return {"head_noun": noun_lemma, "hypernyms": [], "hyponyms": [], "synonyms": []}

    synonyms = set()
    hypernyms = set()
    hyponyms = set()

    for sense in senses:
        synset = sense.synset

        # Extract synonyms
        for s in synset.senses:
            candidate = s.name.lower()
            if excluded_set and candidate in excluded_set:
                continue
            synonyms.add(candidate)

        # Extract hypernyms
        for hyper in synset.hypernyms:
            if hasattr(hyper, 'title') and hyper.title:
                candidate = hyper.title.lower()
            else:
                candidate = ", ".join(s.name.lower() for s in hyper.senses)
            if excluded_set and candidate in excluded_set:
                continue
            hypernyms.add(candidate)
            # If a hypernym contains hyponyms, add them as well
            for hypo in hyper.hyponyms:
                if hasattr(hypo, 'title') and hypo.title:
                    candidate_hypo = hypo.title.lower()
                else:
                    candidate_hypo = ", ".join(s.name.lower() for s in hypo.senses)
                if excluded_set and candidate_hypo in excluded_set:
                    continue
                hyponyms.add(candidate_hypo)

        # Extract hyponyms directly from the synset
        for hypo in synset.hyponyms:
            if hasattr(hypo, 'title') and hypo.title:
                candidate = hypo.title.lower()
            else:
                candidate = ", ".join(s.name.lower() for s in hypo.senses)
            if excluded_set and candidate in excluded_set:
                continue
            hyponyms.add(candidate)

    return {
        "head_noun": noun_lemma,
        "hypernyms": list(hypernyms),
        "hyponyms": list(hyponyms),
        "synonyms": list(synonyms)
    }

def combine_relations(*relation_dicts):
    """
    Combines results from different sources, removes duplicates,
    and sorts the lists.
    """
    combined = {
        "hypernyms": set(),
        "hyponyms": set(),
        "synonyms": set()
    }
    for rel in relation_dicts:
        if rel is None:
            continue
        combined["hypernyms"].update(rel.get("hypernyms", []))
        combined["hyponyms"].update(rel.get("hyponyms", []))
        combined["synonyms"].update(rel.get("synonyms", []))
    return {
        "hypernyms": sorted(list(combined["hypernyms"])),
        "hyponyms": sorted(list(combined["hyponyms"])),
        "synonyms": sorted(list(combined["synonyms"]))
    }

def load_fasttext_model(model_path):
    print(f"[INFO] Loading fastText model from: {model_path}")
    ft_model = fasttext.load_facebook_model(model_path)
    print("[INFO] fastText model loaded successfully.")
    return ft_model

def get_pos_tag(word):
    parsed_word = morph.parse(word)[0]
    return parsed_word.tag.POS

def is_noun(word):
    for p in morph.parse(word):
        if p.tag.POS == 'NOUN':
            return True
    return False

def get_word_embedding(word, ft_model):
    if word in ft_model.wv.key_to_index:
        return ft_model.wv[word]
    else:
        return np.zeros(ft_model.vector_size, dtype=np.float32)

def cosine_similarity(vec1, vec2):
    denom = (np.linalg.norm(vec1) * np.linalg.norm(vec2))
    if denom == 0.0:
        return 0.0
    return float(np.dot(vec1, vec2) / denom)

def compute_similarity_for_relations(head_noun, related_words, ft_model, min_sim=0.5, noun_only=True):
    head_vec = get_word_embedding(head_noun, ft_model)
    out = []
    for original_word in related_words:
        base_word, context = process_word_string(original_word)
        if noun_only and not is_noun(base_word):
            continue
        if context:
            word_vec = get_word_embedding(base_word, ft_model)
            context_vec = get_word_embedding(context, ft_model)
            sim1 = cosine_similarity(head_vec, word_vec)
            sim2 = cosine_similarity(head_vec, context_vec)
            sim = (sim1 + sim2) / 2.0
            result_entry = {"word": base_word, "context": context, "similarity": sim}
        else:
            word_vec = get_word_embedding(base_word, ft_model)
            sim = cosine_similarity(head_vec, word_vec)
            result_entry = {"word": base_word, "similarity": sim}
        if sim >= min_sim:
            out.append(result_entry)
    return out

import re

def process_word_string(word):
    if ',' in word:
        word = word.split(',')[0].strip()
    match = re.search(r'^(.*?)\s*\((.*?)\)\s*$', word)
    if match:
        base_word = match.group(1).strip()
        context = match.group(2).strip()
        return base_word, context
    return word, None

def compute_similarity_for_relations(head_noun, related_words, ft_model, min_sim=0.5, noun_only=True, allowed_set=None):
    head_vec = get_word_embedding(head_noun, ft_model)
    out = []
    for original_word in related_words:
        base_word, context = process_word_string(original_word)
        if allowed_set is not None and base_word not in allowed_set:
            continue
        if noun_only and not is_noun(base_word):
            continue
        if context:
            word_vec = get_word_embedding(base_word, ft_model)
            context_vec = get_word_embedding(context, ft_model)
            sim1 = cosine_similarity(head_vec, word_vec)
            sim2 = cosine_similarity(head_vec, context_vec)
            sim = (sim1 + sim2) / 2.0
            result_entry = {"word": base_word, "context": context, "similarity": sim}
        else:
            word_vec = get_word_embedding(base_word, ft_model)  # Если есть ошибка, используйте: get_word_embedding(base_word, ft_model)
            sim = cosine_similarity(head_vec, word_vec)
            result_entry = {"word": base_word, "similarity": sim}
        if sim >= min_sim and sim < 1:
            out.append(result_entry)
    return out

def main():
    if not os.path.exists(PATH_FILTERED):
        print("[ERROR] No filtered_data.xlsx found.")
        raise
    df = pd.read_excel(PATH_FILTERED)
    if COLUMN_PHRASE not in df.columns:
        print(f"[ERROR] Column '{COLUMN_PHRASE}' not found in DataFrame.")
        raise

    if os.path.exists(PATH_WIKIWORDNET):
        wiki_conn = sqlite3.connect(PATH_WIKIWORDNET)
    else:
        wiki_conn = None
        print("[WARNING] No wikiwordnet.db found, skipping WikiWordNet lookups.")

    try:
        wn = RuWordNet(filename_or_session='/home/milkorna/Documents/ruwordnet-2021.db')  # Если нужно указать путь к файлу, можно передать аргумент filename_or_session
    except Exception as e:
        print("[ERROR] Failed to initialize RuWordNet:", e)
        raise

    if os.path.exists(PATH_FASTTEXT):
        ft_model = load_fasttext_model(PATH_FASTTEXT)
    else:
        print(f"[ERROR] FastText model not found at {PATH_FASTTEXT}.")
        raise

    if not os.path.exists(CORPUS_PATH):
        print(f"[ERROR] Corpus file not found at {CORPUS_PATH}.")
        raise FileNotFoundError(CORPUS_PATH)
    with open(CORPUS_PATH, "r", encoding="utf-8") as f:
        corpus_data = json.load(f)
    allowed_words = set(corpus_data["4_wordFrequency"].keys())

    results = {}

    for idx, row in df.iterrows():
        phrase = str(row[COLUMN_PHRASE]).strip()
        if not phrase:
            continue

        head_noun = find_head_noun_natasha(phrase)
        phrase_context = str(row["context"]).strip() if "context" in row and pd.notna(row["context"]) else ""

        if head_noun and wiki_conn:
            wiki_info, excluded_set = get_wikiwordnet_relations(wiki_conn, head_noun, phrase_context, ft_model)
        else:
            wiki_info, excluded_set = None, set()

        if head_noun and wn:
            ru_info = get_ruwordnet_relations(wn, head_noun, excluded_set)
        else:
            ru_info = None

        combined_info = combine_relations(wiki_info, ru_info) if head_noun else {
            "hypernyms": [],
            "hyponyms": [],
            "synonyms": []
        }

        if head_noun:
            hypernyms_with_sim = compute_similarity_for_relations(head_noun,
                                                                  combined_info["hypernyms"],
                                                                  ft_model,
                                                                  min_sim=0.7,
                                                                  noun_only=True,
                                                                  allowed_set=allowed_words)
            hyponyms_with_sim = compute_similarity_for_relations(head_noun,
                                                                 combined_info["hyponyms"],
                                                                 ft_model,
                                                                 min_sim=0.75,
                                                                 noun_only=True,
                                                                 allowed_set=allowed_words)
            synonyms_with_sim = compute_similarity_for_relations(head_noun,
                                                                 combined_info["synonyms"],
                                                                 ft_model,
                                                                 min_sim=0.7,
                                                                 noun_only=True,
                                                                 allowed_set=allowed_words)
        else:
            hypernyms_with_sim = []
            hyponyms_with_sim = []
            synonyms_with_sim = []

        all_sims = []
        for rel_items in [hypernyms_with_sim, hyponyms_with_sim, synonyms_with_sim]:
            for item in rel_items:
                if "similarity" in item:
                    all_sims.append(item["similarity"])
        avg_sim = sum(all_sims) / len(all_sims) if all_sims else 0.0

        results[phrase] = {
            "head_noun": head_noun,
            "hypernyms": hypernyms_with_sim,
            "hyponyms": hyponyms_with_sim,
            "synonyms": synonyms_with_sim,
            "context": phrase_context
        }

    if wiki_conn:
        wiki_conn.close()

    def update_candidate(agg_dict, candidate_dict):
        word = candidate_dict["word"]
        if word in agg_dict:
            agg_dict[word]["similarities"].append(candidate_dict["similarity"])
            if candidate_dict.get("context"):
                agg_dict[word]["contexts"].add(candidate_dict["context"])
        else:
            agg_dict[word] = {
                "similarities": [candidate_dict["similarity"]],
                "contexts": set()
            }
            if candidate_dict.get("context"):
                agg_dict[word]["contexts"].add(candidate_dict["context"])

    head_data = {}

    head_data = {}
    for phrase, data in results.items():
        head = data.get("head_noun")
        if not head:
            continue
        if head not in head_data:
            head_data[head] = {
                "hypernyms": {},
                "hyponyms": {},
                "synonyms": {},
                "related_terms": set()
            }
        head_data[head]["related_terms"].add(phrase)
        for rel in ["hypernyms", "hyponyms", "synonyms"]:
            for candidate in data.get(rel, []):
                update_candidate(head_data[head][rel], candidate)

    for head in head_data:
        for rel in ["hypernyms", "hyponyms", "synonyms"]:
            candidates = []
            for word, info in head_data[head][rel].items():
                avg_sim = sum(info["similarities"]) / len(info["similarities"]) if info["similarities"] else 0.0
                contexts = "; ".join(sorted(info["contexts"])) if info["contexts"] else ""
                candidates.append({
                    "word": word,
                    "context": contexts,
                    "similarity": avg_sim
                })
            head_data[head][rel] = sorted(candidates, key=lambda x: x["similarity"], reverse=True)
        head_data[head]["related_terms"] = sorted(list(head_data[head]["related_terms"]))

    with open(PATH_HEADS_JSON, "w", encoding="utf-8") as f:
        json.dump(head_data, f, ensure_ascii=False, indent=4)

    print("[INFO] Head nouns data saved to:", PATH_HEADS_JSON)

if __name__ == "__main__":
    main()
