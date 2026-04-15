#ifndef MMAPHANDLER_H
#define MMAPHANDLER_H

#include <QString>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static constexpr size_t MMAP_LAZY_THRESHOLD = 10ULL * 1024 * 1024;

class MMapHandler {
  public:
	MMapHandler() : m_data(nullptr), m_size(0) {}
	~MMapHandler() { unmap(); }

	bool mapFile(const QString& path);
	void unmap();

	void releasePagesAround(size_t offset, size_t keepWindow = 4ULL * 1024 * 1024) const;

	void releaseAll() const;

	const char* data() const { return m_data; }
	size_t size() const { return m_size; }
	bool isMapped() const { return m_data != nullptr; }

  private:
	char* m_data;
	size_t m_size;
};

#endif