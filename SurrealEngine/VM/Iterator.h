#pragma once

#include "ExpressionValue.h"
#include "Collision/TopLevel/CollisionHit.h"

#include <vector>

class UZoneInfo;
class UActor;
class ULevel;
class ULevelInfo;
struct IntObject;

// The levels an iterator called on an actor of callerLevel walks, in walk order: the actor's own
// level, never the level being played. The engine ticks the entry level beside the played one, so
// its scripts iterate while the played level is the engine's current level, and an iterator that
// followed the current level would show them the played level's actors. entryLevel and playedLevel
// are the engine's two running levels, kept as parameters for a form that walks more than one.
//
// This rule is inferred, not read from the original engine: the stock scripts iterate the calling
// actor's own level (the replication info refills its player list with AllActors every 0.2 s, and
// that list must not hold another level's records). The choice is a function of these three
// handles alone, which keeps it checkable without an engine.
std::vector<ULevel*> IteratorLevels(ULevel* callerLevel, ULevel* entryLevel, ULevel* playedLevel);

// The levels for an iterator function called on self (the frame's actor for a bare foreach, the
// named actor for foreach Other.AllActors), read from the one place that knows the engine's levels.
// An actor in no level gets an empty list and a logged script error naming iterator.
std::vector<ULevel*> ActorLevels(UObject* self, const char* iterator);

class Iterator
{
public:
	virtual ~Iterator() = default;
	virtual bool Next() = 0;

	size_t StartStatementIndex = 0;
	size_t EndStatementIndex = 0;
};

// Added in Deus Ex and Unreal 227
class AllObjectsIterator : public Iterator
{
public:
	// InOuter is a 227-exclusive parameter
	AllObjectsIterator(UObject* BaseClass, UObject** ReturnValue, UObject* InOuter = nullptr);
	bool Next() override;

private:
	UObject* BaseClass = nullptr;
	UObject** ReturnValue = nullptr;
	UObject* InOuter = nullptr;
	GCObjectList m_Objects;
	GCObjectList::iterator m_Iterator;
};

// Added in Unreal 227.
class AllParticlesIterator : public Iterator
{
public:
	// TODO: Implement the iterator
	// Iterates through all alive particles. Note that these are NOT in level actors.
	AllParticlesIterator(UObject** ReturnValue);
	bool Next() override;

private:
	UObject** ReturnValue = nullptr;
};

class AllActorsIterator : public Iterator
{
public:
	AllActorsIterator(std::vector<ULevel*> Levels, UObject* BaseClass, UObject** ReturnValue, NameString MatchTag);
	// Unreal 227i version of AllActors
	AllActorsIterator(std::vector<ULevel*> Levels, UObject* BaseClass, UObject** ReturnValue, NameString MatchTag, NameString MatchEvent);
	// Unreal 227k version of AllActors
	// TODO: Handle bAllLevels parameter
	AllActorsIterator(std::vector<ULevel*> Levels, UObject* BaseClass, UObject** ReturnValue, NameString MatchTag, NameString MatchEvent, bool bAllLevels);
	bool Next() override;

private:
	std::vector<ULevel*> Levels;
	size_t levelIndex = 0;
	UObject* BaseClass = nullptr;
	UObject** ReturnValue = nullptr;
	NameString MatchTag = "";
	NameString MatchEvent = "";
	bool bAllLevels = false;
	size_t index = 0;
};

// Added in Unreal Gold 227
class AllFilesIterator : public Iterator
{
public:
	// Iterates through all files of a type (Extensions are the usual Unreal Package extensions, or all files, if the FileExtension string is empty)
	AllFilesIterator(const std::string& FileExtension, const std::string& FilePrefix, std::string& FileName);
	bool Next() override;

private:
	std::string FileExtension;
	std::string FilePrefix;
	std::string& FileName;
	Array<std::string> FoundFiles;
	Array<std::string>::iterator iterator;
};

class BasedActorsIterator : public Iterator
{
public:
	BasedActorsIterator(UActor* Caller, UObject* BaseClass, UObject** Actor);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	size_t index = 0;

	Array<UActor*> BasedActors;
	Array<UActor*>::iterator iterator;
};

// An Iterator for iterating through all child actors of a given actor (e.g. due to being spawned by them)
class ChildActorsIterator : public Iterator
{
public:
	ChildActorsIterator(UActor* Caller, UObject* BaseClass, UObject** Actor);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	size_t index = 0;

	Array<UActor*> ChildActors;
	Array<UActor*>::iterator iterator;
};

class CycleActorsIterator : public Iterator
{
public:
	CycleActorsIterator(std::vector<ULevel*> Levels, UObject* BaseClass, UObject** Actor, int* outIndex);
	bool Next() override;
private:
	std::vector<ULevel*> Levels;
	size_t levelIndex = 0;
	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	int* outIndex = nullptr;
	int currentIndex = 0;
	size_t actorsIndex = 0;
};

class IntDescIterator : public Iterator
{
public:
	IntDescIterator(std::string& className, std::string* entryName, std::string* desc, std::optional<bool> bSingleNames);
	bool Next() override;
private:
	std::string ClassName;
	std::string* EntryName;
	std::string* Desc;
	std::optional<bool> bSingleNames;

	Array<IntObject> IntObjects;
	Array<IntObject>::iterator it;
};

class RadiusActorsIterator : public Iterator
{
public:
	RadiusActorsIterator(const std::vector<ULevel*>& Levels, UObject* BaseClass, UObject** Actor, float Radius, vec3 Location);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	float Radius = 0.0f;
	vec3 Location;
	size_t index = 0;

	Array<UActor*> RadiusActors;
	Array<UActor*>::iterator iterator;
};

class TouchingActorsIterator : public Iterator
{
public:
	TouchingActorsIterator(UActor* Caller, UObject* BaseClass, UObject** outActor);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** outActor = nullptr;
	size_t index = 0;

	Array<UActor*> TouchingActors;
	Array<UActor*>::iterator iterator;
};

class TraceActorsIterator : public Iterator
{
public:
	TraceActorsIterator(UActor* SelfActor, UObject* BaseClass, UObject** Actor, vec3* HitLoc, vec3* HitNorm, const vec3& End, const vec3& Start, const vec3& Extent);
	bool Next() override;

	UActor* SelfActor = nullptr;
	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	vec3* HitLoc = nullptr;
	vec3* HitNorm = nullptr;
	vec3 End = vec3(0.0f);
	vec3 Start = vec3(0.0f);
	vec3 Extent = vec3(0.0f);
	size_t index = 0;

	struct TraceInfo
	{
		UActor* tracedActor;
		vec3 HitLoc;
		vec3 HitNorm;
	};

	Array<TraceInfo> tracedActors;
	Array<TraceInfo>::iterator iterator;
};

class VisibleActorsIterator : public Iterator
{
public:
	VisibleActorsIterator(const std::vector<ULevel*>& Levels, UActor* Caller, UObject* BaseClass, UObject** Actor, float Radius, const vec3& Location);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	float Radius = 0.0f;
	vec3 Location = vec3(0.0f);
	size_t index = 0;

	Array<UActor*> VisibleActors;
	Array<UActor*>::iterator iterator;
};

class VisibleCollidingActorsIterator : public Iterator
{
public:
	VisibleCollidingActorsIterator(const std::vector<ULevel*>& Levels, UObject* BaseClass, UObject** ReturnValue, float Radius, const vec3& Location, bool IgnoreHidden);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** ReturnValue = nullptr;
	float Radius = 0.0f;
	vec3 Location = vec3(0.0f);
	bool IgnoreHidden = false;
	size_t index = 0;
	Array<UActor*> HitActors;
};

// Iterator for iterating through all actors in a given Zone
class ZoneActorsIterator : public Iterator
{
public:
	ZoneActorsIterator(const std::vector<ULevel*>& Levels, UZoneInfo* zone, UObject* BaseClass, UObject** Actor);
	bool Next() override;

	UZoneInfo* Zone = nullptr;
	UObject* BaseClass = nullptr;
	UObject** Actor = nullptr;
	size_t index = 0;

	Array<UActor*> ZoneActors;
	Array<UActor*>::iterator iterator;
};

// Added in Deus Ex
class TraceTextureIterator : public Iterator
{
public:
	TraceTextureIterator(const std::vector<ULevel*>& Levels, ULevelInfo* LevelInfo, UObject* BaseClass, UObject** OutActor, NameString* TexName, NameString* TexGroup, int* Flags, vec3* HitLoc, vec3* HitNorm, const vec3& End, const vec3& Start, const vec3& Extent);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** OutActor = nullptr;
	NameString* TexName = nullptr;
	NameString* TexGroup = nullptr;
	int* flags = nullptr;
	vec3* HitLoc = nullptr;
	vec3* HitNorm = nullptr;
	vec3 End = vec3(0, 0, 0);
	vec3 Start = vec3(0, 0, 0);
	vec3 Extent = vec3(0, 0, 0);

private:
	ULevel* Level = nullptr;
	ULevelInfo* LevelInfo = nullptr;
	CollisionHitList m_CollList;
	CollisionHitList::const_iterator m_Iterator;
};

// Added in Deus Ex
class TraceVisibleActorsIterator : public Iterator
{
public:
	TraceVisibleActorsIterator(const std::vector<ULevel*>& Levels, UObject* BaseClass, UObject** OutActor, vec3* HitLoc, vec3* HitNorm, const vec3& End, const vec3& Start, const vec3& Extent);
	bool Next() override;

	UObject* BaseClass = nullptr;
	UObject** OutActor = nullptr;
	vec3* HitLoc = nullptr;
	vec3* HitNorm = nullptr;
	vec3 End = vec3(0, 0, 0);
	vec3 Start = vec3(0, 0, 0);
	vec3 Extent = vec3(0, 0, 0);

private:
	CollisionHitList m_CollList;
	CollisionHitList::const_iterator m_Iterator;
};