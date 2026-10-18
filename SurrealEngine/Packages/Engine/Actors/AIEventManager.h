#pragma once

#include "Package/NameString.h"
#include <map>
#include <vector>

class UActor;

// What an actor asked to hear about through AISetEventCallback: the script function to call when the
// event fires, an optional function that scores a source, and which sight checks apply to it.
struct AIEventRegistration
{
	NameString Event;
	NameString Callback;
	NameString ScoreCallback;
	bool bCheckVisibility = false;
	bool bCheckDir = false;
	bool bCheckCylinder = false;
	bool bCheckLOS = false;
};

// Deus Ex routes AI events (noise, distress, carcasses and so on) through one manager per level.
// LevelInfo.InitEventManager creates it empty when the level starts; the event natives fill it.
class AIEventManager
{
public:
	std::map<UActor*, std::vector<AIEventRegistration>> Registrations;
};
