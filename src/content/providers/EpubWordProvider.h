#ifndef EPUB_WORD_PROVIDER_H
#define EPUB_WORD_PROVIDER_H

#include <SD.h>

#include <cstdint>

#include "../xml/SimpleXmlParser.h"
#include "StringWordProvider.h"
#include "WordProvider.h"

class EpubWordProvider : public WordProvider {
 public:
  // path: SD path to epub file
  // bufSize: decompressed text buffer size (default 4096)
  EpubWordProvider(const char* path, size_t bufSize = 4096);
  ~EpubWordProvider() override;
  bool isValid() const {
    return valid_;
  }

  bool hasNextWord() override;
  bool hasPrevWord() override;
  String getNextWord() override;
  String getPrevWord() override;

  float getPercentage() override;
  float getPercentage(int index) override;
  void setPosition(int index) override;
  int getCurrentIndex() override;
  char peekChar(int offset = 0) override;
  bool isInsideWord() override;
  void ungetWord() override;
  void reset() override;

 private:
  bool valid_ = false;
  size_t bufSize_ = 0;

  String epubPath_;
  String xhtmlPath_;  // Path to extracted XHTML file
  SimpleXmlParser* parser_ = nullptr;

  size_t prevFilePos_;                                              // Previous parser file position for ungetWord()
  bool prevInsideParagraph_ = false;                                // Previous paragraph state for ungetWord()
  SimpleXmlParser::NodeType prevNodeType_ = SimpleXmlParser::None;  // Previous node type for ungetWord()
  size_t prevTextNodeStart_ = 0;                                    // Previous text node start for ungetWord()
  size_t prevTextNodeEnd_ = 0;                                      // Previous text node end for ungetWord()
  String prevElementName_;                                          // Previous element name for ungetWord()
  bool prevIsEmptyElement_ = false;                                 // Previous empty element flag for ungetWord()
  size_t fileSize_;                                                 // Total file size for percentage calculation
  bool insideParagraph_ = false;                                    // Track if we're inside a <p> tag
  bool pendingNewline_ = false;  // For backward reading: newline to emit before next paragraph content
};

#endif
