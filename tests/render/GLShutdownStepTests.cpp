#include "Precomp.h"
#include "TestRunner.h"
#include "RenderDevice/OpenGL/GLShutdownStep.h"
#include <stdexcept>
#include <string>
#include <vector>

// These cover the guard around each step of the OpenGL device's shutdown. Running the guard
// against a real device needs a live GL context and is checked by running maps.

namespace
{
	struct NotAnException
	{
	};
}

TEST(RenderShutdownStepLogsAndAbsorbsAThrow)
{
	std::vector<std::string> lines;
	bool ok = GLShutdownStep("UnmapVertices", [] { throw std::runtime_error("Could not unmap ScenePass.VertexBuffer"); }, [&](const std::string& line) { lines.push_back(line); });
	CHECK(!ok);
	CHECK_EQ(lines.size(), size_t(1));
	CHECK_EQ(lines[0], std::string("UnmapVertices: Could not unmap ScenePass.VertexBuffer"));
}

TEST(RenderShutdownStepSuccessLogsNothing)
{
	std::vector<std::string> lines;
	int runs = 0;
	bool ok = GLShutdownStep("Textures.reset", [&] { runs++; }, [&](const std::string& line) { lines.push_back(line); });
	CHECK(ok);
	CHECK_EQ(runs, 1);
	CHECK(lines.empty());
}

TEST(RenderShutdownStepLogsANonStdException)
{
	std::vector<std::string> lines;
	bool ok = GLShutdownStep("ReleaseScenePass", [] { throw NotAnException(); }, [&](const std::string& line) { lines.push_back(line); });
	CHECK(!ok);
	CHECK_EQ(lines.size(), size_t(1));
	CHECK_EQ(lines[0], std::string("ReleaseScenePass: unknown exception"));
}
