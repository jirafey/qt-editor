#include "linuxconsole.h"
#include "linuxcommandrunner.h"
#include <QVBoxLayout>

LinuxConsole::LinuxConsole(QWidget* parent) : QWidget(parent) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);

	m_output = new QTextEdit(this);
	m_output->setReadOnly(true);
	m_output->setFont(QFont("Monospace", 10));
	m_output->setStyleSheet("background:#1e1e1e; color:#d4d4d4;");

	m_input = new QLineEdit(this);
	m_input->setFont(QFont("Monospace", 10));
	m_input->setStyleSheet("background:#1e1e1e; color:#d4d4d4; border-top:1px solid #444;");
	m_input->setPlaceholderText("$ enter command...");

	layout->addWidget(m_output);
	layout->addWidget(m_input);

	connect(m_input, &QLineEdit::returnPressed, this, &LinuxConsole::executeCommand);
}

void LinuxConsole::executeCommand() {
	QString cmd = m_input->text().trimmed();
	if (cmd.isEmpty()) return;
	m_input->clear();
	m_output->append("<span style='color:#569cd6'>$ " + cmd.toHtmlEscaped() + "</span>");

	LinuxCommandRunner* runner = new LinuxCommandRunner(this);
	connect(runner, &LinuxCommandRunner::outputReady, this,
			[this](const QString& out) { m_output->append(out.toHtmlEscaped()); });
	connect(runner, &LinuxCommandRunner::errorReady, this, [this](const QString& err) {
		m_output->append("<span style='color:#f44'>" + err.toHtmlEscaped() + "</span>");
	});
	runner->runShell(cmd, "");
}

void LinuxConsole::runCommand(const QString& cmd) {
	m_input->setText(cmd);
	executeCommand();
}