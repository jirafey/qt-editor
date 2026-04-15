#ifndef EDITORVIEW_H
#define EDITORVIEW_H

#include <QAbstractScrollArea>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QListWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QScrollBar>
#include <QString>

#include "editorcursor.h"
#include "editorkeyhandler.h"
#include "macromanager.h"
#include "piecetable.h"
#include "syntaxhighlighter.h"

class MacroManager;

class EditorView : public QAbstractScrollArea, public EditorKeyHandler::Delegate {
	Q_OBJECT
  public:
	explicit EditorView(QWidget* parent = nullptr);
	void afterSave();
	void simulateKeyEvent(int key, Qt::KeyboardModifiers mods, const QString& text = {});
	void injectKeyEvent(QKeyEvent* e);

	TextPos cursorPos() const { return m_cursor.current; }
	EditorCursor& cursor() { return m_cursor; }
	PieceTable& pieceTable() { return m_pieceTable; }
	SyntaxHighlighter& highlighter() { return m_highlighter; }

	void setMacroManager(MacroManager* mm) { m_macroManager = mm; }

	void reloadColors();
	void goToLine(int line);
	void highlightSearchResult(int line, int col, int len);
	void clearHighlight();

	bool hasSelection() const;
	QString selectedText() const;
	void deleteSelection();

	void undo();
	void redo();

	void saveHistory(const QString& path);
	void loadHistory(const QString& path);

	void syncLocalClipboard();
	void incrementCurrentNumber(int delta);

	void pushUndo() override;
	void onContentChanged() override;
	void onCursorMoved() override;
	QString clipboardText() override;
	void setClipboardText(const QString& t) override;

  signals:
	void openFileRequested(const QString& path, int line);

  private slots:
	void insertCompletion(QListWidgetItem* item);

  protected:
	bool focusNextPrevChild(bool) override { return false; }
	void focusInEvent(QFocusEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

  private:
	int hitColumn(int xPos, const QString& text) const;
	void updateScrollBars();
	int gutterWidth() const;
	int lineHeight() const;
	void ensureCursorVisible();
	static std::pair<int, int> wordBounds(const QString& text, int col);

	PieceTable m_pieceTable;
	EditorCursor m_cursor;
	SyntaxHighlighter m_highlighter;

	QListWidget* m_completionPopup = nullptr;

	int m_hlLine = -1;
	int m_hlCol = 0;
	int m_hlLen = 0;

	int m_clickCount = 0;
	int m_lastClickLine = -1;
	QElapsedTimer m_clickTimer;

	bool m_undoPending = false;
	QChar m_lastCharType;

	struct Snapshot {
		QList<Piece> pieces;
		TextPos cursor;
	};
	QList<Snapshot> m_undoStack;
	QList<Snapshot> m_redoStack;

	QString m_localClipboard;

	MacroManager* m_macroManager = nullptr;
};

#endif