#include "editorview.h"
#include "editorkeyhandler.h"
#include "incrementaction.h"
#include "settingsmanager.h"

#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <QFile>
#include <QFontMetrics>
#include <QJsonArray>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextLayout>
#include <QWheelEvent>
#include <climits>
#include <utility>

static constexpr int MAX_LINE_LEN = 10000;

EditorView::EditorView(QWidget* parent) : QAbstractScrollArea(parent) {
	setFocusPolicy(Qt::StrongFocus);
	viewport()->setCursor(Qt::IBeamCursor);
	viewport()->setMouseTracking(true);

	QFont f("Monospace", 11);
	f.setStyleHint(QFont::TypeWriter);
	setFont(f);
	viewport()->setFont(f);

	verticalScrollBar()->setSingleStep(1);
	horizontalScrollBar()->setSingleStep(20);
	updateScrollBars();

	m_completionPopup = new QListWidget(this);
	m_completionPopup->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
	m_completionPopup->hide();
	m_completionPopup->setStyleSheet("QListWidget { background: #252526; color: #d4d4d4; border: 1px solid #454545; }");
	connect(m_completionPopup, &QListWidget::itemActivated, this, &EditorView::insertCompletion);
}

void EditorView::insertCompletion(QListWidgetItem* item) {
	m_completionPopup->hide();
	if (!item) return;

	const QString insertText = item->data(Qt::UserRole).toString();
	const QString txt = m_pieceTable.getLineText(m_cursor.current.line, m_cursor.current.col + 100);

	int start = m_cursor.current.col;
	while (start > 0 && (txt.at(start - 1).isLetterOrNumber() || txt.at(start - 1) == '_')) --start;
	const int len = m_cursor.current.col - start;

	pushUndo();
	if (len > 0) m_pieceTable.remove(m_cursor.current.line, start, len);

	const int backOffset = insertText.endsWith("()") ? 1 : 0;
	m_pieceTable.insert(m_cursor.current.line, start, insertText);

	m_cursor.current.col = start + (int)insertText.length() - backOffset;
	m_cursor.anchor = m_cursor.current;
	onContentChanged();
}

void EditorView::afterSave() {
	m_undoStack.clear();
	m_redoStack.clear();
	m_pieceTable.compactAddedBuffer();
	m_undoPending = false;
	m_lastCharType = QChar();
}

int EditorView::gutterWidth() const {
	QFontMetrics fm(font());
	int digits = QString::number(qMax(1, m_pieceTable.lineCount())).length();
	return fm.horizontalAdvance(QString(digits + 1, '9')) + 20;
}

int EditorView::lineHeight() const { return QFontMetrics(font()).lineSpacing(); }

void EditorView::updateScrollBars() {
	QFontMetrics fm(font());
	const int lh = lineHeight();
	const int lines = m_pieceTable.lineCount();
	const int totalH = lines * lh;
	const int viewH = viewport()->height();

	verticalScrollBar()->setRange(0, qMax(0, totalH - viewH));
	verticalScrollBar()->setPageStep(viewH);

	const int scrollY = verticalScrollBar()->value();
	const int firstLine = scrollY / lh;
	const int lastLine = qMin(firstLine + (viewH / lh) + 100, lines - 1);

	int maxW = 0;
	for (int i = firstLine; i <= lastLine; ++i) {
		int w = fm.horizontalAdvance(m_pieceTable.getLineText(i, MAX_LINE_LEN));
		if (w > maxW) maxW = w;
	}
	const int viewW = viewport()->width() - gutterWidth() - 10;
	horizontalScrollBar()->setRange(0, qMax(0, maxW - viewW + 40));
	horizontalScrollBar()->setPageStep(viewW);
}

void EditorView::ensureCursorVisible() {
	const int lh = lineHeight();
	const int gw = gutterWidth();
	const int scrollY = verticalScrollBar()->value();
	const int scrollX = horizontalScrollBar()->value();
	const int vh = viewport()->height();
	const int vw = viewport()->width() - gw - 10;

	const int cursorY = m_cursor.current.line * lh;
	if (cursorY < scrollY)
		verticalScrollBar()->setValue(cursorY);
	else if (cursorY + lh > scrollY + vh)
		verticalScrollBar()->setValue(cursorY + lh - vh);

	QFontMetrics fm(font());
	const QString txt = m_pieceTable.getLineText(m_cursor.current.line, MAX_LINE_LEN);
	const int col = qMin(m_cursor.current.col, (int)txt.length());
	const int cursorX = fm.horizontalAdvance(txt.left(col));

	if (cursorX < scrollX)
		horizontalScrollBar()->setValue(cursorX);
	else if (cursorX > scrollX + vw)
		horizontalScrollBar()->setValue(cursorX - vw + 20);
}

void EditorView::reloadColors() {
	m_highlighter.reloadColors();
	viewport()->update();
}

void EditorView::paintEvent(QPaintEvent*) {
	QPainter p(viewport());
	const auto& c = SettingsManager::instance().colors();

	const int lh = lineHeight();
	const int gw = gutterWidth();
	const int scrollY = verticalScrollBar()->value();
	const int scrollX = horizontalScrollBar()->value();
	const int vw = viewport()->width();
	const int vh = viewport()->height();

	p.fillRect(viewport()->rect(), c.background);

	QFontMetrics fm(font());

	const int totalLines = m_pieceTable.lineCount();
	const int firstLine = scrollY / lh;
	const int lastLine = qMin(firstLine + (vh / lh) + 2, totalLines - 1);

	p.fillRect(0, 0, gw, vh, c.gutter);
	p.setPen(QColor(60, 60, 60));
	p.drawLine(gw, 0, gw, vh);

	const bool hasSel = hasSelection();
	TextPos selStart, selEnd;
	if (hasSel) {
		selStart = m_cursor.anchor < m_cursor.current ? m_cursor.anchor : m_cursor.current;
		selEnd = m_cursor.anchor < m_cursor.current ? m_cursor.current : m_cursor.anchor;
	}

	QByteArray rawLine;
	rawLine.reserve(512);

	for (int i = firstLine; i <= lastLine; ++i) {
		m_pieceTable.getLineTextInto(i, rawLine, MAX_LINE_LEN);
		const QString txt = QString::fromUtf8(rawLine);

		const int y = i * lh - scrollY;

		if (i == m_cursor.current.line) p.fillRect(gw + 1, y, vw - gw - 1, lh, c.lineHighlight);

		if (i == m_hlLine && m_hlLen > 0) {
			const int hx = gw + 5 - scrollX + fm.horizontalAdvance(txt.left(m_hlCol));
			const int hw = fm.horizontalAdvance(txt.mid(m_hlCol, m_hlLen));
			p.fillRect(hx, y, hw, lh, c.searchHighlight);
		}

		if (hasSel && i >= selStart.line && i <= selEnd.line) {
			const int startCol = (i == selStart.line) ? selStart.col : 0;
			const int endCol = (i == selEnd.line) ? selEnd.col : (int)txt.length();
			if (startCol < endCol || i < selEnd.line) {
				const int sx = gw + 5 - scrollX + fm.horizontalAdvance(txt.left(startCol));
				const int ex = (i < selEnd.line) ? vw : gw + 5 - scrollX + fm.horizontalAdvance(txt.left(endCol));
				p.fillRect(sx, y, ex - sx, lh, c.selection);
			}
		}

		p.setPen(i == m_cursor.current.line ? c.gutterFgCurrent : c.gutterFg);
		p.drawText(0, y, gw - 8, lh, Qt::AlignRight | Qt::AlignVCenter, QString::number(i + 1));

		if (!txt.isEmpty()) {
			QTextLayout layout(txt, font());
			layout.setFormats(m_highlighter.getLineFormat(txt));
			layout.beginLayout();
			QTextLine tl = layout.createLine();
			if (tl.isValid()) {
				tl.setLineWidth(vw);
				layout.endLayout();
				p.setPen(c.foreground);
				layout.draw(&p, QPointF(gw + 5 - scrollX, y));
			} else {
				layout.endLayout();
			}
		}

		if (i == m_cursor.current.line) {
			const int col = qMin(m_cursor.current.col, (int)txt.length());
			const int cx = gw + 5 - scrollX + fm.horizontalAdvance(txt.left(col));
			p.setPen(QPen(c.cursor, 2));
			p.drawLine(cx, y + 2, cx, y + lh - 2);
		}
	}
}

void EditorView::onContentChanged() {
	updateScrollBars();
	ensureCursorVisible();
	viewport()->update();
}

void EditorView::onCursorMoved() {
	ensureCursorVisible();
	viewport()->update();
}

void EditorView::syncLocalClipboard() { m_localClipboard = QApplication::clipboard()->text(); }

QString EditorView::clipboardText() {
	if (m_macroManager && m_macroManager->isPlaying()) return m_localClipboard;
	return QApplication::clipboard()->text();
}

void EditorView::setClipboardText(const QString& t) {
	m_localClipboard = t;
	QApplication::clipboard()->setText(t);
}

void EditorView::pushUndo() {
	m_undoPending = false;
	m_lastCharType = QChar();

	Snapshot s;
	s.pieces = m_pieceTable.pieces();
	s.cursor = m_cursor.current;
	m_undoStack.append(s);
	m_redoStack.clear();

	if (m_undoStack.size() > 200) m_undoStack.removeFirst();
}

void EditorView::undo() {
	if (m_undoStack.isEmpty()) return;

	Snapshot cur;
	cur.pieces = m_pieceTable.pieces();
	cur.cursor = m_cursor.current;
	m_redoStack.append(cur);

	const Snapshot& s = m_undoStack.last();
	m_pieceTable.setPieces(s.pieces);
	m_cursor.current = s.cursor;
	m_cursor.anchor = s.cursor;
	m_undoStack.removeLast();

	updateScrollBars();
	ensureCursorVisible();
	viewport()->update();
}

void EditorView::redo() {
	if (m_redoStack.isEmpty()) return;

	Snapshot cur;
	cur.pieces = m_pieceTable.pieces();
	cur.cursor = m_cursor.current;
	m_undoStack.append(cur);

	const Snapshot& s = m_redoStack.last();
	m_pieceTable.setPieces(s.pieces);
	m_cursor.current = s.cursor;
	m_cursor.anchor = s.cursor;
	m_redoStack.removeLast();

	updateScrollBars();
	ensureCursorVisible();
	viewport()->update();
}

void EditorView::simulateKeyEvent(int key, Qt::KeyboardModifiers mods, const QString& text) {
	QKeyEvent ev(QEvent::KeyPress, key, mods, text, false, 1);
	keyPressEvent(&ev);
}

void EditorView::injectKeyEvent(QKeyEvent* e) { keyPressEvent(e); }

void EditorView::keyPressEvent(QKeyEvent* e) {
	if (m_completionPopup->isVisible()) {
		switch (e->key()) {
		case Qt::Key_Escape:
			m_completionPopup->hide();
			return;
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
			QApplication::sendEvent(m_completionPopup, e);
			return;
		case Qt::Key_Tab:
			insertCompletion(m_completionPopup->currentItem());
			return;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			m_completionPopup->hide();
			break;
		default:
			break;
		}
	}

	if (m_macroManager && m_macroManager->isRecording()) m_macroManager->record(e);

	if (!EditorKeyHandler::handle(e, m_pieceTable, m_cursor, m_undoPending, m_lastCharType, *this)) {
		QAbstractScrollArea::keyPressEvent(e);
	}
}

void EditorView::focusInEvent(QFocusEvent* e) {
	QAbstractScrollArea::focusInEvent(e);
	m_undoPending = false;
	m_lastCharType = QChar();
}

void EditorView::mousePressEvent(QMouseEvent* e) {
	setFocus();
	const int totalLines = m_pieceTable.lineCount();

	const int line = qBound(0, (e->pos().y() + verticalScrollBar()->value()) / lineHeight(), totalLines - 1);
	const QString lineTxt = m_pieceTable.getLineText(line, MAX_LINE_LEN);
	const int col = hitColumn(e->pos().x(), lineTxt);

	const long elapsed = m_clickTimer.isValid() ? m_clickTimer.elapsed() : 9999;
	const int interval = qMax(QApplication::doubleClickInterval(), 400);

	if (elapsed < interval && m_clickCount >= 2 && m_lastClickLine == line) {
		m_clickCount = 3;
		m_cursor.anchor = {line, 0};
		m_cursor.current = {line, m_pieceTable.lineLength(line)};
		m_cursor.selecting = true;
		m_clickTimer.restart();
		viewport()->update();
		return;
	}

	m_clickCount = (elapsed > interval) ? 1 : m_clickCount + 1;
	m_lastClickLine = line;
	m_clickTimer.restart();

	m_cursor.current = {line, col};
	if (e->modifiers() & Qt::ShiftModifier) {
		m_cursor.selecting = true;
	} else {
		m_cursor.anchor = m_cursor.current;
		m_cursor.selecting = false;
	}
	viewport()->update();
}

void EditorView::mouseDoubleClickEvent(QMouseEvent* e) {
	const int line =
		qBound(0, (e->pos().y() + verticalScrollBar()->value()) / lineHeight(), m_pieceTable.lineCount() - 1);
	const int col = hitColumn(e->pos().x(), m_pieceTable.getLineText(line, MAX_LINE_LEN));

	m_clickCount = 2;
	m_lastClickLine = line;
	m_clickTimer.restart();

	auto [lo, hi] = wordBounds(m_pieceTable.getLineText(line, MAX_LINE_LEN), col);
	m_cursor.anchor = {line, lo};
	m_cursor.current = {line, hi};
	m_cursor.selecting = true;
	viewport()->update();
}

void EditorView::mouseMoveEvent(QMouseEvent* e) {
	if (!(e->buttons() & Qt::LeftButton)) return;
	const int line =
		qBound(0, (e->pos().y() + verticalScrollBar()->value()) / lineHeight(), m_pieceTable.lineCount() - 1);
	m_cursor.current = {line, hitColumn(e->pos().x(), m_pieceTable.getLineText(line, MAX_LINE_LEN))};
	m_cursor.selecting = true;
	viewport()->update();
}

void EditorView::mouseReleaseEvent(QMouseEvent*) {}

void EditorView::wheelEvent(QWheelEvent* e) {
	const int steps = e->angleDelta().y() / 40;
	verticalScrollBar()->setValue(verticalScrollBar()->value() - steps * lineHeight());
}

void EditorView::resizeEvent(QResizeEvent* e) {
	QAbstractScrollArea::resizeEvent(e);
	updateScrollBars();
}

int EditorView::hitColumn(int xPos, const QString& txt) const {
	QFontMetrics fm(font());
	const int gw = gutterWidth();
	const int scrollX = horizontalScrollBar()->value();
	int col = 0, minD = INT_MAX;
	for (int i = 0; i <= (int)txt.length(); ++i) {
		const int x = gw + 5 - scrollX + fm.horizontalAdvance(txt.left(i));
		const int diff = qAbs(xPos - x);
		if (diff < minD) {
			minD = diff;
			col = i;
		} else
			break;
	}
	return qBound(0, col, qMax(0, (int)txt.length()));
}

std::pair<int, int> EditorView::wordBounds(const QString& text, int col) {
	if (text.isEmpty()) return {0, 0};
	col = qBound(0, col, (int)text.length() - 1);

	auto isWord = [](QChar c) { return c.isLetterOrNumber() || c == '_'; };
	const bool onWord = isWord(text[col]);

	int lo = col, hi = col;
	while (lo > 0 && isWord(text[lo - 1]) == onWord) --lo;
	while (hi < (int)text.length() - 1 && isWord(text[hi + 1]) == onWord) ++hi;
	return {lo, hi + 1};
}

void EditorView::goToLine(int line) {
	line = qBound(0, line, m_pieceTable.lineCount() - 1);
	m_cursor.current = {line, 0};
	m_cursor.anchor = m_cursor.current;
	ensureCursorVisible();
	viewport()->update();
}

void EditorView::highlightSearchResult(int line, int col, int len) {
	m_hlLine = line;
	m_hlCol = col;
	m_hlLen = len;
	line = qBound(0, line, m_pieceTable.lineCount() - 1);
	m_cursor.anchor = {line, col};
	m_cursor.current = {line, col + len};
	m_cursor.selecting = true;
	ensureCursorVisible();
	viewport()->update();
}

void EditorView::clearHighlight() {
	m_hlLine = -1;
	m_hlLen = 0;
	viewport()->update();
}

bool EditorView::hasSelection() const { return m_cursor.anchor != m_cursor.current; }

QString EditorView::selectedText() const {
	if (!hasSelection()) return {};
	const TextPos start = m_cursor.anchor < m_cursor.current ? m_cursor.anchor : m_cursor.current;
	const TextPos end = m_cursor.anchor < m_cursor.current ? m_cursor.current : m_cursor.anchor;

	QString result;
	for (int i = start.line; i <= end.line; ++i) {
		const QString line = m_pieceTable.getLineText(i);
		const int from = (i == start.line) ? start.col : 0;
		const int to = (i == end.line) ? end.col : (int)line.length();
		result += line.mid(from, to - from);
		if (i < end.line) result += '\n';
	}
	return result;
}

void EditorView::deleteSelection() {
	if (!hasSelection()) return;
	const TextPos start = m_cursor.anchor < m_cursor.current ? m_cursor.anchor : m_cursor.current;
	const TextPos end = m_cursor.anchor < m_cursor.current ? m_cursor.current : m_cursor.anchor;

	const size_t startOff = m_pieceTable.getOffsetFromLineCol(start.line, start.col);
	const size_t endOff = m_pieceTable.getOffsetFromLineCol(end.line, end.col);

	if (endOff > startOff) m_pieceTable.remove(startOff, endOff - startOff);

	m_cursor.current = start;
	m_cursor.anchor = start;
	updateScrollBars();
	viewport()->update();
}

void EditorView::incrementCurrentNumber(int delta) {
	pushUndo();
	if (IncrementAction::run(m_pieceTable, m_cursor, delta)) onContentChanged();
}

void EditorView::saveHistory(const QString& path) {
	if (m_pieceTable.totalSize() > 50 * 1024 * 1024) return;

	QFile f(path + ".history");
	if (!f.open(QIODevice::WriteOnly)) return;
	QDataStream out(&f);
	out << m_pieceTable.addedBuffer() << (quint32)m_undoStack.size();
	for (const auto& s : std::as_const(m_undoStack)) {
		out << s.cursor.line << s.cursor.col << (quint32)s.pieces.size();
		for (const auto& piece : s.pieces)
			out << (int)piece.bufferType << (quint64)piece.start << (quint64)piece.length;
	}
}

void EditorView::loadHistory(const QString& path) {
	QFile f(path + ".history");
	if (f.size() > 10 * 1024 * 1024 || !f.open(QIODevice::ReadOnly)) return;
	QDataStream in(&f);

	QByteArray added;
	quint32 stackSize;
	in >> added >> stackSize;

	if (stackSize > 100000 || added.size() > 50 * 1024 * 1024) return;

	m_pieceTable.setAddedBuffer(added);

	m_undoStack.clear();
	for (quint32 i = 0; i < stackSize; ++i) {
		Snapshot s;
		quint32 pSize;
		in >> s.cursor.line >> s.cursor.col >> pSize;
		if (pSize > 100000) return;
		s.pieces.reserve(pSize);
		for (quint32 j = 0; j < pSize; ++j) {
			int type;
			quint64 start, length;
			in >> type >> start >> length;
			s.pieces.append(Piece((BufferType)type, start, length));
		}
		m_undoStack.append(s);
	}
	m_redoStack.clear();
}