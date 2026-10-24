#include "Precomp.h"
#include "TestRunner.h"
#include "Packages/Engine/Actors/Pawn/UPawn_AIMath.h"
#include <algorithm>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// The plain-value parts of the Unreal Gold path node iterator: the cheapest node sequence between two
// nodes, the fixed store of at most 64 nodes it is kept in, and the cursor that walks the store. The
// iterator's engine-side code only numbers the navigation points and calls these.

namespace
{
	// 0 -> 1 (10) -> 2 (10) -> 3 (5); 0 -> 2 (50) is the slow way; node 4 has no edge.
	AIMath::Graph Chain5()
	{
		AIMath::Graph graph(5);
		graph[0] = { { 1, 10, false }, { 2, 50, false } };
		graph[1] = { { 2, 10, false } };
		graph[2] = { { 3, 5, false } };
		return graph;
	}

	// The slot a cursor function answered, or -1 for no node.
	int Slot(std::optional<int> slot)
	{
		return slot ? *slot : -1;
	}

	// Cheapest costs by repeated relaxation, written independently of the Dijkstra under test.
	std::vector<int> Relax(const AIMath::Graph& graph, int start, bool usePruned)
	{
		std::vector<int> best(graph.size(), AIMath::Unreachable);
		best[start] = 0;
		for (size_t pass = 0; pass < graph.size(); pass++)
		{
			for (size_t from = 0; from < graph.size(); from++)
			{
				if (best[from] == AIMath::Unreachable)
					continue;
				for (const AIMath::Edge& edge : graph[from])
				{
					if (edge.pruned && !usePruned)
						continue;
					int candidate = best[from] + edge.cost;
					if (best[edge.to] == AIMath::Unreachable || candidate < best[edge.to])
						best[edge.to] = candidate;
				}
			}
		}
		return best;
	}

	// The cheapest allowed edge from one node to another, or -1 when none joins them.
	int CheapestEdge(const AIMath::Graph& graph, int from, int to, bool usePruned)
	{
		int cheapest = -1;
		for (const AIMath::Edge& edge : graph[from])
		{
			if (edge.to != to || (edge.pruned && !usePruned))
				continue;
			if (cheapest < 0 || edge.cost < cheapest)
				cheapest = edge.cost;
		}
		return cheapest;
	}

	AIMath::Graph RandomGraph(std::mt19937& rng)
	{
		int nodes = 1 + (int)(rng() % 12);
		AIMath::Graph graph(nodes);
		for (int from = 0; from < nodes; from++)
		{
			int edges = (int)(rng() % (nodes + 2));
			for (int e = 0; e < edges; e++)
				graph[from].push_back({ (int)(rng() % nodes), (int)(rng() % 51), rng() % 3 == 0 });
		}
		return graph;
	}

	std::string Describe(unsigned seed, int start, int end, bool usePruned)
	{
		std::ostringstream text;
		text << "seed " << seed << " from " << start << " to " << end << " usePruned " << usePruned << ": ";
		return text.str();
	}
}

TEST(AIShortestPathReturnsTheCheapestNodesInOrder)
{
	AIMath::Graph graph = Chain5();

	std::optional<AIMath::PathResult> toThree = AIMath::ShortestPath(graph, 0, 3, false);
	CHECK(toThree.has_value());
	CHECK_EQ(toThree->cost, 25);
	CHECK((toThree->nodes == std::vector<int>{ 0, 1, 2, 3 }));

	std::optional<AIMath::PathResult> toTwo = AIMath::ShortestPath(graph, 0, 2, false);
	CHECK(toTwo.has_value());
	CHECK_EQ(toTwo->cost, 20);
	CHECK((toTwo->nodes == std::vector<int>{ 0, 1, 2 }));
}

TEST(AIShortestPathCostMatchesShortestDistancesOnRandomGraphs)
{
	for (unsigned seed = 1; seed <= 60; seed++)
	{
		std::mt19937 rng(seed);
		AIMath::Graph graph = RandomGraph(rng);
		int nodes = (int)graph.size();

		for (bool usePruned : { false, true })
		{
			for (int start = 0; start < nodes; start++)
			{
				std::vector<int> distances = AIMath::ShortestDistances(graph, start, usePruned);
				std::vector<int> reference = Relax(graph, start, usePruned);

				for (int end = 0; end < nodes; end++)
				{
					std::string where = Describe(seed, start, end, usePruned);
					CHECK_MSG(distances[end] == reference[end], where + "the two references disagree");

					std::optional<AIMath::PathResult> path = AIMath::ShortestPath(graph, start, end, usePruned);
					CHECK_MSG(path.has_value() == (distances[end] != AIMath::Unreachable), where + "a path exists exactly when the end is reached");
					if (!path)
						continue;

					CHECK_MSG(path->cost == distances[end], where + "the cost is the cheapest cost");
					CHECK_MSG(!path->nodes.empty() && path->nodes.front() == start && path->nodes.back() == end, where + "the nodes run from the start to the end");

					std::vector<int> sorted = path->nodes;
					std::sort(sorted.begin(), sorted.end());
					CHECK_MSG(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end(), where + "no node repeats");

					int summed = 0;
					for (size_t i = 1; i < path->nodes.size(); i++)
					{
						int edgeCost = CheapestEdge(graph, path->nodes[i - 1], path->nodes[i], usePruned);
						CHECK_MSG(edgeCost >= 0, where + "each step follows an allowed edge");
						summed += edgeCost;
					}
					CHECK_MSG(summed == path->cost, where + "the edge costs along the nodes add up to the cost");
				}
			}
		}
	}
}

TEST(AIShortestPathUnreachableOrOutOfRangeIsEmpty)
{
	AIMath::Graph graph = Chain5();
	CHECK(!AIMath::ShortestPath(graph, 0, 4, false).has_value());
	CHECK(!AIMath::ShortestPath(graph, 3, 0, false).has_value());
	CHECK(!AIMath::ShortestPath(graph, -1, 2, false).has_value());
	CHECK(!AIMath::ShortestPath(graph, 0, -1, false).has_value());
	CHECK(!AIMath::ShortestPath(graph, 5, 2, false).has_value());
	CHECK(!AIMath::ShortestPath(graph, 0, 5, true).has_value());
	CHECK(!AIMath::ShortestPath(AIMath::Graph(), 0, 0, false).has_value());
}

TEST(AIShortestPathFromANodeToItselfIsThatNode)
{
	AIMath::Graph graph = Chain5();
	for (int node = 0; node < (int)graph.size(); node++)
	{
		std::optional<AIMath::PathResult> path = AIMath::ShortestPath(graph, node, node, false);
		CHECK_MSG(path.has_value(), "node " + std::to_string(node) + " has no path to itself");
		CHECK((path->nodes == std::vector<int>{ node }));
		CHECK_EQ(path->cost, 0);
	}
}

TEST(AIShortestPathUsesPrunedEdgesOnlyWhenAllowed)
{
	// 0 -> 2 is a three-unit shortcut the level builder pruned; the way round costs twenty.
	AIMath::Graph graph(3);
	graph[0] = { { 1, 10, false }, { 2, 3, true } };
	graph[1] = { { 2, 10, false } };

	std::optional<AIMath::PathResult> around = AIMath::ShortestPath(graph, 0, 2, false);
	CHECK(around.has_value());
	CHECK_EQ(around->cost, 20);
	CHECK((around->nodes == std::vector<int>{ 0, 1, 2 }));

	std::optional<AIMath::PathResult> shortcut = AIMath::ShortestPath(graph, 0, 2, true);
	CHECK(shortcut.has_value());
	CHECK_EQ(shortcut->cost, 3);
	CHECK((shortcut->nodes == std::vector<int>{ 0, 2 }));
}

TEST(AIPathStoreKeepsTheNodesInOrder)
{
	AIMath::PathStore store = AIMath::StorePath({ 4, 2, 9 }, 30);
	CHECK_EQ(store.count, 3);
	CHECK_EQ(store.nodes[0], 4);
	CHECK_EQ(store.nodes[1], 2);
	CHECK_EQ(store.nodes[2], 9);
	CHECK_EQ(store.cost, 30);

	CHECK_EQ(AIMath::StorePath({}, 0).count, 0);
}

TEST(AIPathStoreKeepsAtMostSixtyFourNodes)
{
	std::vector<int> nodes(100);
	for (int i = 0; i < 100; i++)
		nodes[i] = i;

	AIMath::PathStore store = AIMath::StorePath(nodes, 777);
	CHECK_EQ(store.count, 64);
	CHECK_EQ(store.nodes[0], 0);
	CHECK_EQ(store.nodes[63], 63);
	CHECK_EQ(store.cost, 777);

	CHECK_EQ(AIMath::StorePath(std::vector<int>(nodes.begin(), nodes.begin() + 64), 5).count, 64);
	CHECK_EQ(AIMath::StorePath(std::vector<int>(nodes.begin(), nodes.begin() + 63), 5).count, 63);
}

TEST(AIPathCursorFirstAndLastReposition)
{
	AIMath::PathCursor cursor{ 3, 1 };
	CHECK_EQ(Slot(AIMath::CursorFirst(cursor)), 0);
	CHECK_EQ(cursor.index, 0);
	CHECK_EQ(Slot(AIMath::CursorLast(cursor)), 2);
	CHECK_EQ(cursor.index, 2);
	CHECK_EQ(Slot(AIMath::CursorFirst(cursor)), 0);
	CHECK_EQ(cursor.index, 0);

	cursor.index = 1;
	CHECK_EQ(Slot(AIMath::CursorCurrent(cursor)), 1);
	CHECK_EQ(cursor.index, 1);
}

TEST(AIPathCursorNextWalksToTheEndThenReturnsNone)
{
	AIMath::PathCursor cursor{ 3, 0 };
	CHECK_EQ(Slot(AIMath::CursorNext(cursor)), 1);
	CHECK_EQ(Slot(AIMath::CursorNext(cursor)), 2);
	CHECK_EQ(Slot(AIMath::CursorNext(cursor)), -1);
	CHECK_EQ(Slot(AIMath::CursorNext(cursor)), -1);
	CHECK_EQ(cursor.index, 2);
	CHECK_EQ(Slot(AIMath::CursorCurrent(cursor)), 2);
}

TEST(AIPathCursorPreviousWalksBackThenReturnsNone)
{
	AIMath::PathCursor cursor{ 3, 0 };
	CHECK_EQ(Slot(AIMath::CursorLast(cursor)), 2);
	CHECK_EQ(Slot(AIMath::CursorPrevious(cursor)), 1);
	CHECK_EQ(Slot(AIMath::CursorPrevious(cursor)), 0);
	CHECK_EQ(Slot(AIMath::CursorPrevious(cursor)), -1);
	CHECK_EQ(Slot(AIMath::CursorPrevious(cursor)), -1);
	CHECK_EQ(cursor.index, 0);
	CHECK_EQ(Slot(AIMath::CursorCurrent(cursor)), 0);
}

TEST(AIPathCursorOfAnEmptyPathReturnsNoneEverywhere)
{
	AIMath::PathCursor cursor{ 0, 0 };
	CHECK_EQ(Slot(AIMath::CursorFirst(cursor)), -1);
	CHECK_EQ(Slot(AIMath::CursorNext(cursor)), -1);
	CHECK_EQ(Slot(AIMath::CursorPrevious(cursor)), -1);
	CHECK_EQ(Slot(AIMath::CursorCurrent(cursor)), -1);
	CHECK_EQ(Slot(AIMath::CursorLast(cursor)), -1);
	CHECK_EQ(cursor.index, 0);
}

TEST(AIPathCursorWalksEveryLengthAndTerminates)
{
	using Move = std::optional<int> (*)(AIMath::PathCursor&);
	const Move moves[] = { AIMath::CursorFirst, AIMath::CursorNext, AIMath::CursorPrevious, AIMath::CursorCurrent, AIMath::CursorLast };

	for (int count = 0; count <= AIMath::MaxPathNodes; count++)
	{
		int lastSlot = std::max(count - 1, 0);

		// Whatever index a script left behind, every move answers a slot of the path or none and keeps the index inside it.
		for (int index = -3; index <= AIMath::MaxPathNodes + 2; index++)
		{
			for (Move move : moves)
			{
				AIMath::PathCursor cursor{ count, index };
				int slot = Slot(move(cursor));
				std::string where = "count " + std::to_string(count) + " index " + std::to_string(index) + ": ";
				CHECK_MSG(slot == -1 || (slot >= 0 && slot < count), where + "the slot " + std::to_string(slot) + " is outside the path");
				CHECK_MSG(cursor.index >= 0 && cursor.index <= lastSlot, where + "the index " + std::to_string(cursor.index) + " left the path");
			}
		}

		if (count == 0)
			continue;

		// Forwards from the first node: slots 1 to count-1, then none.
		AIMath::PathCursor forward{ count, 0 };
		CHECK_EQ(Slot(AIMath::CursorFirst(forward)), 0);
		for (int slot = 1; slot < count; slot++)
			CHECK_EQ(Slot(AIMath::CursorNext(forward)), slot);
		CHECK_EQ(Slot(AIMath::CursorNext(forward)), -1);

		// Backwards from the last node: slots count-2 to 0, then none.
		AIMath::PathCursor backward{ count, 0 };
		CHECK_EQ(Slot(AIMath::CursorLast(backward)), count - 1);
		for (int slot = count - 2; slot >= 0; slot--)
			CHECK_EQ(Slot(AIMath::CursorPrevious(backward)), slot);
		CHECK_EQ(Slot(AIMath::CursorPrevious(backward)), -1);

		// The Predator's loops end within count + 1 calls: "while Next" over the whole path, and
		// "while not reachable Previous" from the last node, whichever node (or none) is reachable.
		AIMath::PathCursor whileNext{ count, 0 };
		std::optional<int> node = AIMath::CursorFirst(whileNext);
		int calls = 0;
		while (node && calls <= count + 1)
		{
			node = AIMath::CursorNext(whileNext);
			calls++;
		}
		CHECK_MSG(!node && calls <= count, "count " + std::to_string(count) + ": the loop over Next did not end in time");

		for (int reachable = -1; reachable < count; reachable++)
		{
			AIMath::PathCursor whilePrevious{ count, 0 };
			std::optional<int> back = AIMath::CursorLast(whilePrevious);
			int steps = 0;
			while (back && *back != reachable && steps <= count + 1)
			{
				back = AIMath::CursorPrevious(whilePrevious);
				steps++;
			}
			std::string where = "count " + std::to_string(count) + " reachable " + std::to_string(reachable) + ": ";
			CHECK_MSG(steps <= count, where + "the loop over Previous did not end in time");
			CHECK_MSG(Slot(back) == reachable, where + "the loop over Previous stopped at " + std::to_string(Slot(back)));
		}
	}
}
