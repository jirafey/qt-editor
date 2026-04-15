#include "fileoperations.h"
#include <QFileDialog>
#include <QSaveFile>

QString FileOperations::getOpenPath(QWidget* parent) {
	return QFileDialog::getOpenFileName(parent, "Open File", "",
										"All Files (*);;Text Files (*.txt);;C++ Files (*.cpp *.h *.hpp)", nullptr,
										QFileDialog::DontUseNativeDialog);
}

QString FileOperations::getSavePath(QWidget* parent) {
	return QFileDialog::getSaveFileName(parent, "Save File As", "",
										"All Files (*);;Text Files (*.txt);;C++ Files (*.cpp *.h *.hpp)", nullptr,
										QFileDialog::DontUseNativeDialog);
}

bool FileOperations::saveFile(PieceTable& table, const QString& path) {
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)) return false;

	table.writeTo(file);

	bool ok = file.commit();
	if (ok) table.releaseOriginalPages();

	return ok;
}