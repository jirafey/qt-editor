#pragma once

#include "mmaphandler.h"
#include "piece.h"
#include <QByteArray>
#include <QIODevice>
#include <QList>
#include <QString>
#include <map>

static constexpr int LINE_BLOCK = 4096;
static constexpr size_t LAZY_THRESHOLD = 10ULL * 1024 * 1024;
static constexpr size_t ESTIMATE_SAMPLE = 65536;

class PieceTable {
  public:
	PieceTable();

	void loadOriginal(const QString& path);
	void releaseOriginalPages();
	void compactAddedBuffer();

	void insert(int line, int col, const QString& text);
	void remove(int line, int col, int len);

	void insert(size_t offset, const QString& text, bool rebuild = true);
	void remove(size_t offset, size_t length, bool rebuild = true);

	void writeTo(QIODevice& dev) const;

	QString getLineText(int idx, int maxLength = -1) const;
	void getLineTextInto(int idx, QByteArray& out, int maxLength = -1) const;

	int lineLength(int idx) const;

	int lineCount() const;
	size_t totalSize() const { return m_totalBytes; }
	char getCharAt(size_t offset) const;
	size_t getOffsetFromLineCol(int line, int col) const;

	const QList<Piece>& pieces() const { return m_pieces; }
	void setPieces(const QList<Piece>& p) {
		m_pieces = p;
		m_totalBytes = 0;
		for (const auto& piece : p) m_totalBytes += piece.length;
		invalidateIndex();
	}

	QByteArray addedBuffer() const { return m_added; }
	void setAddedBuffer(const QByteArray& b) { m_added = b; }

	int addedBufferSize() const { return m_added.size(); }

  private:
	void invalidateIndex();
	void invalidateIndexFrom(size_t offset);
	size_t offsetOfLineStart(int idx) const;
	size_t scanForwardLines(size_t startOffset, int lines, int& crossed) const;

	const char* pieceData(const Piece& p) const;

	MMapHandler m_originalMap;
	QByteArray m_added;
	QList<Piece> m_pieces;
	size_t m_totalBytes = 0;

	mutable std::map<int, size_t> m_anchors;
	mutable int m_lineCount = 1;
	mutable bool m_fullySampled = false;
	mutable int m_estimatedLines = -1;

	mutable int m_lastQueriedLine = -1;
	mutable size_t m_lastQueriedOffset = 0;
};