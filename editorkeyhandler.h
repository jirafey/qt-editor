#ifndef EDITORKEYHANDLER_H
#define EDITORKEYHANDLER_H

#include "editorcursor.h"
#include "piecetable.h"
#include <QKeyEvent>

class EditorView;

class EditorKeyHandler {
  public:
	struct Delegate {
		virtual void onContentChanged() = 0;
		virtual void onCursorMoved() = 0;
		virtual void pushUndo() = 0;
		virtual void undo() = 0;
		virtual void redo() = 0;
		virtual QString clipboardText() = 0;
		virtual void setClipboardText(const QString& t) = 0;
		virtual ~Delegate() = default;
	};

	static bool handle(QKeyEvent* e, PieceTable& table, EditorCursor& cursor, bool& undoPending, QChar& lastCharType,
					   Delegate& d);
};

#endif