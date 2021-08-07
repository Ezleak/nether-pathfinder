#include "PathFinder.h"
#include "BinaryHeapOpenSet.h"

#include <unordered_map>
#include <memory>
#include <array>
#include <algorithm>
#include <functional>

#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"


template<typename K, typename V>
using map_t = absl::flat_hash_map<K, V>;

// never returns null
PathNode* getNodeAtPosition(map_t<NodePos, std::unique_ptr<PathNode>>& map, const NodePos& pos, const BlockPos& goal) {
    auto iter = map.find(pos);
    if (iter == map.end()) {
        auto node = std::make_unique<PathNode>(pos, goal);
        auto it = map.emplace(pos, std::move(node));
        assert(it.second);
        return it.first->second.get();
    } else {
        return iter->second.get();
    }
}


Path createPath(map_t<NodePos, std::unique_ptr<PathNode>>& map, const PathNode* start, const PathNode* end,
                const BlockPos& startPos, const BlockPos& goal, Path::Type pathType) {
    std::vector<std::unique_ptr<PathNode>> tempNodes;
    std::vector<BlockPos> tempPath;

    const PathNode* current = end;
    while (current) {
        auto& uniqueptr = map.at(current->pos);
        tempNodes.push_back(std::move(uniqueptr));
        tempPath.push_back(current->pos.absolutePosCenter());
        current = current->previous;
    }

    //auto nodes = decltype(tempNodes)(tempPath.rbegin(), tempPath.rend());
    auto nodes = decltype(tempNodes){};
    nodes.reserve(tempNodes.size());
    std::move(tempNodes.rbegin(), tempNodes.rend(), std::back_inserter(nodes));

    auto path = decltype(tempPath){};
    path.reserve(tempPath.size());
    std::move(tempPath.rbegin(), tempPath.rend(), std::back_inserter(path));

    return Path{
            pathType,
            startPos,
            goal,
            std::move(path),
            std::move(nodes)
    };
}

Chunk&
getOrGenChunk(map_t<ChunkPos, std::unique_ptr<Chunk>>& cache, const ChunkPos& pos, const ChunkGeneratorHell& generator,
              ParallelExec& executor, std::mutex& chunkMut) {
    chunkMut.lock();
    auto it = cache.find(pos);
    if (it != cache.end()) {
        chunkMut.unlock();
        return *it->second;
    } else {
        chunkMut.unlock();
        std::unique_ptr ptr = std::make_unique<Chunk>();
        auto& chunk = *ptr;
        generator.generateChunk(pos.x, pos.z, *ptr, executor);
        chunkMut.lock();
        cache.emplace(pos, std::move(ptr));
        chunkMut.unlock();
        return chunk;
    }
}

// This is called inside of a big neighbor cube and returns the 4 sub cubes that are adjacent to the original cube.
// The face argument is relative to the original cube.
// This size argument is the size of the sub cubes.
// origin leans towards north/west/down
template<Face face, Size sz>
std::array<BlockPos, 4> neighborCubes(const BlockPos& origin) {
    // origin leans towards north/west/down
    constexpr auto size = getSize(sz);
    switch (face) {
        case Face::UP: {
            const BlockPos& corner = origin;
            const BlockPos oppositeCorner = origin.east(size).south(size);
            return {corner, corner.east(size), corner.south(size), oppositeCorner};
        }
        case Face::DOWN: {
            const BlockPos corner = origin.up(size);
            const BlockPos oppositeCorner = origin.east(size).south(size);
            return {corner, corner.east(size), corner.south(size), oppositeCorner};
        }
        case Face::NORTH: {
            const BlockPos corner = origin.south(size);
            const BlockPos oppositeCorner = corner.east(size).up(size);
            return {corner, corner.east(size), corner.up(size), oppositeCorner};
        }
        case Face::SOUTH: {
            const BlockPos& corner = origin;
            const BlockPos oppositeCorner = corner.east(size).up(size);
            return {corner, corner.east(size), corner.up(size), oppositeCorner};
        }
        case Face::EAST: {
            const BlockPos& corner = origin;
            const BlockPos oppositeCorner = corner.south(size).up(size);
            return {corner, corner.south(size), corner.up(size), oppositeCorner};
        }
        case Face::WEST: {
            const BlockPos corner = origin.east(size);
            const BlockPos oppositeCorner = corner.south(size).up(size);
            return {corner, corner.south(size), corner.up(size), oppositeCorner};
        }
    }
}


// face is relative to the original cube
template<Face face, Size size, bool sizeChange>
void forEachNeighborInCube(const Chunk& chunk, const NodePos& neighborNode, auto& callback) {
    if constexpr (sizeChange) {
        callback(neighborNode);
        return;
    }
    const auto pos = neighborNode.absolutePosZero();
    if (chunk.isEmpty<size>(pos.x, pos.y, pos.z)) {
        callback(neighborNode);
        return;
    }
    if constexpr (size != Size::X1) {
        constexpr auto nextSize = static_cast<Size>(static_cast<int>(size) - 1);
        // Don't shrink cubes to X1 because they suck and make the path try to squeeze through small areas
        // TODO: allow this to be configurable?
        if constexpr (nextSize == Size::X1) return;
        const std::array subCubes = neighborCubes<face, nextSize>(pos);
        for (const BlockPos& subCube : subCubes) {
            forEachNeighborInCube<face, nextSize, false>(chunk, NodePos{nextSize, subCube}, callback);
        }
    }
}


template<Face face, Size originalSize>
void growThenIterateInner(const Chunk& chunk, const NodePos& pos, auto& callback) {
    const auto bpos = pos.absolutePosZero();

    switch (originalSize) {
        case Size::X1:
            if (!chunk.isEmpty<Size::X2>(bpos.x, bpos.y, bpos.z)) {
                forEachNeighborInCube<face, Size::X1, originalSize != Size::X1>(chunk, pos, callback);
                return;
            }
        case Size::X2:
            if (!chunk.isEmpty<Size::X4>(bpos.x, bpos.y, bpos.z)) {
                forEachNeighborInCube<face, Size::X2, originalSize != Size::X2>(chunk, NodePos{Size::X2, bpos},
                                                                                callback);
                return;
            }
        case Size::X4:
            if (!chunk.isEmpty<Size::X8>(bpos.x, bpos.y, bpos.z)) {
                forEachNeighborInCube<face, Size::X4, originalSize != Size::X4>(chunk, NodePos{Size::X4, bpos},
                                                                                callback);
                return;
            }
        case Size::X8:
            if (!chunk.isEmpty<Size::X16>(bpos.x, bpos.y, bpos.z)) {
                forEachNeighborInCube<face, Size::X8, originalSize != Size::X8>(chunk, NodePos{Size::X8, bpos},
                                                                                callback);
                return;
            }
        case Size::X16:
            forEachNeighborInCube<face, Size::X16, originalSize != Size::X16>(chunk, NodePos{Size::X16, bpos},
                                                                              callback);
    }
}


template<Face face>
void growThenIterateOuter(const Chunk& chunk, const NodePos& pos, auto& callback) {
#define CASE(sz) case sz: growThenIterateInner<face, sz>(chunk, pos, callback); return;
    switch (pos.size) {
        CASE(Size::X1)
        CASE(Size::X2)
        CASE(Size::X4)
        CASE(Size::X8)
        CASE(Size::X16)
    }
#undef CASE
}

bool isInBounds(const BlockPos& pos) {
    return pos.y >= 0 && pos.y < 128;
}

constexpr double MIN_DIST_PATH = 5; // might want to increase this

std::optional<Path>
bestPathSoFar(map_t<NodePos, std::unique_ptr<PathNode>>& map, const PathNode* start, const PathNode* end,
              const BlockPos& startPos, const BlockPos& goal) {
    const double distSq = startPos.distanceToSq(end->pos.absolutePosCenter());

    if (distSq > MIN_DIST_PATH * MIN_DIST_PATH) {
        return createPath(map, start, end, startPos, goal, Path::Type::SEGMENT);
    } else {
        return std::nullopt;
    }

    // assume there's always a way
    //return createPath(map, start, end, startPos, goal, Path::Type::SEGMENT);
}

bool inGoal(const NodePos& node, const BlockPos& goal) {
    // TODO: test if goal is in cube
    return node.absolutePosCenter().distanceToSq(goal) <= 16 * 16;
}

std::optional<Path> findPath0(const BlockPos& start, const BlockPos& goal, const ChunkGeneratorHell& gen) {

    map_t<ChunkPos, std::unique_ptr<Chunk>> chunkCache;
    map_t<NodePos, std::unique_ptr<PathNode>> map;
    map_t<ChunkPos, bool> doneFull;
    BinaryHeapOpenSet openSet;

    PathNode* const startNode = getNodeAtPosition(map, NodePos{Size::X1, start}, goal);
    startNode->cost = 0;
    startNode->combinedCost = startNode->estimatedCostToGoal;
    openSet.insert(startNode);
    std::mutex chunkMutRaw;
    std::mutex& chunkMut = std::ref(chunkMutRaw);
    ParallelExec executors[4];
    ParallelExecutor<4> topExecutor;
    getOrGenChunk(chunkCache, start.toChunkPos(), gen, executors[0], chunkMut);

    PathNode* bestSoFar = startNode;
    double bestHeuristicSoFar = startNode->estimatedCostToGoal;

    using namespace std::chrono_literals;
    const auto startTime = std::chrono::system_clock::now();
    const auto primaryTimeoutTime = startTime + 500ms;
    const auto failureTimeout = startTime + 30s;

    bool failing = true;
    int numNodes = 0;
    while (!openSet.isEmpty()) {
        constexpr int timeCheckInterval = 1 << 6;
        if ((numNodes & (timeCheckInterval - 1)) == 0) { // only call this once every 64 nodes
            auto now = std::chrono::system_clock::now();

            if (now >= failureTimeout || (!failing && now >= primaryTimeoutTime)) {
                break;
            }
        }

        PathNode* currentNode = openSet.removeLowest();

        // TODO: get the right sub cube
        if (inGoal(currentNode->pos, goal)) {
            return createPath(map, startNode, currentNode, start, goal, Path::Type::FINISHED);
        }
        const auto pos = currentNode->pos;
        const auto size = pos.size;
        const auto bpos = pos.absolutePosZero();
        const ChunkPos cpos = bpos.toChunkPos();
        const Chunk& currentChunk = *chunkCache[cpos];
        const ChunkPos cposNorth = bpos.north(16).toChunkPos();
        const ChunkPos cposSouth = bpos.south(16).toChunkPos();
        const ChunkPos cposEast = bpos.east(16).toChunkPos();
        const ChunkPos cposWest = bpos.west(16).toChunkPos();
        if (!doneFull.count(cpos)) {
            topExecutor.compute(
                    [&] {
                        return getOrGenChunk(chunkCache, cposNorth, gen, executors[0], chunkMut);
                    },
                    [&] {
                        return getOrGenChunk(chunkCache, cposSouth, gen, executors[1], chunkMut);
                    },
                    [&] {
                        return getOrGenChunk(chunkCache, cposEast, gen, executors[2], chunkMut);
                    },
                    [&] {
                        return getOrGenChunk(chunkCache, cposWest, gen, executors[3], chunkMut);
                    }
            );
            doneFull.emplace(cpos, true);
        }
        const Chunk& north = *chunkCache[cposNorth];
        const Chunk& south = *chunkCache[cposSouth];
        const Chunk& east = *chunkCache[cposEast];
        const Chunk& west = *chunkCache[cposWest];

        auto callback = [&](const NodePos& neighborPos) {
            PathNode* neighborNode = getNodeAtPosition(map, neighborPos, goal);
            auto sqrtSize = [](Size sz) { return sqrt(getSize(sz)); };
            const double cost = 1;//sqrtSize(neighborNode->pos.size);//getSize(neighborNode->pos.size);
            const double tentativeCost = currentNode->cost + cost;
            constexpr double MIN_IMPROVEMENT = 0.01;
            if (neighborNode->cost - tentativeCost > MIN_IMPROVEMENT) {
                neighborNode->previous = currentNode;
                neighborNode->cost = tentativeCost;
                neighborNode->combinedCost = tentativeCost + neighborNode->estimatedCostToGoal;

                if (neighborNode->isOpen()) {
                    openSet.update(neighborNode);
                } else {
                    openSet.insert(neighborNode);//dont double count, dont insert into open set if it's already there
                }

                const double heuristic = neighborNode->combinedCost;
                if (bestHeuristicSoFar - heuristic > MIN_IMPROVEMENT) {
                    bestHeuristicSoFar = heuristic;
                    bestSoFar = neighborNode;

                    if (failing &&
                        start.distanceToSq(neighborPos.absolutePosCenter()) > MIN_DIST_PATH * MIN_DIST_PATH) {
                        failing = false;
                    }
                }
            }
        };

        [&]<size_t... I>(std::index_sequence<I...>) {
            ([&] {
                constexpr Face face = ALL_FACES[I];
                const NodePos neighborNodePos{size, bpos.offset(face, getSize(size))};
                const BlockPos origin = neighborNodePos.absolutePosZero();
                if constexpr (face == Face::UP || face == Face::DOWN) {
                    if (!isInBounds(origin)) return;
                }
                const ChunkPos neighborCpos = origin.toChunkPos();
                const Chunk& chunk =
                        face == Face::UP || face == Face::DOWN ? currentChunk :
                        face == Face::NORTH ? neighborCpos == cpos ? currentChunk : north :
                        face == Face::SOUTH ? neighborCpos == cpos ? currentChunk : south :
                        face == Face::EAST ? neighborCpos == cpos ? currentChunk : east :
                        /* face == Face::WEST */ neighborCpos == cpos ? currentChunk : west;

                growThenIterateOuter<face>(chunk, neighborNodePos, callback);
            }(), ...);
        }(std::make_index_sequence<ALL_FACES.size()>{});
    }

    return bestPathSoFar(map, startNode, bestSoFar, start, goal);
}

void appendPath(Path& path, Path&& segment) {
    path.blocks.insert(path.blocks.end(), segment.blocks.begin(), segment.blocks.end());
    path.nodes.insert(path.nodes.end(), std::move_iterator{segment.nodes.begin()},
                      std::move_iterator{segment.nodes.end()});
}

Path splicePaths(std::vector<Path>&& paths) {
    Path path = std::move(paths.at(0));

    std::for_each(paths.begin() + 1, paths.end(), [&path](Path& segment) {
        appendPath(path, std::move(segment));
    });

    return path;
}

std::optional<Path> findPath(const BlockPos& start, const BlockPos& goal, const ChunkGeneratorHell& gen) {
    if (!isInBounds(start)) throw "troll";

    std::vector<Path> segments;

    while (true) {
        const BlockPos lastPathEnd = !segments.empty() ? segments.back().getEndPos() : start;
        std::optional path = findPath0(lastPathEnd, goal, gen);
        if (!path.has_value()) {
            break;
        } else {
            const bool finished = path->type == Path::Type::FINISHED;
            segments.push_back(std::move(*path));
            if (finished) break;
        }
    }

    if (!segments.empty()) {
        return splicePaths(std::move(segments));
    } else {
        return std::nullopt;
    }
}