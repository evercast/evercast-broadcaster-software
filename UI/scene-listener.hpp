#pragma once

#include "window-basic-main.hpp"
#include "obs-frontend-api/obs-frontend-api.h"

class OBSBasic;

class SceneListener {
public:
	SceneListener(OBSBasic *parent);
	~SceneListener();

	static void frontendEventHandler(enum obs_frontend_event event, void *ptr);

private:
	OBSBasic *_parent;
};

