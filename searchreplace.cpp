#include "searchreplace.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

static SearchResult searchInLine(const QString& line, const QString& query, int startCol, bool cs, bool fuzzy,
								 bool forward) {
	if (query.isEmpty()) return {};
	if (!fuzzy) {
		int idx = forward ? line.indexOf(query, startCol, cs ? Qt::CaseSensitive : Qt::CaseInsensitive)
						  : line.lastIndexOf(query, startCol, cs ? Qt::CaseSensitive : Qt::CaseInsensitive);
		if (idx >= 0) return {0, idx, (int)query.length()};
		return {};
	}

	if (forward) {
		int mStart = -1, qIdx = 0;
		for (int c = startCol; c < line.length(); ++c) {
			if ((cs ? line[c] : line[c].toLower()) == (cs ? query[qIdx] : query[qIdx].toLower())) {
				if (qIdx == 0) mStart = c;
				qIdx++;
				if (qIdx == query.length()) return {0, mStart, c - mStart + 1};
			}
		}
	} else {
		int mEnd = -1, qIdx = query.length() - 1;
		int limit = (startCol < 0 || startCol >= line.length()) ? line.length() - 1 : startCol;
		for (int c = limit; c >= 0; --c) {
			if ((cs ? line[c] : line[c].toLower()) == (cs ? query[qIdx] : query[qIdx].toLower())) {
				if (qIdx == query.length() - 1) mEnd = c;
				qIdx--;
				if (qIdx < 0) return {0, c, mEnd - c + 1};
			}
		}
	}
	return {};
}

SearchResult SearchReplace::findNext(PieceTable& table, const QString& query, int fromLine, int fromCol, bool cs,
									 bool wrap, bool fuzzy) {
	QByteArray buf;
	buf.reserve(4096);
	for (int i = fromLine; i < table.lineCount(); ++i) {
		table.getLineTextInto(i, buf);
		auto res = searchInLine(QString::fromUtf8(buf), query, (i == fromLine) ? fromCol : 0, cs, fuzzy, true);
		if (res.isValid()) {
			res.line = i;
			return res;
		}
	}
	if (wrap) {
		for (int i = 0; i <= fromLine; ++i) {
			table.getLineTextInto(i, buf);
			auto res = searchInLine(QString::fromUtf8(buf), query, 0, cs, fuzzy, true);
			if (res.isValid()) {
				res.line = i;
				return res;
			}
		}
	}
	return {};
}

SearchResult SearchReplace::findPrev(PieceTable& table, const QString& query, int fromLine, int fromCol, bool cs,
									 bool wrap, bool fuzzy) {
	QByteArray buf;
	buf.reserve(4096);
	if (fromCol > 0) {
		table.getLineTextInto(fromLine, buf);
		auto res = searchInLine(QString::fromUtf8(buf), query, fromCol - 1, cs, fuzzy, false);
		if (res.isValid()) {
			res.line = fromLine;
			return res;
		}
	}
	for (int i = fromLine - 1; i >= 0; --i) {
		table.getLineTextInto(i, buf);
		auto res = searchInLine(QString::fromUtf8(buf), query, -1, cs, fuzzy, false);
		if (res.isValid()) {
			res.line = i;
			return res;
		}
	}
	if (wrap) {
		for (int i = table.lineCount() - 1; i >= fromLine; --i) {
			table.getLineTextInto(i, buf);
			auto res = searchInLine(QString::fromUtf8(buf), query, -1, cs, fuzzy, false);
			if (res.isValid()) {
				if (i == fromLine && res.col >= fromCol) continue;
				res.line = i;
				return res;
			}
		}
	}
	return {};
}

void SearchReplace::replaceAll(PieceTable& table, const QString& query, const QString& repl, bool cs, bool fuzzy) {
	QByteArray buf;
	buf.reserve(4096);
	for (int i = table.lineCount() - 1; i >= 0; --i) {
		int pos = 0;
		while (true) {
			table.getLineTextInto(i, buf);
			auto res = searchInLine(QString::fromUtf8(buf), query, pos, cs, fuzzy, true);
			if (!res.isValid()) break;

			size_t offset = table.getOffsetFromLineCol(i, res.col);
			size_t endOff = table.getOffsetFromLineCol(i, res.col + res.length);

			table.remove(offset, endOff - offset);
			table.insert(offset, repl);
			pos = res.col + repl.length();
		}
	}
}

int SearchReplace::countMatches(PieceTable& table, const QString& query, bool cs, bool fuzzy) {
	int count = 0;
	QByteArray buf;
	buf.reserve(4096);
	for (int i = 0; i < table.lineCount(); ++i) {
		int pos = 0;
		while (true) {
			table.getLineTextInto(i, buf);
			auto res = searchInLine(QString::fromUtf8(buf), query, pos, cs, fuzzy, true);
			if (!res.isValid()) break;
			count++;
			pos = res.col + res.length;
		}
	}
	return count;
}

SearchReplaceDialog::SearchReplaceDialog(QWidget* parent) : QDialog(parent) {
	setWindowTitle("Search & Replace");
	auto* l = new QVBoxLayout(this);

	auto* r1 = new QHBoxLayout();
	m_searchEdit = new QLineEdit(this);
	m_findPrevBtn = new QPushButton("◁ Prev", this);
	m_findNextBtn = new QPushButton("Next ▷", this);
	m_findNextBtn->setDefault(true);
	r1->addWidget(new QLabel("Find:", this));
	r1->addWidget(m_searchEdit);
	r1->addWidget(m_findPrevBtn);
	r1->addWidget(m_findNextBtn);
	l->addLayout(r1);

	auto* r2 = new QHBoxLayout();
	m_caseSensitive = new QCheckBox("Case sensitive", this);
	m_wrap = new QCheckBox("Wrap around", this);
	m_wrap->setChecked(true);
	m_fuzzy = new QCheckBox("Fuzzy search", this);
	m_fuzzy->setChecked(true);
	r2->addWidget(m_caseSensitive);
	r2->addWidget(m_wrap);
	r2->addWidget(m_fuzzy);
	l->addLayout(r2);

	auto* r3 = new QHBoxLayout();
	m_replaceEdit = new QLineEdit(this);
	m_replaceBtn = new QPushButton("Replace", this);
	m_replaceAllBtn = new QPushButton("Replace All", this);
	r3->addWidget(new QLabel("Replace:", this));
	r3->addWidget(m_replaceEdit);
	r3->addWidget(m_replaceBtn);
	r3->addWidget(m_replaceAllBtn);
	l->addLayout(r3);

	connect(m_findNextBtn, &QPushButton::clicked, this, [this]() {
		emit findRequested(m_searchEdit->text(), m_caseSensitive->isChecked(), m_wrap->isChecked(), true,
						   m_fuzzy->isChecked());
	});
	connect(m_findPrevBtn, &QPushButton::clicked, this, [this]() {
		emit findRequested(m_searchEdit->text(), m_caseSensitive->isChecked(), m_wrap->isChecked(), false,
						   m_fuzzy->isChecked());
	});
	connect(m_replaceBtn, &QPushButton::clicked, this, [this]() {
		emit replaceRequested(m_searchEdit->text(), m_replaceEdit->text(), m_caseSensitive->isChecked(),
							  m_fuzzy->isChecked());
	});
	connect(m_replaceAllBtn, &QPushButton::clicked, this, [this]() {
		emit replaceAllRequested(m_searchEdit->text(), m_replaceEdit->text(), m_caseSensitive->isChecked(),
								 m_fuzzy->isChecked());
	});
	connect(m_searchEdit, &QLineEdit::returnPressed, m_findNextBtn, &QPushButton::click);
}
