#ifndef EPUB_READER_H
#define EPUB_READER_H

#include <Arduino.h>
#include <SD.h>

extern "C" {
#include "epub_parser.h"
}

struct SpineItem {
  String idref;
  String href;
};

/**
 * EpubReader - Handles EPUB file operations including extraction and caching
 *
 * This class manages:
 * - Opening EPUB files
 * - Extracting files to cache directory (only once)
 * - Parsing container.xml and content.opf
 * - Providing ordered list of content files (spine)
 */
class EpubReader {
 public:
  EpubReader(const char* epubPath);
  ~EpubReader();

  bool isValid() const {
    return valid_;
  }
  String getExtractDir() const {
    return extractDir_;
  }
  String getContentOpfPath() const {
    return contentOpfPath_;
  }
  int getSpineCount() const {
    return spineCount_;
  }
  const SpineItem* getSpineItem(int index) const {
    if (index >= 0 && index < spineCount_) {
      return &spine_[index];
    }
    return nullptr;
  }

  /**
   * Get a file from the EPUB - either from cache or extract it first
   * Returns the full path to the extracted file on SD card
   * Returns empty string if file not found or extraction failed
   */
  String getFile(const char* filename);

 private:
  bool openEpub();
  void closeEpub();
  bool ensureExtractDirExists();
  String getExtractedPath(const char* filename);
  bool isFileExtracted(const char* filename);
  bool extractFile(const char* filename);
  bool parseContainer();
  bool parseContentOpf();

  String epubPath_;
  String extractDir_;
  String contentOpfPath_;
  bool valid_;

  epub_reader* reader_;

  SpineItem* spine_;
  int spineCount_ = 0;
};

#endif
