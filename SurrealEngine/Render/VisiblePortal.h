#pragma once

#include "VisibleNode.h"
#include "BspClipper.h"

class BspNode;
class USkyZoneInfo;
class UWarpZoneInfo;

class VisiblePortal
{
public:
	Array<VisibleNode> Nodes;
	Array<PortalSpan> Spans;
	USkyZoneInfo* SkyZone = nullptr;
	UWarpZoneInfo* WarpZone = nullptr;

	// The plane (normal, offset) a mirror portal reflects about; meaningful when IsMirror is set.
	bool IsMirror = false;
	vec4 MirrorPlane = vec4(0.0f, 0.0f, 0.0f, 1.0f);
};
