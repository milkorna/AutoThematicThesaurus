from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]

# Directories
DATA_DIR = PROJECT_ROOT / 'my_data'
CORPUS_DIR = DATA_DIR / 'nlp_corpus'
RELATIONS_DATA_DIR = PROJECT_ROOT / 'relations'

# General data
PATH_FASTTEXT = PROJECT_ROOT / "my_custom_fasttext_model_finetuned.bin"

PATH_DATA_WITH_OFF = PROJECT_ROOT / "data_with_oof.xlsx"
PATH_DATA = PROJECT_ROOT / "data.xlsx"

PATH_TOTAL_RESULTS = CORPUS_DIR / "total_results.json"
PATH_MNLI_CLASSIFIED_PHRASES = CORPUS_DIR / "classified_phrases.json"
PATH_TERM_CANDIDATES = CORPUS_DIR / "term_candidates.json"

PATH_SENTENCES = CORPUS_DIR / "sentences.json"
PATH_SENTENCES_WITH_PHRASES = CORPUS_DIR / "enriched_sentences.json"

# Relations data
SYNONYMS_DIR = RELATIONS_DATA_DIR / "synonyms_analysis"
HYPERNUM_HYPONYMS_DIR = RELATIONS_DATA_DIR / "hypernym_hyponym_analysis"

PATH_FINAL_SYNONYMS = SYNONYMS_DIR / "final_synonyms.json"

PATH_HYPERNUM_VOTING = HYPERNUM_HYPONYMS_DIR / "hypernym_voting_results.json"
PATH_HYPERNUM_NLI = HYPERNUM_HYPONYMS_DIR / "nli_hypernym_relations.json"
PATH_HYPERNUM_TRIGGERS = HYPERNUM_HYPONYMS_DIR / "extract_relations_by_triggers.json"