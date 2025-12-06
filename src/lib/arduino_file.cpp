#include <SD.h>

extern "C" {

void* arduino_file_open(const char* path) {
  File* f = new File();
  *f = SD.open(path, FILE_READ);
  if (!*f) {
    delete f;
    return nullptr;
  }
  return f;
}

void arduino_file_close(void* handle) {
  if (!handle)
    return;
  File* f = static_cast<File*>(handle);
  f->close();
  delete f;
}

int arduino_file_seek(void* handle, long offset, int whence) {
  if (!handle)
    return -1;
  File* f = static_cast<File*>(handle);

  if (whence == SEEK_SET) {
    return f->seek(offset) ? 0 : -1;
  } else if (whence == SEEK_CUR) {
    return f->seek(f->position() + offset) ? 0 : -1;
  } else if (whence == SEEK_END) {
    return f->seek(f->size() + offset) ? 0 : -1;
  }
  return -1;
}

long arduino_file_tell(void* handle) {
  if (!handle)
    return -1;
  File* f = static_cast<File*>(handle);
  return f->position();
}

size_t arduino_file_read(void* ptr, size_t size, size_t count, void* handle) {
  if (!handle || !ptr)
    return 0;
  File* f = static_cast<File*>(handle);
  size_t bytes_to_read = size * count;
  size_t bytes_read = f->read(static_cast<uint8_t*>(ptr), bytes_to_read);
  return bytes_read / size;  // Return number of elements read
}

}  // extern "C"
