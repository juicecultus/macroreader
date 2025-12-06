#include "EpubReader.h"

#include "../xml/SimpleXmlParser.h"

// Helper function for case-insensitive string comparison
static bool strcasecmp_helper(const String& str1, const char* str2) {
  if (str1.length() != strlen(str2))
    return false;
  for (size_t i = 0; i < str1.length(); i++) {
    char c1 = str1.charAt(i);
    char c2 = str2[i];
    if (c1 >= 'A' && c1 <= 'Z')
      c1 += 32;
    if (c2 >= 'A' && c2 <= 'Z')
      c2 += 32;
    if (c1 != c2)
      return false;
  }
  return true;
}

// Helper function to find next element with given name
static bool findNextElement(SimpleXmlParser* parser, const char* elementName) {
  while (parser->read()) {
    if (parser->getNodeType() == SimpleXmlParser::Element && strcasecmp_helper(parser->getName(), elementName)) {
      return true;
    }
  }
  return false;
}

// File handle for extraction callback
static File g_extract_file;

// Callback to write extracted data to SD card file
static int extract_to_file_callback(const void* data, size_t size, void* user_data) {
  if (!g_extract_file) {
    return 0;  // File not open
  }

  size_t written = g_extract_file.write((const uint8_t*)data, size);
  return (written == size) ? 1 : 0;  // Return 1 for success, 0 for failure
}

EpubReader::EpubReader(const char* epubPath)
    : epubPath_(epubPath), valid_(false), reader_(nullptr), spine_(nullptr), spineCount_(0) {
  Serial.printf("\n=== EpubReader: Opening %s ===\n", epubPath);

  // Verify file exists
  File testFile = SD.open(epubPath);
  if (!testFile) {
    Serial.println("ERROR: Cannot open EPUB file");
    return;
  }
  size_t fileSize = testFile.size();
  testFile.close();
  Serial.printf("EPUB file verified, size: %u bytes\n", fileSize);

  // Create extraction directory based on EPUB filename
  String epubFilename = String(epubPath);
  int lastSlash = epubFilename.lastIndexOf('/');
  if (lastSlash < 0) {
    lastSlash = epubFilename.lastIndexOf('\\');
  }
  if (lastSlash >= 0) {
    epubFilename = epubFilename.substring(lastSlash + 1);
  }
  int lastDot = epubFilename.lastIndexOf('.');
  if (lastDot >= 0) {
    epubFilename = epubFilename.substring(0, lastDot);
  }

#ifdef TEST_BUILD
  extractDir_ = "test/output/epub_" + epubFilename;
#else
  extractDir_ = "/epub_" + epubFilename;
#endif
  Serial.printf("Extract directory: %s\n", extractDir_.c_str());

  if (!ensureExtractDirExists()) {
    return;
  }

  // Parse container.xml to get content.opf path
  if (!parseContainer()) {
    Serial.println("ERROR: Failed to parse container.xml");
    return;
  }

  // Parse content.opf to get spine items
  if (!parseContentOpf()) {
    Serial.println("ERROR: Failed to parse content.opf");
    return;
  }

  valid_ = true;
  Serial.println("EpubReader initialized successfully\n");
}

EpubReader::~EpubReader() {
  closeEpub();
  if (spine_) {
    delete[] spine_;
    spine_ = nullptr;
  }
  Serial.println("EpubReader destroyed");
}

bool EpubReader::openEpub() {
  if (reader_) {
    return true;  // Already open
  }

  epub_error err = epub_open(epubPath_.c_str(), &reader_);
  if (err != EPUB_OK) {
    Serial.printf("ERROR: Failed to open EPUB: %s\n", epub_get_error_string(err));
    reader_ = nullptr;
    return false;
  }

  Serial.println("EPUB opened for reading");
  return true;
}

void EpubReader::closeEpub() {
  if (reader_) {
    epub_close(reader_);
    reader_ = nullptr;
    Serial.println("EPUB closed");
  }
}

bool EpubReader::ensureExtractDirExists() {
  if (!SD.exists(extractDir_.c_str())) {
    if (!SD.mkdir(extractDir_.c_str())) {
      Serial.printf("ERROR: Failed to create directory %s\n", extractDir_.c_str());
      return false;
    }
    Serial.printf("Created directory: %s\n", extractDir_.c_str());
  }
  return true;
}

String EpubReader::getExtractedPath(const char* filename) {
  String path = extractDir_ + "/" + String(filename);
  return path;
}

bool EpubReader::isFileExtracted(const char* filename) {
  String path = getExtractedPath(filename);
  bool exists = SD.exists(path.c_str());
  if (exists) {
    Serial.printf("File already extracted: %s\n", filename);
  }
  return exists;
}

bool EpubReader::extractFile(const char* filename) {
  Serial.printf("\n=== Extracting %s ===\n", filename);

  // Open EPUB if not already open
  if (!openEpub()) {
    return false;
  }

  // Find the file in the EPUB
  uint32_t fileIndex;
  epub_error err = epub_locate_file(reader_, filename, &fileIndex);
  if (err != EPUB_OK) {
    Serial.printf("ERROR: File not found in EPUB: %s\n", filename);
    return false;
  }

  // Get file info
  epub_file_info info;
  err = epub_get_file_info(reader_, fileIndex, &info);
  if (err != EPUB_OK) {
    Serial.printf("ERROR: Failed to get file info: %s\n", epub_get_error_string(err));
    return false;
  }

  Serial.printf("Found file at index %d (size: %u bytes)\n", fileIndex, info.uncompressed_size);

  // Create subdirectories if needed
  String extractPath = getExtractedPath(filename);
  int lastSlash = extractPath.lastIndexOf('/');
  if (lastSlash > 0) {
    String dirPath = extractPath.substring(0, lastSlash);

    // Create all parent directories
    int pos = 0;
    while (pos < dirPath.length()) {
      int nextSlash = dirPath.indexOf('/', pos + 1);
      if (nextSlash == -1) {
        nextSlash = dirPath.length();
      }

      String subDir = dirPath.substring(0, nextSlash);
      if (!SD.exists(subDir.c_str())) {
        if (!SD.mkdir(subDir.c_str())) {
          Serial.printf("ERROR: Failed to create directory %s\n", subDir.c_str());
          return false;
        }
      }

      pos = nextSlash;
    }
  }

  // Extract to file
  Serial.printf("Extracting to: %s\n", extractPath.c_str());

  g_extract_file = SD.open(extractPath.c_str(), FILE_WRITE);
  if (!g_extract_file) {
    Serial.printf("ERROR: Failed to open file for writing: %s\n", extractPath.c_str());
    return false;
  }

  err = epub_extract_streaming(reader_, fileIndex, extract_to_file_callback, nullptr, 4096);
  g_extract_file.close();

  if (err != EPUB_OK) {
    Serial.printf("ERROR: Extraction failed: %s\n", epub_get_error_string(err));
    return false;
  }

  Serial.printf("Successfully extracted %s\n", filename);
  return true;
}

String EpubReader::getFile(const char* filename) {
  if (!valid_) {
    Serial.println("ERROR: EpubReader not valid");
    return String("");
  }

  // Check if file is already extracted
  if (isFileExtracted(filename)) {
    return getExtractedPath(filename);
  }

  // Need to extract it
  if (!extractFile(filename)) {
    return String("");
  }

  return getExtractedPath(filename);
}

bool EpubReader::parseContainer() {
  // Get container.xml (will extract if needed)
  // Check if file is already extracted
  const char* filename = "META-INF/container.xml";
  String containerPath;

  if (isFileExtracted(filename)) {
    containerPath = getExtractedPath(filename);
  } else {
    // Need to extract it
    if (!extractFile(filename)) {
      Serial.println("ERROR: Failed to extract container.xml");
      return false;
    }
    containerPath = getExtractedPath(filename);
  }

  if (containerPath.isEmpty()) {
    Serial.println("ERROR: Failed to get container.xml path");
    return false;
  }

  Serial.printf("Parsing container: %s\n", containerPath.c_str());

  // Parse container.xml to get content.opf path
  // Allocate parser on heap to avoid stack overflow (parser has 8KB buffer)
  SimpleXmlParser* parser = new SimpleXmlParser();
  if (!parser->open(containerPath.c_str())) {
    Serial.println("ERROR: Failed to open container.xml for parsing");
    delete parser;
    return false;
  }

  // Find <rootfile> element and get full-path attribute
  if (findNextElement(parser, "rootfile")) {
    contentOpfPath_ = parser->getAttribute("full-path");
  }

  parser->close();
  delete parser;

  if (contentOpfPath_.isEmpty()) {
    Serial.println("ERROR: Could not find content.opf path in container.xml");
    return false;
  }

  Serial.printf("Found content.opf: %s\n", contentOpfPath_.c_str());
  return true;
}

bool EpubReader::parseContentOpf() {
  // Get content.opf file
  const char* filename = contentOpfPath_.c_str();
  String opfPath;

  if (isFileExtracted(filename)) {
    opfPath = getExtractedPath(filename);
  } else {
    // Need to extract it
    if (!extractFile(filename)) {
      Serial.println("ERROR: Failed to extract content.opf");
      return false;
    }
    opfPath = getExtractedPath(filename);
  }

  if (opfPath.isEmpty()) {
    Serial.println("ERROR: Failed to get content.opf path");
    return false;
  }

  Serial.printf("Parsing content.opf: %s\n", opfPath.c_str());

  // Parse content.opf to get spine items
  // Allocate parser on heap to avoid stack overflow (parser has 8KB buffer)
  SimpleXmlParser* parser = new SimpleXmlParser();
  if (!parser->open(opfPath.c_str())) {
    Serial.println("ERROR: Failed to open content.opf for parsing");
    delete parser;
    return false;
  }

  Serial.println("\n=== Parsing Manifest ===");

  // Step 1: Build a map of manifest items (id -> href)
  // We'll use a simple array approach since we have limited items
  struct ManifestItem {
    String id;
    String href;
  };

  const int MAX_MANIFEST_ITEMS = 100;
  ManifestItem* manifest = new ManifestItem[MAX_MANIFEST_ITEMS];
  int manifestCount = 0;

  // Find all <item> elements in <manifest>
  while (findNextElement(parser, "item") && manifestCount < MAX_MANIFEST_ITEMS) {
    String id = parser->getAttribute("id");
    String href = parser->getAttribute("href");

    if (!id.isEmpty() && !href.isEmpty()) {
      manifest[manifestCount].id = id;
      manifest[manifestCount].href = href;
      manifestCount++;
    }
  }

  Serial.printf("Found %d manifest items\\n", manifestCount);

  // Step 2: Parse spine to get ordered list of idrefs
  Serial.println("\\n=== Parsing Spine ===");

  // Close and reopen to start from beginning
  parser->close();
  if (!parser->open(opfPath.c_str())) {
    Serial.println("ERROR: Failed to reopen content.opf for spine parsing");
    delete parser;
    delete[] manifest;
    return false;
  }

  // Use a temporary list to collect spine items
  const int INITIAL_CAPACITY = 20;
  int capacity = INITIAL_CAPACITY;
  SpineItem* tempSpine = new SpineItem[capacity];
  spineCount_ = 0;

  while (findNextElement(parser, "itemref")) {
    String idref = parser->getAttribute("idref");

    if (!idref.isEmpty()) {
      // Look up href in manifest
      String href = "";
      for (int i = 0; i < manifestCount; i++) {
        if (manifest[i].id == idref) {
          href = manifest[i].href;
          break;
        }
      }

      if (!href.isEmpty()) {
        // Grow array if needed
        if (spineCount_ >= capacity) {
          capacity *= 2;
          SpineItem* newSpine = new SpineItem[capacity];
          for (int i = 0; i < spineCount_; i++) {
            newSpine[i].idref = tempSpine[i].idref;
            newSpine[i].href = tempSpine[i].href;
          }
          delete[] tempSpine;
          tempSpine = newSpine;
        }

        tempSpine[spineCount_].idref = idref;
        tempSpine[spineCount_].href = href;
        Serial.printf("  [%d] %s -> %s\n", spineCount_, idref.c_str(), href.c_str());
        spineCount_++;
      } else {
        Serial.printf("WARNING: No manifest entry for idref: %s\n", idref.c_str());
      }
    }
  }

  // Allocate final spine array with exact size and copy data
  spine_ = new SpineItem[spineCount_];
  for (int i = 0; i < spineCount_; i++) {
    spine_[i].idref = tempSpine[i].idref;
    spine_[i].href = tempSpine[i].href;
  }
  delete[] tempSpine;

  delete[] manifest;
  parser->close();
  delete parser;

  Serial.printf("\nSpine parsed successfully: %d items\n", spineCount_);
  return true;
}
