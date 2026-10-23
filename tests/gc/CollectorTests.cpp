#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UStruct.h"
#include "Packages/Core/UObject.h"
#include "Packages/Core/Properties/UObjectProperty.h"
#include "Packages/Core/Properties/UStructProperty.h"

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

	// Builds the classes, properties and instances the tests collect. Nothing reaches a class's
	// property list or a struct's member list by itself, since those are C++ members that no
	// mark visits, so everything of that kind is held by a root here. Instances are not.
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
	fixture.NewInstance(cls, "Unreferenced");
	first->Value<UObject*>(link->DataOffset) = second;
	second->Value<UObject*>(link->DataOffset) = third;

	// The collection is withheld until the object property mark is fixed: it hands the address
	// of the property's slot to the collector as if it were an object, so collecting here writes
	// into the memory next to the property data and never marks the object the slot holds.
	CHECK_MSG(false, "not collecting: the object property mark treats its slot as an object and corrupts the memory beside it");
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

	// Withheld for the same reason as in GcMarksThroughObjectProperties: the first element of the
	// array is marked through the same slot mark, so a collection here corrupts memory.
	CHECK_MSG(false, "not collecting: the object property mark treats its slot as an object and corrupts the memory beside it");
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
