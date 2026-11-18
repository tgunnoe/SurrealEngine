#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UStruct.h"
#include "Packages/Core/UObject.h"
#include "Packages/Core/Properties/UObjectProperty.h"
#include "Packages/Core/Properties/UStructProperty.h"
#include "Packages/Core/Properties/UArrayProperty.h"
#include "Packages/Core/Properties/UMapProperty.h"
#include "Packages/Core/Properties/UStrProperty.h"
#include "Packages/Engine/Resources/Level/UModel.h"
#include "Packages/Engine/Actors/Info/UZoneInfo.h"
#include <set>
#if defined(__GLIBC__)
#include <malloc.h>
#endif

// These call the collector and read its statistics and live list directly. Each test runs in its
// own process, so a collection sweeps only what the test itself allocated; what a test wants to
// keep it holds through a root, and it asks whether an object survived by looking for its address
// in the live list. A swept object is never read.
//
// A Package cannot be built without a PackageManager, so the package holds are not covered here;
// they sit on the same GCRoot that GcRootKeepsAndReleases pins.

namespace
{
	bool IsLive(GCObject* obj)
	{
		for (GCObject* live : GC::GetObjects())
		{
			if (live == obj)
				return true;
		}
		return false;
	}

	size_t CountLive()
	{
		size_t count = 0;
		for (GCObject* live : GC::GetObjects())
		{
			(void)live;
			count++;
		}
		return count;
	}

	// Builds the classes, properties and instances the tests collect. The classes and their
	// properties are held by a root here, which is simply no longer the only way to keep them: a
	// held struct marks its property list and its base. Instances are not held.
	class Fixture
	{
	public:
		template<typename T>
		T* Hold(T* obj)
		{
			roots.emplace_back(static_cast<GCObject*>(obj));
			return obj;
		}

		UClass* NewClass(const char* name, std::initializer_list<UProperty*> properties, size_t size, size_t alignment)
		{
			UClass* cls = Hold(GC::Alloc<UClass>(name, nullptr, ObjectFlags::NoFlags));
			for (UProperty* prop : properties)
				cls->Properties.push_back(Hold(prop));
			cls->StructSize = size;
			cls->StructAlignment = alignment;
			cls->PropertyData.Init(cls);
			return cls;
		}

		// UObject::Mark reads the instance's property data, so every instance lays it out, the
		// leaves of a reference chain included.
		UObject* NewInstance(UClass* cls, const char* name)
		{
			UObject* obj = GC::Alloc<UObject>(name, cls, ObjectFlags::NoFlags);
			obj->PropertyData.Init(cls);
			return obj;
		}

	private:
		std::vector<GCRoot<GCObject>> roots;
	};
}

TEST(GcMarksThroughObjectProperties)
{
	Fixture fixture;
	UObjectProperty* link = GC::Alloc<UObjectProperty>("Link", nullptr, ObjectFlags::NoFlags);
	UClass* cls = fixture.NewClass("GcChainClass", { link }, sizeof(UObject*), alignof(UObject*));

	// Only the first object is held: the second is reached through its property, the third
	// through the second's, and the fourth through nothing.
	UObject* first = fixture.Hold(fixture.NewInstance(cls, "First"));
	UObject* second = fixture.NewInstance(cls, "Second");
	UObject* third = fixture.NewInstance(cls, "Third");
	UObject* unreferenced = fixture.NewInstance(cls, "Unreferenced");
	first->Value<UObject*>(link->DataOffset) = second;
	second->Value<UObject*>(link->DataOffset) = third;

	const size_t before = GC::GetStats().numObjects;
	GC::Collect();
	const size_t after = GC::GetStats().numObjects;
	const bool firstLive = IsLive(first);
	const bool secondLive = IsLive(second);
	const bool thirdLive = IsLive(third);
	const bool unreferencedLive = IsLive(unreferenced);

	CHECK_MSG(firstLive, "the held object was swept");
	CHECK_MSG(secondLive, "an object held by a property of a held object was swept");
	CHECK_MSG(thirdLive, "an object two property hops from a root was swept");
	CHECK_MSG(!unreferencedLive, "an object reachable from no root survived");
	CHECK_EQ(before - after, size_t{ 1 });
}

TEST(GcMarksStructMemberArrays)
{
	Fixture fixture;

	// A struct of two object slots, as a static array of dimension two, held by a class through
	// a struct-valued property.
	UObjectProperty* member = GC::Alloc<UObjectProperty>("Members", nullptr, ObjectFlags::NoFlags);
	member->ArrayDimension = 2;
	UStruct* structType = fixture.Hold(GC::Alloc<UStruct>("GcMemberStruct", nullptr, ObjectFlags::NoFlags));
	structType->Properties.push_back(fixture.Hold(member));
	structType->StructSize = 2 * sizeof(UObject*);
	structType->StructAlignment = alignof(UObject*);

	UStructProperty* holder = GC::Alloc<UStructProperty>("Holder", nullptr, ObjectFlags::NoFlags);
	holder->Struct = structType;
	UClass* cls = fixture.NewClass("GcStructClass", { holder }, structType->StructSize, structType->StructAlignment);
	UClass* leafClass = fixture.NewClass("GcLeafClass", {}, 0, 1);

	UObject* instance = fixture.Hold(fixture.NewInstance(cls, "Instance"));
	UObject* firstElement = fixture.NewInstance(leafClass, "FirstElement");
	UObject* secondElement = fixture.NewInstance(leafClass, "SecondElement");
	void* structData = instance->PropertyData.Ptr(holder);
	*static_cast<UObject**>(member->GetElement(structData, 0)) = firstElement;
	*static_cast<UObject**>(member->GetElement(structData, 1)) = secondElement;

	const size_t before = GC::GetStats().numObjects;
	GC::Collect();
	const size_t after = GC::GetStats().numObjects;
	const bool firstLive = IsLive(firstElement);
	const bool secondLive = IsLive(secondElement);

	CHECK_MSG(firstLive, "the first element of a static array inside a struct was swept");
	CHECK_MSG(secondLive, "the second element of a static array inside a struct was swept");
	CHECK_EQ(after, before);
}

namespace
{
	struct ThrowsFromConstructor : GCObject
	{
		ThrowsFromConstructor() { throw std::runtime_error("constructor threw"); }
		GCAllocation* Mark(GCAllocation* marklist) override { return marklist; }
	};

	// Allocates an object of its own before it throws, so the block under construction is no
	// longer the head of the allocation list when the allocator gives it back.
	struct AllocatesThenThrows : GCObject
	{
		AllocatesThenThrows(UObject*& child)
		{
			child = GC::Alloc<UObject>("Child", nullptr, ObjectFlags::NoFlags);
			throw std::runtime_error("constructor threw");
		}
		GCAllocation* Mark(GCAllocation* marklist) override { return marklist; }
	};
}

// No collection runs here: the failure being pinned is in the bookkeeping a throw leaves behind,
// and a collection would walk the freed block that bookkeeping still lists.
TEST(GcAllocUnlinksOnThrow)
{
	const GCStats before = GC::GetStats();
	try
	{
		GC::Alloc<ThrowsFromConstructor>();
	}
	catch (...)
	{
	}
	const GCStats after = GC::GetStats();

	CHECK_EQ(after.numObjects, before.numObjects);
	CHECK_EQ(after.memoryUsage, before.memoryUsage);
	CHECK_EQ(CountLive(), after.numObjects);

	// The object the constructor allocated stays, as it would under any allocator, and the
	// block that threw is found and removed although it is no longer the head of the list.
	UObject* child = nullptr;
	try
	{
		GC::Alloc<AllocatesThenThrows>(child);
	}
	catch (...)
	{
	}
	const GCStats afterChild = GC::GetStats();

	CHECK_EQ(afterChild.numObjects, after.numObjects + 1);
	CHECK_EQ(afterChild.memoryUsage, after.memoryUsage + sizeof(GCAllocation) + sizeof(UObject));
	CHECK_MSG(IsLive(child), "the object the constructor allocated is missing from the live list");
	CHECK_EQ(CountLive(), afterChild.numObjects);
}

TEST(GcRootKeepsAndReleases)
{
	Fixture fixture;
	UClass* cls = fixture.NewClass("GcPlainClass", {}, 0, 1);
	UObject* sibling = fixture.NewInstance(cls, "Sibling");
	UObject* held = fixture.NewInstance(cls, "Held");
	const size_t before = GC::GetStats().numObjects;

	{
		GCRoot<UObject> holder(held);
		GC::Collect();
		const size_t during = GC::GetStats().numObjects;
		const bool siblingLive = IsLive(sibling);
		const bool heldLive = IsLive(held);

		CHECK_MSG(!siblingLive, "an unreferenced object survived a collection");
		CHECK_MSG(heldLive, "an object held by a root was swept");
		CHECK_EQ(before - during, size_t{ 1 });
	}

	GC::Collect();
	const size_t released = GC::GetStats().numObjects;
	const bool heldLive = IsLive(held);

	CHECK_MSG(!heldLive, "an object survived after its root was destroyed");
	CHECK_EQ(before - released, size_t{ 2 });
}

TEST(GcCountersFollowTheSweep)
{
	Fixture fixture;
	UClass* cls = fixture.NewClass("GcPlainClass", {}, 0, 1);

	// Two types of different size, so a count taken from the wrong type shows.
	fixture.NewInstance(cls, "FirstUnreferenced");
	fixture.NewInstance(cls, "SecondUnreferenced");
	GC::Alloc<UStruct>("ThirdUnreferenced", nullptr, ObjectFlags::NoFlags);

	const GCStats before = GC::GetStats();
	GC::Collect();
	const GCStats after = GC::GetStats();

	const size_t sweptSize = 2 * (sizeof(GCAllocation) + sizeof(UObject)) + sizeof(GCAllocation) + sizeof(UStruct);
	CHECK_EQ(before.numObjects - after.numObjects, size_t{ 3 });
	CHECK_EQ(before.memoryUsage - after.memoryUsage, sweptSize);
	CHECK_EQ(CountLive(), after.numObjects);
}


TEST(GcMarksTheMembersOfAStruct)
{
	Fixture fixture;

	// A struct keeps its property list and its base as C++ members, which no property data reaches. The struct
	// is held; its base, its base's own base and the properties of both are not.
	UStruct* grandBase = GC::Alloc<UStruct>("GcGrandBase", nullptr, ObjectFlags::NoFlags);
	UStruct* base = GC::Alloc<UStruct>("GcBase", nullptr, ObjectFlags::NoFlags);
	UStruct* derived = fixture.Hold(GC::Alloc<UStruct>("GcDerived", nullptr, ObjectFlags::NoFlags));
	UObjectProperty* ownProperty = GC::Alloc<UObjectProperty>("Own", nullptr, ObjectFlags::NoFlags);
	UObjectProperty* inheritedProperty = GC::Alloc<UObjectProperty>("Inherited", nullptr, ObjectFlags::NoFlags);
	derived->BaseStruct = base;
	base->BaseStruct = grandBase;
	derived->Properties.push_back(ownProperty);
	base->Properties.push_back(inheritedProperty);
	UObject* unreferenced = GC::Alloc<UObject>("Unreferenced", nullptr, ObjectFlags::NoFlags);

	const size_t before = GC::GetStats().numObjects;
	GC::Collect();
	const size_t after = GC::GetStats().numObjects;
	const bool baseLive = IsLive(base);
	const bool grandBaseLive = IsLive(grandBase);
	const bool ownLive = IsLive(ownProperty);
	const bool inheritedLive = IsLive(inheritedProperty);
	const bool unreferencedLive = IsLive(unreferenced);

	CHECK_MSG(baseLive, "the base of a held struct was swept");
	CHECK_MSG(grandBaseLive, "the base of a base of a held struct was swept");
	CHECK_MSG(ownLive, "a property in the list of a held struct was swept");
	CHECK_MSG(inheritedLive, "a property in the list of the base of a held struct was swept");
	CHECK_MSG(!unreferencedLive, "an object reachable from no root survived");
	CHECK_EQ(before - after, size_t{ 1 });
}

TEST(GcMarksTheMembersOfAClass)
{
	Fixture fixture;

	// A class is a struct whose property list a loaded game fills with the property objects of its base, among them
	// the ones the engine builds for Object, which belong to no package. Same shape, through a held class.
	UClass* baseClass = GC::Alloc<UClass>("GcBaseClass", nullptr, ObjectFlags::NoFlags);
	UClass* cls = fixture.Hold(GC::Alloc<UClass>("GcMemberClass", nullptr, ObjectFlags::NoFlags));
	UObjectProperty* nativeProperty = GC::Alloc<UObjectProperty>("Outer", nullptr, ObjectFlags::Native);
	cls->BaseStruct = baseClass;
	cls->Properties.push_back(nativeProperty);

	GC::Collect();
	const bool baseLive = IsLive(baseClass);
	const bool propertyLive = IsLive(nativeProperty);

	CHECK_MSG(baseLive, "the base class of a held class was swept");
	CHECK_MSG(propertyLive, "a property in the list of a held class was swept");
}

TEST(FindRegionTakesTheZoneFromItsOwnModel)
{
	// The engine global stays null: a lookup that asked the engine for the zone would dereference it.
	UModel* model = GC::Alloc<UModel>("ZoneModel", nullptr, ObjectFlags::NoFlags);
	BspNode node{};
	node.Front = -1;
	node.Back = -1;
	node.PlaneZ = 1.0f;
	node.Zone0 = 1;
	node.Zone1 = 1;
	model->Nodes.push_back(node);
	model->Zones.resize(2);
	model->Zones[0].ZoneActor = nullptr;
	model->Zones[1].ZoneActor = nullptr;

	UZoneInfo* levelZone = GC::Alloc<UZoneInfo>("LevelZone", nullptr, ObjectFlags::NoFlags);
	UZoneInfo* ownZone = GC::Alloc<UZoneInfo>("OwnZone", nullptr, ObjectFlags::NoFlags);

	PointRegion without = model->FindRegion(vec3(0.0f, 0.0f, 0.0f), levelZone);
	model->Zones[1].ZoneActor = ownZone;
	PointRegion with = model->FindRegion(vec3(0.0f, 0.0f, 0.0f), levelZone);

	CHECK_MSG(without.ZoneNumber == 1, "the leaf is not in zone 1");
	CHECK_MSG(without.Zone == levelZone, "a zone with no zone actor did not fall back to the level's zone");
	CHECK_MSG(with.Zone == ownZone, "the zone actor of the model's own zone table was not returned");
}

namespace
{
	// A value whose construction and destruction the tests count. It remembers the address of
	// every live copy, so a destruct of an address that was never constructed, or one destructed
	// twice, shows as a bad destruct instead of passing unseen.
	struct Tracked
	{
		static inline std::set<const Tracked*> live;
		static inline size_t badDestructs = 0;
		// When positive, counts down on each copy construction and throws on reaching zero.
		static inline int copiesUntilThrow = 0;

		int id = 0;

		Tracked() { live.insert(this); }
		Tracked(const Tracked& other) : id(other.id)
		{
			if (copiesUntilThrow > 0 && --copiesUntilThrow == 0)
				throw std::runtime_error("copy threw");
			live.insert(this);
		}
		Tracked& operator=(const Tracked& other) { id = other.id; return *this; }
		~Tracked()
		{
			if (live.erase(this) == 0)
				badDestructs++;
		}

		bool operator==(const Tracked& other) const { return id == other.id; }
		bool operator<(const Tracked& other) const { return id < other.id; }
	};

	typedef UPropertyT<Tracked> TrackedProperty;

	// Gives each property its own, 16-byte aligned slot and returns the size that covers them.
	size_t Place(std::initializer_list<UProperty*> properties)
	{
		size_t offset = 0;
		for (UProperty* prop : properties)
		{
			offset = (offset + 15) & ~size_t{ 15 };
			prop->DataOffset.DataOffset = offset;
			offset += prop->ArraySize();
		}
		return (offset + 15) & ~size_t{ 15 };
	}

	bool AnyLive(std::initializer_list<GCObject*> objects)
	{
		for (GCObject* obj : objects)
		{
			if (IsLive(obj))
				return true;
		}
		return false;
	}

	void FillMap(UMapProperty* mapProp, void* data, int firstKey)
	{
		auto& map = *static_cast<UMapProperty::Map*>(data);
		for (int i = 0; i < 2; i++)
		{
			MapPropertyValue key(mapProp->Key);
			MapPropertyValue value(mapProp->Value);
			static_cast<Tracked*>(key.Data)->id = firstKey + i;
			map.emplace(key, value);
		}
	}
}

TEST(SweepDestructsTheValuesOfASweptObject)
{
	Fixture fixture;
	TrackedProperty* value = GC::Alloc<TrackedProperty>("Value", nullptr, ObjectFlags::NoFlags);
	value->ArrayDimension = 3;
	const size_t size = Place({ value });
	UClass* cls = fixture.NewClass("GcTrackedClass", { value }, size, 16);

	UObject* instances[4];
	for (UObject*& instance : instances)
		instance = fixture.NewInstance(cls, "Instance");
	const size_t before = Tracked::live.size();

	GC::Collect();
	const size_t after = Tracked::live.size();
	const bool anyLive = AnyLive({ instances[0], instances[1], instances[2], instances[3] });

	CHECK_EQ(before, size_t{ 15 });
	CHECK_MSG(!anyLive, "an unreferenced instance survived the collection");
	CHECK_EQ(after, size_t{ 3 });
	CHECK_EQ(Tracked::badDestructs, size_t{ 0 });
}

TEST(SweepDestructsArrayElementsAndMapEntries)
{
	Fixture fixture;
	TrackedProperty* element = fixture.Hold(GC::Alloc<TrackedProperty>("Element", nullptr, ObjectFlags::NoFlags));
	TrackedProperty* key = fixture.Hold(GC::Alloc<TrackedProperty>("Key", nullptr, ObjectFlags::NoFlags));
	TrackedProperty* mapped = fixture.Hold(GC::Alloc<TrackedProperty>("Mapped", nullptr, ObjectFlags::NoFlags));
	UArrayProperty* array = GC::Alloc<UArrayProperty>("Array", nullptr, ObjectFlags::NoFlags);
	array->Inner = element;
	UMapProperty* map = GC::Alloc<UMapProperty>("Map", nullptr, ObjectFlags::NoFlags);
	map->Key = key;
	map->Value = mapped;
	const size_t size = Place({ array, map });
	UClass* cls = fixture.NewClass("GcContainerClass", { array, map }, size, 16);
	const size_t baseline = Tracked::live.size();

	UObject* instances[3];
	int firstKey = 0;
	for (UObject*& instance : instances)
	{
		instance = fixture.NewInstance(cls, "Instance");
		static_cast<ScriptArray*>(instance->PropertyData.Ptr(array))->Resize(5);
		FillMap(map, instance->PropertyData.Ptr(map), firstKey);
		firstKey += 2;
	}
	const size_t filled = Tracked::live.size();

	GC::Collect();
	const size_t after = Tracked::live.size();

	CHECK_EQ(filled - baseline, size_t{ 3 * (5 + 2 * 2) });
	CHECK_EQ(after, baseline);
	CHECK_EQ(Tracked::badDestructs, size_t{ 0 });
}

TEST(SweepDestructsStructMembers)
{
	Fixture fixture;
	TrackedProperty* single = GC::Alloc<TrackedProperty>("Single", nullptr, ObjectFlags::NoFlags);
	TrackedProperty* pair = GC::Alloc<TrackedProperty>("Pair", nullptr, ObjectFlags::NoFlags);
	pair->ArrayDimension = 2;
	UStruct* structType = fixture.Hold(GC::Alloc<UStruct>("GcTrackedStruct", nullptr, ObjectFlags::NoFlags));
	structType->Properties.push_back(fixture.Hold(single));
	structType->Properties.push_back(fixture.Hold(pair));
	structType->StructSize = Place({ single, pair });
	structType->StructAlignment = 16;

	UStructProperty* holder = GC::Alloc<UStructProperty>("Holder", nullptr, ObjectFlags::NoFlags);
	holder->Struct = structType;
	const size_t size = Place({ holder });
	UClass* cls = fixture.NewClass("GcStructValueClass", { holder }, size, 16);
	const size_t baseline = Tracked::live.size();

	UObject* instances[3];
	for (UObject*& instance : instances)
		instance = fixture.NewInstance(cls, "Instance");
	const size_t filled = Tracked::live.size();

	GC::Collect();
	const size_t after = Tracked::live.size();

	CHECK_EQ(filled - baseline, size_t{ 3 * 3 });
	CHECK_EQ(after, baseline);
	CHECK_EQ(Tracked::badDestructs, size_t{ 0 });
}

// The instance is allocated before its class and the properties the class lays it out by, and
// the sweep visits the newest allocation first, so a sweep that destructs an object when it frees
// it reads a property object it has already freed.
TEST(SweepsAClassBeforeItsInstances)
{
	UObject* instance = GC::Alloc<UObject>("Early", nullptr, ObjectFlags::NoFlags);
	TrackedProperty* element = GC::Alloc<TrackedProperty>("Element", nullptr, ObjectFlags::NoFlags);
	UArrayProperty* array = GC::Alloc<UArrayProperty>("Array", nullptr, ObjectFlags::NoFlags);
	array->Inner = element;
	UClass* cls = GC::Alloc<UClass>("GcLateClass", nullptr, ObjectFlags::NoFlags);
	cls->Properties.push_back(array);
	cls->StructSize = Place({ array });
	cls->StructAlignment = 16;
	cls->PropertyData.Init(cls);
	instance->Class = cls;
	instance->PropertyData.Init(cls);
	static_cast<ScriptArray*>(instance->PropertyData.Ptr(array))->Resize(5);
	const size_t filled = Tracked::live.size();

	GC::Collect();
	const size_t after = Tracked::live.size();
	const bool anyLive = AnyLive({ instance, cls, array, element });

	CHECK_EQ(filled, size_t{ 5 });
	CHECK_MSG(!anyLive, "an unreferenced object, class or property survived the collection");
	CHECK_EQ(after, size_t{ 0 });
	CHECK_EQ(Tracked::badDestructs, size_t{ 0 });
}

#if defined(__GLIBC__)
namespace
{
	size_t HeapInUse()
	{
		struct mallinfo2 info = mallinfo2();
		return info.uordblks + info.hblkhd;
	}
}
#endif

// The tracked type cannot see what a std::string owns, so this one reads the allocator.
TEST(SweepReleasesTheStringsOfASweptObject)
{
#if defined(__GLIBC__)
	Fixture fixture;
	UStrProperty* text = GC::Alloc<UStrProperty>("Text", nullptr, ObjectFlags::NoFlags);
	const size_t size = Place({ text });
	UClass* cls = fixture.NewClass("GcStringClass", { text }, size, 16);
	const size_t before = HeapInUse();

	for (int i = 0; i < 64; i++)
	{
		UObject* instance = fixture.NewInstance(cls, "Instance");
		*static_cast<std::string*>(instance->PropertyData.Ptr(text)) = std::string(200000, 'x');
	}
	const size_t filled = HeapInUse();

	GC::Collect();
	const size_t after = HeapInUse();

	CHECK_MSG(filled > before + 10 * 1024 * 1024, "the instances did not allocate their strings");
	CHECK_MSG(after < before + 2 * 1024 * 1024, "the strings of the swept instances were not released");
#endif
}

TEST(SweepLeavesASurvivorsValuesAlone)
{
	Fixture fixture;
	TrackedProperty* value = GC::Alloc<TrackedProperty>("Value", nullptr, ObjectFlags::NoFlags);
	UStrProperty* text = GC::Alloc<UStrProperty>("Text", nullptr, ObjectFlags::NoFlags);
	const size_t size = Place({ value, text });
	UClass* cls = fixture.NewClass("GcSurvivorClass", { value, text }, size, 16);

	UObject* held = fixture.Hold(fixture.NewInstance(cls, "Held"));
	fixture.NewInstance(cls, "SweptFirst");
	fixture.NewInstance(cls, "SweptSecond");
	Tracked* heldValue = static_cast<Tracked*>(held->PropertyData.Ptr(value));
	heldValue->id = 7;
	const std::string written(100, 's');
	*static_cast<std::string*>(held->PropertyData.Ptr(text)) = written;
	const size_t before = Tracked::live.size();

	GC::Collect();
	const size_t after = Tracked::live.size();
	const bool heldLive = IsLive(held);
	const bool valueLive = Tracked::live.count(heldValue) == 1;

	CHECK_MSG(heldLive, "the held instance was swept");
	CHECK_MSG(valueLive, "the value of a surviving instance was destructed");
	CHECK_MSG(before - after <= 2, "a collection destructed more values than its swept instances held");
	CHECK_EQ(heldValue->id, 7);
	CHECK_MSG(*static_cast<std::string*>(held->PropertyData.Ptr(text)) == written, "the string of a surviving instance changed");
	CHECK_EQ(Tracked::badDestructs, size_t{ 0 });
}

TEST(SweepSkipsABlockWhoseInitThrew)
{
	Fixture fixture;
	TrackedProperty* value = GC::Alloc<TrackedProperty>("Value", nullptr, ObjectFlags::NoFlags);
	value->ArrayDimension = 3;
	const size_t size = Place({ value });
	UClass* cls = fixture.NewClass("GcThrowingClass", { value }, size, 16);

	// The second element's copy throws, leaving a block with one constructed element and two
	// slots no constructor ever ran in.
	UObject* instance = GC::Alloc<UObject>("HalfBuilt", nullptr, ObjectFlags::NoFlags);
	bool threw = false;
	Tracked::copiesUntilThrow = 2;
	try
	{
		instance->PropertyData.Init(cls);
	}
	catch (const std::exception&)
	{
		threw = true;
	}
	Tracked::copiesUntilThrow = 0;

	GC::Collect();
	const bool instanceLive = IsLive(instance);

	CHECK_MSG(threw, "the instance's block was built without the copy throwing");
	CHECK_MSG(!instanceLive, "the half-built instance survived the collection");
	CHECK_EQ(Tracked::badDestructs, size_t{ 0 });
}
