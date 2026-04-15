#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	app.setApplicationName("TextEditor");
	app.setOrganizationName("MyEditor");
	app.setApplicationVersion("1.0.0");

	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
	darkPalette.setColor(QPalette::WindowText, QColor(212, 212, 212));
	darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
	darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
	darkPalette.setColor(QPalette::ToolTipBase, QColor(50, 50, 50));
	darkPalette.setColor(QPalette::ToolTipText, QColor(212, 212, 212));
	darkPalette.setColor(QPalette::Text, QColor(212, 212, 212));
	darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
	darkPalette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(38, 79, 120));
	darkPalette.setColor(QPalette::HighlightedText, Qt::white);
	darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(80, 80, 80));
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(80, 80, 80));
	app.setPalette(darkPalette);

	app.setStyle(QStyleFactory::create("Fusion"));

	app.setStyleSheet("QMenuBar { background: #2d2d2d; color: #ccc; }"
					  "QMenuBar::item:selected { background: #3a3a3a; }"
					  "QMenu { background: #2d2d2d; color: #ccc; border: 1px solid #444; }"
					  "QMenu::item:selected { background: #3a6ea8; }"
					  "QTabWidget::pane { border: none; }"
					  "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 5px 12px; "
					  "               border: 1px solid #444; border-bottom: none; margin-right: 1px; }"
					  "QTabBar::tab:selected { background: #1e1e1e; color: #fff; }"
					  "QTabBar::tab:hover { background: #383838; }"
					  "QDockWidget { titlebar-close-icon: url(none); color: #ccc; }"
					  "QDockWidget::title { background: #2d2d2d; padding: 4px; }"
					  "QScrollBar:vertical { background: #2d2d2d; width: 12px; }"
					  "QScrollBar::handle:vertical { background: #555; border-radius: 4px; min-height: 20px; }"
					  "QScrollBar::handle:vertical:hover { background: #777; }"
					  "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
					  "QScrollBar:horizontal { background: #2d2d2d; height: 12px; }"
					  "QScrollBar::handle:horizontal { background: #555; border-radius: 4px; min-width: 20px; }"
					  "QScrollBar::handle:horizontal:hover { background: #777; }"
					  "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
					  "QDialog { background: #2d2d2d; }"
					  "QLineEdit { background: #1e1e1e; color: #ddd; border: 1px solid #555; "
					  "            padding: 3px 6px; border-radius: 3px; }"
					  "QPushButton { background: #3a3a3a; color: #ccc; border: 1px solid #555; "
					  "              padding: 4px 12px; border-radius: 3px; }"
					  "QPushButton:hover { background: #4a4a4a; }"
					  "QPushButton:pressed { background: #2a2a2a; }"
					  "QCheckBox { color: #ccc; }"
					  "QSpinBox { background: #1e1e1e; color: #ddd; border: 1px solid #555; padding: 2px; }"
					  "QGroupBox { color: #aaa; border: 1px solid #444; border-radius: 4px; "
					  "            margin-top: 8px; padding-top: 8px; }"
					  "QGroupBox::title { subcontrol-origin: margin; left: 8px; }"
					  "QInputDialog QLineEdit { background: #1e1e1e; color: #ddd; }"
					  "QLabel { color: #ccc; }");

	MainWindow w;
	w.show();
	return app.exec();
}