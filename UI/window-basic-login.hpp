#pragma once

#include <obs.hpp>
#include <memory>

#include "ui_OBSBasicLogin.h"

#include "auth-evercast.hpp"

class OBSBasic;

class OBSBasicLogin : public QDialog {
        Q_OBJECT

private:
        std::unique_ptr<Ui::OBSBasicLogin> ui;

private slots:

	void on_buttonBox_accepted();
        void on_buttonBox_rejected();

public:
        OBSBasicLogin(OBSBasic *parent, const EvercastAuth::Credentials& creds);

        EvercastAuth::Credentials credentials;
	bool accepted = false;

};
