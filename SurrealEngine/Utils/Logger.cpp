
#include "Precomp.h"
#include "Utils/Logger.h"
#include "Utils/JsonValue.h"
#include "Utils/File.h"
#include "VM/Frame.h"
#include <unordered_set>
#include <cstdio>
#include <cstring>
#include <csignal>
#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

void LogMessage(const std::string& message)
{
	Logger::Get()->LogMessage(message);
}

void LogUnimplemented(const std::string& message)
{
	Logger::Get()->LogUnimplemented(message);
}

void Logger::LogMessage(const std::string& message)
{
	if (SmokeLog::Get()->IsOpen())
	{
		if (message.rfind("Script error: ", 0) == 0)
			SmokeLog::Get()->ScriptError(message.substr(14));
		else if (message.rfind("Unimplemented: ", 0) == 0)
			SmokeLog::Get()->Stub(message.substr(15));
	}

	if (!Frame::Callstack.empty() && Frame::Callstack.back()->Func)
	{
		std::string name;

		if (Frame::Callstack.size() > 1)
		{
			UStruct* func = Frame::Callstack[Frame::Callstack.size() - 2]->Func;
			for (UStruct* s = func; s != nullptr; s = s->StructParent)
			{
				if (name.empty())
					name = s->Name.ToString();
				else
					name = s->Name.ToString() + "." + name;
			}
		}
		else
		{
			name = UObject::GetUClassFullName(Frame::Callstack.front()->Object).ToString();
		}

		for (const std::string& text : SplitNewlines(message))
		{
			LogMessageLine line;
			line.Time = time;
			line.Source = name;
			line.Text = text;
			Log.push_back(std::move(line));
			if (printLogDebugger)
				printLogDebugger(Log.back());
		}
	}
	else
	{
		for (const std::string& text : SplitNewlines(message))
		{
			LogMessageLine line;
			line.Time = time;
			line.Text = text;
			Log.push_back(std::move(line));
			if (printLogDebugger)
				printLogDebugger(Log.back());
		}
	}
}

void Logger::LogUnimplemented(const std::string& message)
{
	static std::unordered_set<std::string> seenMessages;
	if (seenMessages.insert(message).second)
	{
		LogMessage("Unimplemented: " + message);
	}
}

std::vector<std::string> Logger::SplitNewlines(const std::string& str)
{
	std::vector<std::string> lines;
	size_t pos = 0;
	for (size_t i = 0, count = str.size(); i < count; i++)
	{
		if (str[i] == '\n')
		{
			lines.push_back(str.substr(pos, i - pos));
			if (!lines.back().empty() && lines.back().back() == '\r')
				lines.back().pop_back();
			pos = i + 1;
		}
	}
	lines.push_back(str.substr(pos));
	if (!lines.back().empty() && lines.back().back() == '\r')
		lines.back().pop_back();
	return lines;
}

Logger* Logger::Get()
{
	static Logger logger;
	return &logger;
}

void Logger::SaveLog(const std::string& filename)
{
	// We want this formatted in a specific way so its reasonably readable both by humans and still by a json parser.
	std::string json = "[";
	bool firstLine = true;
	for (const LogMessageLine& line : Log)
	{
		if (!firstLine)
			json += ',';
		firstLine = false;
		json += '\n';
		JsonValue logMessage = JsonValue::array();
		logMessage.items().push_back(JsonValue::number(line.Time));
		logMessage.items().push_back(JsonValue::string(line.Source));
		logMessage.items().push_back(JsonValue::string(line.Text));
		json += logMessage.to_json(false);
	}
	json += "\n]\n";
	File::write_all_text(filename, json);
}

void Logger::SaveLogAsPlaintext(const std::string& filename) const
{
	std::string finalLog;

	for (const LogMessageLine& line : Log)
	{
		const auto source = line.Source.empty() ? "<Surreal Engine>" : line.Source;
		finalLog += "[" + std::to_string(line.Time) + "] " + source + ": " + line.Text + "\n";
	}

	File::write_all_text(filename, finalLog);
}


std::list<LogMessageLine> Logger::LoadLog(const std::string& filename)
{
	std::list<LogMessageLine> log;
	JsonValue json = JsonValue::parse(File::read_all_text(filename));
	for (const JsonValue& logMessage : json.items())
	{
		LogMessageLine line;
		line.Time = logMessage.at(0).to_float();
		line.Source = logMessage.at(1).to_string();
		line.Text = logMessage.at(2).to_string();
		log.push_back(std::move(line));
	}
	return log;
}


#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

SmokeLog* SmokeLog::Get()
{
	static SmokeLog log;
	return &log;
}

bool SmokeLog::Open(const std::string& path)
{
#ifdef WIN32
	fd = _open(path.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, 0644);
#else
	fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
#endif
	return fd >= 0;
}

std::string SmokeLog::Flatten(const std::string& text)
{
	std::string out = text;
	for (char& c : out)
	{
		if (c == '\n' || c == '\r')
			c = ' ';
	}
	return out;
}

void SmokeLog::WriteLine(const std::string& line)
{
	if (fd < 0 || finished)
		return;
	std::string data = line + "\n";
	const char* p = data.data();
	size_t left = data.size();
	while (left > 0)
	{
#ifdef WIN32
		int n = _write(fd, p, (unsigned int)left);
#else
		ssize_t n = write(fd, p, left);
#endif
		if (n <= 0)
			break;
		p += n;
		left -= (size_t)n;
	}
}

void SmokeLog::Event(const std::string& kind, const std::string& text)
{
	WriteLine(text.empty() ? kind : kind + " " + Flatten(text));
}

void SmokeLog::ScriptError(const std::string& text)
{
	scriptErrors++;
	Event("SCRIPT_ERROR", text);
}

void SmokeLog::Stub(const std::string& text)
{
	stubHits++;
	Event("STUB", text);
}

void SmokeLog::MissingNative(const std::string& text)
{
	missingNatives++;
	Event("MISSING_NATIVE", text);
}

void SmokeLog::Exception(const std::string& text)
{
	exceptions++;
	Event("EXCEPTION", text);
}

void SmokeLog::Fps(double fps)
{
	fpsSum += fps;
	fpsSamples++;
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%.1f", fps);
	Event("FPS", buf);
}

void SmokeLog::Fatal(const std::string& text)
{
	Event("FATAL", text);
	finished = true;
}

void SmokeLog::Summary()
{
	char buf[160];
	std::snprintf(buf, sizeof(buf), "script_errors=%d stub_hits=%d missing_natives=%d exceptions=%d fps_avg=%.1f",
		scriptErrors, stubHits, missingNatives, exceptions, fpsSamples > 0 ? fpsSum / fpsSamples : 0.0);
	Event("SUMMARY", buf);
	finished = true;
}

#ifndef WIN32
namespace
{
	int fatalFd = -1;

	// Only async-signal-safe calls: the process state is untrustworthy here.
	void SmokeFatalSignal(int sig)
	{
		char buf[32] = "FATAL signal ";
		size_t len = std::strlen(buf);
		char digits[12];
		int n = 0;
		for (int v = sig; v > 0; v /= 10)
			digits[n++] = (char)('0' + v % 10);
		while (n > 0)
			buf[len++] = digits[--n];
		buf[len++] = '\n';
		ssize_t r = write(fatalFd, buf, len);
		(void)r;
		_exit(128 + sig);
	}
}
#endif

void SmokeLog::InstallSignalHandlers()
{
#ifndef WIN32
	fatalFd = fd;

	// The handler runs on its own stack so a stack overflow can still be reported.
	static char altstack[64 * 1024];
	stack_t ss = {};
	ss.ss_sp = altstack;
	ss.ss_size = sizeof(altstack);
	sigaltstack(&ss, nullptr);

	struct sigaction sa = {};
	sa.sa_handler = SmokeFatalSignal;
	sa.sa_flags = SA_ONSTACK | SA_NODEFER;
	sigemptyset(&sa.sa_mask);
	for (int sig : { SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL })
		sigaction(sig, &sa, nullptr);
#endif
}
