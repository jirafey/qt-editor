#include "incrementaction.h"
#include <QRegularExpression>

IncrementAction::NumberInfo IncrementAction::findFirstNumber(const QString& text, int startCol) {

	static QRegularExpression re(R"((?<![A-Za-z0-9_])-?\d+)");
	auto it = re.globalMatch(text, startCol);
	while (it.hasNext()) {
		auto m = it.next();
		NumberInfo info;
		info.col = m.capturedStart();
		info.length = m.capturedLength();
		bool ok;
		info.value = m.captured().toLongLong(&ok);
		if (ok) return info;
	}
	return {-1, 0, 0};
}

bool IncrementAction::run(PieceTable& table, EditorCursor& cursor, int delta) {
	int line = cursor.current.line;
	if (line < 0 || line >= table.lineCount()) return false;

	QString txt = table.getLineText(line, 10000);
	NumberInfo info = findFirstNumber(txt, 0);
	if (info.col < 0) return false;

	long long newVal = info.value + (long long)delta;
	QString newStr = QString::number(newVal);

	size_t numOff = table.getOffsetFromLineCol(line, info.col);
	size_t endOff = table.getOffsetFromLineCol(line, info.col + info.length);

	table.remove(numOff, endOff - numOff);
	table.insert(numOff, newStr);

	cursor.current.line = line;
	cursor.current.col = info.col + newStr.length();
	cursor.anchor = cursor.current;
	return true;
}