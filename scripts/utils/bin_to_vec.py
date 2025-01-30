from fasttext import load_model
import argparse

def save_vectors_in_chunks(model_path, output_path, chunk_size=1000):
    f = load_model(model_path)
    words = f.get_words()
    dim = f.get_dimension()
    
    with open(output_path, 'w') as out_file:
        out_file.write(f"{len(words)} {dim}\n")
        for i in range(0, len(words), chunk_size):
            chunk_words = words[i:i + chunk_size]
            for w in chunk_words:
                v = f.get_word_vector(w)
                vstr = " ".join(map(str, v))
                out_file.write(f"{w} {vstr}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=("Print fasttext .vec file to stdout from .bin file in chunks")
    )
    parser.add_argument(
        "model",
        help="Model to use",
    )
    parser.add_argument(
        "output",
        help="Output file to save vectors",
    )
    args = parser.parse_args()

    save_vectors_in_chunks(args.model, args.output)
