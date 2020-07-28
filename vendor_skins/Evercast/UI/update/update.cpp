#include "update-helpers.hpp"
#include "update-window.hpp"
#include "remote-text.hpp"
#include "qt-wrappers.hpp"
#include "update.hpp"
#include "obs-app.hpp"
#include "obsconfig.h"

#include <QMessageBox>

#include <string>
#include <mutex>

#include <util/util.hpp>
#include <jansson.h>

#include <time.h>
#include <strsafe.h>
#include <shellapi.h>

using namespace std;

/* ------------------------------------------------------------------------ */

#ifndef EBS_DEFAULT_UPDATE_URL
#define EBS_DEFAULT_UPDATE_URL "https://v2.evercast.us/api/graphql"
#endif

#ifndef EBS_DOWNLOAD_URL
#define EBS_DOWNLOAD_URL "https://www.evercast.us/ebs"
#endif

/* ------------------------------------------------------------------------ */

static json_t* ReadChildNode(const json_t* parent, const char* key)
{
	json_t *node = json_object_get(parent, key);
	if (!node) {
		throw strprintf("Could not read node %s in JSON.", key);
	}

	return node;
}

static void ParseEBSVersionResponse(string &ebsVersionResponse, bool *updatesAvailable, string& notes, string& updateVersion)
{
	json_error_t error;
	Json root(json_loads(ebsVersionResponse.c_str(), 0, &error));

	if (!root)
		throw strprintf("Failed to read EBS version response (%d): %s",
			error.line, error.text);

	if (!json_is_object(root.get()))
		throw string("Response received was not an object.");

	json_t *node = ReadChildNode(root, "data");
	node = ReadChildNode(node, "getEBSUpgradeInfo");

	if (json_is_null(node)) {

		*updatesAvailable = false;
		return;
	}

	json_t *upgrade_available_node = ReadChildNode(node, "upgradeAvailable");
	*updatesAvailable = json_boolean_value(upgrade_available_node);

	json_t *version_node = ReadChildNode(node, "version");
	updateVersion = json_string_value(version_node);

	json_t *note_node = ReadChildNode(node, "releaseNotes");
	notes = json_string_value(note_node);
}

/* ------------------------------------------------------------------------ */

void AutoUpdateThread::infoMsg(const QString& title, const QString& text)
{
	OBSMessageBox::information(App()->GetMainWindow(), title, text);
}

void AutoUpdateThread::info(const QString& title, const QString& text)
{
	QMetaObject::invokeMethod(this, "infoMsg", Qt::BlockingQueuedConnection,
		Q_ARG(QString, title), Q_ARG(QString, text));
}

int AutoUpdateThread::queryUpdateSlot(bool localManualUpdate,
	const QString& text)
{
	OBSUpdate updateDlg(App()->GetMainWindow(), localManualUpdate, text);
	return updateDlg.exec();
}

int AutoUpdateThread::queryUpdate(bool localManualUpdate, const char* text_utf8)
{
	int ret = OBSUpdate::No;
	QString text = text_utf8;
	QMetaObject::invokeMethod(this, "queryUpdateSlot",
		Qt::BlockingQueuedConnection,
		Q_RETURN_ARG(int, ret),
		Q_ARG(bool, localManualUpdate),
		Q_ARG(QString, text));
	return ret;
}

bool AutoUpdateThread::EBSVersionQuery(bool manualUpdate, std::string &str, std::string &error, long *responseCode, bool *updatesAvailable, string &version, string &notes)
		     
{
	string signature;
	vector<string> extraHeaders;
	const char *query = "{\"query\": \"{  getEBSUpgradeInfo(currentVersion: \\\"" EBS_VERSION "\\\") {    upgradeAvailable    version    releaseNotes  } } \", \"variables\": null}";

	config_set_default_string(GetGlobalConfig(), "General", "EBSUpdateUrl", EBS_DEFAULT_UPDATE_URL);
	string updateUrl = config_get_string(GetGlobalConfig(), "General", "EBSUpdateUrl");

	bool success = GetRemoteFile(updateUrl.c_str(), str, error, responseCode,
				     "application/json", query, extraHeaders,
				     &signature);

	if (!success || (*responseCode != 200 && *responseCode != 304)) {
		if (*responseCode == 404)
			return false;

		throw strprintf("Failed to fetch download page content: %s",
			error.c_str());
	}

	/* ----------------------------------- *
	 * check page for update           */
	ParseEBSVersionResponse(str, updatesAvailable, notes, version);

	if (!(*updatesAvailable)) {
		if (manualUpdate)
			info(QTStr("Updater.NoUpdatesAvailable.Title"),
				QTStr("Updater.NoUpdatesAvailable.Text"));
		return false;
	}

	/* ----------------------------------- *
	 * skip this version if set to skip    */

	string skipUpdateVer = config_get_string(GetGlobalConfig(), "General", "SkipUpdateVersion");
	if (!manualUpdate && version == skipUpdateVer)
		return false;

	return success;
}

void AutoUpdateThread::run()
try {
	long responseCode;
	string text;
	string error;
	bool updatesAvailable = false;
	bool success;

	struct FinishedTrigger {
		inline ~FinishedTrigger()
		{
			QMetaObject::invokeMethod(App()->GetMainWindow(),
				"updateCheckFinished");
		}
	} finishedTrigger;

	auto VideoActive = [this]() {
		if (obs_video_active()) {
			if (manualUpdate)
				info(QTStr("Updater.Running.Title"),
					QTStr("Updater.Running.Text"));
			return true;
		}

		return false;
	};

	/* ----------------------------------- *
	 * warn if running or gc locked        */

	if (VideoActive())
		return;

	/* ----------------------------------- *
	 * get update info from server            */

	string version, notes;
	success = EBSVersionQuery(manualUpdate, text, error, &responseCode, &updatesAvailable, version, notes);
	if (!success)
		return;

	if (!updatesAvailable)
		return;

	/* ----------------------------------- *
	 * warn again if running or gc locked  */

	if (VideoActive())
		return;

	/* ----------------------------------- *
	 * query user for update               */

	int queryResult = queryUpdate(manualUpdate, notes.c_str());

	if (queryResult == OBSUpdate::No) {
		if (!manualUpdate) {
			long long t = (long long)time(nullptr);
			config_set_int(GetGlobalConfig(), "General",
				"LastUpdateCheck", t);
		}
		return;
	}
	else if (queryResult == OBSUpdate::Skip) {
		config_set_string(GetGlobalConfig(), "General", "SkipUpdateVersion", version.c_str());
		return;
	}

	system("start " EBS_DOWNLOAD_URL);

	/* force OBS to perform another update check immediately after updating
	* in case of issues with the new version */
	config_set_int(GetGlobalConfig(), "General", "LastUpdateCheck", 0);
	config_set_string(GetGlobalConfig(), "General", "SkipUpdateVersion", "");

	QMetaObject::invokeMethod(App()->GetMainWindow(), "close");
}
catch (string text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
}

/* ------------------------------------------------------------------------ */

