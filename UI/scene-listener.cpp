/*
 * Copyright (c) 2022 Evercast, LLC
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string>
#include <sstream>

#include "obs.h"
#include "scene-listener.hpp"
#include <util/stream-config.h>

SceneListener::SceneListener(OBSBasic *parent)
{
	this->_parent = parent;
	obs_frontend_add_event_callback(frontendEventHandler, this);
}

SceneListener::~SceneListener()
{
	obs_frontend_remove_event_callback(frontendEventHandler, this);
}

static std::string mapSourceIdToName(std::string &sourceId) {
	if ("av_capture_input" == sourceId || "dshow_input" == sourceId) {
		return "Device stream";
	} else if ("ffmpeg_source" == sourceId) {
		return "File stream";
	} else if ("ndi_source" == sourceId) {
		return "NDI";
	} else if ("decklink-input" == sourceId) {
		return "DeckLink";
	} else if ("display_capture" == sourceId ||
		   "monitor_capture" == sourceId) {
		// NOTE: Deliberately leaving out window capture and Mac General Capture
		// in order to ensure problems that arise can be traced to a specific plugin.
		// This is not a problem for the above two because they are specific to the 
		// OS on which they are used.
		return "Screenshare";
	}

	return obs_source_get_display_name(sourceId.c_str());
}

static bool AppendName(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
	UNUSED_PARAMETER(scene);

	std::stringstream *names = (std::stringstream *)param;
	if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, AppendName, param);
	} else {
		if (0 != names->str().length()) {
			(*names) << ";";
		}
		obs_source_t *source = obs_sceneitem_get_source(item);
		std::string sourceId = obs_source_get_id(source);
		std::string sourceDefaultName = mapSourceIdToName(sourceId);
		(*names) << sourceId;
	}

	return true;
}

void SceneListener::frontendEventHandler(enum obs_frontend_event event,
					 void *ptr)
{
	SceneListener *listener = (SceneListener *)ptr;

	if (nullptr == listener) {
		blog(LOG_WARNING, "Null pointer provided to SceneListener event handler");
		return;
	}

	OBSBasic *parent = listener->_parent;

	if (nullptr == parent) {
		blog(LOG_WARNING, "SceneListener was improperly initialized");
		return;
	}

	if (OBS_FRONTEND_EVENT_SCENE_CHANGED == event || OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED == event || OBS_FRONTEND_EVENT_SCENE_ITEM_LIST_CHANGED == event) {
		std::stringstream names;
		OBSScene scene = parent->GetCurrentScene();

		if (nullptr == scene) {
			blog(LOG_WARNING, "SceneListener: OBS Scene was improperly initialized");
			return;
		}

		obs_scene_enum_items(scene, AppendName, &names);

		std::string configType = names.str();
		stream_config_type(configType.c_str(), configType.size());
	}
}

