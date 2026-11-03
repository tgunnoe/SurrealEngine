#pragma once

#include "Engine.h"
#include "VisibleNode.h"
#include "VisibleTranslucent.h"
#include "VisibleCorona.h"
#include "VisibleActor.h"
#include "VisiblePortal.h"
#include "BspClipper.h"
#include "Math/bbox.h"
#include "RenderDevice/RenderDevice.h"

// How far toward the viewer a mirror frame's clip plane is pushed, so that the surface the
// frame was made from is clipped away together with everything behind it.
constexpr float MirrorClipBias = 1.0f;

// Two mirror planes are the same plane when their normals agree and their offsets are this close.
constexpr float MirrorPlaneNormalTolerance = 1e-5f;
constexpr float MirrorPlaneOffsetTolerance = 0.5f;

// The clip plane (signed distance positive on the viewer's side) for the frame that draws the
// mirror surface with this normal and point, seen from the viewer.
inline vec4 MirrorClipPlane(const vec3& normal, const vec3& pointOnPlane, const vec3& viewer)
{
	vec3 n = normalize(normal);
	vec4 plane(n, -dot(n, pointOnPlane));
	if (dot(plane, vec4(viewer, 1.0f)) < 0.0f)
		plane = -plane;
	plane.w -= MirrorClipBias;
	return plane;
}

// Whether a world-space box lies wholly on the negative side of a mirror frame's clip plane.
inline bool MirrorPlaneCullsBox(const BBox& box, const vec4& plane)
{
	// The corner furthest along the plane's normal is the last one to leave the negative side.
	vec3 corner(plane.x >= 0.0f ? box.max.x : box.min.x, plane.y >= 0.0f ? box.max.y : box.min.y, plane.z >= 0.0f ? box.max.z : box.min.z);
	return dot(plane, vec4(corner, 1.0f)) < 0.0f;
}

// The plane (normal, offset) of a mirror surface, as stored in a mirror portal.
inline vec4 MirrorSurfacePlane(const vec3& normal, const vec3& pointOnPlane)
{
	vec3 n = normalize(normal);
	return vec4(n, -dot(n, pointOnPlane));
}

// The index of the held mirror portal that a new mirror surface of this plane joins, or -1.
inline int FindMirrorPortalToJoin(const Array<VisiblePortal>& portals, const vec4& plane)
{
	for (size_t i = 0; i < portals.size(); i++)
	{
		const VisiblePortal& portal = portals[i];
		if (portal.IsMirror &&
			dot(portal.MirrorPlane.xyz(), plane.xyz()) >= 1.0f - MirrorPlaneNormalTolerance &&
			std::fabs(portal.MirrorPlane.w - plane.w) <= MirrorPlaneOffsetTolerance)
		{
			return (int)i;
		}
	}
	return -1;
}

class VisibleFrame
{
public:
	void Process(const vec3& location, const mat4& worldToView, const Coords& viewRotation, bool mirrorFlag = false, int portalDepth = 0, const Array<PortalSpan>& portalSpans = {}, const vec4& portalPlane = vec4(0.0f, 0.0f, 0.0f, 1.0f), bool isMirrorFrame = false);
	void Draw();
	void DrawCoronas();

	RenderDevice* Device = nullptr;

	SceneNode Frame;
	BspClipper Clipper;
	vec4 ViewLocation = vec4(0.0f);
	Coords ViewRotation = {};
	int ViewZone = 0;
	//uint64_t ViewZoneMask = 0;
	int FrameCounter = 0;
	bool MirrorFlag = false;
	int PortalDepth = 0;
	bool IsMirrorFrame = false;
	vec4 ClipPlane = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	Array<VisibleNode> OpaqueNodes;
	Array<VisibleActor> Actors;
	Array<VisibleTranslucent> Translucents;
	Array<VisibleCorona> Coronas;
	Array<VisiblePortal> Portals;

private:
	void SetupSceneFrame(const mat4& worldToView);
	void ProcessNode(BspNode* node);
	void ProcessNodeSurface(BspNode* node, bool front);
	void SortTranslucent();

	void DrawOpaqueNodes();
	void DrawOpaqueActors();
	void DrawTranslucent();
	void DrawPortals();

	int FindZoneAt(const vec3& location);
	int FindZoneAt(const vec4& location, BspNode* node, BspNode* nodes);

	vec3 WarpLocationToOtherSide(UWarpZoneInfo* warpZone, vec3 p);
	vec3 WarpNormalToOtherSide(UWarpZoneInfo* warpZone, vec3 n);
	Coords WarpRotationToOtherSide(UWarpZoneInfo* warpZone, Coords rotation);
};
