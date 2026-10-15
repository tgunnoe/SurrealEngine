#pragma once

#include <string>
#include <list>
#include <functional>
#include <chrono>

void LogMessage(const std::string& message);
void LogUnimplemented(const std::string& message);

struct LogMessageLine
{
	float Time = 0.0f;
	std::string Source;
	std::string Text;
};

// Structured, line-oriented event log for unattended runs. Every line is
// written and flushed immediately so the file survives a crash.
class SmokeLog
{
public:
	static SmokeLog* Get();

	bool Open(const std::string& path);
	bool IsOpen() const { return fd >= 0; }

	void Event(const std::string& kind, const std::string& text = {});
	// Classifies engine log messages (script errors, unimplemented stubs) into events.
	void OnLogMessage(const std::string& message);
	void ScriptError(const std::string& text);
	void ScriptWarning(const std::string& text);
	void Stub(const std::string& text);
	void MissingNative(const std::string& text);
	void Exception(const std::string& text);
	void Fps(double fps);

	// Called once per rendered frame: samples fps about once a second and
	// returns true once exitAfterSeconds (negative = never) has elapsed.
	bool OnFrame(double exitAfterSeconds);
	void Fatal(const std::string& text);
	void Summary();

	// Reports whether a fatal line or summary has already ended the log.
	bool IsFinished() const { return finished; }

	void InstallSignalHandlers();

	// Measured from the first Get() call, which GameApp::main makes at process start.
	double SecondsSinceStart() const { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); }

private:
	static std::string Flatten(const std::string& text);
	void WriteLine(const std::string& line);

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	int fd = -1;
	bool finished = false;
	int scriptErrors = 0;
	int stubHits = 0;
	int missingNatives = 0;
	int exceptions = 0;
	int scriptWarnings = 0;
	double fpsSum = 0.0;
	int fpsSamples = 0;
	double fpsWindowStart = -1.0;
	int fpsWindowFrames = 0;
};

class Logger
{
public:
	static Logger* Get();

	void LogMessage(const std::string& message);
	void LogUnimplemented(const std::string& message);
	void SetTimeSeconds(float value) { time = value; }
	void SetCallback(std::function<void(const LogMessageLine& line)> cb) { printLogDebugger = std::move(cb); }
	const std::list<LogMessageLine>& GetLog() const { return Log; }

	void SaveLog(const std::string& filename);
	void SaveLogAsPlaintext(const std::string& filename) const;
	static std::list<LogMessageLine> LoadLog(const std::string& filename);

private:
	float time = 0.0f;
	std::list<LogMessageLine> Log;
	std::function<void(const LogMessageLine& line)> printLogDebugger;
	static std::vector<std::string> SplitNewlines(const std::string& str);

	Logger() = default;
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
};
