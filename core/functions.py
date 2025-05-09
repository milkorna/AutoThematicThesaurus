from gensim.models import fasttext
import numpy as np

def load_fasttext_model(model_path):
    """
    Loads a FastText model from the specified path.
    """
    print(f"[INFO] Loading fastText model from: {model_path}")
    model = fasttext.load_facebook_model(model_path)
    print("[INFO] fastText model loaded successfully.")
    return model

def get_phrase_average_embedding(phrase, ft_model):
    """
    Generates the embedding for a phrase using the FastText model.
    If the phrase is empty or no words are known, returns a zero vector.
    """
    if not phrase or not isinstance(phrase, str):
        return np.zeros(ft_model.vector_size, dtype=np.float32)
    words = phrase.split()
    vectors = [ft_model.wv[w] for w in words if w in ft_model.wv.key_to_index]
    if len(vectors) == 0:
        return np.zeros(ft_model.vector_size, dtype=np.float32)
    return np.mean(vectors, axis=0)

def get_weighted_context_embedding(context_str, ft_model):
    """
    Generates a weighted average embedding for a context string using FastText.
    Each part of the context contributes based on the number of words it contains.
    """
    if not context_str or not isinstance(context_str, str):
        return np.zeros(ft_model.vector_size, dtype=np.float32)
    parts = context_str.split('|')
    vectors, weights = [], []
    for part in parts:
        part = part.strip()
        if part:
            emb = get_phrase_average_embedding(part, ft_model)
            if np.any(emb):
                vectors.append(emb)
                weights.append(len(part.split()))
    if len(vectors) == 0:
        return np.zeros(ft_model.vector_size, dtype=np.float32)
    weights = np.array(weights, dtype=np.float32)
    weighted_sum = np.sum([v * w for v, w in zip(vectors, weights)], axis=0)
    return weighted_sum / weights.sum()

def get_word_embedding(word, ft_model):
    """
    Returns the word vector from the fastText model.
    If the word is not in the vocabulary, returns a zero vector.
    """
    if word in ft_model.wv.key_to_index:
        return ft_model.wv[word]
    else:
        return np.zeros(ft_model.vector_size, dtype=np.float32)

def cosine_similarity(vec1, vec2):
    """
    Returns the cosine similarity between two vectors.
    """
    denom = (np.linalg.norm(vec1) * np.linalg.norm(vec2))
    if denom == 0.0:
        return 0.0
    return float(np.dot(vec1, vec2) / denom)