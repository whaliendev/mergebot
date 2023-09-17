//
// Created by whalien on 16/09/23.
//
#include "mergebot/utils/similarity.h"

#include <re2/re2.h>

#include <cmath>
#include <unordered_map>

namespace mergebot {
namespace util {
namespace details {
int levenshtein_distance(const std::string& s1, const std::string& s2) {
  std::vector<std::vector<int>> dp(s1.size() + 1,
                                   std::vector<int>(s2.size() + 1));

  for (int i = 0; i <= s1.size(); ++i) {
    for (int j = 0; j <= s2.size(); ++j) {
      if (i == 0) {
        dp[i][j] = j;
      } else if (j == 0) {
        dp[i][j] = i;
      } else {
        dp[i][j] = std::min({dp[i - 1][j - 1] + (s1[i - 1] != s2[j - 1]),
                             dp[i - 1][j] + 1, dp[i][j - 1] + 1});
      }
    }
  }

  return dp[s1.size()][s2.size()];
}

std::unordered_map<std::string, int> get_profile(const std::string& str,
                                                 int k) {
  std::unordered_map<std::string, int> profile;
  std::string cleaned_str = str;
  RE2::GlobalReplace(&cleaned_str, "\\s+", " ");

  for (int i = 0; i <= static_cast<int>(cleaned_str.length()) - k; ++i) {
    std::string kgram = cleaned_str.substr(i, k);
    ++profile[kgram];
  }
  return profile;
}

double norm(const std::unordered_map<std::string, int>& profile) {
  double sum = 0.0;
  for (const auto& [kgram, count] : profile) {
    sum += count * count;
  }
  return std::sqrt(sum);
}

}  // namespace details

double cosine(const std::vector<double>& a, const std::vector<double>& b) {
  if (a.size() != b.size()) {
    return SimilarityErrKind::ErrDimensionMismatch;
  }
  if (a.empty() || b.empty()) {
    return SimilarityErrKind::ErrEmptyVector;
  }
  double dot = 0.0, denom_a = 0.0, denom_b = 0.0;
  for (auto ait = a.begin(), bit = b.begin(); ait != a.end(); ++ait, ++bit) {
    dot += (*ait) * (*bit);
    denom_a += (*ait) * (*ait);
    denom_b += (*bit) * (*bit);
  }
  if (denom_a == 0.0 || denom_b == 0.0) {
    return SimilarityErrKind::ErrZeroVector;
  }
  return dot / (sqrt(denom_a) * sqrt(denom_b));
}

double string_levenshtein(const std::string& s1, const std::string& s2) {
  int distance = details::levenshtein_distance(s1, s2);
  return 1.0 - static_cast<double>(distance) / std::max(s1.size(), s2.size());
}

double string_cosine(const std::string& s1, const std::string& s2, int k) {
  if (s1.empty() || s2.empty()) {
    return ErrEmptyVector;
  }

  auto profile1 = details::get_profile(s1, k);
  auto profile2 = details::get_profile(s2, k);

  if (profile1.empty() || profile2.empty()) {
    return ErrZeroVector;
  }

  double dot_product = 0.0;
  for (const auto& [kgram, count] : profile1) {
    if (profile2.find(kgram) != profile2.end()) {
      dot_product += count * profile2[kgram];
    }
  }

  double norm1 = details::norm(profile1);
  double norm2 = details::norm(profile2);

  if (norm1 == 0.0 || norm2 == 0.0) {
    return ErrZeroVector;
  }

  return dot_product / (norm1 * norm2);
}
}  // namespace util
}  // namespace mergebot