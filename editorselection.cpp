#include "editorselection.h"

bool EditorSelection::hasSelection(const EditorCursor& cursor) { return cursor.anchor != cursor.current; }

QString EditorSelection::selectedText(const EditorCursor& cursor, PieceTable& table) {
	if (!hasSelection(cursor)) return {};
	TextPos start = cursor.anchor < cursor.current ? cursor.anchor : cursor.current;
	TextPos end = cursor.anchor < cursor.current ? cursor.current : cursor.anchor;

	QString result;
	QByteArray buf;
	buf.reserve(2048);

	const int MAX_COPY_BYTES = 50 * 1024 * 1024;
	int bytesCopied = 0;

	for (int i = start.line; i <= end.line; ++i) {
		table.getLineTextInto(i, buf);

		if (buf.isEmpty() && table.getOffsetFromLineCol(i, 0) >= table.totalSize() && i > 0) { break; }

		QString line = QString::fromUtf8(buf);
		int from = (i == start.line) ? start.col : 0;
		int to = (i == end.line) ? end.col : (int)line.length();
		result += line.mid(from, to - from);
		bytesCopied += (to - from);

		if (i < end.line) {
			result += '\n';
			bytesCopied++;
		}

		if (bytesCopied > MAX_COPY_BYTES) {
			result += "\n...[Selection cut to 50MB to protect using too much RAM]";
			break;
		}
	}
	return result;
}

bool EditorSelection::deleteSelection(EditorCursor& cursor, PieceTable& table) {
	if (!hasSelection(cursor)) return false;
	TextPos start = cursor.anchor < cursor.current ? cursor.anchor : cursor.current;
	TextPos end = cursor.anchor < cursor.current ? cursor.current : cursor.anchor;

	size_t startOff = table.getOffsetFromLineCol(start.line, start.col);
	size_t endOff = table.getOffsetFromLineCol(end.line, end.col);

	if (endOff > startOff) table.remove(startOff, endOff - startOff);

	cursor.current = start;
	cursor.anchor = start;
	return true;
}