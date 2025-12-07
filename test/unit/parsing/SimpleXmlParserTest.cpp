#include <iostream>
#include <string>
#include <vector>

#include "content/xml/SimpleXmlParser.h"
#include "test_config.h"
#include "test_utils.h"

// Test toggles
#define TEST_TEXT_NODE_BIDIRECTIONAL_READING true
#define TEST_FORWARD_NODE_READING true
#define TEST_BACKWARD_NODE_READING true
#define TEST_SEEK_TO_POSITION true

struct NodeInfo {
  SimpleXmlParser::NodeType type;
  String name;
  String textContent;  // For text nodes: first 50 chars
  int textLength;      // For text nodes: total length
  bool isEmpty;

  bool matches(const NodeInfo& other) const {
    return type == other.type && name == other.name && textContent == other.textContent &&
           textLength == other.textLength && isEmpty == other.isEmpty;
  }

  String toString() const {
    String result;
    switch (type) {
      case SimpleXmlParser::Element:
        result = "Element: <" + name + ">";
        if (isEmpty)
          result += " (self-closing)";
        break;
      case SimpleXmlParser::Text: {
        char buf[32];
        sprintf(buf, "%d", textLength);
        result = "Text: \"" + textContent + "\" [" + String(buf) + " chars]";
        break;
      }
      case SimpleXmlParser::EndElement:
        result = "EndElement: </" + name + ">";
        break;
      case SimpleXmlParser::Comment:
        result = "Comment";
        break;
      case SimpleXmlParser::ProcessingInstruction:
        result = "ProcessingInstruction: " + name;
        break;
      default:
        result = "Other node type: " + String((int)type);
        break;
    }
    return result;
  }
};

// Test: Read text nodes forward and backward and verify they match
void testTextNodeBidirectionalReading(TestUtils::TestRunner& runner, const char* xhtmlPath) {
  std::cout << "\n=== Test: Text Node Bidirectional Reading ===\n";

  SimpleXmlParser xmlReader;
  bool opened = xmlReader.open(xhtmlPath);
  runner.expectTrue(opened, "Successfully opened XHTML file");

  if (opened) {
    int nodeCount = 0;
    while (xmlReader.read()) {
      if (xmlReader.getNodeType() == SimpleXmlParser::Text) {
        // Read text forward
        String forwardText = "";
        while (xmlReader.hasMoreTextChars()) {
          char c = xmlReader.readTextNodeCharForward();
          forwardText += c;
        }

        // Read text backward
        String backwardText = "";
        while (xmlReader.hasMoreTextCharsBackward()) {
          char c = xmlReader.readPrevTextNodeChar();
          backwardText = String(c) + backwardText;
        }

        if (forwardText != backwardText) {
          std::cout << "\n*** TEXT MISMATCH in text node " << nodeCount << " ***\n";
          std::cout << "  Forward:  \"" << forwardText.c_str() << "\"\n";
          std::cout << "  Backward: \"" << backwardText.c_str() << "\"\n";
          runner.expectTrue(false, "Forward and backward text should match");
          xmlReader.close();
          return;
        }
        nodeCount++;
      }
    }
    std::cout << "Verified " << nodeCount << " text nodes - all match!\n";
    runner.expectTrue(true, "All text nodes match between forward and backward reading");
    xmlReader.close();
  }
}

// Test: Read all nodes forward and store them
std::vector<NodeInfo> testForwardNodeReading(TestUtils::TestRunner& runner, const char* xhtmlPath) {
  std::cout << "\n=== Test: Forward Node Reading ===\n";

  SimpleXmlParser xmlReader;
  std::vector<NodeInfo> forwardNodes;

  bool opened = xmlReader.open(xhtmlPath);
  runner.expectTrue(opened, "Successfully opened XHTML file");

  if (opened) {
    int nodeCount = 0;
    while (xmlReader.read()) {
      NodeInfo node;
      node.type = xmlReader.getNodeType();
      node.name = xmlReader.getName();
      node.isEmpty = xmlReader.isEmptyElement();

      // For text nodes, read characters
      if (node.type == SimpleXmlParser::Text) {
        node.textContent = "";
        node.textLength = 0;
        while (xmlReader.hasMoreTextChars()) {
          char c = xmlReader.readTextNodeCharForward();
          node.textContent += c;
          node.textLength++;
        }
      } else {
        node.textContent = "";
        node.textLength = 0;
      }

      forwardNodes.push_back(node);
      nodeCount++;

      // Print first 20 nodes
      if (nodeCount <= 20) {
        std::cout << nodeCount << ". " << node.toString().c_str() << "\n";
      }
    }

    std::cout << "\nTotal nodes read forward: " << forwardNodes.size() << "\n";

    // Save complete forward output to file
    FILE* fwdFile = fopen("test/output/forward_complete.txt", "w");
    if (fwdFile) {
      for (size_t i = 0; i < forwardNodes.size(); i++) {
        fprintf(fwdFile, "%zu. %s\n", i + 1, forwardNodes[i].toString().c_str());
      }
      fclose(fwdFile);
      std::cout << "Complete forward output saved to test/output/forward_complete.txt\n";
    }

    xmlReader.close();
  }

  return forwardNodes;
}

// Test: Read all nodes backward and compare with forward reading
void testBackwardNodeReading(TestUtils::TestRunner& runner, const char* xhtmlPath,
                             const std::vector<NodeInfo>& forwardNodes) {
  std::cout << "\n=== Test: Backward Node Reading ===\n";

  SimpleXmlParser xmlReader;
  std::vector<NodeInfo> backwardNodes;

  bool opened = xmlReader.open(xhtmlPath);
  runner.expectTrue(opened, "Successfully reopened XHTML file");

  if (opened) {
    // Seek to end of file before reading backward
    xmlReader.seekToFilePosition(xmlReader.getFileSize());

    int nodeCount = 0;
    bool mismatchFound = false;

    while (xmlReader.readBackward()) {
      NodeInfo node;
      node.type = xmlReader.getNodeType();
      node.name = xmlReader.getName();
      node.isEmpty = xmlReader.isEmptyElement();

      // For text nodes, read characters (backward reading gives chars in reverse, so prepend)
      if (node.type == SimpleXmlParser::Text) {
        node.textContent = "";
        node.textLength = 0;
        while (xmlReader.hasMoreTextCharsBackward()) {
          char c = xmlReader.readPrevTextNodeChar();
          // Prepend character since we're reading backward
          node.textContent = String(c) + node.textContent;
          node.textLength++;
        }
      } else {
        node.textContent = "";
        node.textLength = 0;
      }

      backwardNodes.push_back(node);
      nodeCount++;

      // Print first 20 nodes from backward
      if (nodeCount <= 20) {
        std::cout << nodeCount << ". " << node.toString().c_str() << "\n";
      }

      // Check if this node matches the corresponding forward node
      size_t forwardIndex = forwardNodes.size() - nodeCount;
      if (forwardIndex < forwardNodes.size()) {
        if (!forwardNodes[forwardIndex].matches(node)) {
          std::cout << "\n*** MISMATCH DETECTED at backward node " << nodeCount << " ***\n";
          std::cout << "  Expected (forward #" << forwardIndex << "): " << forwardNodes[forwardIndex].toString().c_str()
                    << "\n";
          std::cout << "  Got (backward #" << nodeCount << "):      " << node.toString().c_str() << "\n";
          mismatchFound = true;
          runner.expectTrue(false, "Backward node should match forward node");
          break;
        }
      }
    }

    std::cout << "\nTotal nodes read backward: " << backwardNodes.size() << "\n";

    // Save complete backward output to file
    FILE* bwdFile = fopen("test/output/backward_complete.txt", "w");
    if (bwdFile) {
      for (size_t i = 0; i < backwardNodes.size(); i++) {
        fprintf(bwdFile, "%zu. %s\n", i + 1, backwardNodes[i].toString().c_str());
      }
      fclose(bwdFile);
      std::cout << "Complete backward output saved to test/output/backward_complete.txt\n";
    }

    if (!mismatchFound) {
      std::cout << "\n--- Verifying complete match ---\n";

      runner.expectTrue(forwardNodes.size() == backwardNodes.size(),
                        "Forward and backward should have same number of nodes");

      if (forwardNodes.size() == backwardNodes.size()) {
        std::cout << "All " << forwardNodes.size() << " nodes match! Forward and backward reading are consistent.\n";
        runner.expectTrue(true, "All nodes match between forward and backward");
      }
    }

    xmlReader.close();
  }
}

// Helper struct for storing text node info during seek tests
struct TextNodeInfo {
  size_t startPos;
  size_t endPos;
  String fullText;
};

// Helper struct for storing element info during seek tests
struct ElementInfo {
  size_t position;
  String name;
  bool isEmpty;
  SimpleXmlParser::NodeType type;
};

// Helper struct for storing complete node state during seek tests
struct SavedNodeState {
  size_t filePosition;
  SimpleXmlParser::NodeType nodeType;
  String name;
  bool isEmpty;
  size_t textNodeStart;
  size_t textNodeEnd;
  String fullText;  // Complete text content for text nodes

  String toString() const {
    String result;
    switch (nodeType) {
      case SimpleXmlParser::Element:
        result = "Element: <" + name + ">";
        if (isEmpty)
          result += " (self-closing)";
        break;
      case SimpleXmlParser::Text: {
        char buf[32];
        sprintf(buf, "%zu", fullText.length());
        String preview = fullText.length() > 50 ? fullText.substring(0, 50) + "..." : fullText;
        result = "Text: \"" + preview + "\" [" + String(buf) + " chars]";
        break;
      }
      case SimpleXmlParser::EndElement:
        result = "EndElement: </" + name + ">";
        break;
      case SimpleXmlParser::Comment:
        result = "Comment";
        break;
      case SimpleXmlParser::ProcessingInstruction:
        result = "ProcessingInstruction: " + name;
        break;
      default:
        result = "Other node type: " + String((int)nodeType);
        break;
    }
    return result;
  }

  bool matches(const SavedNodeState& other) const {
    if (nodeType != other.nodeType)
      return false;
    if (name != other.name)
      return false;
    if (isEmpty != other.isEmpty)
      return false;
    if (nodeType == SimpleXmlParser::Text) {
      if (fullText != other.fullText)
        return false;
    }
    return true;
  }
};

// Test: Seek to various file positions and verify correct node context
void testSeekToPosition(TestUtils::TestRunner& runner, const char* xhtmlPath) {
  std::cout << "\n=== Test: Seek To File Position ===\n";

  SimpleXmlParser xmlReader;
  bool opened = xmlReader.open(xhtmlPath);
  runner.expectTrue(opened, "Successfully opened XHTML file");

  if (!opened) {
    return;
  }

  // ============================================================
  // Phase 1: Forward pass - collect all node states with positions
  // ============================================================
  std::cout << "\n--- Phase 1: Forward pass - collecting all node states ---\n";

  std::vector<SavedNodeState> forwardStates;
  size_t positionBeforeRead = 0;
  while (true) {
    positionBeforeRead = xmlReader.getFilePosition();
    if (!xmlReader.read()) {
      break;
    }

    SavedNodeState state;
    state.nodeType = xmlReader.getNodeType();
    state.name = xmlReader.getName();
    state.isEmpty = xmlReader.isEmptyElement();
    state.textNodeStart = xmlReader.getTextNodeStart();
    state.textNodeEnd = xmlReader.getTextNodeEnd();
    state.fullText = "";

    // For text nodes, use textNodeStart as the position to seek to
    // For other nodes, use the position before read() was called
    if (state.nodeType == SimpleXmlParser::Text) {
      state.filePosition = state.textNodeStart;
      while (xmlReader.hasMoreTextChars()) {
        state.fullText += xmlReader.readTextNodeCharForward();
      }
    } else {
      state.filePosition = positionBeforeRead;
    }

    forwardStates.push_back(state);
  }

  std::cout << "  Collected " << forwardStates.size() << " nodes in forward pass\n";

  // Print first few nodes
  int printCount = std::min((size_t)5, forwardStates.size());
  for (int i = 0; i < printCount; i++) {
    std::cout << "    [" << i << "] @ " << forwardStates[i].filePosition << ": " << forwardStates[i].toString().c_str()
              << "\n";
  }

  // ============================================================
  // Phase 2: Backward pass - collect all node states with positions
  // ============================================================
  std::cout << "\n--- Phase 2: Backward pass - collecting all node states ---\n";

  xmlReader.seekToFilePosition(xmlReader.getFileSize());

  std::vector<SavedNodeState> backwardStates;
  while (xmlReader.readBackward()) {
    SavedNodeState state;
    state.nodeType = xmlReader.getNodeType();
    state.name = xmlReader.getName();
    state.isEmpty = xmlReader.isEmptyElement();
    state.textNodeStart = xmlReader.getTextNodeStart();
    state.textNodeEnd = xmlReader.getTextNodeEnd();
    state.fullText = "";

    // For text nodes, use textNodeStart as the position to seek to
    // For other nodes, use getFilePosition() which should be at node start after readBackward
    if (state.nodeType == SimpleXmlParser::Text) {
      state.filePosition = state.textNodeStart;
      while (xmlReader.hasMoreTextCharsBackward()) {
        state.fullText = String(xmlReader.readPrevTextNodeChar()) + state.fullText;
      }
    } else {
      state.filePosition = xmlReader.getFilePosition();
    }

    backwardStates.push_back(state);
  }

  std::cout << "  Collected " << backwardStates.size() << " nodes in backward pass\n";

  // Print first few backward nodes
  printCount = std::min((size_t)5, backwardStates.size());
  for (int i = 0; i < printCount; i++) {
    std::cout << "    [" << i << "] @ " << backwardStates[i].filePosition << ": "
              << backwardStates[i].toString().c_str() << "\n";
  }

  // ============================================================
  // Phase 3: Verify forward states by seeking to each position
  // ============================================================
  std::cout << "\n--- Phase 3: Verify all forward positions by seeking ---\n";

  int forwardMismatches = 0;
  for (size_t i = 0; i < forwardStates.size(); i++) {
    const SavedNodeState& expected = forwardStates[i];

    bool seekOk = xmlReader.seekToFilePosition(expected.filePosition);
    if (!seekOk) {
      std::cout << "  ERROR: Failed to seek to forward position " << expected.filePosition << " for node " << i << "\n";
      forwardMismatches++;
      continue;
    }

    // First, verify that getFilePosition() returns the position we seeked to
    size_t posAfterSeek = xmlReader.getFilePosition();
    if (posAfterSeek != expected.filePosition) {
      std::cout << "  POSITION MISMATCH at node " << i << ": seeked to " << expected.filePosition
                << " but getFilePosition() returned " << posAfterSeek << "\n";
      forwardMismatches++;
      continue;
    }

    // For text nodes, seekToFilePosition already sets up the text node state
    // For other nodes, we need to call read() to parse the node
    if (xmlReader.getNodeType() != SimpleXmlParser::Text) {
      if (!xmlReader.read()) {
        std::cout << "  ERROR: Failed to read node at forward position " << expected.filePosition << " for node " << i
                  << "\n";
        forwardMismatches++;
        continue;
      }
    }

    // Read the node at this position
    SavedNodeState actual;
    actual.nodeType = xmlReader.getNodeType();
    actual.name = xmlReader.getName();
    actual.isEmpty = xmlReader.isEmptyElement();
    actual.textNodeStart = xmlReader.getTextNodeStart();
    actual.textNodeEnd = xmlReader.getTextNodeEnd();
    actual.fullText = "";

    if (actual.nodeType == SimpleXmlParser::Text) {
      while (xmlReader.hasMoreTextChars()) {
        actual.fullText += xmlReader.readTextNodeCharForward();
      }
    }

    if (!expected.matches(actual)) {
      std::cout << "  MISMATCH at forward node " << i << " (pos " << expected.filePosition << "):\n";
      std::cout << "    Expected: " << expected.toString().c_str() << "\n";
      std::cout << "    Actual:   " << actual.toString().c_str() << "\n";
      forwardMismatches++;
    }
  }

  std::cout << "  Forward verification: " << (forwardStates.size() - forwardMismatches) << "/" << forwardStates.size()
            << " positions match\n";
  runner.expectTrue(forwardMismatches == 0, "All forward positions should produce exact same output when seeked to");

  // ============================================================
  // Phase 4: Verify backward states by seeking to each position
  // ============================================================
  std::cout << "\n--- Phase 4: Verify all backward positions by seeking ---\n";

  int backwardMismatches = 0;
  for (size_t i = 0; i < backwardStates.size(); i++) {
    const SavedNodeState& expected = backwardStates[i];

    bool seekOk = xmlReader.seekToFilePosition(expected.filePosition);
    if (!seekOk) {
      std::cout << "  ERROR: Failed to seek to backward position " << expected.filePosition << " for node " << i
                << "\n";
      backwardMismatches++;
      continue;
    }

    // For text nodes, seekToFilePosition already sets up the text node state
    // For other nodes, we need to call read() to parse the node
    if (xmlReader.getNodeType() != SimpleXmlParser::Text) {
      if (!xmlReader.read()) {
        std::cout << "  ERROR: Failed to read node at backward position " << expected.filePosition << " for node " << i
                  << "\n";
        backwardMismatches++;
        continue;
      }
    }

    // Read the node at this position
    SavedNodeState actual;
    actual.nodeType = xmlReader.getNodeType();
    actual.name = xmlReader.getName();
    actual.isEmpty = xmlReader.isEmptyElement();
    actual.textNodeStart = xmlReader.getTextNodeStart();
    actual.textNodeEnd = xmlReader.getTextNodeEnd();
    actual.fullText = "";

    if (actual.nodeType == SimpleXmlParser::Text) {
      while (xmlReader.hasMoreTextChars()) {
        actual.fullText += xmlReader.readTextNodeCharForward();
      }
    }

    if (!expected.matches(actual)) {
      std::cout << "  MISMATCH at backward node " << i << " (pos " << expected.filePosition << "):\n";
      std::cout << "    Expected: " << expected.toString().c_str() << "\n";
      std::cout << "    Actual:   " << actual.toString().c_str() << "\n";
      backwardMismatches++;
    }
  }

  std::cout << "  Backward verification: " << (backwardStates.size() - backwardMismatches) << "/"
            << backwardStates.size() << " positions match\n";
  runner.expectTrue(backwardMismatches == 0, "All backward positions should produce exact same output when seeked to");

  // ============================================================
  // Phase 5: Cross-verify forward and backward positions match
  // ============================================================
  std::cout << "\n--- Phase 5: Cross-verify forward and backward states ---\n";

  runner.expectTrue(forwardStates.size() == backwardStates.size(),
                    "Forward and backward should have same number of nodes");

  if (forwardStates.size() == backwardStates.size()) {
    int crossMismatches = 0;
    for (size_t i = 0; i < forwardStates.size(); i++) {
      size_t backwardIndex = backwardStates.size() - 1 - i;
      const SavedNodeState& fwd = forwardStates[i];
      const SavedNodeState& bwd = backwardStates[backwardIndex];

      if (!fwd.matches(bwd)) {
        std::cout << "  Cross-mismatch at node " << i << ":\n";
        std::cout << "    Forward:  " << fwd.toString().c_str() << "\n";
        std::cout << "    Backward: " << bwd.toString().c_str() << "\n";
        crossMismatches++;
      }
    }
    std::cout << "  Cross-verification: " << (forwardStates.size() - crossMismatches) << "/" << forwardStates.size()
              << " nodes match\n";
    runner.expectTrue(crossMismatches == 0, "Forward and backward states should match when reversed");
  }

  // ============================================================
  // Phase 6: Random access verification - seek in random order
  // ============================================================
  std::cout << "\n--- Phase 6: Random access verification ---\n";

  // Create a shuffled index order (simple pseudo-random)
  std::vector<size_t> randomOrder;
  for (size_t i = 0; i < forwardStates.size(); i++) {
    randomOrder.push_back(i);
  }
  // Simple shuffle using a basic LCG-like approach
  for (size_t i = randomOrder.size() - 1; i > 0; i--) {
    size_t j = (i * 31337 + 12345) % (i + 1);
    std::swap(randomOrder[i], randomOrder[j]);
  }

  int randomMismatches = 0;
  size_t randomTestCount = std::min((size_t)100, forwardStates.size());  // Test up to 100 random positions
  for (size_t t = 0; t < randomTestCount; t++) {
    size_t i = randomOrder[t];
    const SavedNodeState& expected = forwardStates[i];

    bool seekOk = xmlReader.seekToFilePosition(expected.filePosition);
    if (!seekOk) {
      randomMismatches++;
      continue;
    }

    // For text nodes, seekToFilePosition already sets up the text node state
    // For other nodes, we need to call read() to parse the node
    if (xmlReader.getNodeType() != SimpleXmlParser::Text) {
      if (!xmlReader.read()) {
        randomMismatches++;
        continue;
      }
    }

    SavedNodeState actual;
    actual.nodeType = xmlReader.getNodeType();
    actual.name = xmlReader.getName();
    actual.isEmpty = xmlReader.isEmptyElement();
    actual.fullText = "";

    if (actual.nodeType == SimpleXmlParser::Text) {
      while (xmlReader.hasMoreTextChars()) {
        actual.fullText += xmlReader.readTextNodeCharForward();
      }
    }

    if (!expected.matches(actual)) {
      std::cout << "  Random access mismatch at node " << i << " (pos " << expected.filePosition << "):\n";
      std::cout << "    Expected: " << expected.toString().c_str() << "\n";
      std::cout << "    Actual:   " << actual.toString().c_str() << "\n";
      randomMismatches++;
    }
  }

  std::cout << "  Random access verification: " << (randomTestCount - randomMismatches) << "/" << randomTestCount
            << " positions match\n";
  runner.expectTrue(randomMismatches == 0, "Random access to stored positions should produce exact same output");

  // ============================================================
  // Additional tests below using the collected data
  // ============================================================

  // Build textNodes and elements from forwardStates for compatibility with remaining tests
  std::vector<TextNodeInfo> textNodes;
  std::vector<ElementInfo> elements;

  for (const auto& state : forwardStates) {
    if (state.nodeType == SimpleXmlParser::Text && state.fullText.length() > 0) {
      TextNodeInfo info;
      info.startPos = state.textNodeStart;
      info.endPos = state.textNodeEnd;
      info.fullText = state.fullText;
      textNodes.push_back(info);
    } else if (state.nodeType == SimpleXmlParser::Element || state.nodeType == SimpleXmlParser::EndElement) {
      ElementInfo info;
      info.position = state.filePosition;
      info.name = state.name;
      info.isEmpty = state.isEmpty;
      info.type = state.nodeType;
      elements.push_back(info);
    }
  }

  std::cout << "\nCollected " << textNodes.size() << " text nodes and " << elements.size() << " elements\n";

  // ============================================================
  // Test 1: Seek to exact start of text nodes
  // ============================================================
  std::cout << "\n--- Test 1: Seek to exact start of text nodes ---\n";
  int testCount = std::min((size_t)5, textNodes.size());
  for (int i = 0; i < testCount; i++) {
    const TextNodeInfo& info = textNodes[i];
    bool seekOk = xmlReader.seekToFilePosition(info.startPos);
    runner.expectTrue(seekOk, "Seek to text start succeeded");

    if (seekOk) {
      runner.expectTrue(xmlReader.getNodeType() == SimpleXmlParser::Text, "Node type is Text at start position");

      if (xmlReader.getNodeType() == SimpleXmlParser::Text) {
        // Read first char and verify it matches
        if (xmlReader.hasMoreTextChars()) {
          char c = xmlReader.readTextNodeCharForward();
          char expected = info.fullText.length() > 0 ? info.fullText[0] : '\0';
          runner.expectTrue(c == expected, "First char matches expected");
          std::cout << "  Text[" << i << "] start @ " << info.startPos << " -> first char: '" << c << "' (expected: '"
                    << expected << "')\n";
        }
      }
    }
  }

  // ============================================================
  // Test 2: Seek to middle of text nodes
  // ============================================================
  std::cout << "\n--- Test 2: Seek to middle of text nodes ---\n";
  for (int i = 0; i < testCount; i++) {
    const TextNodeInfo& info = textNodes[i];
    if (info.fullText.length() < 3)
      continue;

    size_t midOffset = info.fullText.length() / 2;
    size_t midPos = info.startPos + midOffset;

    bool seekOk = xmlReader.seekToFilePosition(midPos);
    runner.expectTrue(seekOk, "Seek to text middle succeeded");

    if (seekOk) {
      runner.expectTrue(xmlReader.getNodeType() == SimpleXmlParser::Text, "Node type is Text at middle position");

      if (xmlReader.getNodeType() == SimpleXmlParser::Text) {
        // Read char and verify it matches expected position
        if (xmlReader.hasMoreTextChars()) {
          char c = xmlReader.readTextNodeCharForward();
          char expected = info.fullText[midOffset];
          runner.expectTrue(c == expected, "Middle char matches expected");
          std::cout << "  Text[" << i << "] mid @ " << midPos << " (offset " << midOffset << ") -> char: '" << c
                    << "' (expected: '" << expected << "')\n";
        }
      }
    }
  }

  // ============================================================
  // Test 3: Seek to end of text nodes (one before end)
  // ============================================================
  std::cout << "\n--- Test 3: Seek to end of text nodes ---\n";
  for (int i = 0; i < testCount; i++) {
    const TextNodeInfo& info = textNodes[i];
    if (info.fullText.length() < 2)
      continue;

    size_t endOffset = info.fullText.length() - 1;
    size_t endPos = info.startPos + endOffset;

    bool seekOk = xmlReader.seekToFilePosition(endPos);
    runner.expectTrue(seekOk, "Seek to text end succeeded");

    if (seekOk) {
      runner.expectTrue(xmlReader.getNodeType() == SimpleXmlParser::Text, "Node type is Text at end position");

      if (xmlReader.getNodeType() == SimpleXmlParser::Text) {
        if (xmlReader.hasMoreTextChars()) {
          char c = xmlReader.readTextNodeCharForward();
          char expected = info.fullText[endOffset];
          runner.expectTrue(c == expected, "End char matches expected");
          std::cout << "  Text[" << i << "] end @ " << endPos << " -> char: '" << c << "' (expected: '" << expected
                    << "')\n";
        }
      }
    }
  }

  // ============================================================
  // Test 4: Seek within same text node multiple times (random access)
  // ============================================================
  std::cout << "\n--- Test 4: Random access within same text node ---\n";
  if (textNodes.size() > 0) {
    // Find a long text node for this test
    const TextNodeInfo* longNode = nullptr;
    for (const auto& node : textNodes) {
      if (node.fullText.length() >= 50) {
        longNode = &node;
        break;
      }
    }

    if (longNode) {
      std::cout << "  Using text node with " << longNode->fullText.length() << " chars\n";

      // Seek to various positions in different orders
      size_t len = longNode->fullText.length();
      std::vector<size_t> offsets = {0, len - 1, len / 2, len / 4, (len * 3) / 4, 10, 5};

      for (size_t offset : offsets) {
        if (offset >= longNode->fullText.length())
          continue;

        size_t pos = longNode->startPos + offset;
        xmlReader.seekToFilePosition(pos);

        runner.expectTrue(xmlReader.getNodeType() == SimpleXmlParser::Text, "Still in text node after random seek");

        if (xmlReader.hasMoreTextChars()) {
          char c = xmlReader.readTextNodeCharForward();
          char expected = longNode->fullText[offset];
          runner.expectTrue(c == expected, "Char at random position matches");
          std::cout << "    Offset " << offset << " -> '" << c << "' (expected: '" << expected << "')\n";
        }
      }
    } else {
      std::cout << "  No text node with 50+ chars found, skipping\n";
    }
  }

  // ============================================================
  // Test 5: Seek to element positions
  // ============================================================
  std::cout << "\n--- Test 5: Seek to element positions ---\n";
  int elemTestCount = std::min((size_t)5, elements.size());
  for (int i = 0; i < elemTestCount; i++) {
    const ElementInfo& info = elements[i];
    bool seekOk = xmlReader.seekToFilePosition(info.position);
    runner.expectTrue(seekOk, "Seek to element position succeeded");

    if (seekOk) {
      std::cout << "  Elem[" << i << "] @ " << info.position << " -> NodeType: " << (int)xmlReader.getNodeType()
                << ", Name: " << xmlReader.getName().c_str() << "\n";
    }
  }

  // ============================================================
  // Test 6: Seek to file boundaries
  // ============================================================
  std::cout << "\n--- Test 6: Seek to file boundaries ---\n";

  bool seekStart = xmlReader.seekToFilePosition(0);
  runner.expectTrue(seekStart, "Seek to position 0 succeeded");
  std::cout << "  Seek to 0 -> NodeType: " << (int)xmlReader.getNodeType() << "\n";

  size_t fileSize = xmlReader.getFileSize();
  bool seekEnd = xmlReader.seekToFilePosition(fileSize);
  runner.expectTrue(seekEnd, "Seek to end of file succeeded");
  std::cout << "  Seek to " << fileSize << " (EOF) -> NodeType: " << (int)xmlReader.getNodeType() << "\n";

  // Seek to near-end positions
  if (fileSize > 100) {
    bool seekNearEnd = xmlReader.seekToFilePosition(fileSize - 50);
    runner.expectTrue(seekNearEnd, "Seek to near-end position succeeded");
    std::cout << "  Seek to " << (fileSize - 50) << " (near EOF) -> NodeType: " << (int)xmlReader.getNodeType() << "\n";
  }

  // ============================================================
  // Test 7: Seek and then continue reading forward
  // ============================================================
  std::cout << "\n--- Test 7: Seek and continue reading forward ---\n";
  if (textNodes.size() > 2) {
    // Seek to middle of file
    const TextNodeInfo& midNode = textNodes[textNodes.size() / 2];
    xmlReader.seekToFilePosition(midNode.startPos);

    std::cout << "  After seeking to text node at " << midNode.startPos << ":\n";

    // Read next few nodes
    for (int i = 0; i < 5; i++) {
      if (xmlReader.read()) {
        if (xmlReader.getNodeType() == SimpleXmlParser::Text) {
          String text = "";
          for (int j = 0; j < 20 && xmlReader.hasMoreTextChars(); j++) {
            text += xmlReader.readTextNodeCharForward();
          }
          std::cout << "    Node " << i << ": Text \"" << text.c_str() << "...\"\n";
        } else if (xmlReader.getNodeType() == SimpleXmlParser::Element) {
          std::cout << "    Node " << i << ": Element <" << xmlReader.getName().c_str() << ">\n";
        } else if (xmlReader.getNodeType() == SimpleXmlParser::EndElement) {
          std::cout << "    Node " << i << ": EndElement </" << xmlReader.getName().c_str() << ">\n";
        }
      }
    }
  }

  // ============================================================
  // Test 8: Seek and then continue reading backward
  // ============================================================
  std::cout << "\n--- Test 8: Seek and continue reading backward ---\n";
  if (textNodes.size() > 2) {
    // Seek to middle of file
    const TextNodeInfo& midNode = textNodes[textNodes.size() / 2];
    xmlReader.seekToFilePosition(midNode.endPos);

    std::cout << "  After seeking to " << midNode.endPos << ":\n";

    // Read previous few nodes
    for (int i = 0; i < 5; i++) {
      if (xmlReader.readBackward()) {
        if (xmlReader.getNodeType() == SimpleXmlParser::Text) {
          String text = "";
          int charCount = 0;
          while (xmlReader.hasMoreTextCharsBackward() && charCount < 20) {
            text = String(xmlReader.readPrevTextNodeChar()) + text;
            charCount++;
          }
          std::cout << "    Node " << i << ": Text \"..." << text.c_str() << "\"\n";
        } else if (xmlReader.getNodeType() == SimpleXmlParser::Element) {
          std::cout << "    Node " << i << ": Element <" << xmlReader.getName().c_str() << ">\n";
        } else if (xmlReader.getNodeType() == SimpleXmlParser::EndElement) {
          std::cout << "    Node " << i << ": EndElement </" << xmlReader.getName().c_str() << ">\n";
        }
      }
    }
  }

  // ============================================================
  // Test 9: Bidirectional navigation from seek position
  // ============================================================
  std::cout << "\n--- Test 9: Bidirectional navigation from seek position ---\n";
  if (textNodes.size() > 5) {
    const TextNodeInfo& node = textNodes[5];
    size_t midPos = node.startPos + node.fullText.length() / 2;

    xmlReader.seekToFilePosition(midPos);
    std::cout << "  Seeked to middle of text node at " << midPos << "\n";

    // Read forward from this position
    String forwardText = "";
    while (xmlReader.hasMoreTextChars() && forwardText.length() < 15) {
      forwardText += xmlReader.readTextNodeCharForward();
    }
    std::cout << "    Forward from mid: \"" << forwardText.c_str() << "\"\n";

    // Seek back to same position and read backward
    xmlReader.seekToFilePosition(midPos);
    String backwardText = "";
    while (xmlReader.hasMoreTextCharsBackward() && backwardText.length() < 15) {
      backwardText = String(xmlReader.readPrevTextNodeChar()) + backwardText;
    }
    std::cout << "    Backward from mid: \"" << backwardText.c_str() << "\"\n";

    // Verify: forward + backward should cover a range around midPos
    runner.expectTrue(forwardText.length() > 0, "Forward reading from seek position works");
    runner.expectTrue(backwardText.length() > 0, "Backward reading from seek position works");
  }

  // ============================================================
  // Test 10: Seek to every character in a short text node
  // ============================================================
  std::cout << "\n--- Test 10: Seek to every position in a text node ---\n";
  // Find a text node with 10-30 characters
  const TextNodeInfo* shortNode = nullptr;
  for (const auto& node : textNodes) {
    if (node.fullText.length() >= 10 && node.fullText.length() <= 30) {
      shortNode = &node;
      break;
    }
  }

  if (shortNode) {
    std::cout << "  Testing text node: \"" << shortNode->fullText.c_str() << "\" (" << shortNode->fullText.length()
              << " chars)\n";

    bool allMatch = true;
    for (size_t i = 0; i < shortNode->fullText.length(); i++) {
      size_t pos = shortNode->startPos + i;
      xmlReader.seekToFilePosition(pos);

      if (xmlReader.getNodeType() != SimpleXmlParser::Text) {
        std::cout << "    ERROR: Position " << pos << " is not Text node!\n";
        allMatch = false;
        break;
      }

      if (xmlReader.hasMoreTextChars()) {
        char c = xmlReader.readTextNodeCharForward();
        char expected = shortNode->fullText[i];
        if (c != expected) {
          std::cout << "    ERROR: Position " << i << " expected '" << expected << "' got '" << c << "'\n";
          allMatch = false;
          break;
        }
      }
    }

    if (allMatch) {
      std::cout << "    All " << shortNode->fullText.length() << " positions verified correctly!\n";
    }
    runner.expectTrue(allMatch, "All character positions in text node are seekable and correct");
  } else {
    std::cout << "  No suitable text node found (10-30 chars), skipping\n";
  }

  // ============================================================
  // Test 11: Seek consistency - seek to same position multiple times
  // ============================================================
  std::cout << "\n--- Test 11: Seek consistency (repeated seeks to same position) ---\n";
  if (textNodes.size() > 0) {
    const TextNodeInfo& node = textNodes[0];
    size_t testPos = node.startPos + 5;

    std::cout << "  Seeking to position " << testPos << " multiple times:\n";

    for (int attempt = 0; attempt < 3; attempt++) {
      xmlReader.seekToFilePosition(testPos);
      runner.expectTrue(xmlReader.getNodeType() == SimpleXmlParser::Text, "Consistent node type on repeated seek");

      if (xmlReader.hasMoreTextChars()) {
        char c = xmlReader.readTextNodeCharForward();
        std::cout << "    Attempt " << (attempt + 1) << ": '" << c << "'\n";
      }
    }
  }

  // ============================================================
  // Test 12: Seek across different node types
  // ============================================================
  std::cout << "\n--- Test 12: Seek across different node types ---\n";
  if (textNodes.size() > 3 && elements.size() > 3) {
    // Alternate between text and element positions
    std::vector<std::pair<size_t, std::string>> positions = {
        {textNodes[0].startPos, "text[0]"   },
        {elements[0].position,  "element[0]"},
        {textNodes[1].startPos, "text[1]"   },
        {elements[1].position,  "element[1]"},
        {textNodes[2].startPos, "text[2]"   },
    };

    for (const auto& p : positions) {
      bool seekOk = xmlReader.seekToFilePosition(p.first);
      runner.expectTrue(seekOk, ("Seek to " + p.second + " succeeded").c_str());
      std::cout << "  Seek to " << p.second << " @ " << p.first << " -> NodeType: " << (int)xmlReader.getNodeType()
                << "\n";
    }
  }

  // ============================================================
  // Test 13: restoreState function test
  // ============================================================
  std::cout << "\n--- Test 13: State save and restore ---\n";
  if (textNodes.size() > 0) {
    const TextNodeInfo& node = textNodes[0];

    // Seek to middle of a text node
    size_t savedPos = node.startPos + node.fullText.length() / 2;
    xmlReader.seekToFilePosition(savedPos);

    // Save state
    size_t pos = xmlReader.getFilePosition();
    SimpleXmlParser::NodeType nodeType = xmlReader.getNodeType();
    size_t textStart = xmlReader.getTextNodeStart();
    size_t textEnd = xmlReader.getTextNodeEnd();
    String elemName = xmlReader.getName();
    bool isEmpty = xmlReader.isEmptyElement();

    std::cout << "  Saved state: pos=" << pos << ", nodeType=" << (int)nodeType << ", textStart=" << textStart
              << ", textEnd=" << textEnd << "\n";

    // Read some characters to change state
    String readChars = "";
    for (int i = 0; i < 5 && xmlReader.hasMoreTextChars(); i++) {
      readChars += xmlReader.readTextNodeCharForward();
    }
    std::cout << "  Read chars: \"" << readChars.c_str() << "\"\n";

    // Move to different position
    xmlReader.seekToFilePosition(0);
    std::cout << "  Moved to position 0\n";

    // Restore state
    xmlReader.restoreState(pos, nodeType, textStart, textEnd, elemName, isEmpty);
    std::cout << "  Restored state\n";

    // Verify we can read from restored position
    runner.expectTrue(xmlReader.getNodeType() == nodeType, "Node type matches after restore");
    runner.expectTrue(xmlReader.getTextNodeStart() == textStart, "Text start matches after restore");
    runner.expectTrue(xmlReader.getTextNodeEnd() == textEnd, "Text end matches after restore");

    // Read same chars again
    String readCharsAfterRestore = "";
    for (int i = 0; i < 5 && xmlReader.hasMoreTextChars(); i++) {
      readCharsAfterRestore += xmlReader.readTextNodeCharForward();
    }
    std::cout << "  Read chars after restore: \"" << readCharsAfterRestore.c_str() << "\"\n";

    runner.expectTrue(readChars == readCharsAfterRestore, "Same chars read after state restore");
  }

  std::cout << "\nSeek to position tests completed!\n";
  xmlReader.close();
}

int main(int argc, char** argv) {
  TestUtils::TestRunner runner("SimpleXmlParser Test");

  // const char* xhtmlPath = "data/books/Dr. Mabuse, der Spieler.xhtml";
  const char* xhtmlPath = "data/books/1A9A8A09379E4577B2346DECBE09D19A.xhtml";

  std::vector<NodeInfo> forwardNodes;

  // Run tests based on toggles
#if TEST_TEXT_NODE_BIDIRECTIONAL_READING
  testTextNodeBidirectionalReading(runner, xhtmlPath);
#endif

#if TEST_FORWARD_NODE_READING
  forwardNodes = testForwardNodeReading(runner, xhtmlPath);
#endif

#if TEST_BACKWARD_NODE_READING
  if (forwardNodes.empty()) {
    std::cout << "\nWarning: Forward node reading must be enabled to run backward test\n";
    std::cout << "Running forward reading first...\n";
    forwardNodes = testForwardNodeReading(runner, xhtmlPath);
  }
  testBackwardNodeReading(runner, xhtmlPath, forwardNodes);
#endif

#if TEST_SEEK_TO_POSITION
  testSeekToPosition(runner, xhtmlPath);
#endif

  return runner.allPassed() ? 0 : 1;
}
