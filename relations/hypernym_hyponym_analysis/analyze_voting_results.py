import json
import pandas as pd
import matplotlib.pyplot as plt

def main():
    # Load data from JSON file
    input_json_path = "hypernym_voting_results.json"
    with open(input_json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Convert JSON data into a DataFrame
    all_records = []
    for record in data["results"]:
        query_str = record["query"]
        for hyp_item in record.get("hypernyms", []):
            row = {
                "query": query_str,
                "hypernym": hyp_item.get("hypernym"),
                "vote": hyp_item.get("vote"),
                "is_term_manual": hyp_item.get("is_term_manual"),
                "oof_prob_class": hyp_item.get("oof_prob_class")
            }
            all_records.append(row)

    df = pd.DataFrame(all_records)
    if df.empty:
        print("No hypernyms data found.")
        return

    # Display basic DataFrame information
    print("DataFrame Info:")
    print(df.info())

    # Display the first few rows of the DataFrame
    print("\nHead of DataFrame:")
    print(df.head())

    # Compute and display descriptive statistics for the 'vote' column
    print("\nDescriptive statistics for 'vote':")
    print(df["vote"].describe())

    # Plot histogram for the distribution of votes
    plt.hist(df["vote"].dropna(), bins=20)
    plt.xlabel("Vote value")
    plt.ylabel("Frequency")
    plt.title("Distribution of Hypernym Votes")
    plt.savefig("distribution_of_hypernym_votes.png", dpi=150)
    plt.close()

    # Plot histogram for the distribution of probability values (oof_prob_class)
    plt.hist(df["oof_prob_class"].dropna(), bins=20)
    plt.xlabel("Probability value (oof_prob_class)")
    plt.ylabel("Frequency")
    plt.title("Distribution of Probability")
    plt.savefig("distribution_of_probability.png", dpi=150)
    plt.close()

    # Compute the average vote grouped by 'is_term_manual'
    group_by_manual = df.groupby("is_term_manual")["vote"].mean()
    print("\nAverage vote grouped by 'is_term_manual':")
    print(group_by_manual)

    # Plot bar chart comparing the average vote by 'is_term_manual'
    group_by_manual.plot(kind="bar")
    plt.xlabel("is_term_manual")
    plt.ylabel("Average Vote")
    plt.title("Comparison of Average Vote by is_term_manual")
    plt.savefig("average_vote.png", dpi=150)
    plt.close()

    # Compute the number of hypernyms per query
    hyper_count_by_query = df.groupby("query")["hypernym"].count()
    print("\nNumber of hypernyms per query:")
    print(hyper_count_by_query)

    # Plot horizontal bar chart for the number of hypernyms per query
    hyper_count_by_query.plot(kind="barh", figsize=(8,5))
    plt.xlabel("Count of hypernyms")
    plt.ylabel("Query")
    plt.title("Hypernym Count per Query")
    plt.savefig("hypernym_count_per_query.png", dpi=150)
    plt.close()

    # Compute correlation between 'vote' and 'oof_prob_class'
    corr_value = df[["vote", "oof_prob_class"]].corr().iloc[0,1]
    print(f"\nCorrelation between 'vote' and 'oof_prob_class': {corr_value:.4f}")

    # Plot scatter plot showing the relationship between 'vote' and 'oof_prob_class'
    plt.scatter(df["vote"], df["oof_prob_class"])
    plt.xlabel("Vote")
    plt.ylabel("Probability (oof_prob_class)")
    plt.title("Correlation of Vote and Probability")
    plt.savefig("correlation_of_vote_and_probability.png", dpi=150)
    plt.close()

if __name__ == "__main__":
    main()
