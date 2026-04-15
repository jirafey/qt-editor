#include "editortabmanager.h"
#include "editorview.h"
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

EditorTabManager::EditorTabManager(QWidget* parent) : QTabWidget(parent) {
	setTabsClosable(true);
	setMovable(true);
	connect(this, &QTabWidget::tabCloseRequested, this, &EditorTabManager::closeTab);
}

void EditorTabManager::addNewTab(const QString& filePath) {
	QString title = "Untitled";

	if (filePath != "Untitled" && !filePath.isEmpty()) {
		QFileInfo fi(filePath);

		if (!fi.exists()) {
			QMessageBox::warning(this, "File Not Found",
								 QString("Cannot open:\n%1\n\nThe file does not exist.").arg(filePath));
			return;
		}

		if (!fi.isReadable()) {
			QFile::Permissions perms = fi.permissions();
			auto bit = [&](QFile::Permission p, char c) -> char { return (perms & p) ? c : '-'; };
			QString permStr;
			permStr += bit(QFile::ReadOwner, 'r');
			permStr += bit(QFile::WriteOwner, 'w');
			permStr += bit(QFile::ExeOwner, 'x');
			permStr += bit(QFile::ReadGroup, 'r');
			permStr += bit(QFile::WriteGroup, 'w');
			permStr += bit(QFile::ExeGroup, 'x');
			permStr += bit(QFile::ReadOther, 'r');
			permStr += bit(QFile::WriteOther, 'w');
			permStr += bit(QFile::ExeOther, 'x');

			QMessageBox::warning(this, "Permission Denied",
								 QString("Cannot read:\n%1\n\n"
										 "Current permissions: %2\n\n"
										 "To grant yourself read access, run:\n"
										 "  chmod u+r \"%1\"")
									 .arg(filePath, permStr));
			return;
		}

		EditorView* view = new EditorView(this);
		view->pieceTable().loadOriginal(filePath);
		view->highlighter().setFilePath(filePath);
		title = fi.fileName();
		m_filePaths.append(filePath);

		int idx = addTab(view, title);
		setCurrentIndex(idx);
		view->setFocus();

	} else {
		EditorView* view = new EditorView(this);
		view->highlighter().setFilePath("");
		m_filePaths.append("");
		int idx = addTab(view, title);
		setCurrentIndex(idx);
		view->setFocus();
	}
}

void EditorTabManager::openFile(const QString& path) {
	for (int i = 0; i < count(); ++i) {
		if (i < m_filePaths.size() && m_filePaths[i] == path) {
			setCurrentIndex(i);
			return;
		}
	}
	addNewTab(path);
}

void EditorTabManager::saveCurrentTab(const QString& path) {
	int idx = currentIndex();
	if (idx >= 0 && idx < m_filePaths.size()) {
		m_filePaths[idx] = path;
		setTabText(idx, QFileInfo(path).fileName());
		if (EditorView* v = currentEditor()) v->highlighter().setFilePath(path);
	}
}

QString EditorTabManager::currentFilePath() const {
	int idx = currentIndex();
	if (idx >= 0 && idx < m_filePaths.size()) return m_filePaths[idx];
	return {};
}

void EditorTabManager::setCurrentFilePath(const QString& path) {
	int idx = currentIndex();
	if (idx >= 0 && idx < m_filePaths.size()) {
		m_filePaths[idx] = path;
		setTabText(idx, QFileInfo(path).fileName());
		if (EditorView* v = currentEditor()) v->highlighter().setFilePath(path);
	}
}

EditorView* EditorTabManager::currentEditor() { return qobject_cast<EditorView*>(currentWidget()); }

void EditorTabManager::closeTab(int index) {
	QWidget* w = widget(index);
	if (index < m_filePaths.size()) { m_filePaths.removeAt(index); }
	removeTab(index);

	delete w;

	if (count() == 0) { addNewTab(); }
}