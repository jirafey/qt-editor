#ifndef EDITORSELECTION_H
#define EDITORSELECTION_H

#include "editorcursor.h"
#include "piecetable.h"
#include <QString>

class EditorSelection {
  public:
	static bool hasSelection(const EditorCursor& cursor);
	static QString selectedText(const EditorCursor& cursor, PieceTable& table);
	static bool deleteSelection(EditorCursor& cursor, PieceTable& table);
};

#endif