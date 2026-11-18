#include "Precomp.h"
#include "TestRunner.h"
#include "Package/Package.h"
#include "Packages/Core/UClass.h"
#include "Packages/Core/UObject.h"

// `new(outer)` in script puts the object in the outer's package, so a save that reaches the object
// through the outer exports it with the level. These pin the choice of package. Making the object and
// setting its properties need a running engine and are checked by running a map.

namespace
{
	// A Package cannot be built without a PackageManager, and the choice never reads through the
	// pointer, so distinct addresses stand in for packages.
	char PackageSlots[3];
	Package* const PackageA = reinterpret_cast<Package*>(&PackageSlots[0]);
	Package* const PackageB = reinterpret_cast<Package*>(&PackageSlots[1]);
	Package* const Transient = reinterpret_cast<Package*>(&PackageSlots[2]);

	UObject* MakeOuter(const char* name, Package* package)
	{
		UClass* cls = GC::Alloc<UClass>("TestClass", nullptr, ObjectFlags::NoFlags);
		UObject* outer = GC::Alloc<UObject>(name, cls, ObjectFlags::NoFlags);
		outer->package = package;
		return outer;
	}
}

TEST(NewObjectPackageIsTheOutersPackage)
{
	UObject* outer = MakeOuter("Outer", PackageA);
	CHECK(NewObjectPackage(outer, Transient) == PackageA);
}

TEST(NewObjectPackageFollowsEachOuter)
{
	// A function that returned one fixed package would pass either check alone.
	UObject* first = MakeOuter("First", PackageA);
	UObject* second = MakeOuter("Second", PackageB);
	CHECK(NewObjectPackage(first, Transient) == PackageA);
	CHECK(NewObjectPackage(second, Transient) == PackageB);
}

TEST(NewObjectPackageWithoutOuterIsTransient)
{
	CHECK(NewObjectPackage(nullptr, Transient) == Transient);
}

TEST(NewObjectPackageOfTransientOuterIsTransient)
{
	UObject* outer = MakeOuter("Outer", Transient);
	CHECK(NewObjectPackage(outer, Transient) == Transient);
}

TEST(NewObjectPackageOfPackagelessOuterIsTransient)
{
	// Package::NewObject dereferences the package it is called on, so an outer that has none cannot
	// hand it on.
	UObject* outer = MakeOuter("Outer", nullptr);
	CHECK(NewObjectPackage(outer, Transient) == Transient);
}
