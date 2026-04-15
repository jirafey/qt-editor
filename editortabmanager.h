#ifndef EDITORTABMANAGER_H
#define EDITORTABMANAGER_H

#include <QString>
#include <QTabWidget>

class EditorView;

class EditorTabManager : public QTabWidget {
	Q_OBJECT
  public:
	explicit EditorTabManager(QWidget* parent = nullptr);

	EditorView* currentEditor();
	void addNewTab(const QString& filePath = "Untitled");
	void openFile(const QString& path);
	void saveCurrentTab(const QString& path);

	QString currentFilePath() const;
	void setCurrentFilePath(const QString& path);

  public slots:
	void closeTab(int index);

  private:
	QList<QString> m_filePaths;
};

#endif