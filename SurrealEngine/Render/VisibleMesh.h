#pragma once

#include "Math/mat.h"

class UActor;
class UMesh;
class USkeletalMesh;
struct MeshFace;
class ULodMesh;
class VisibleFrame;

// The vertex offsets of the two frames a sequence plays between, and the blend between them.
// scaledFrame is the animation frame already multiplied by the sequence's frame count.
struct MeshFrameOffsets
{
	int V0;
	int V1;
	float T0;
};

inline MeshFrameOffsets ComputeMeshFrameOffsets(float scaledFrame, int startFrame, int numFrames, int frameVerts, int heldFrames)
{
	int frame0 = (int)scaledFrame;
	int frame1 = frame0 + 1;
	frame0 = frame0 % numFrames;
	frame1 = frame1 % numFrames;
	return { (startFrame + frame0) * frameVerts, (startFrame + frame1) * frameVerts, scaledFrame - (float)frame0 };
}

struct BlendInfo
{
	int offsets[3];
	float t0, t1;
	float weight;
};

class VisibleMesh
{
public:
	bool DrawMesh(VisibleFrame* frame, UActor* actor, bool wireframe, bool translucentPass);

private:
	void DrawDebugInfo(VisibleFrame* frame, UActor* actor);
	bool DrawMeshAtLocation(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, UMesh* mesh, const mat4& meshToWorld, const mat3& meshNormalToWorld, bool translucentPass);
	bool DrawMesh(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, UMesh* mesh, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, bool translucentPass);
	bool DrawMeshDX(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, UMesh* mesh, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, bool translucentPass);
	bool DrawLodMesh(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, ULodMesh* mesh, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, bool translucentPass);
	bool DrawLodMeshDX(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, ULodMesh* mesh, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, bool translucentPass);
	bool DrawLodMeshFace(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, ULodMesh* mesh, const Array<MeshFace>& faces, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, int baseVertexOffset, const int* vertexOffsets, float t0, float t1, bool translucentPass);
	bool DrawLodMeshFaceDX(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, ULodMesh* mesh, const Array<MeshFace>& faces, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, int baseVertexOffset, const int* vertexOffsets, float t0, float t1, bool translucentPass, BlendInfo* blends, int blendCount);
	bool DrawSkeletalMesh(VisibleFrame* frame, UActor* actor, UActor* lightLocationActor, USkeletalMesh* mesh, const mat4& ObjectToWorld, const mat3& ObjectNormalToWorld, bool translucentPass);
	void SetupMeshTextures(UActor* actor, UMesh* mesh);
	void SetupLodMeshTextures(UActor* actor, ULodMesh* mesh);
	void FindAttachmentPoints(ULodMesh* mesh, const mat4& ObjectToWorld, const int* vertexOffsets, float t0, float t1);

	Array<vec3> attachmentTris;
};
