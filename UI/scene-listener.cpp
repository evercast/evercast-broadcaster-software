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
		const char *sourceId = obs_source_get_id(source);
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

