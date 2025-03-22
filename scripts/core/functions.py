from gensim.models import fasttext

def load_fasttext_model(model_path):
    """
    Loads a FastText model from the specified path.
    """
    print(f"[INFO] Loading fastText model from: {model_path}")
    model = fasttext.load_facebook_model(model_path)
    print("[INFO] fastText model loaded successfully.")
    return model