#ifndef SEARCHREPLACE_H
#define SEARCHREPLACE_H

#include "piecetable.h"
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>

struct SearchResult {
	int line = -1;
	int col = -1;
	int length = 0;
	bool isValid() const { return line >= 0; }
};

class SearchReplace {
  public:
	static SearchResult findNext(PieceTable& table, const QString& query, int fromLine, int fromCol, bool cs, bool wrap,
								 bool fuzzy);
	static SearchResult findPrev(PieceTable& table, const QString& query, int fromLine, int fromCol, bool cs, bool wrap,
								 bool fuzzy);
	static void replaceAll(PieceTable& table, const QString& query, const QString& repl, bool cs, bool fuzzy);
	static int countMatches(PieceTable& table, const QString& query, bool cs, bool fuzzy);
};

class SearchReplaceDialog : public QDialog {
	Q_OBJECT
  public:
	explicit SearchReplaceDialog(QWidget* parent = nullptr);
  signals:
	void findRequested(const QString& query, bool cs, bool wrap, bool forward, bool fuzzy);
	void replaceRequested(const QString& query, const QString& repl, bool cs, bool fuzzy);
	void replaceAllRequested(const QString& query, const QString& repl, bool cs, bool fuzzy);

  private:
	QLineEdit* m_searchEdit;
	QLineEdit* m_replaceEdit;
	QCheckBox* m_caseSensitive;
	QCheckBox* m_wrap;
	QCheckBox* m_fuzzy;
	QPushButton* m_findPrevBtn;
	QPushButton* m_findNextBtn;
	QPushButton* m_replaceBtn;
	QPushButton* m_replaceAllBtn;
};
#endif
