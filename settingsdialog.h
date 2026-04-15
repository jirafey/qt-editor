#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H
#include "settingsmanager.h"
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>

class SettingsDialog : public QDialog {
	Q_OBJECT
  public:
	explicit SettingsDialog(QWidget* parent = nullptr);

  signals:
	void settingsChanged();
	void livePreview();

  private slots:
	void onApply();
	void onImport();
	void onExport();
	void onRevert();
	void onColorEdit(const QString& key);
	void onKeybindEdit(int row);
	void onPresetChanged(int index);
	void onUndoColor();

  private:
	void buildEditorTab(QWidget* tab);
	void buildColorsTab(QWidget* tab);
	void buildKeybindsTab(QWidget* tab);
	void populateColors();
	void populateKeybinds();
	void applyToSettings();

	void syncColorsToScheme();
	void pushColorsToUI();

	QTabWidget* m_tabs = nullptr;
	QComboBox* m_fontFamily = nullptr;
	QSpinBox* m_fontSize = nullptr;
	QSpinBox* m_tabWidth = nullptr;
	QCheckBox* m_useSpaces = nullptr;
	QCheckBox* m_wordWrap = nullptr;
	QCheckBox* m_syntaxHighlight = nullptr;

	QComboBox* m_presetCombo = nullptr;
	QPushButton* m_undoBtn = nullptr;
	QTableWidget* m_colorTable = nullptr;
	QMap<QString, QColor> m_pendingColors;
	QList<QMap<QString, QColor>> m_colorHistory;
	QMap<QString, QLabel*> m_colorPreviews;

	QTableWidget* m_keyTable = nullptr;
	QMap<QString, QString> m_pendingKeys;

	ColorScheme m_backupColors;
	EditorOptions m_backupOptions;
	bool m_hasBackup = false;
};

#endif