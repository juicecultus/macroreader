#ifndef ENGLISH_HYPHENATION_H
#define ENGLISH_HYPHENATION_H

#include "HyphenationStrategy.h"

class EnglishHyphenation : public HyphenationStrategy {
 public:
  std::vector<size_t> hyphenate(const std::string& word, size_t minWordLength = 6,
                                size_t minFragmentLength = 3) override;

  Language getLanguage() const override {
    return Language::ENGLISH;
  }
};

#endif  // ENGLISH_HYPHENATION_H