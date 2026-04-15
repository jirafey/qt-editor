#ifndef FILEOPERATIONS_H
#define FILEOPERATIONS_H

#include "piecetable.h"
#include <QString>
#include <QWidget>

class FileOperations {
  public:
	static bool saveFile(PieceTable& table, const QString& path);
	static QString getOpenPath(QWidget* parent);
	static QString getSavePath(QWidget* parent);
};

#endif