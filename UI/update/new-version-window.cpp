#include "new-version-window.hpp"

EBSNewVersion::EBSNewVersion(QWidget *parent, const QString &text)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint |
				  Qt::WindowCloseButtonHint),
	  ui(new Ui_EBSNewVersion)
{
	ui->setupUi(this);
	ui->text->setHtml(text);
}

void EBSNewVersion::on_ok_clicked() {
	done(0);
}
