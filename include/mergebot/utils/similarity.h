//
// Created by whalien on 16/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_UTILS_SIMILARITY_H
#define MB_INCLUDE_MERGEBOT_UTILS_SIMILARITY_H
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
namespace mergebot {
namespace util {
enum SimilarityErrKind : int8_t {
  ErrDimensionMismatch = -1,
  ErrEmptyVector = -2,
  ErrZeroVector = -3,
};

class SimilarityErrKindEnum {
 public:
  static const char* ToString(SimilarityErrKind kind) {
    switch (kind) {
      case SimilarityErrKind::ErrDimensionMismatch:
        return "dimension mismatch";
      case SimilarityErrKind::ErrEmptyVector:
        return "empty vector";
      case SimilarityErrKind::ErrZeroVector:
        return "zero vector";
      default:
        return "unknown error";
    }
  }
};

double cosine(const std::vector<double>& a, const std::vector<double>& b);

template <typename T>
double jaccard(std::vector<T>& a, std::vector<T>& b) {
  if (a.empty() || b.empty()) {
    return SimilarityErrKind::ErrEmptyVector;
  }
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());

  std::vector<T> intersection;
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                        std::back_inserter(intersection));
  std::vector<T> union_;
  std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                 std::back_inserter(union_));
  return static_cast<double>(intersection.size()) / union_.size();
}

template <typename T>
double dice(std::vector<T>& a, std::vector<T>& b) {
  if (a.empty() || b.empty()) {
    return SimilarityErrKind::ErrEmptyVector;
  }
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());

  std::vector<T> intersection;
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                        std::back_inserter(intersection));
  return static_cast<double>(2 * intersection.size()) / (a.size() + b.size());
}

double string_levenshtein(const std::string& s1, const std::string& s2);

/// @brief use k-gram to calcuate string similarity
///
/// k-shingling is the operation of transforming a string (or text document)
/// into a set of n-grams, which can be used to measure the similarity between
/// two strings or documents
///
/// Generally speaking, a k-gram is any sequence of k tokens. We use here the
/// definition from Leskovec, Rajaraman &amp; Ullman (2014), "Mining of Massive
/// Datasets", Cambridge University Press: Multiple subsequent spaces are
/// replaced by a single space, and a k-gram is a sequence of k characters.
///
/// Default value of k is 3. A good rule of thumb is to imagine that there are
/// only 20 characters and estimate the number of k-shingles as 20^k. For small
/// documents like e-mails, k = 5 is a recommended value. For large documents,
/// such as research articles, k = 9 is considered a safe choice.
/// \param s1 one string
/// \param s2 the other to calculate similarity
/// \param k k-gram window size
/// \return similarity
double string_cosine(const std::string& s1, const std::string& s2, int k = 7);
};  // namespace util
}  // namespace mergebot
#endif  // MB_INCLUDE_MERGEBOT_UTILS_SIMILARITY_H
