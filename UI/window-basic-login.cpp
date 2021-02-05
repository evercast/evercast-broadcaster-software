
#include "window-basic-login.hpp"

#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

OBSBasicLogin::OBSBasicLogin(OBSBasic *parent,
			     const EvercastAuth::Credentials& creds,
                             const std::string& wantedRoomUrl)
	: QDialog(parent)
        , ui(new Ui::OBSBasicLogin)
{

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        ui->setupUi(this);

        ui->userEmail->setText(QT_UTF8(creds.email.c_str()));
        ui->userPassword->setText(QT_UTF8(creds.password.c_str()));
	ui->roomURL->setText(QT_UTF8(wantedRoomUrl.c_str()));

}

void OBSBasicLogin::on_buttonBox_accepted() {
        credentials.email = QT_TO_UTF8(ui->userEmail->text());
        credentials.password = QT_TO_UTF8(ui->userPassword->text());
	credentials.trackingId = "de66eff5-1896-4d11-9097-4d6fed6d9f0a";
        roomUrl = QT_TO_UTF8(ui->roomURL->text());
	accepted = true;
        done(DialogCode::Accepted);
}

void OBSBasicLogin::on_buttonBox_rejected() {
        done(DialogCode::Rejected);
}