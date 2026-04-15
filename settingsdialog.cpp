#include "settingsdialog.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

void SettingsDialog::syncColorsToScheme() {
	auto applyOne = [&](const QString& k, QColor& field) {
		if (m_pendingColors.contains(k)) field = m_pendingColors[k];
	};
	auto& c = SettingsManager::instance().colors();
	applyOne("background", c.background);
	applyOne("text", c.foreground);
	applyOne("current line", c.lineHighlight);
	applyOne("selection", c.selection);
	applyOne("left tab background", c.gutter);
	applyOne("left tab text", c.gutterFg);
	applyOne("left tab text current line", c.gutterFgCurrent);
	applyOne("cursor", c.cursor);
	applyOne("search highlight", c.searchHighlight);
	applyOne("syntax highlight Keyword", c.synKeyword);
	applyOne("syntax highlight Preprocessor", c.synPreprocessor);
	applyOne("syntax highlight String", c.synString);
	applyOne("syntax highlight Comment", c.synComment);
	applyOne("syntax highlight Number", c.synNumber);
	emit livePreview();
}

void SettingsDialog::pushColorsToUI() {
	for (int i = 0; i < m_colorTable->rowCount(); ++i) {
		auto* item = m_colorTable->item(i, 0);
		if (item && m_pendingColors.contains(item->text())) {
			auto* preview = qobject_cast<QLabel*>(m_colorTable->cellWidget(i, 1));
			if (preview) {
				preview->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 2px;")
										   .arg(m_pendingColors[item->text()].name(QColor::HexArgb)));
			}
		}
	}
}

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
	setWindowTitle("Settings");
	setMinimumSize(700, 500);

	m_tabs = new QTabWidget(this);
	auto* mainLayout = new QVBoxLayout(this);

	QWidget* editorTab = new QWidget(this);
	QWidget* colorsTab = new QWidget(this);
	QWidget* keybindsTab = new QWidget(this);

	m_tabs->addTab(editorTab, "Editor");
	m_tabs->addTab(colorsTab, "Colors");
	m_tabs->addTab(keybindsTab, "Keybinds");

	buildEditorTab(editorTab);
	buildColorsTab(colorsTab);
	buildKeybindsTab(keybindsTab);

	mainLayout->addWidget(m_tabs);

	auto* btnRow = new QHBoxLayout();
	auto* applyBtn = new QPushButton("Apply", this);
	auto* importBtn = new QPushButton("Import", this);
	auto* exportBtn = new QPushButton("Export", this);
	auto* closeBtn = new QPushButton("Close", this);
	auto* revertBtn = new QPushButton("Revert", this);

	btnRow->addWidget(importBtn);
	btnRow->addWidget(exportBtn);
	btnRow->addStretch();
	btnRow->addWidget(revertBtn);
	btnRow->addWidget(applyBtn);
	btnRow->addWidget(closeBtn);
	mainLayout->addLayout(btnRow);

	connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
	connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImport);
	connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExport);
	connect(revertBtn, &QPushButton::clicked, this, &SettingsDialog::onRevert);
	connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

	m_backupColors = SettingsManager::instance().colors();
	m_backupOptions = SettingsManager::instance().options();
	m_hasBackup = true;
}

void SettingsDialog::buildEditorTab(QWidget* tab) {
	auto* layout = new QVBoxLayout(tab);
	auto* formLayout = new QGridLayout();

	m_fontFamily = new QComboBox(tab);
	m_fontFamily->addItems({"Monospace", "Courier New", "DejaVu Sans Mono", "Liberation Mono", "Consolas"});

	m_fontSize = new QSpinBox(tab);
	m_fontSize->setRange(6, 72);

	m_tabWidth = new QSpinBox(tab);
	m_tabWidth->setRange(1, 16);

	m_useSpaces = new QCheckBox("Use spaces for tabs", tab);
	m_wordWrap = new QCheckBox("Word wrap", tab);
	m_syntaxHighlight = new QCheckBox("Enable Syntax Highlighting", tab);

	const auto& opts = SettingsManager::instance().options();
	int idx = m_fontFamily->findText(opts.fontFamily);
	if (idx >= 0) m_fontFamily->setCurrentIndex(idx);

	m_fontSize->setValue(opts.fontSize);
	m_tabWidth->setValue(opts.tabWidth);
	m_useSpaces->setChecked(opts.useSpaces);
	m_wordWrap->setChecked(opts.wordWrap);
	m_syntaxHighlight->setChecked(opts.enableSyntaxHighlighting);

	formLayout->addWidget(new QLabel("Font Family:"), 0, 0);
	formLayout->addWidget(m_fontFamily, 0, 1);
	formLayout->addWidget(new QLabel("Font Size:"), 1, 0);
	formLayout->addWidget(m_fontSize, 1, 1);
	formLayout->addWidget(new QLabel("Tab Width:"), 2, 0);
	formLayout->addWidget(m_tabWidth, 2, 1);

	layout->addLayout(formLayout);
	layout->addWidget(m_useSpaces);
	layout->addWidget(m_wordWrap);
	layout->addWidget(m_syntaxHighlight);
	layout->addStretch();
}

void SettingsDialog::buildColorsTab(QWidget* tab) {
	auto* layout = new QVBoxLayout(tab);

	auto* topRow = new QHBoxLayout();
	topRow->addWidget(new QLabel("Theme Preset:", tab));
	m_presetCombo = new QComboBox(tab);
	m_presetCombo->addItems({"Custom", "Default Dark", "Light", "High Contrast Dark", "High Contrast Light"});
	topRow->addWidget(m_presetCombo);

	m_undoBtn = new QPushButton("Undo Last Color Change", tab);
	m_undoBtn->setEnabled(false);
	topRow->addWidget(m_undoBtn);

	topRow->addStretch();
	layout->addLayout(topRow);

	connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onPresetChanged);
	connect(m_undoBtn, &QPushButton::clicked, this, &SettingsDialog::onUndoColor);

	m_colorTable = new QTableWidget(0, 2, tab);
	m_colorTable->setHorizontalHeaderLabels({"Color Key", "Preview"});
	m_colorTable->horizontalHeader()->setStretchLastSection(true);
	m_colorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_colorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	layout->addWidget(m_colorTable);

	populateColors();

	connect(m_colorTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
		auto* item = m_colorTable->item(row, 0);
		if (item) onColorEdit(item->text());
	});
}

void SettingsDialog::buildKeybindsTab(QWidget* tab) {
	auto* layout = new QVBoxLayout(tab);
	m_keyTable = new QTableWidget(0, 3, tab);
	m_keyTable->setHorizontalHeaderLabels({"Category", "Action", "Shortcut"});
	m_keyTable->horizontalHeader()->setStretchLastSection(true);
	m_keyTable->setSortingEnabled(true);
	m_keyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_keyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	layout->addWidget(m_keyTable);

	populateKeybinds();

	connect(m_keyTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { onKeybindEdit(row); });
}

void SettingsDialog::populateColors() {
	auto& c = SettingsManager::instance().colors();
	struct Entry {
		QString key;
		QColor color;
		QString tooltip;
	};
	QList<Entry> entries = {
		{"background", c.background, "The main background color of the editor"},
		{"text", c.foreground, "The default color for standard text"},
		{"current line", c.lineHighlight, "Highlight color for the line containing the cursor"},
		{"selection", c.selection, "Background color for selected text"},
		{"left tab background", c.gutter, "Background color of the line numbers gutter"},
		{"left tab text", c.gutterFg, "Color of the standard line numbers"},
		{"left tab text current line", c.gutterFgCurrent, "Color of the currently active line number"},
		{"cursor", c.cursor, "Color of the blinking text cursor"},
		{"search highlight", c.searchHighlight, "Highlight color for text matching your search query"},
		{"syntax highlight Keyword", c.synKeyword, "Color for C/C++ keywords (e.g. int, class, return)"},
		{"syntax highlight Preprocessor", c.synPreprocessor, "Color for directives (e.g. #include, #define)"},
		{"syntax highlight String", c.synString, "Color for text strings and character literals"},
		{"syntax highlight Comment", c.synComment, "Color for code comments (// or /* */)"},
		{"syntax highlight Number", c.synNumber, "Color for numeric values (123, 0xFF, etc.)"}};

	m_colorTable->setRowCount(entries.size());
	for (int i = 0; i < entries.size(); ++i) {
		auto* item = new QTableWidgetItem(entries[i].key);
		item->setToolTip(entries[i].tooltip);
		m_colorTable->setItem(i, 0, item);

		auto* preview = new QLabel;
		preview->setFixedSize(50, 20);
		m_colorTable->setCellWidget(i, 1, preview);
		m_pendingColors[entries[i].key] = entries[i].color;
	}
	pushColorsToUI();
}

void SettingsDialog::populateKeybinds() {
	const auto& binds = SettingsManager::instance().keys().binds;
	m_keyTable->setSortingEnabled(false);
	m_keyTable->setRowCount(0);
	int row = 0;
	for (auto it = binds.constBegin(); it != binds.constEnd(); ++it) {
		QString key = it.key();
		if (key.startsWith("media")) continue;

		QString category = "Editor";
		if (key.startsWith("macro")) category = "Custom Macros";

		m_keyTable->insertRow(row);
		m_keyTable->setItem(row, 0, new QTableWidgetItem(category));
		m_keyTable->setItem(row, 1, new QTableWidgetItem(key));
		m_keyTable->setItem(row, 2, new QTableWidgetItem(it.value().toString()));
		m_pendingKeys[key] = it.value().toString();
		row++;
	}
	m_keyTable->setSortingEnabled(true);
	m_keyTable->sortItems(0);
}

void SettingsDialog::onColorEdit(const QString& key) {
	QColor originalColor = m_pendingColors.value(key);
	QMap<QString, QColor> stateBefore = m_pendingColors;

	QColorDialog dialog(originalColor, this);
	dialog.setWindowTitle("Pick color for: " + key);
	dialog.setOption(QColorDialog::ShowAlphaChannel);

	connect(&dialog, &QColorDialog::currentColorChanged, this, [this, key](const QColor& c) {
		m_pendingColors[key] = c;
		pushColorsToUI();
		syncColorsToScheme();
	});

	if (dialog.exec() == QDialog::Rejected) {
		m_pendingColors[key] = originalColor;
		pushColorsToUI();
		syncColorsToScheme();
	} else {
		m_colorHistory.append(stateBefore);
		m_undoBtn->setEnabled(true);
		m_presetCombo->blockSignals(true);
		m_presetCombo->setCurrentIndex(0);
		m_presetCombo->blockSignals(false);
	}
}

void SettingsDialog::onUndoColor() {
	if (m_colorHistory.isEmpty()) return;
	m_pendingColors = m_colorHistory.takeLast();
	m_undoBtn->setEnabled(!m_colorHistory.isEmpty());

	pushColorsToUI();
	syncColorsToScheme();

	m_presetCombo->blockSignals(true);
	m_presetCombo->setCurrentIndex(0);
	m_presetCombo->blockSignals(false);
}

void SettingsDialog::onPresetChanged(int index) {
	if (index == 0) return; // Custom

	m_colorHistory.append(m_pendingColors);
	m_undoBtn->setEnabled(true);

	ColorScheme p;
	if (index == 1) { // Default Dark
		p.background = QColor("#1e1e1e");
		p.foreground = QColor("#d4d4d4");
		p.lineHighlight = QColor(255, 255, 255, 18);
		p.selection = QColor("#264f78b4");
		p.gutter = QColor("#1e1e1e");
		p.gutterFg = QColor("#858585");
		p.gutterFgCurrent = QColor("#c6c6c6");
		p.cursor = QColor("#aeafad");
		p.searchHighlight = QColor(255, 200, 0, 80);
		p.synKeyword = QColor("#569cd6");
		p.synPreprocessor = QColor("#c586c0");
		p.synString = QColor("#ce9178");
		p.synComment = QColor("#6a9955");
		p.synNumber = QColor("#b5cea8");
	} else if (index == 2) { // Light
		p.background = QColor("#ffffff");
		p.foreground = QColor("#000000");
		p.lineHighlight = QColor(0, 0, 0, 15);
		p.selection = QColor("#add6ff");
		p.gutter = QColor("#f3f3f3");
		p.gutterFg = QColor("#666666");
		p.gutterFgCurrent = QColor("#000000");
		p.cursor = QColor("#000000");
		p.searchHighlight = QColor(255, 200, 0, 120);
		p.synKeyword = QColor("#0000ff");
		p.synPreprocessor = QColor("#800080");
		p.synString = QColor("#a31515");
		p.synComment = QColor("#008000");
		p.synNumber = QColor("#098658");
	} else if (index == 3) { // High Contrast Dark
		p.background = QColor("#000000");
		p.foreground = QColor("#ffffff");
		p.lineHighlight = QColor("#222222");
		p.selection = QColor("#008080");
		p.gutter = QColor("#000000");
		p.gutterFg = QColor("#aaaaaa");
		p.gutterFgCurrent = QColor("#ffffff");
		p.cursor = QColor("#ffffff");
		p.searchHighlight = QColor("#808000");
		p.synKeyword = QColor("#ffff00");
		p.synPreprocessor = QColor("#ff00ff");
		p.synString = QColor("#00ffff");
		p.synComment = QColor("#00ff00");
		p.synNumber = QColor("#ff8000");
	} else if (index == 4) { // High Contrast Light
		p.background = QColor("#ffffff");
		p.foreground = QColor("#000000");
		p.lineHighlight = QColor("#e0e0e0");
		p.selection = QColor("#000080");
		p.gutter = QColor("#ffffff");
		p.gutterFg = QColor("#000000");
		p.gutterFgCurrent = QColor("#000000");
		p.cursor = QColor("#000000");
		p.searchHighlight = QColor("#ffff00");
		p.synKeyword = QColor("#0000ff");
		p.synPreprocessor = QColor("#ff00ff");
		p.synString = QColor("#aa0000");
		p.synComment = QColor("#008000");
		p.synNumber = QColor("#800000");
	}

	m_pendingColors["background"] = p.background;
	m_pendingColors["text"] = p.foreground;
	m_pendingColors["current line"] = p.lineHighlight;
	m_pendingColors["selection"] = p.selection;
	m_pendingColors["left tab background"] = p.gutter;
	m_pendingColors["left tab text"] = p.gutterFg;
	m_pendingColors["left tab text current line"] = p.gutterFgCurrent;
	m_pendingColors["cursor"] = p.cursor;
	m_pendingColors["search highlight"] = p.searchHighlight;
	m_pendingColors["syntax highlight Keyword"] = p.synKeyword;
	m_pendingColors["syntax highlight Preprocessor"] = p.synPreprocessor;
	m_pendingColors["syntax highlight String"] = p.synString;
	m_pendingColors["syntax highlight Comment"] = p.synComment;
	m_pendingColors["syntax highlight Number"] = p.synNumber;

	pushColorsToUI();
	syncColorsToScheme();
}

void SettingsDialog::onKeybindEdit(int row) {
	auto* actionItem = m_keyTable->item(row, 1);
	auto* keyItem = m_keyTable->item(row, 2);
	if (!actionItem || !keyItem) return;

	bool ok;
	QString newKey = QInputDialog::getText(
		this, "Edit Shortcut", "Shortcut for '" + actionItem->text() + "':", QLineEdit::Normal, keyItem->text(), &ok);

	if (ok) {
		keyItem->setText(newKey);
		m_pendingKeys[actionItem->text()] = newKey;
	}
}

void SettingsDialog::onApply() {
	applyToSettings();
	emit settingsChanged();
}

void SettingsDialog::onRevert() {
	if (!m_hasBackup) return;

	m_pendingColors.clear();
	auto& colors = SettingsManager::instance().colors();
	colors = m_backupColors;

	auto& options = SettingsManager::instance().options();
	options = m_backupOptions;

	m_colorHistory.clear();
	m_undoBtn->setEnabled(false);

	populateColors();
	populateKeybinds();

	const auto& opts = m_backupOptions;
	int idx = m_fontFamily->findText(opts.fontFamily);
	if (idx >= 0) m_fontFamily->setCurrentIndex(idx);
	m_fontSize->setValue(opts.fontSize);
	m_tabWidth->setValue(opts.tabWidth);
	m_useSpaces->setChecked(opts.useSpaces);
	m_wordWrap->setChecked(opts.wordWrap);
	m_syntaxHighlight->setChecked(opts.enableSyntaxHighlighting);

	emit livePreview();
}

void SettingsDialog::applyToSettings() {
	auto& sm = SettingsManager::instance();
	sm.options().fontFamily = m_fontFamily->currentText();
	sm.options().fontSize = m_fontSize->value();
	sm.options().tabWidth = m_tabWidth->value();
	sm.options().useSpaces = m_useSpaces->isChecked();
	sm.options().wordWrap = m_wordWrap->isChecked();
	sm.options().enableSyntaxHighlighting = m_syntaxHighlight->isChecked();

	syncColorsToScheme();

	for (auto it = m_pendingKeys.constBegin(); it != m_pendingKeys.constEnd(); ++it) {
		sm.keys().binds[it.key()] = QKeySequence(it.value());
	}
	sm.save();
}

void SettingsDialog::onImport() {
	QString path = QFileDialog::getOpenFileName(this, "Import Settings", "", "JSON (*.json)");
	if (!path.isEmpty()) {
		m_colorHistory.append(m_pendingColors);
		m_undoBtn->setEnabled(true);

		SettingsManager::instance().load(path);
		populateColors();
		populateKeybinds();
		emit livePreview();
	}
}

void SettingsDialog::onExport() {
	QString path = QFileDialog::getSaveFileName(this, "Export Settings", "settings.json", "JSON (*.json)");
	if (!path.isEmpty()) SettingsManager::instance().save(path);
}