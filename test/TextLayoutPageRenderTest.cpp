#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../src/core/EInkDisplay.h"
#include "../src/rendering/TextRenderer.h"
#include "../src/resources/fonts/NotoSans26.h"
#include "../src/ui/screens/textview/GreedyLayoutStrategy.h"
#include "../src/ui/screens/textview/KnuthPlassLayoutStrategy.h"
#include "../src/ui/screens/textview/StringWordProvider.h"
#include "mocks/WString.h"
#include "mocks/platform_stubs.h"
#include "test_config.h"
#include "test_utils.h"

// For test build convenience we include some implementation units so the
// single-file test binary links without changing the global build tasks.
#include "../src/ui/screens/textview/GreedyLayoutStrategy.cpp"
#include "../src/ui/screens/textview/KnuthPlassLayoutStrategy.cpp"
#include "../src/ui/screens/textview/StringWordProvider.cpp"

int main() {
  TestUtils::TestRunner runner("TextLayout Page Render Test");

  // Create display with dummy pins
  EInkDisplay display(TestConfig::DUMMY_PIN, TestConfig::DUMMY_PIN, TestConfig::DUMMY_PIN, TestConfig::DUMMY_PIN,
                      TestConfig::DUMMY_PIN, TestConfig::DUMMY_PIN);

  // Initialize (no-op for many functions in desktop stubs)
  display.begin();

  // Clear to white (0xFF is white in driver)
  display.clearScreen(0xFF);

  // Render some text onto the frame buffer using the TextRenderer
  TextRenderer renderer(display);
  // Use our local font
  renderer.setFont(&NotoSans26);
  renderer.setTextColor(TextRenderer::COLOR_BLACK);
  // Render text layout and pagination test
  const int margin = TestConfig::TEST_MARGIN;
  int x = margin;
  int y = 32;
  const std::string filepath = TestConfig::DEFAULT_TEST_FILE;
  std::ifstream infile(filepath);
  if (!infile) {
    std::cerr << "Failed to open '" << filepath << "'\n";
    return 1;
  }

  // Ensure output directory exists
  std::filesystem::create_directories(TestConfig::TEST_OUTPUT_DIR);

  int page = 0;

  auto savePage = [&](int pageIndex, String postfix) {
    // Save the currently rendered framebuffer first. `displayBuffer`
    // swaps the internal buffers, so calling it before saving would
    // write the (now swapped) inactive/empty buffer to disk.
    std::ostringstream ss;
    ss << TestConfig::TEST_OUTPUT_DIR << "/output_" << std::setw(3) << std::setfill('0') << pageIndex
       << std::setfill(' ') << postfix.c_str() << ".pbm";
    std::string out = ss.str();
    display.saveFrameBufferAsPBM(out.c_str());
  };

  // Read entire file into memory and use StringWordProvider + TextLayout
  std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
  String fullText(content.c_str());

  // Precompute a typical line height for spacing and populate layout config
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  renderer.getTextBounds("Ag", x, y, &tbx, &tby, &tbw, &tbh);
  const int lineSpacing = (int)tbh + TestConfig::TEST_LINE_SPACING;

  StringWordProvider provider(fullText);
  GreedyLayoutStrategy layout;
  // KnuthPlassLayoutStrategy layout;
  LayoutStrategy::LayoutConfig layoutConfig;
  layoutConfig.marginLeft = TestConfig::DEFAULT_MARGIN_LEFT;
  layoutConfig.marginRight = TestConfig::DEFAULT_MARGIN_RIGHT;
  layoutConfig.marginTop = TestConfig::DEFAULT_MARGIN_TOP;
  layoutConfig.marginBottom = TestConfig::DEFAULT_MARGIN_BOTTOM;
  layoutConfig.lineHeight = TestConfig::DEFAULT_LINE_HEIGHT;
  layoutConfig.minSpaceWidth = TestConfig::DEFAULT_MIN_SPACE_WIDTH;
  layoutConfig.pageWidth = TestConfig::DISPLAY_WIDTH;
  layoutConfig.pageHeight = TestConfig::DISPLAY_HEIGHT;
  layoutConfig.alignment = LayoutStrategy::ALIGN_LEFT;

  bool disableRendering = true;  // Disable rendering for performance testing

  // Test mode: false = normal (jump to end of page), true = incremental (move one word at a time)
  bool incrementalMode = true;
  // const int maxPages = 1;  // Limit for incremental mode to prevent excessive iterations
  const int maxPages = 99999;  // Limit for incremental mode to prevent excessive iterations

  // Traverse the entire document forward, and immediately check backward navigation
  std::vector<std::pair<int, int>> pageRanges;  // pair<start, end>
  int pageStart = 0;
  int pageIndex = 0;

  while (true) {
    if (!disableRendering) {
      display.clearScreen(0xFF);
    }

    provider.setPosition(pageStart);

    int endPos = layout.layoutText(provider, renderer, layoutConfig, disableRendering);
    // record the start and end positions for this page
    pageRanges.push_back(std::make_pair(pageStart, endPos));

    if (!disableRendering) {
      savePage(pageIndex, "_0");
    }

    // test backward navigation from current page
    {
      int expectedPrevStart = pageRanges[pageIndex].first;
      int expectedPrevEnd = pageRanges[pageIndex].second;

      int computedPrevStart = layout.getPreviousPageStart(provider, renderer, layoutConfig, endPos);

      // Render the computed previous page to determine its end position
      if (!disableRendering) {
        display.clearScreen(0xFF);
      }
      provider.setPosition(computedPrevStart);
      int computedPrevEnd = layout.layoutText(provider, renderer, layoutConfig, disableRendering);

      bool startMatch = (computedPrevStart == expectedPrevStart);
      bool endMatch = (computedPrevEnd == expectedPrevEnd);

      if (!startMatch || !endMatch) {
        std::string errorMsg = "Page " + std::to_string(pageIndex) +
                               " backward check - computedPrevStart=" + std::to_string(computedPrevStart) +
                               " expectedPrevStart=" + std::to_string(expectedPrevStart) +
                               ", computedPrevEnd=" + std::to_string(computedPrevEnd) +
                               " expectedPrevEnd=" + std::to_string(expectedPrevEnd);
        std::cerr << errorMsg << "\n";
        runner.expectTrue(false, "Backward navigation from page " + std::to_string(pageIndex), errorMsg);
      }
    }

    // Stop if we've reached the end of the provider
    if (provider.getPercentage(endPos) >= 1.0f) {
      ++pageIndex;
      break;
    }

    // Safety: if no progress, break to avoid infinite loop
    if (endPos <= pageStart) {
      std::cerr << "No progress while laying out page " << pageIndex << ", stopping traversal.\n";
      ++pageIndex;
      break;
    }

    // Safety: limit pages in incremental mode
    if (incrementalMode && pageIndex + 1 >= maxPages) {
      std::cerr << "Reached max page limit (" << maxPages << ") in incremental mode, stopping.\n";
      ++pageIndex;
      break;
    }

    if (incrementalMode) {
      // Move one word forward from the start of the current page
      provider.setPosition(pageStart);
      layout.test_getNextLineDefault(
          provider, renderer, layoutConfig.pageWidth - layoutConfig.marginLeft - layoutConfig.marginRight, *(new bool));
      int nextPos = provider.getCurrentIndex();

      // If we can't move forward, we're done
      if (nextPos <= pageStart) {
        std::cerr << "Cannot advance one word from position " << pageStart << ", stopping.\n";
        ++pageIndex;
        break;
      }
      pageStart = nextPos;
    } else {
      // Normal mode: jump to the end of the current page
      pageStart = endPos;
    }
    ++pageIndex;
  }

  std::cout << "Forward traversal produced " << pageRanges.size() << " pages.\n";

  // Write out all page start/end positions to console and a file for inspection
  std::string rangesPath = TestConfig::TEST_OUTPUT_DIR + "/page_ranges.txt";
  std::ofstream rangesOut(rangesPath);
  if (!rangesOut) {
    std::cerr << "Warning: unable to open " << rangesPath << " for writing\n";
  }
  for (size_t i = 0; i < pageRanges.size(); ++i) {
    int s = pageRanges[i].first;
    int e = pageRanges[i].second;
    if (rangesOut)
      rangesOut << i << " " << s << " " << e << "\n";
  }
  if (rangesOut)
    rangesOut.close();

  return runner.allPassed() ? 0 : 1;
}
