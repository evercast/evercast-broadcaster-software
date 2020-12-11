#pragma once

#include <QDialog>

#include "ui_EBSNewVersion.h"

class EBSNewVersion : public QDialog {
	Q_OBJECT

public:
	EBSNewVersion(QWidget *parent, const QString &text);

public slots:
	void on_ok_clicked();

private:
	std::unique_ptr<Ui_EBSNewVersion> ui;
};
