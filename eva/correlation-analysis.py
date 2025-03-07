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

#              Project  Correct  Attempt  Total  Precision  Accuracy  Intervention
# 7  frameworks/native      121      155    179     0.7806    0.6760        0.8659
# 0          flameshot       14       19     26     0.7368    0.5385        0.7308
# 1        system/core       83      117    160     0.7094    0.5188        0.7312
# 8            rocksdb      118      203    245     0.5813    0.4816        0.8286
# 5               tmux       39       69     93     0.5652    0.4194        0.7419
# 2      frameworks/av      105      164    264     0.6402    0.3977        0.6212
# 4           aosp/art       43       67    128     0.6418    0.3359        0.5234
# 6             duckdb       47       83    143     0.5663    0.3287        0.5804
# 9              redis       19       28     68     0.6786    0.2794        0.4118
# 3               grpc       42      108    152     0.3889    0.2763        0.7105
