#include "mmaphandler.h"
#include <sys/mman.h>

bool MMapHandler::mapFile(const QString& path) {
	unmap();

	int fd = open(path.toLocal8Bit().constData(), O_RDONLY);
	if (fd == -1) return false;

	off_t sz = lseek(fd, 0, SEEK_END);
	if (sz <= 0) {
		close(fd);
		m_size = 0;
		return sz == 0;
	}
	m_size = (size_t)sz;

	int flags = MAP_PRIVATE;
	if (m_size < MMAP_LAZY_THRESHOLD) flags |= MAP_POPULATE;

	m_data = static_cast<char*>(mmap(nullptr, m_size, PROT_READ, flags, fd, 0));
	close(fd);

	if (m_data == MAP_FAILED) {
		m_data = nullptr;
		m_size = 0;
		return false;
	}

	if (m_size < MMAP_LAZY_THRESHOLD) {
		madvise(m_data, m_size, MADV_SEQUENTIAL);
	} else {
		madvise(m_data, m_size, MADV_RANDOM);
	}

	return true;
}

void MMapHandler::unmap() {
	if (m_data && m_data != MAP_FAILED) {

		madvise(m_data, m_size, MADV_DONTNEED);
		munmap(m_data, m_size);
	}
	m_data = nullptr;
	m_size = 0;
}

void MMapHandler::releasePagesAround(size_t offset, size_t keepWindow) const {
	if (!m_data || offset <= keepWindow) return;
	static const size_t PAGE = 4096;
	size_t releaseEnd = (offset - keepWindow) & ~(PAGE - 1);
	if (releaseEnd == 0) return;
	madvise(m_data, releaseEnd, MADV_DONTNEED);
}

void MMapHandler::releaseAll() const {
	if (!m_data || m_size == 0) return;
	madvise(m_data, m_size, MADV_DONTNEED);
}