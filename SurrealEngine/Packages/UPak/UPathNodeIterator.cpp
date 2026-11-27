
#include "Precomp.h"
#include "UPathNodeIterator.h"
#include "Utils/Logger.h"

void UPakPathNodeIterator::BuildPath(vec3& start, vec3& end)
{
	LogUnimplemented("PathNodeIterator.BuildPath()");
	NodeIndex() = 0;
}

UNavigationPoint* UPakPathNodeIterator::GetFirst()
{
	LogUnimplemented("PathNodeIterator.GetFirst()");
	return nullptr;
}

UNavigationPoint* UPakPathNodeIterator::GetPrevious()
{
	LogUnimplemented("PathNodeIterator.GetPrevious()");
	return nullptr;
}

UNavigationPoint* UPakPathNodeIterator::GetCurrent()
{
	LogUnimplemented("PathNodeIterator.GetCurrent()");
	return nullptr;
}

UNavigationPoint* UPakPathNodeIterator::GetNext()
{
	LogUnimplemented("PathNodeIterator.GetNext()");
	return nullptr;
}

UNavigationPoint* UPakPathNodeIterator::GetLast()
{
	LogUnimplemented("PathNodeIterator.GetLast()");
	return nullptr;
}

UNavigationPoint* UPakPathNodeIterator::GetLastVisible()
{
	LogUnimplemented("PathNodeIterator.GetLastVisible()");
	return nullptr;
}
