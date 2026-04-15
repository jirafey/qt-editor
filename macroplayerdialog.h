#ifndef MACROPLAYERDIALOG_H
#define MACROPLAYERDIALOG_H

#include "macromanager.h"
#include <QDialog>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

class EditorView;

class MacroPlayerDialog : public QDialog {
	Q_OBJECT
  public:
	explicit MacroPlayerDialog(MacroManager* mgr, EditorView* view, QWidget* parent = nullptr);

  signals:
	void shortcutAssigned(const QString& macroName, const QKeySequence& key);

  private slots:
	void onPlay();
	void onDelete();
	void onAssignKey();
	void refreshTable();
	void refreshStepTable(const QString& macroName);

  private:
	void buildUi();
	QString selectedMacroName() const;

	MacroManager* m_mgr;
	EditorView* m_view;

	QTableWidget* m_table;
	QLabel* m_debugLabel;

	QTableWidget* m_stepTable;

	QSpinBox* m_playCountBox;
	QPushButton* m_playBtn;
	QPushButton* m_deleteBtn;
	QPushButton* m_assignKeyBtn;
	QKeySequenceEdit* m_keyEdit;

	QMap<QString, QKeySequence> m_shortcuts;
};

#endif