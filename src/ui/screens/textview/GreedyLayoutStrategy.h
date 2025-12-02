#ifndef GREEDY_LAYOUT_STRATEGY_H
#define GREEDY_LAYOUT_STRATEGY_H

#include <cstdint>

#include "LayoutStrategy.h"

class GreedyLayoutStrategy : public LayoutStrategy {
 public:
  GreedyLayoutStrategy();
  ~GreedyLayoutStrategy();

  Type getType() const override {
    return GREEDY;
  }

  // Main interface implementation
  int layoutText(WordProvider& provider, TextRenderer& renderer, const LayoutConfig& config,
                 bool disableRendering = false) override;

 private:
  void renderLine(const std::vector<LayoutStrategy::Word>& line, TextRenderer& renderer, int16_t x, int16_t y,
                  int16_t maxWidth, TextAlignment alignment);

 public:
  // Test-only public wrapper to exercise internal line layout helpers from unit tests.
  // Delegates to the strategy-specific getNextLine
  std::vector<LayoutStrategy::Word> test_getNextLine(WordProvider& provider, TextRenderer& renderer, int16_t maxWidth,
                                                     bool& isParagraphEnd);
  Paragraph test_layoutParagraph(WordProvider& provider, TextRenderer& renderer, int16_t maxWidth);
};

#endif
