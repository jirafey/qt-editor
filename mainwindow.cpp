#include "mainwindow.h"
#include "editortabmanager.h"
#include "editorview.h"
#include "fileoperations.h"
#include "fuzzysearcher.h"
#include "linuxconsole.h"
#include "macromanager.h"
#include "macroplayerdialog.h"
#include "searchreplace.h"
#include "settingsdialog.h"
#include "settingsmanager.h"
#include "shortcutmanager.h"
#include "ui_mainwindow.h"
#include <QPushButton>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	setWindowTitle("TextEditor");
	resize(1200, 800);

	m_tabManager = new EditorTabManager(this);
	setCentralWidget(m_tabManager);

	m_macroManager = new MacroManager(this);
	m_shortcutMgr = new ShortcutManager(this);

	m_searchDialog = new SearchReplaceDialog(this);

	setupMenus();
	setupStatusBar();
	setupTerminalDock();
	setupShortcuts();
	applySettings();

	m_macroManager->fromJson(SettingsManager::instance().macros);
	for (const QString& name : m_macroManager->macroNames()) {
		QKeySequence seq = SettingsManager::instance().keys().binds.value("macro_" + name);
		if (!seq.isEmpty()) { bindMacroShortcut(name, seq); }
	}

	onNewFile();

	QTimer* statusTimer = new QTimer(this);
	connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
	statusTimer->start(200);

	connect(m_searchDialog, &SearchReplaceDialog::findRequested, this, &MainWindow::onFindRequested);
	connect(m_searchDialog, &SearchReplaceDialog::replaceRequested, this, &MainWindow::onReplaceRequested);
	connect(m_searchDialog, &SearchReplaceDialog::replaceAllRequested, this, &MainWindow::onReplaceAll);

	connect(m_macroManager, &MacroManager::recordingStarted, this, [this]() {
		m_statusMode->setText("● REC");
		m_statusMode->setStyleSheet("color: #f44; font-weight: bold;");
	});
	connect(m_macroManager, &MacroManager::recordingStopped, this, [this](const QString& name) {
		m_statusMode->setText("READY");
		m_statusMode->setStyleSheet("");
		if (!name.isEmpty()) statusBar()->showMessage("Macro '" + name + "' saved.", 2000);
	});

	qApp->installEventFilter(this);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupMenus() {
	QMenu* file = menuBar()->addMenu("&File");
	file->addAction("&New", QKeySequence::New, this, &MainWindow::onNewFile);
	file->addAction("&Open...", QKeySequence::Open, this, &MainWindow::onOpenFile);
	file->addSeparator();
	file->addAction("&Save", QKeySequence::Save, this, &MainWindow::onSaveFile);
	file->addAction("Save &As...", QKeySequence::SaveAs, this, &MainWindow::onSaveFileAs);
	file->addSeparator();
	file->addAction("&Close Tab", SettingsManager::instance().keys().binds.value("closeTab", QKeySequence::Close), this,
					&MainWindow::onCloseCurrentTab);
	file->addSeparator();
	file->addAction("E&xit", QKeySequence::Quit, qApp, &QApplication::quit);

	QMenu* edit = menuBar()->addMenu("&Edit");
	edit->addAction("&Undo", QKeySequence::Undo, this, &MainWindow::onUndo);
	edit->addAction("&Redo", QKeySequence::Redo, this, &MainWindow::onRedo);
	edit->addSeparator();
	edit->addAction("&Cut", QKeySequence::Cut, this, &MainWindow::onCut);
	edit->addAction("C&opy", QKeySequence::Copy, this, &MainWindow::onCopy);
	edit->addAction("&Paste", QKeySequence::Paste, this, &MainWindow::onPaste);
	edit->addAction("Select &All", QKeySequence::SelectAll, this, &MainWindow::onSelectAll);
	edit->addSeparator();
	edit->addAction("Go to &Line...", QKeySequence(Qt::CTRL | Qt::Key_G), this, &MainWindow::onGoToLine);

	QMenu* search = menuBar()->addMenu("&Search");
	search->addAction("&Find / Replace...", QKeySequence::Find, this, &MainWindow::onFind);

	QMenu* view = menuBar()->addMenu("&View");
	view->addAction("Toggle &Terminal", QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft), this,
					&MainWindow::onToggleTerminal);
	view->addSeparator();
	view->addAction("Zoom &In", QKeySequence(Qt::CTRL | Qt::Key_Plus), this, &MainWindow::onZoomIn);
	view->addAction("Zoom &Out", QKeySequence(Qt::CTRL | Qt::Key_Minus), this, &MainWindow::onZoomOut);
	view->addAction("&Reset Zoom", QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::onZoomReset);

	QMenu* tools = menuBar()->addMenu("&Tools");
	tools->addAction("&Run in Terminal", QKeySequence(Qt::CTRL | Qt::Key_F5), this, &MainWindow::onRunInTerminal);
	tools->addSeparator();
	tools->addAction("&Start Macro Recording", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), this,
					 &MainWindow::onStartMacroRecord);
	tools->addAction("S&top Macro Recording", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), this,
					 &MainWindow::onStopMacroRecord);
	tools->addAction("&Macro Manager...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), this,
					 &MainWindow::onPlayMacro);
	tools->addAction("&Increment Number...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L), this,
					 &MainWindow::onIncrementNumber);
	tools->addSeparator();
	tools->addAction("&Settings...", this, &MainWindow::onShowSettings);

	QMenu* help = menuBar()->addMenu("&Help");
	help->addAction("&About", this, &MainWindow::onAbout);
	help->addAction("Keyboard &Shortcuts", this, [this]() {
		QString msg = "<b>Keyboard Shortcuts</b><br><br><table border='0' cellspacing='4'>";
		auto binds = SettingsManager::instance().keys().binds;
		for (auto it = binds.constBegin(); it != binds.constEnd(); ++it) {
			msg += "<tr><td align='right'><b>" + it.value().toString() + "</b></td><td width='15'></td><td>" +
				   it.key() + "</td></tr>";
		}
		msg += "</table>";
		QMessageBox::information(this, "Keyboard Shortcuts", msg);
	});
}

void MainWindow::setupStatusBar() {
	m_statusFile = new QLabel("Untitled", this);
	m_statusPos = new QLabel("Ln 1, Col 1", this);
	m_statusLines = new QLabel("0 lines", this);
	m_statusMode = new QLabel("READY", this);

	m_statusFile->setMinimumWidth(200);
	m_statusPos->setMinimumWidth(120);
	m_statusLines->setMinimumWidth(80);
	m_statusMode->setMinimumWidth(80);

	statusBar()->addWidget(m_statusFile, 2);
	statusBar()->addPermanentWidget(m_statusLines);
	statusBar()->addPermanentWidget(m_statusPos);
	statusBar()->addPermanentWidget(m_statusMode);

	statusBar()->setStyleSheet("QStatusBar { background: #252525; color: #aaa; border-top: 1px solid #333; }"
							   "QStatusBar::item { border: none; }"
							   "QLabel { color: #aaa; padding: 0 6px; }");
}

void MainWindow::setupTerminalDock() {
	m_console = new LinuxConsole(this);
	m_terminalDock = new QDockWidget("Terminal", this);
	m_terminalDock->setWidget(m_console);
	m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
	addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
	m_terminalDock->hide();
}

void MainWindow::setupShortcuts() {
	m_shortcutMgr->registerAction("new", QKeySequence::New, [this]() { onNewFile(); });
	m_shortcutMgr->registerAction("open", QKeySequence::Open, [this]() { onOpenFile(); });
	m_shortcutMgr->registerAction("save", QKeySequence::Save, [this]() { onSaveFile(); });
	m_shortcutMgr->registerAction("quickOpen", QKeySequence(Qt::CTRL | Qt::Key_K), [this]() { onQuickOpen(); });
	m_shortcutMgr->registerAction("findNext", QKeySequence::FindNext, [this]() { onFindNext(); });
	m_shortcutMgr->registerAction("findPrev", QKeySequence::FindPrevious, [this]() { onFindPrev(); });
}

void MainWindow::applySettings() {
	auto& sm = SettingsManager::instance();

	m_macroManager->fromJson(sm.macros);
	for (const QString& name : m_macroManager->macroNames()) {
		QKeySequence seq = sm.keys().binds.value("macro_" + name);
		if (!seq.isEmpty()) { bindMacroShortcut(name, seq); }
	}

	QFont f = sm.getFont();
	if (m_tabManager) {
		for (int i = 0; i < m_tabManager->count(); ++i) {
			EditorView* ev = qobject_cast<EditorView*>(m_tabManager->widget(i));
			if (ev) {
				ev->setFont(f);
				ev->reloadColors();
				ev->viewport()->update();
			}
		}
	}
	m_fontSize = sm.options().fontSize;

	menuBar()->clear();
	setupMenus();
}

void MainWindow::onNewFile() {
	if (m_tabManager) {
		m_tabManager->addNewTab();
		setupEditor(m_tabManager->currentEditor(), "");
		updateStatusBar();
	}
}

void MainWindow::onOpenFile() {
	QString path = FileOperations::getOpenPath(this);
	if (path.isEmpty() || !m_tabManager) return;

	SettingsManager::instance().recentFiles.removeAll(path);
	SettingsManager::instance().recentFiles.prepend(path);

	m_tabManager->openFile(path);
	EditorView* v = m_tabManager->currentEditor();
	setupEditor(v, path);
	if (v) v->loadHistory(path);

	updateStatusBar();
}

void MainWindow::onSaveFile() {
	EditorView* view = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!view) return;

	QString path = m_tabManager->currentFilePath();
	if (path.isEmpty()) {
		onSaveFileAs();
		return;
	}
	if (FileOperations::saveFile(view->pieceTable(), path)) {
		view->afterSave();
		view->saveHistory(path);
		statusBar()->showMessage("Saved: " + path, 2000);
	} else {
		QMessageBox::warning(this, "Save Error", "Could not save file:\n" + path);
	}
}

void MainWindow::onSaveFileAs() {
	EditorView* view = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!view) return;

	QString path = FileOperations::getSavePath(this);
	if (path.isEmpty()) return;

	if (FileOperations::saveFile(view->pieceTable(), path)) {
		m_tabManager->setCurrentFilePath(path);

		view->afterSave();
		view->saveHistory(path);

		SettingsManager::instance().recentFiles.removeAll(path);
		SettingsManager::instance().recentFiles.prepend(path);

		statusBar()->showMessage("Saved: " + path, 2000);
	} else {
		QMessageBox::warning(this, "Save Error", "Could not save file:\n" + path);
	}
}

void MainWindow::onCloseCurrentTab() {
	if (!m_tabManager) return;
	int idx = m_tabManager->currentIndex();
	if (idx >= 0) m_tabManager->closeTab(idx);
}

void MainWindow::onUndo() { sendCtrlKey(Qt::Key_Z); }
void MainWindow::onRedo() { sendCtrlKey(Qt::Key_Y); }
void MainWindow::onCopy() { sendCtrlKey(Qt::Key_C); }
void MainWindow::onCut() { sendCtrlKey(Qt::Key_X); }
void MainWindow::onPaste() { sendCtrlKey(Qt::Key_V); }
void MainWindow::onSelectAll() { sendCtrlKey(Qt::Key_A); }

void MainWindow::onQuickOpen() {
	QDialog dlg(this);
	dlg.setWindowTitle("Quick Open (Project & Recent)");
	dlg.setMinimumWidth(450);
	QVBoxLayout l(&dlg);
	QLineEdit edit(&dlg);
	edit.setPlaceholderText("Type file name to search...");
	QListWidget list(&dlg);
	l.addWidget(&edit);
	l.addWidget(&list);

	QStringList allFiles = SettingsManager::instance().recentFiles;
	QDirIterator it(QDir::currentPath(), QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext() && allFiles.size() < 2000) {
		QString f = it.next();
		if (!allFiles.contains(f)) allFiles.append(f);
	}

	auto updateList = [&]() {
		list.clear();
		QStringList matches = FuzzySearcher::search(edit.text(), allFiles, 20);
		list.addItems(matches);
		if (list.count() > 0) list.setCurrentRow(0);
	};

	updateList();
	connect(&edit, &QLineEdit::textChanged, &dlg, updateList);
	connect(&edit, &QLineEdit::returnPressed, &dlg, &QDialog::accept);
	connect(&list, &QListWidget::itemDoubleClicked, &dlg, &QDialog::accept);

	if (dlg.exec() == QDialog::Accepted && list.currentItem() && m_tabManager) {
		QString path = list.currentItem()->text();
		m_tabManager->openFile(path);
		EditorView* v = m_tabManager->currentEditor();
		setupEditor(v, path);
		if (v) v->loadHistory(path);
		updateStatusBar();
	}
}

void MainWindow::onGoToLine() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v) return;
	bool ok;
	int line = QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1, v->pieceTable().lineCount(), 1, &ok);
	if (ok) v->goToLine(line - 1);
}

void MainWindow::onFind() {
	if (m_searchDialog) {
		m_searchDialog->show();
		m_searchDialog->raise();
		m_searchDialog->activateWindow();
	}
}

void MainWindow::onFindNext() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v || m_lastQuery.isEmpty())
		onFind();
	else
		onFindRequested(m_lastQuery, m_lastCaseSensitive, true, true, m_lastFuzzy);
}

void MainWindow::onFindPrev() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v || m_lastQuery.isEmpty())
		onFind();
	else
		onFindRequested(m_lastQuery, m_lastCaseSensitive, true, false, m_lastFuzzy);
}

void MainWindow::onFindRequested(const QString& query, bool cs, bool wrap, bool forward, bool fuzzy) {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v || query.isEmpty()) return;
	m_lastQuery = query;
	m_lastCaseSensitive = cs;
	m_lastFuzzy = fuzzy;

	auto p = v->cursorPos();
	int startCol = p.col;

	if (v->hasSelection()) {
		startCol = forward ? qMax(v->cursor().anchor.col, v->cursor().current.col)
						   : qMin(v->cursor().anchor.col, v->cursor().current.col);
	}

	SearchResult res = forward ? SearchReplace::findNext(v->pieceTable(), query, p.line, startCol, cs, wrap, fuzzy)
							   : SearchReplace::findPrev(v->pieceTable(), query, p.line, startCol, cs, wrap, fuzzy);

	if (res.isValid()) {
		v->highlightSearchResult(res.line, res.col, res.length);
		bool wrapped = forward ? (res.line < p.line || (res.line == p.line && res.col <= p.col))
							   : (res.line > p.line || (res.line == p.line && res.col >= p.col));
		QString msg = "Found at line " + QString::number(res.line + 1) + ", col " + QString::number(res.col + 1);
		if (wrapped) msg += " ↺ (Wrapped around)";
		statusBar()->showMessage(msg, 3000);
	} else {
		v->clearHighlight();
		statusBar()->showMessage("Not found: " + query, 2000);
	}
}

void MainWindow::onReplaceRequested(const QString& query, const QString& repl, bool cs, bool fuzzy) {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v || query.isEmpty()) return;
	SearchResult res = SearchReplace::findNext(v->pieceTable(), query, 0, 0, cs, true, fuzzy);

	if (res.isValid()) {
		v->pushUndo();
		size_t offset = v->pieceTable().getOffsetFromLineCol(res.line, res.col);
		size_t endOff = v->pieceTable().getOffsetFromLineCol(res.line, res.col + res.length);
		v->pieceTable().remove(offset, endOff - offset);
		v->pieceTable().insert(offset, repl);
		v->highlightSearchResult(res.line, res.col, repl.length());
		v->onContentChanged();
		statusBar()->showMessage("Replaced 1 occurrence.", 2000);
	} else {
		statusBar()->showMessage("Not found: " + query, 2000);
	}
}

void MainWindow::onReplaceAll(const QString& query, const QString& repl, bool cs, bool fuzzy) {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v || query.isEmpty()) return;
	v->pushUndo();
	int before = SearchReplace::countMatches(v->pieceTable(), query, cs, fuzzy);
	SearchReplace::replaceAll(v->pieceTable(), query, repl, cs, fuzzy);
	v->clearHighlight();
	v->onContentChanged();
	statusBar()->showMessage("Replaced " + QString::number(before) + " occurrence(s).", 3000);
}

void MainWindow::onToggleTerminal() {
	if (m_terminalDock && m_terminalDock->isVisible()) {
		m_terminalDock->hide();
		if (m_tabManager) {
			if (EditorView* v = m_tabManager->currentEditor()) v->setFocus();
		}
	} else if (m_terminalDock) {
		m_terminalDock->show();
		if (m_console) m_console->setFocus();
	}
}

void MainWindow::onZoomIn() {
	m_fontSize = qMin(m_fontSize + 1, 36);
	QFont f = SettingsManager::instance().getFont();
	f.setPointSize(m_fontSize);
	SettingsManager::instance().setFont(f);
	applySettings();
	if (EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr) {
		v->setFont(f);
		v->viewport()->update();
	}
}

void MainWindow::onZoomOut() {
	m_fontSize = qMax(m_fontSize - 1, 6);
	QFont f = SettingsManager::instance().getFont();
	f.setPointSize(m_fontSize);
	SettingsManager::instance().setFont(f);
	applySettings();
	if (EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr) {
		v->setFont(f);
		v->viewport()->update();
	}
}

void MainWindow::onZoomReset() {
	m_fontSize = 11;
	QFont f = SettingsManager::instance().getFont();
	f.setPointSize(m_fontSize);
	SettingsManager::instance().setFont(f);
	applySettings();
	if (EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr) {
		v->setFont(f);
		v->viewport()->update();
	}
}

void MainWindow::onRunInTerminal() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v || !m_terminalDock || !m_console) return;

	QString path = m_tabManager->currentFilePath();
	QString cmd;

	if (!path.isEmpty()) {
		onSaveFile();

		if (path.endsWith(".cpp")) {
			QString outExe = QDir::tempPath() + "/a.out";
			cmd = "g++ \"" + path + "\" -o \"" + outExe + "\" && \"" + outExe + "\"";
		} else if (path.endsWith(".c")) {
			QString outExe = QDir::tempPath() + "/a.out";
			cmd = "gcc \"" + path + "\" -o \"" + outExe + "\" && \"" + outExe + "\"";
		} else if (path.endsWith(".py")) {
			cmd = "python3 \"" + path + "\"";
		} else if (path.endsWith(".sh")) {
			cmd = "bash \"" + path + "\"";
		} else if (path.endsWith(".js")) {
			cmd = "node \"" + path + "\"";
		} else {
			cmd = "\"" + path + "\"";
		}
	} else {
		cmd = v->selectedText().trimmed();
		if (cmd.isEmpty()) {
			statusBar()->showMessage("No file path or selection to run.", 2000);
			return;
		}
	}

	m_terminalDock->show();
	m_console->runCommand(cmd);
}

void MainWindow::onStartMacroRecord() {
	if (m_macroManager) m_macroManager->startRecording();
	statusBar()->showMessage("Macro recording started… (Ctrl+Shift+T to stop)", 0);
}

void MainWindow::onStopMacroRecord() {
	if (!m_macroManager || !m_macroManager->isRecording()) return;
	bool ok;
	QString name = QInputDialog::getText(this, "Save Macro", "Macro name:", QLineEdit::Normal,
										 "macro" + QString::number(m_macroManager->macroNames().size() + 1), &ok);
	if (ok) m_macroManager->stopRecording(name);
}

void MainWindow::bindMacroShortcut(const QString& macroName, const QKeySequence& key) {
	if (m_macroShortcuts.contains(macroName)) {
		delete m_macroShortcuts[macroName];
		m_macroShortcuts.remove(macroName);
	}
	for (auto it = m_macroShortcuts.begin(); it != m_macroShortcuts.end();) {
		if (it.value()->key() == key) {
			delete it.value();
			it = m_macroShortcuts.erase(it);
		} else {
			++it;
		}
	}
	auto* sc = new QShortcut(key, this);
	m_macroShortcuts[macroName] = sc;
	connect(sc, &QShortcut::activated, this, [this, macroName]() {
		EditorView* view = m_tabManager ? m_tabManager->currentEditor() : nullptr;
		if (view && m_macroManager) {
			m_macroManager->play(macroName, view);
			statusBar()->showMessage("Macro '" + macroName + "' played.", 1500);
		}
	});
	SettingsManager::instance().keys().binds["macro_" + macroName] = key;
}

void MainWindow::onPlayMacro() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	MacroPlayerDialog dlg(m_macroManager, v, this);
	connect(
		&dlg, &MacroPlayerDialog::shortcutAssigned, this, [this](const QString& macroName, const QKeySequence& key) {
			bindMacroShortcut(macroName, key);
			statusBar()->showMessage("Shortcut " + key.toString() + " assigned to macro '" + macroName + "'.", 3000);
		});
	dlg.exec();
}

void MainWindow::onIncrementNumber() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v) return;

	bool ok;
	int times =
		QInputDialog::getInt(this, "Increment Number", "How many times to increment by 1?", 1, -9999, 9999, 1, &ok);
	if (!ok || times == 0) return;

	if (m_macroManager && m_macroManager->isRecording()) m_macroManager->recordIncrementStep(times);

	v->pushUndo();
	v->incrementCurrentNumber(times);
}

void MainWindow::onShowSettings() {
	SettingsDialog dlg(this);
	connect(&dlg, &SettingsDialog::settingsChanged, this, &MainWindow::applySettings);
	connect(&dlg, &SettingsDialog::livePreview, this, [this]() {
		if (EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr) {
			v->setFont(SettingsManager::instance().getFont());
			v->reloadColors();
			v->viewport()->update();
		}
	});
	dlg.exec();
}

void MainWindow::onAbout() {
	static const QString aboutText =
		"<h3>TextEditor</h3>"
		"<p>A configurable text editor built with <b>Qt 6</b> and <b>C++17</b> for Linux.</p>"
		"<p>Features:</p>"
		"<ul>"
		"<li>Piece-table based editing (efficient insert/delete)</li>"
		"<li>Syntax highlighting (C/C++ keywords)</li>"
		"<li>Multi-tab editing</li>"
		"<li>Find &amp; Replace</li>"
		"<li>Undo/Redo</li>"
		"<li>Selection, Copy/Paste</li>"
		"<li>Macro recording &amp; playback</li>"
		"<li>Integrated Linux terminal</li>"
		"<li>Keyboard shortcut manager</li>"
		"</ul>";

	QMessageBox::about(this, "About TextEditor", aboutText);
}

void MainWindow::updateStatusBar() {
	EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr;
	if (!v) return;
	QString path = m_tabManager->currentFilePath();
	m_statusFile->setText(path.isEmpty() ? "Untitled" : path);
	m_statusLines->setText(QString::number(v->pieceTable().lineCount()) + " lines");
	TextPos pos = v->cursorPos();
	m_statusPos->setText("Ln " + QString::number(pos.line + 1) + ", Col " + QString::number(pos.col + 1));

	if (m_macroManager && m_macroManager->isRecording()) {
		m_statusMode->setText("● REC");
		m_statusMode->setStyleSheet("color: #f44; font-weight: bold;");
	} else {
		m_statusMode->setText("READY");
		m_statusMode->setStyleSheet("");
	}
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (m_macroManager) SettingsManager::instance().macros = m_macroManager->toJson();
	SettingsManager::instance().save();
	event->accept();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
	if (event->type() == QEvent::KeyPress && m_shortcutMgr) {
		if (m_shortcutMgr->handleEvent(static_cast<QKeyEvent*>(event))) { return true; }
	}
	return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setupEditor(EditorView* v, const QString& path) {
	Q_UNUSED(path);
	if (!v) return;
	auto& sm = SettingsManager::instance();
	v->setFont(sm.getFont());
	v->setMacroManager(m_macroManager);

	connect(v, &EditorView::openFileRequested, this, [this](const QString& targetPath, int line) {
		if (m_tabManager) {
			m_tabManager->openFile(targetPath);
			if (EditorView* newV = m_tabManager->currentEditor()) newV->goToLine(line);
		}
	});

	v->setFocus();
}

void MainWindow::sendCtrlKey(int key) {
	if (EditorView* v = m_tabManager ? m_tabManager->currentEditor() : nullptr) {
		QKeyEvent ev(QEvent::KeyPress, key, Qt::ControlModifier);
		v->injectKeyEvent(&ev);
	}
}