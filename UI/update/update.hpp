#pragma once

#include <QThread>
#include <QString>

class AutoUpdateThread : public QThread {
	Q_OBJECT

	bool manualUpdate;
	bool user_confirmed = false;

	virtual void run() override;

	void info(const QString &title, const QString &text);
	int queryUpdate(bool manualUpdate, const char *text_utf8);

private slots:
	void infoMsg(const QString &title, const QString &text);
	void newVersionMsg(const QString& releaseNotes);
	int queryUpdateSlot(bool manualUpdate, const QString &text);

	bool EBSVersionQuery(bool manualUpdate, std::string &str, std::string &error, long *responseCode, bool *updatesAvailable, bool *isBeta, std::string &version, std::string &notes);

public:
	AutoUpdateThread(bool manualUpdate_) : manualUpdate(manualUpdate_) {}
};

