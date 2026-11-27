#pragma once

#include "Math/vec.h"
#include <set>

class UActor;
class BspNode;

class CollisionHit
{
public:
	CollisionHit() = default;
	CollisionHit(float fraction, vec3 normal, UActor* actor, BspNode* node, BspNode* nodeHead) : Fraction(fraction), Normal(normal), Actor(actor), Node(node), NodeHead(nodeHead) { }

	float Fraction = 1.0;
	vec3 Normal = vec3(0.0);
	UActor* Actor = nullptr;
	BspNode* Node = nullptr;
	BspNode* NodeHead = nullptr;
};

class CollisionHitList
{
public:
	CollisionHitList()
	{
		items = (CollisionHit*)buffer;
		count = 0;
		capacity = buffercapacity;
	}

	CollisionHitList(const CollisionHitList& other)
	{
		items = (CollisionHit*)buffer;
		count = 0;
		capacity = buffercapacity;

		push_back(other);
	}

	~CollisionHitList()
	{
		clear();

		if (capacity > buffercapacity)
			delete[](uint64_t*)items;
	}

	CollisionHitList& operator=(const CollisionHitList& other)
	{
		if (this != &other)
		{
			clear();
			push_back(other);
		}
		return *this;
	}

	typedef CollisionHit* iterator;
	typedef const CollisionHit* const_iterator;

	iterator begin() { return items; }
	iterator end() { return items + count; }
	const_iterator begin() const { return items; }
	const_iterator end() const { return items + count; }

	bool empty() const { return count == 0; }

	CollisionHit& front() { return items[0]; }
	const CollisionHit& front() const { return items[0]; }

	CollisionHit& back() { return items[count - 1]; }
	const CollisionHit& back() const { return items[count - 1]; }

	void clear()
	{
		for (size_t i = 0, c = count; i < c; i++)
			items[i].~CollisionHit();
		count = 0;
	}

	void push_back(const CollisionHit& hit)
	{
		if (hit.Fraction < 1.0f && hit.Actor == nullptr)
			Exception::Throw("Tried to add a hit without an actor");

		if (count != capacity)
		{
			new((uint8_t*)&items[count]) CollisionHit(hit);
			count++;
		}
		else
		{
			reserve(capacity << 1);

			new((uint8_t*)&items[count]) CollisionHit(hit);
			count++;
		}
	}

	void push_back(const CollisionHitList& list)
	{
		for (size_t i = 0, count = list.count; i < count; i++)
		{
			push_back(list.items[i]);
		}
	}

private:
	void reserve(size_t newcapacity)
	{
		if (count < newcapacity)
		{
			size_t allocsize = (sizeof(CollisionHit) * newcapacity + 7) / 8;
			CollisionHit* newitems = (CollisionHit*)new uint64_t[allocsize];

			for (size_t i = 0, c = count; i < c; i++)
			{
				new ((uint8_t*)&newitems[i]) CollisionHit(std::move(items[i]));
				items[i].~CollisionHit();
			}

			CollisionHit* olditems = items;
			items = newitems;

			if (capacity > buffercapacity)
				delete[](uint64_t*)olditems;

			capacity = newcapacity;
		}
	}

	static const int buffercapacity = 16;
	uint64_t buffer[(sizeof(CollisionHit) * buffercapacity + 7) / 8];

	CollisionHit* items;
	size_t count;
	size_t capacity;
};

// The hits of a trace or an overlap test, one per actor: the first hit of each actor in the order
// given (a trace sorts by fraction, so that is its nearest), but every hit on the level's LevelInfo
// and every hit with no actor. A world hit carries the LevelInfo as its actor, and each world hit is
// a different surface, which the texture trace and the ladder test read.
inline CollisionHitList UniqueActorHits(const CollisionHitList& hits, const UActor* levelInfo)
{
	std::set<const UActor*> seenActors;
	CollisionHitList uniqueHits;
	for (const CollisionHit& hit : hits)
	{
		if (!hit.Actor || hit.Actor == levelInfo || seenActors.insert(hit.Actor).second)
			uniqueHits.push_back(hit);
	}
	return uniqueHits;
}
