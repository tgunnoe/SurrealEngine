#include "Precomp.h"
#include "TestRunner.h"
#include "Render/VisibleMesh.h"
#include <random>

// A mesh's animation sequence can declare more frames than the mesh holds. These tests cover the
// vertex offsets a sequence is drawn from.

namespace
{
	// Both offsets must leave all frameVerts vertices of the one frame the mesh holds addressable.
	void CheckOffsetsInsideOneFrame(int numFrames, int frameVerts)
	{
		for (float animFrame : { 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.999f })
		{
			MeshFrameOffsets o = ComputeMeshFrameOffsets(animFrame * numFrames, 0, numFrames, frameVerts, 1);
			CHECK(o.V0 >= 0 && o.V0 + (frameVerts - 1) < frameVerts);
			CHECK(o.V1 >= 0 && o.V1 + (frameVerts - 1) < frameVerts);
		}
	}
}

TEST(MeshFrameOffsetsInsideASMDPick)
{
	CheckOffsetsInsideOneFrame(6, 48);
}

TEST(MeshFrameOffsetsInsideSeed)
{
	CheckOffsetsInsideOneFrame(2, 62);
}

TEST(MeshFrameOffsetsInRangeAreUnchanged)
{
	std::mt19937 rng(777);
	for (int i = 0; i < 2000; i++)
	{
		int start = (int)(rng() % 40);
		int num = 1 + (int)(rng() % 40);
		int frameVerts = 1 + (int)(rng() % 200);
		int held = start + num + (int)(rng() % 5);
		float animFrame = (float)(rng() % 100000) / 100000.0f;

		float scaled = animFrame * num;
		MeshFrameOffsets o = ComputeMeshFrameOffsets(scaled, start, num, frameVerts, held);

		int f0 = (int)scaled;
		int f1 = f0 + 1;
		f0 %= num;
		f1 %= num;
		CHECK(o.V0 == (start + f0) * frameVerts);
		CHECK(o.V1 == (start + f1) * frameVerts);
		CHECK(o.T0 == scaled - (float)f0);
	}
}
