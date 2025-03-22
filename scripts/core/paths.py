from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]

DATA_DIR = PROJECT_ROOT / 'my_data'
CORPUS_DIR = DATA_DIR / 'nlp_corpus'
RELATIONS_DATA_DIR = PROJECT_ROOT / 'relations'


print(PROJECT_ROOT)
print(RELATIONS_DATA_DIR)