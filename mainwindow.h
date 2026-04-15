#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QShortcut>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class EditorTabManager;
class LinuxConsole;
class SearchReplaceDialog;
class MacroManager;
class ShortcutManager;
class EditorView;
class QPushButton;

class MainWindow : public QMainWindow {
	Q_OBJECT
  public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

  protected:
	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

  private slots:
	void bindMacroShortcut(const QString& macroName, const QKeySequence& key);

	void onNewFile();
	void onOpenFile();
	void onSaveFile();
	void onSaveFileAs();
	void onCloseCurrentTab();

	void onUndo();
	void onRedo();
	void onCopy();
	void onCut();
	void onPaste();
	void onSelectAll();
	void onGoToLine();
	void onQuickOpen();

	void onFind();
	void onFindNext();
	void onFindPrev();
	void onReplaceAll(const QString& query, const QString& repl, bool cs, bool fuzzy);
	void onFindRequested(const QString& query, bool cs, bool wrap, bool forward, bool fuzzy);
	void onReplaceRequested(const QString& query, const QString& repl, bool cs, bool fuzzy);

	void onToggleTerminal();
	void onZoomIn();
	void onZoomOut();
	void onZoomReset();

	void onRunInTerminal();
	void onShowSettings();
	void onStartMacroRecord();
	void onStopMacroRecord();
	void onPlayMacro();
	void onIncrementNumber();

	void onAbout();

	void updateStatusBar();

  private:
	void setupEditor(EditorView* v, const QString& path);
	void sendCtrlKey(int key);
	QMap<QString, QShortcut*> m_macroShortcuts;
	void setupMenus();
	void setupStatusBar();
	void setupTerminalDock();
	void setupShortcuts();
	void applySettings();

	Ui::MainWindow* ui;
	EditorTabManager* m_tabManager = nullptr;
	QDockWidget* m_terminalDock = nullptr;
	LinuxConsole* m_console = nullptr;
	SearchReplaceDialog* m_searchDialog = nullptr;
	MacroManager* m_macroManager = nullptr;
	ShortcutManager* m_shortcutMgr = nullptr;

	QLabel* m_statusPos = nullptr;
	QLabel* m_statusMode = nullptr;
	QLabel* m_statusFile = nullptr;
	QLabel* m_statusLines = nullptr;

	QString m_lastQuery;
	bool m_lastCaseSensitive = false;
	bool m_lastFuzzy = true;

	int m_fontSize = 11;
	int m_untitledCount = 1;
};

#endif