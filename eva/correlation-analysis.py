# /// script
# requires-python = ">=3.9"
# dependencies = [
#     "numpy",
#     "pandas",
#     "scipy",
# ]
# ///
import pandas as pd
import numpy as np
from scipy.stats import pearsonr

# Create DataFrame from provided data
data = {
    "Project": [
        "flameshot",
        "system/core",
        "frameworks/av",
        "grpc",
        "aosp/art",
        "tmux",
        "duckdb",
        "frameworks/native",
        "rocksdb",
        "redis",
    ],
    "Correct": [14, 83, 105, 42, 43, 39, 47, 121, 118, 19],
    "Attempt": [19, 117, 164, 108, 67, 69, 83, 155, 203, 28],
    "Total": [26, 160, 264, 152, 128, 93, 143, 179, 245, 68],
}
df = pd.DataFrame(data)

# Calculate metrics with rounding
df["Precision"] = (df["Correct"] / df["Attempt"]).round(4)
df["Accuracy"] = (df["Correct"] / df["Total"]).round(4)
print("DataFrame after initial calculations:\n", df)

# Add Intervention column (below formulas are mathematically equivalent)
df["Intervention"] = (df["Attempt"] / df["Total"]).round(4)
# df['Intervention'] = (df['Accuracy'] / df['Precision']).round(4)
print("\nDataFrame after calculated Intervention:\n", df)

# Calculate Pearson correlation and p-value
corr_coef, p_val = pearsonr(df["Accuracy"], df["Intervention"])

# Hypothesis testing at alpha=0.05
alpha = 0.05
if p_val < alpha:
    test_result = "Reject null hypothesis, significant correlation."
else:
    test_result = "Fail to reject null hypothesis, no significant correlation."

print(f"\nPearson correlation: {corr_coef:.4f}, p-value: {p_val:.4f}")
print(test_result)

# Export final DataFrame
# sort the result by Accuracy before export
df = df.sort_values(by="Accuracy", ascending=False)
df.to_csv("block-level-complete.csv", index=False)
print("\nDataFrame exported to block-level-complete.csv")
