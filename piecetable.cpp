#include "piecetable.h"
#include <algorithm>
#include <climits>
#include <cstring>

void PieceTable::releaseOriginalPages() { m_originalMap.releaseAll(); }

void PieceTable::compactAddedBuffer() {
	size_t needed = 0;
	for (const Piece& p : m_pieces)
		if (p.bufferType == BufferType::Add) needed += p.length;

	if (needed == 0) {
		m_added.clear();
		return;
	}
	if (needed >= (size_t)m_added.size()) return;

	QByteArray compact;
	compact.reserve((int)needed);

	for (Piece& p : m_pieces) {
		if (p.bufferType != BufferType::Add) continue;
		size_t newStart = (size_t)compact.size();
		compact.append(m_added.constData() + p.start, (int)p.length);
		p.start = newStart;
	}

	m_added = std::move(compact);
}

const char* PieceTable::pieceData(const Piece& p) const {
	return (p.bufferType == BufferType::Original) ? (m_originalMap.data() + p.start) : (m_added.constData() + p.start);
}

size_t PieceTable::scanForwardLines(size_t startOffset, int lines, int& linesCrossed) const {
	linesCrossed = 0;
	if (lines <= 0) return startOffset;

	size_t acc = 0;
	bool started = false;

	for (int i = 0; i < m_pieces.size(); ++i) {
		size_t pEnd = acc + m_pieces[i].length;
		if (!started) {
			if (startOffset >= pEnd) {
				acc = pEnd;
				continue;
			}
			started = true;
		}
		const char* data = pieceData(m_pieces[i]);
		size_t from = (startOffset > acc) ? startOffset - acc : 0;
		size_t remain = m_pieces[i].length - from;
		const char* ptr = data + from;

		while (remain > 0) {
			const char* nl = static_cast<const char*>(std::memchr(ptr, '\n', remain));
			if (nl) {
				++linesCrossed;
				if (linesCrossed == lines) { return acc + (nl - data) + 1; }
				size_t advance = (nl - ptr) + 1;
				ptr += advance;
				remain -= advance;
			} else {
				break;
			}
		}
		acc = pEnd;
	}
	return m_totalBytes;
}

size_t PieceTable::offsetOfLineStart(int idx) const {
	if (idx <= 0) {
		m_lastQueriedLine = 0;
		m_lastQueriedOffset = 0;
		return 0;
	}

	if (idx == m_lastQueriedLine + 1 && m_lastQueriedLine >= 0) {
		int crossed = 0;
		size_t nextOff = scanForwardLines(m_lastQueriedOffset, 1, crossed);
		m_lastQueriedLine = idx;
		m_lastQueriedOffset = nextOff;
		return nextOff;
	}

	auto it = m_anchors.upper_bound(idx);
	--it;

	int anchorLine = it->first;
	size_t anchorOff = it->second;
	int linesToScan = idx - anchorLine;

	size_t resultOffset = 0;

	if (linesToScan <= 50000) {
		int crossed = 0;
		resultOffset = scanForwardLines(anchorOff, linesToScan, crossed);

		if (linesToScan >= LINE_BLOCK && m_anchors.find(idx) == m_anchors.end()) { m_anchors[idx] = resultOffset; }
	} else {
		double fraction = (double)idx / (double)std::max(1, lineCount());
		size_t guessOffset = (size_t)(fraction * m_totalBytes);

		if (guessOffset >= m_totalBytes) {
			resultOffset = m_totalBytes;
		} else {
			int crossed = 0;
			resultOffset = scanForwardLines(guessOffset, 1, crossed);
		}

		m_anchors[idx] = resultOffset;

		m_originalMap.releasePagesAround(resultOffset, 8ULL * 1024 * 1024);
	}

	m_lastQueriedLine = idx;
	m_lastQueriedOffset = resultOffset;
	return resultOffset;
}

int PieceTable::lineLength(int idx) const {
	if (idx < 0) return 0;
	size_t lineStart = offsetOfLineStart(idx);
	if (lineStart >= m_totalBytes) return 0;

	int chars = 0;
	size_t acc = 0;
	bool started = false;
	for (const Piece& p : m_pieces) {
		size_t pEnd = acc + p.length;
		if (!started) {
			if (lineStart >= pEnd) {
				acc = pEnd;
				continue;
			}
			started = true;
		}
		const char* data = pieceData(p);
		size_t from = (lineStart > acc) ? lineStart - acc : 0;
		for (size_t j = from; j < p.length;) {
			unsigned char uc = (unsigned char)data[j];
			if (uc == '\n') return chars;

			int charBytes;
			if (uc < 0x80)
				charBytes = 1;
			else if (uc < 0xE0)
				charBytes = 2;
			else if (uc < 0xF0)
				charBytes = 3;
			else
				charBytes = 4;

			chars++;
			j += charBytes;
		}
		acc = pEnd;
	}
	return chars;
}

int PieceTable::lineCount() const {
	if (m_fullySampled) return m_lineCount;

	if (m_totalBytes <= LAZY_THRESHOLD) {
		int crossed = 0;
		scanForwardLines(0, INT_MAX, crossed);
		m_lineCount = crossed + 1;
		m_fullySampled = true;
		return m_lineCount;
	}

	if (m_estimatedLines < 0) {
		size_t sample = std::min(m_totalBytes, ESTIMATE_SAMPLE);
		int newlines = 0;
		size_t counted = 0;
		for (const Piece& p : m_pieces) {
			if (counted >= sample) break;
			const char* data = pieceData(p);
			size_t take = std::min(p.length, sample - counted);
			for (size_t j = 0; j < take; ++j)
				if (data[j] == '\n') ++newlines;
			counted += take;
		}
		double density = (newlines > 0) ? (double)newlines / (double)sample : 1.0 / 80.0;
		m_estimatedLines = std::max(1, (int)(density * (double)m_totalBytes) + 1);
	}

	int maxAnchorLine = m_anchors.empty() ? 0 : m_anchors.rbegin()->first;
	return std::max({maxAnchorLine + 1, m_estimatedLines});
}

PieceTable::PieceTable() { invalidateIndex(); }

void PieceTable::loadOriginal(const QString& path) {
	m_originalMap.unmap();
	m_pieces.clear();
	m_added.clear();

	if (m_originalMap.mapFile(path) && m_originalMap.size() > 0)
		m_pieces.append(Piece(BufferType::Original, 0, m_originalMap.size()));

	m_totalBytes = m_originalMap.isMapped() ? m_originalMap.size() : 0;
	invalidateIndex();
}

void PieceTable::invalidateIndex() {
	m_anchors.clear();
	m_anchors[0] = 0;
	m_lineCount = 1;
	m_fullySampled = (m_totalBytes == 0);
	m_estimatedLines = -1;
	m_lastQueriedLine = -1;
	m_lastQueriedOffset = 0;
}

void PieceTable::invalidateIndexFrom(size_t offset) {
	auto it = m_anchors.begin();
	while (it != m_anchors.end()) {
		if (it->second > offset) {
			it = m_anchors.erase(it);
		} else {
			++it;
		}
	}
	m_lastQueriedLine = -1;
	m_fullySampled = false;
}

char PieceTable::getCharAt(size_t offset) const {
	size_t acc = 0;
	for (const Piece& p : m_pieces) {
		if (offset < acc + p.length) return pieceData(p)[offset - acc];
		acc += p.length;
	}
	return 0;
}

void PieceTable::getLineTextInto(int idx, QByteArray& out, int maxLength) const {
	out.clear();
	if (idx < 0) return;

	size_t lineStart = offsetOfLineStart(idx);
	if (lineStart >= m_totalBytes) return;

	size_t acc = 0;
	bool started = false;
	for (const Piece& p : m_pieces) {
		size_t pEnd = acc + p.length;
		if (!started) {
			if (lineStart >= pEnd) {
				acc = pEnd;
				continue;
			}
			started = true;
		}
		const char* data = pieceData(p);
		size_t from = (lineStart > acc) ? lineStart - acc : 0;
		for (size_t j = from; j < p.length; ++j) {
			char ch = data[j];
			if (ch == '\n') return;
			out.append(ch);
			if (maxLength > 0 && out.size() >= maxLength) return;
		}
		acc = pEnd;
	}
}

QString PieceTable::getLineText(int idx, int maxLength) const {
	if (idx < 0) return {};

	size_t lineStart = offsetOfLineStart(idx);
	if (lineStart >= m_totalBytes) return {};

	QByteArray buf;
	buf.reserve(256);

	size_t acc = 0;
	bool started = false;
	for (const Piece& p : m_pieces) {
		size_t pEnd = acc + p.length;
		if (!started) {
			if (lineStart >= pEnd) {
				acc = pEnd;
				continue;
			}
			started = true;
		}
		const char* data = pieceData(p);
		size_t from = (lineStart > acc) ? lineStart - acc : 0;
		for (size_t j = from; j < p.length; ++j) {
			char c = data[j];
			if (c == '\n') goto done;
			buf.append(c);
			if (maxLength > 0 && buf.size() >= maxLength) goto done;
		}
		acc = pEnd;
	}
done:
	return QString::fromUtf8(buf);
}

size_t PieceTable::getOffsetFromLineCol(int line, int col) const {
	if (line < 0) return 0;

	size_t lineStart = offsetOfLineStart(line);
	if (col <= 0) return lineStart;

	int chars = 0;
	size_t acc = 0;
	bool started = false;

	for (const Piece& p : m_pieces) {
		size_t pEnd = acc + p.length;
		if (!started) {
			if (lineStart >= pEnd) {
				acc = pEnd;
				continue;
			}
			started = true;
		}
		const char* data = pieceData(p);
		size_t from = (lineStart > acc) ? lineStart - acc : 0;
		for (size_t j = from; j < p.length;) {
			size_t bytePos = acc + j;

			if (chars == col) return bytePos;

			unsigned char uc = (unsigned char)data[j];
			if (uc == '\n') return bytePos;

			int charBytes;
			if (uc < 0x80)
				charBytes = 1;
			else if (uc < 0xE0)
				charBytes = 2;
			else if (uc < 0xF0)
				charBytes = 3;
			else
				charBytes = 4;

			chars++;
			j += charBytes;
		}
		acc = pEnd;
	}
	if (chars == col) return m_totalBytes;
	return m_totalBytes;
}

void PieceTable::insert(int line, int col, const QString& text) { insert(getOffsetFromLineCol(line, col), text, true); }

void PieceTable::remove(int line, int col, int len) {
	size_t s = getOffsetFromLineCol(line, col);
	size_t e = getOffsetFromLineCol(line, col + len);
	if (e > s) remove(s, e - s, true);
}

void PieceTable::insert(size_t offset, const QString& text, bool rebuild) {
	QByteArray utf8 = text.toUtf8();
	if (utf8.isEmpty()) return;

	size_t addStart = (size_t)m_added.size();
	m_added.append(utf8);
	Piece newPiece(BufferType::Add, addStart, (size_t)utf8.size());

	if (m_pieces.isEmpty()) {
		m_pieces.append(newPiece);
		m_totalBytes += utf8.size();
		if (rebuild) invalidateIndexFrom(offset);
		return;
	}

	if (offset > m_totalBytes) offset = m_totalBytes;

	bool inserted = false;
	size_t acc = 0;
	for (int i = 0; i < m_pieces.size(); ++i) {
		size_t pEnd = acc + m_pieces[i].length;
		if (offset >= acc && offset <= pEnd) {
			size_t splitPos = offset - acc;
			Piece old = m_pieces[i];
			m_pieces.removeAt(i);
			int idx = i;
			if (splitPos > 0) m_pieces.insert(idx++, Piece(old.bufferType, old.start, splitPos));
			m_pieces.insert(idx++, newPiece);
			if (splitPos < old.length)
				m_pieces.insert(idx, Piece(old.bufferType, old.start + splitPos, old.length - splitPos));
			inserted = true;
			break;
		}
		acc += m_pieces[i].length;
	}
	if (!inserted) m_pieces.append(newPiece);

	m_totalBytes += utf8.size();
	if (rebuild) invalidateIndexFrom(offset);
}

void PieceTable::writeTo(QIODevice& dev) const {
	for (const Piece& p : m_pieces) {
		if (p.length == 0) continue;
		dev.write(pieceData(p), (qint64)p.length);
	}
}

void PieceTable::remove(size_t offset, size_t length, bool rebuild) {
	if (length == 0 || offset >= m_totalBytes) return;
	if (offset + length > m_totalBytes) length = m_totalBytes - offset;

	size_t end = offset + length;
	QList<Piece> result;
	result.reserve(m_pieces.size() + 1);
	size_t acc = 0;

	for (const Piece& p : m_pieces) {
		size_t pStart = acc;
		size_t pEnd = acc + p.length;
		acc = pEnd;
		if (pEnd <= offset || pStart >= end) {
			result.append(p);
		} else {
			if (pStart < offset) result.append(Piece(p.bufferType, p.start, offset - pStart));
			if (pEnd > end) result.append(Piece(p.bufferType, p.start + (end - pStart), pEnd - end));
		}
	}

	m_pieces = std::move(result);
	m_totalBytes -= length;
	if (rebuild) invalidateIndexFrom(offset);
}