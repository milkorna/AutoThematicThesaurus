import json
import fasttext
import tempfile

from core.paths import CORPUS_DIR, PATH_FASTTEXT

def prepare_documents_from_json(json_path):
    """
    Groups sentences by 'docNum' from a JSON file and combines them into documents.
    Returns a list of texts where each text corresponds to a single document.
    """
    with open(json_path, 'r', encoding='utf-8') as infile:
        data = json.load(infile)
        documents = {}

        # Group sentences by 'docNum'
        for sentence in data['sentences']:
            doc_num = sentence['docNum']
            normalized_str = sentence.get('normalizedStr', '')
            if normalized_str:  # Ensure the sentence is not empty
                documents.setdefault(doc_num, []).append(normalized_str)

        # Combine sentences into documents
        return [" ".join(doc_sentences) for doc_sentences in documents.values()]

def train_fasttext_on_documents(documents, model_path, model_type="skipgram", lr=0.02, epoch=5, dim=300):
    """
    Trains a FastText model on the provided documents.
    documents: List of texts (each text corresponds to one document).
    model_path: Path to save the trained model.
    model_type: Type of FastText model ("skipgram" or "cbow").
    lr: Learning rate.
    epoch: Number of training epochs.
    dim: Dimensionality of the word vectors.
    """
    # Create a temporary file to pass data to FastText
    with tempfile.NamedTemporaryFile(mode='w+', delete=False) as temp_file:
        for document in documents:
            temp_file.write(document + "\n")
        temp_file_path = temp_file.name

    # Train the FastText model
    model = fasttext.train_unsupervised(
        input=temp_file_path,
        model=model_type,
        lr=lr,
        epoch=epoch,
        dim=dim
    )

    # Save the trained model
    model.save_model(model_path)
    print(f"Model saved at: {model_path}")

# Paths
json_path = CORPUS_DIR / "sentences.json"

# Execution steps
documents = prepare_documents_from_json(json_path)
train_fasttext_on_documents(documents, PATH_FASTTEXT)
