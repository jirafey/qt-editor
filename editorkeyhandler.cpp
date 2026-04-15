#include "editorkeyhandler.h"
#include "editorcursor.h"
#include "editorselection.h"
#include "settingsmanager.h"

bool EditorKeyHandler::handle(QKeyEvent* e, PieceTable& table, EditorCursor& cursor, bool& undoPending,
							  QChar& lastCharType, EditorKeyHandler::Delegate& d) {
	int lineCount = table.lineCount();
	bool shift = e->modifiers() & Qt::ShiftModifier;

	if (e->modifiers() & Qt::ControlModifier) {
		switch (e->key()) {
		case Qt::Key_Z:
			d.undo();
			return true;
		case Qt::Key_Y:
			d.redo();
			return true;

		case Qt::Key_C: {
			QString sel = EditorSelection::selectedText(cursor, table);
			if (sel.isEmpty()) { sel = table.getLineText(cursor.current.line, 50000) + "\n"; }
			d.setClipboardText(sel);
			return true;
		}
		case Qt::Key_D: {
			QString line = table.getLineText(cursor.current.line, 10000);
			d.pushUndo();
			table.insert(cursor.current.line, (int)line.length(), "\n" + line);
			cursor.current.line++;
			cursor.current.col = qMin(cursor.current.col, (int)line.length());
			cursor.anchor = cursor.current;
			d.onContentChanged();
			return true;
		}
		case Qt::Key_X: {
			QString sel = EditorSelection::selectedText(cursor, table);
			if (!sel.isEmpty()) {
				d.setClipboardText(sel);
				d.pushUndo();
				EditorSelection::deleteSelection(cursor, table);
				d.onContentChanged();
			} else {
				sel = table.getLineText(cursor.current.line, 50000) + "\n";
				d.setClipboardText(sel);
				d.pushUndo();

				size_t offset = table.getOffsetFromLineCol(cursor.current.line, 0);
				size_t endOff = table.getOffsetFromLineCol(cursor.current.line + 1, 0);
				table.remove(offset, endOff - offset);

				cursor.current.col = 0;
				cursor.anchor = cursor.current;
				d.onContentChanged();
			}
			return true;
		}
		case Qt::Key_V: {
			QString text = d.clipboardText();
			if (text.isEmpty()) return true;
			if (!undoPending) {
				d.pushUndo();
				undoPending = true;
			}
			EditorSelection::deleteSelection(cursor, table);
			table.insert(cursor.current.line, cursor.current.col, text);
			QStringList lines = text.split('\n');
			if (lines.size() > 1) {
				cursor.current.line += lines.size() - 1;
				cursor.current.col = lines.last().length();
			} else {
				cursor.current.col += text.length();
			}
			cursor.anchor = cursor.current;
			d.onContentChanged();
			return true;
		}
		case Qt::Key_A:
			cursor.anchor = {0, 0};
			cursor.current = {lineCount - 1, table.lineLength(lineCount - 1)};
			cursor.selecting = true;
			d.onCursorMoved();
			return true;
		case Qt::Key_Home:
			cursor.current = {0, 0};
			if (!shift) cursor.anchor = cursor.current;
			d.onCursorMoved();
			return true;
		case Qt::Key_End:
			cursor.current = {lineCount - 1, table.lineLength(lineCount - 1)};
			if (!shift) cursor.anchor = cursor.current;
			d.onCursorMoved();
			return true;
		default:
			break;
		}
	}

	switch (e->key()) {
	case Qt::Key_Up:
		if (cursor.current.line > 0) {
			cursor.current.line--;
			cursor.current.col = qMin(cursor.current.col, table.lineLength(cursor.current.line));
		}
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;

	case Qt::Key_Down:
		if (cursor.current.line < lineCount - 1) {
			cursor.current.line++;
			cursor.current.col = qMin(cursor.current.col, table.lineLength(cursor.current.line));
		}
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;

	case Qt::Key_Left: {
		if (e->modifiers() & Qt::ControlModifier) {
			QString txt = table.getLineText(cursor.current.line, 10000);
			int col = cursor.current.col;
			if (col == 0) {
				if (cursor.current.line > 0) {
					cursor.current.line--;
					cursor.current.col = table.lineLength(cursor.current.line);
				}
			} else {
				col--;
				auto isWord = [](QChar c) { return c.isLetterOrNumber() || c == '_'; };
				bool wasWord = isWord(txt.at(col));
				while (col > 0 && isWord(txt.at(col - 1)) == wasWord) { col--; }
				cursor.current.col = col;
			}
		} else {
			if (cursor.current.col > 0) {
				cursor.current.col--;
			} else if (cursor.current.line > 0) {
				cursor.current.line--;
				cursor.current.col = table.lineLength(cursor.current.line);
			}
		}
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;
	}

	case Qt::Key_Right: {
		int len = table.lineLength(cursor.current.line);
		if (e->modifiers() & Qt::ControlModifier) {
			QString txt = table.getLineText(cursor.current.line, 10000);
			int col = cursor.current.col;
			if (col == len) {
				if (cursor.current.line < lineCount - 1) {
					cursor.current.line++;
					cursor.current.col = 0;
				}
			} else {
				auto isWord = [](QChar c) { return c.isLetterOrNumber() || c == '_'; };
				bool wasWord = isWord(txt.at(col));
				while (col < len && isWord(txt.at(col)) == wasWord) { col++; }
				cursor.current.col = col;
			}
		} else {
			if (cursor.current.col < len) {
				cursor.current.col++;
			} else if (cursor.current.line < lineCount - 1) {
				cursor.current.line++;
				cursor.current.col = 0;
			}
		}
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;
	}
	case Qt::Key_Home:
		cursor.current.col = 0;
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;

	case Qt::Key_End:
		cursor.current.col = table.lineLength(cursor.current.line);
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;

	case Qt::Key_PageUp: {
		cursor.current.line = qMax(0, cursor.current.line - 20);
		cursor.current.col = qMin(cursor.current.col, table.lineLength(cursor.current.line));
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;
	}
	case Qt::Key_PageDown: {
		cursor.current.line = qMin(lineCount - 1, cursor.current.line + 20);
		cursor.current.col = qMin(cursor.current.col, table.lineLength(cursor.current.line));
		if (!shift) cursor.anchor = cursor.current;
		d.onCursorMoved();
		return true;
	}

	case Qt::Key_Backtab:
	case Qt::Key_Tab: {
		d.pushUndo();
		int tabWidth = SettingsManager::instance().options().tabWidth;
		bool useSpaces = SettingsManager::instance().options().useSpaces;
		QString tabStr = useSpaces ? QString(tabWidth, ' ') : "\t";
		bool isShift = (e->key() == Qt::Key_Backtab) || (e->modifiers() & Qt::ShiftModifier);

		int startLine = qMin(cursor.current.line, cursor.anchor.line);
		int endLine = qMax(cursor.current.line, cursor.anchor.line);

		if (EditorSelection::hasSelection(cursor) || isShift) {
			for (int i = startLine; i <= endLine; ++i) {
				QString line = table.getLineText(i);
				size_t offset = table.getOffsetFromLineCol(i, 0);

				if (isShift) {
					int removeCount = 0;
					if (line.startsWith('\t')) {
						removeCount = 1;
					} else {
						while (removeCount < tabWidth && removeCount < line.length() && line[removeCount] == ' ') {
							removeCount++;
						}
					}
					if (removeCount > 0) {
						table.remove(offset, removeCount);
						if (cursor.current.line == i) cursor.current.col = qMax(0, cursor.current.col - removeCount);
						if (cursor.anchor.line == i) cursor.anchor.col = qMax(0, cursor.anchor.col - removeCount);
					}
				} else {
					table.insert(offset, tabStr);
					if (cursor.current.line == i) cursor.current.col += tabStr.length();
					if (cursor.anchor.line == i) cursor.anchor.col += tabStr.length();
				}
			}
		} else {
			table.insert(cursor.current.line, cursor.current.col, tabStr);
			cursor.current.col += tabStr.length();
			cursor.anchor = cursor.current;
		}
		d.onContentChanged();
		return true;
	}

	case Qt::Key_Backspace:
		d.pushUndo();

		if (EditorSelection::hasSelection(cursor)) {
			EditorSelection::deleteSelection(cursor, table);
		} else if (cursor.current.col > 0) {
			int tabWidth = SettingsManager::instance().options().tabWidth;
			bool useSpaces = SettingsManager::instance().options().useSpaces;
			QString line = table.getLineText(cursor.current.line, cursor.current.col + 10);

			int targetCol = ((cursor.current.col - 1) / tabWidth) * tabWidth;
			int spacesToDelete = cursor.current.col - targetCol;

			bool allSpaces = true;
			for (int i = 0; i < spacesToDelete; ++i) {
				int checkCol = cursor.current.col - 1 - i;
				if (checkCol < 0 || checkCol >= line.length() || line[checkCol] != ' ') {
					allSpaces = false;
					break;
				}
			}

			if (useSpaces && allSpaces && spacesToDelete > 0) {
				size_t off = table.getOffsetFromLineCol(cursor.current.line, cursor.current.col - spacesToDelete);
				size_t offEnd = table.getOffsetFromLineCol(cursor.current.line, cursor.current.col);
				if (offEnd > off) table.remove(off, offEnd - off);
				cursor.current.col -= spacesToDelete;
			} else {
				size_t off = table.getOffsetFromLineCol(cursor.current.line, cursor.current.col - 1);
				size_t offEnd = table.getOffsetFromLineCol(cursor.current.line, cursor.current.col);
				if (offEnd > off) table.remove(off, offEnd - off);
				cursor.current.col--;
			}
			cursor.anchor = cursor.current;
		} else if (cursor.current.line > 0) {
			int prev = cursor.current.line - 1;
			int prevLen = table.lineLength(prev);
			size_t off1 = table.getOffsetFromLineCol(prev, prevLen);
			size_t off2 = table.getOffsetFromLineCol(cursor.current.line, 0);

			table.remove(off1, off2 - off1);
			cursor.current.line = prev;
			cursor.current.col = prevLen;
			cursor.anchor = cursor.current;
		}

		d.onContentChanged();
		return true;
	case Qt::Key_Delete:
		d.pushUndo();
		if (EditorSelection::hasSelection(cursor)) {
			EditorSelection::deleteSelection(cursor, table);
		} else {
			int len = table.lineLength(cursor.current.line);
			if (cursor.current.col < len) {
				table.remove(cursor.current.line, cursor.current.col, 1);
			} else if (cursor.current.line < lineCount - 1) {
				size_t off1 = table.getOffsetFromLineCol(cursor.current.line, len);
				size_t off2 = table.getOffsetFromLineCol(cursor.current.line + 1, 0);
				table.remove(off1, off2 - off1);
			}
			cursor.anchor = cursor.current;
		}
		d.onContentChanged();
		return true;

	case Qt::Key_Return:
	case Qt::Key_Enter:
		d.pushUndo();
		EditorSelection::deleteSelection(cursor, table);
		table.insert(cursor.current.line, cursor.current.col, "\n");
		cursor.current.line++;
		cursor.current.col = 0;
		cursor.anchor = cursor.current;
		d.onContentChanged();
		return true;

	default:
		break;
	}

	if (!e->text().isEmpty() && e->text().at(0).isPrint()) {
		QChar ch = e->text().at(0);
		if ((ch == ')' || ch == '}' || ch == ']' || ch == '"' || ch == '\'') &&
			cursor.current.col < table.lineLength(cursor.current.line)) {
			if (table.getLineText(cursor.current.line, cursor.current.col + 2).at(cursor.current.col) == ch) {
				cursor.current.col++;
				cursor.anchor = cursor.current;
				d.onCursorMoved();
				return true;
			}
		}

		bool isWord = ch.isLetterOrNumber() || ch == '_';
		bool wasWord = lastCharType.isLetterOrNumber() || lastCharType == '_';
		if (!undoPending || isWord != wasWord) {
			d.pushUndo();
			undoPending = true;
		}
		lastCharType = ch;
		EditorSelection::deleteSelection(cursor, table);

		QString insertStr = e->text();
		int cursorBackOffset = 0;

		if (ch == '{') {
			insertStr = "{}";
			cursorBackOffset = 1;
		} else if (ch == '(') {
			insertStr = "()";
			cursorBackOffset = 1;
		} else if (ch == '[') {
			insertStr = "[]";
			cursorBackOffset = 1;
		} else if (ch == '"') {
			insertStr = "\"\"";
			cursorBackOffset = 1;
		} else if (ch == '\'') {
			insertStr = "''";
			cursorBackOffset = 1;
		}

		table.insert(cursor.current.line, cursor.current.col, insertStr);
		cursor.current.col += insertStr.length() - cursorBackOffset;
		cursor.anchor = cursor.current;
		d.onContentChanged();
		return true;
	}

	return false;
}