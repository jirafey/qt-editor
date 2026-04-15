#include "macroplayerdialog.h"
#include "editorview.h"

#include "settingsmanager.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

static QString describeStep(const MacroStep& s) {
	if (s.type == MacroStepType::IncrementNumber) {
		return QString("[Increment] +%1 to first number on line").arg(s.incrementDelta);
	}

	QString mod;
	if (s.mods & Qt::ControlModifier) mod += "Ctrl+";
	if (s.mods & Qt::ShiftModifier) mod += "Shift+";
	if (s.mods & Qt::AltModifier) mod += "Alt+";

	if (!s.text.isEmpty() && s.text.at(0).isPrint() && !(s.mods & Qt::ControlModifier) && !(s.mods & Qt::AltModifier)) {
		return QString("Type '%1'").arg(s.text);
	}

	QString keyName;
	switch (s.key) {
	case Qt::Key_Up:
		keyName = "Up";
		break;
	case Qt::Key_Down:
		keyName = "Down";
		break;
	case Qt::Key_Left:
		keyName = "Left";
		break;
	case Qt::Key_Right:
		keyName = "Right";
		break;
	case Qt::Key_Home:
		keyName = "Home";
		break;
	case Qt::Key_End:
		keyName = "End";
		break;
	case Qt::Key_Return:
		keyName = "Enter";
		break;
	case Qt::Key_Backspace:
		keyName = "Backspace";
		break;
	case Qt::Key_Delete:
		keyName = "Delete";
		break;
	case Qt::Key_Tab:
		keyName = "Tab";
		break;
	case Qt::Key_PageUp:
		keyName = "PageUp";
		break;
	case Qt::Key_PageDown:
		keyName = "PageDown";
		break;
	case Qt::Key_C:
		keyName = "C";
		break;
	case Qt::Key_V:
		keyName = "V";
		break;
	case Qt::Key_X:
		keyName = "X";
		break;
	case Qt::Key_Z:
		keyName = "Z";
		break;
	case Qt::Key_A:
		keyName = "A";
		break;
	case Qt::Key_S:
		keyName = "S";
		break;
	default:
		keyName = QKeySequence(s.key).toString();
		break;
	}
	return mod + keyName;
}

MacroPlayerDialog::MacroPlayerDialog(MacroManager* mgr, EditorView* view, QWidget* parent)
	: QDialog(parent), m_mgr(mgr), m_view(view) {
	setWindowTitle("Macro Manager");
	setMinimumSize(640, 520);

	for (const QString& name : m_mgr->macroNames()) {
		QKeySequence seq = SettingsManager::instance().keys().binds.value("macro_" + name);
		if (!seq.isEmpty()) { m_shortcuts[name] = seq; }
	}

	buildUi();
	refreshTable();
}

void MacroPlayerDialog::buildUi() {
	auto* root = new QVBoxLayout(this);
	root->setSpacing(8);

	auto* splitter = new QSplitter(Qt::Horizontal, this);

	auto* leftWidget = new QWidget(this);
	auto* leftLayout = new QVBoxLayout(leftWidget);
	leftLayout->setContentsMargins(0, 0, 0, 0);

	auto* listGroup = new QGroupBox("Recorded Macros", this);
	auto* listLayout = new QVBoxLayout(listGroup);

	m_table = new QTableWidget(0, 3, this);
	m_table->setHorizontalHeaderLabels({"Name", "Shortcut", "Steps"});
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setAlternatingRowColors(true);
	listLayout->addWidget(m_table);
	leftLayout->addWidget(listGroup);

	auto* keyGroup = new QGroupBox("Assign Playback Shortcut", this);
	auto* keyLayout = new QHBoxLayout(keyGroup);
	keyLayout->addWidget(new QLabel("Key:", this));
	m_keyEdit = new QKeySequenceEdit(this);
	m_keyEdit->setMaximumWidth(160);
	keyLayout->addWidget(m_keyEdit);
	m_assignKeyBtn = new QPushButton("Assign to Selected", this);
	keyLayout->addWidget(m_assignKeyBtn);
	keyLayout->addStretch();
	leftLayout->addWidget(keyGroup);

	splitter->addWidget(leftWidget);

	auto* rightWidget = new QWidget(this);
	auto* rightLayout = new QVBoxLayout(rightWidget);
	rightLayout->setContentsMargins(0, 0, 0, 0);

	auto* stepGroup = new QGroupBox("Steps", this);
	auto* stepLayout = new QVBoxLayout(stepGroup);

	m_stepTable = new QTableWidget(0, 2, this);
	m_stepTable->setHorizontalHeaderLabels({"#", "Description"});
	m_stepTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_stepTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_stepTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_stepTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_stepTable->setAlternatingRowColors(true);
	stepLayout->addWidget(m_stepTable);

	m_debugLabel = new QLabel("Select a macro to view its steps.", this);
	m_debugLabel->setWordWrap(true);
	m_debugLabel->setStyleSheet("font-size: 12px; color: gray;");
	stepLayout->addWidget(m_debugLabel);

	rightLayout->addWidget(stepGroup);
	splitter->addWidget(rightWidget);

	splitter->setSizes({300, 320});
	root->addWidget(splitter, 1);

	auto* btnRow = new QHBoxLayout();
	m_playBtn = new QPushButton("▶  Play", this);
	m_playBtn->setDefault(true);

	m_playCountBox = new QSpinBox(this);
	m_playCountBox->setRange(1, 9999);
	m_playCountBox->setValue(1);
	m_playCountBox->setPrefix("Times: ");

	m_deleteBtn = new QPushButton("✕  Delete Macro", this);
	auto* closeBtn = new QPushButton("Close", this);

	btnRow->addWidget(m_playBtn);
	btnRow->addWidget(m_playCountBox);
	btnRow->addSpacing(10);
	btnRow->addWidget(m_deleteBtn);
	btnRow->addStretch();
	btnRow->addWidget(closeBtn);
	root->addLayout(btnRow);

	connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row, int, int, int) {
		if (row < 0) {
			m_stepTable->setRowCount(0);
			return;
		}
		auto* item = m_table->item(row, 0);
		if (item) refreshStepTable(item->text());
	});

	connect(m_playBtn, &QPushButton::clicked, this, &MacroPlayerDialog::onPlay);
	connect(m_deleteBtn, &QPushButton::clicked, this, &MacroPlayerDialog::onDelete);
	connect(m_assignKeyBtn, &QPushButton::clicked, this, &MacroPlayerDialog::onAssignKey);
	connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void MacroPlayerDialog::refreshTable() {
	m_table->setRowCount(0);
	for (const QString& name : m_mgr->macroNames()) {
		int row = m_table->rowCount();
		m_table->insertRow(row);
		m_table->setItem(row, 0, new QTableWidgetItem(name));
		QString keyStr = m_shortcuts.contains(name) ? m_shortcuts[name].toString() : "(none)";
		m_table->setItem(row, 1, new QTableWidgetItem(keyStr));
		m_table->setItem(row, 2, new QTableWidgetItem(QString::number(m_mgr->stepsFor(name).size())));
	}
	m_stepTable->setRowCount(0);
	m_debugLabel->setText("Select a macro to view its steps.");
}

void MacroPlayerDialog::refreshStepTable(const QString& macroName) {
	m_stepTable->setRowCount(0);
	const auto& steps = m_mgr->stepsFor(macroName);
	for (int i = 0; i < steps.size(); ++i) {
		int row = m_stepTable->rowCount();
		m_stepTable->insertRow(row);
		m_stepTable->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1)));
		m_stepTable->setItem(row, 1, new QTableWidgetItem(describeStep(steps[i])));
	}
	m_debugLabel->setText(
		macroName.isEmpty() ? "" : QString("%1 step(s) in '%2'").arg(QString::number(steps.size()), macroName));
}

QString MacroPlayerDialog::selectedMacroName() const {
	int row = m_table->currentRow();
	if (row < 0) return {};
	auto* item = m_table->item(row, 0);
	return item ? item->text() : QString();
}

void MacroPlayerDialog::onPlay() {
	QString name = selectedMacroName();
	if (name.isEmpty()) {
		QMessageBox::information(this, "Macro Manager", "Select a macro to play.");
		return;
	}
	int times = m_playCountBox->value();
	if (m_view) {
		for (int i = 0; i < times; ++i) { m_mgr->play(name, m_view); }
	}
}

void MacroPlayerDialog::onDelete() {
	QString name = selectedMacroName();
	if (name.isEmpty()) return;
	m_mgr->removeMacro(name);
	m_shortcuts.remove(name);
	SettingsManager::instance().keys().binds.remove("macro_" + name);
	refreshTable();
}

void MacroPlayerDialog::onAssignKey() {
	QString name = selectedMacroName();
	if (name.isEmpty()) {
		QMessageBox::information(this, "Macro Manager", "Select a macro first.");
		return;
	}
	QKeySequence seq = m_keyEdit->keySequence();
	if (seq.isEmpty()) {
		QMessageBox::information(this, "Macro Manager", "Press a key combination first.");
		return;
	}

	auto& binds = SettingsManager::instance().keys().binds;
	QString conflictName;
	for (auto it = binds.begin(); it != binds.end(); ++it) {
		if (it.value() == seq && !it.key().startsWith("macro_")) {
			conflictName = "Standard Action: " + it.key();
			break;
		}
	}
	if (conflictName.isEmpty()) {
		for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
			if (it.value() == seq && it.key() != name) {
				conflictName = "Other Macro: " + it.key();
				break;
			}
		}
	}

	if (!conflictName.isEmpty()) {
		auto reply =
			QMessageBox::warning(this, "Shortcut In Use",
								 QString("The shortcut %1 is already assigned to '%2'.\nDo you want to overwrite it?")
									 .arg(seq.toString(), conflictName),
								 QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No) return;
	}

	m_shortcuts[name] = seq;
	refreshTable();

	const QStringList names = m_mgr->macroNames();
	int row = names.indexOf(name);
	if (row >= 0) m_table->selectRow(row);

	emit shortcutAssigned(name, seq);
}