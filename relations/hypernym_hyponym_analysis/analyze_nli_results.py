import json
import pandas as pd
import matplotlib.pyplot as plt

def main():
    # Path to the JSON file containing NLI model results
    input_json_path = "nli_hypernym_relations.json"

    # Load data from JSON
    with open(input_json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    relations = data.get("relations", [])

    if not relations:
        print("No relations found in the JSON file.")
        return

    # Convert JSON data to a Pandas DataFrame
    df = pd.json_normalize(relations, sep='.')

    # Display DataFrame information
    print("DataFrame info:")
    print(df.info())

    # Display the first few rows of the DataFrame
    print("\nDataFrame head:")
    print(df.head())

    # Perform descriptive statistics on selected numerical columns
    print("\nDescriptive statistics:")
    score_cols = ["avg_ent_score", "avg_cont_score", "avg_neutral_score"]
    print(df[score_cols].describe())

    # Plot histogram for the distribution of the average entailment score
    plt.hist(df["avg_ent_score"].dropna(), bins=20)
    plt.xlabel("Average Entailment Score")
    plt.ylabel("Frequency")
    plt.title("Distribution of Average Entailment Scores")
    plt.savefig("avg_ent_score.png", dpi=150)
    plt.close()

    # Plot histogram for the distribution of the average contradiction score
    plt.hist(df["avg_cont_score"].dropna(), bins=20)
    plt.xlabel("Average Contradiction Score")
    plt.ylabel("Frequency")
    plt.title("Distribution of Average Contradiction Scores")
    plt.savefig("avg_cont_score.png", dpi=150)
    plt.close()

    # Scatter plot: avg_cont_score (X-axis) vs. avg_ent_score (Y-axis)
    plt.scatter(df["avg_cont_score"], df["avg_ent_score"])
    plt.xlabel("Average Contradiction Score")
    plt.ylabel("Average Entailment Score")
    plt.title("Contradiction vs. Entailment")
    plt.savefig("contradiction_vs_entailment.png", dpi=150)
    plt.close()

    # Compute the average scores grouped by document number
    grouped_by_doc = df.groupby("docNum")[score_cols].mean()


    # Display the grouped results
    print("\nAverage scores grouped by docNum:")
    print(grouped_by_doc)

if __name__ == "__main__":
    main()
