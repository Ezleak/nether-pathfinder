#pragma once

#include <vector>
#include <optional>
#include <unordered_set>

#include <jni.h>

#include "Utils.h"
#include "ChunkGeneratorHell.h"
#include "PathNode.h"
#include "ChunkGen.h"

enum class FakeChunkMode {
    GENERATE = 0
    ,AIR = 1
    //,SOLID = 2
};

struct Path {
    enum class Type {
        SEGMENT,
        FINISHED
    };

    Type type;
    BlockPos start;
    BlockPos goal; // where the path wants to go, not necessarily where it ends
    std::vector<BlockPos> blocks;
    std::vector<std::unique_ptr<PathNode>> nodes;
    cache_t chunkCache;

    [[nodiscard]] const BlockPos& getEndPos() const {
        // This should basically never be empty
        return !blocks.empty() ? blocks.back() : this->start;
    }
};

struct Context {
    ChunkGeneratorHell generator;
    std::optional<std::string> baritoneCache;
    std::mutex cacheMutex;
    cache_t chunkCache;
    ParallelExecutor<4> topExecutor;
    std::array<ChunkGenExec, 4> executors;
    std::atomic_flag cancelFlag;
    std::unordered_set<RegionPos> checkedRegions;


    explicit Context(int64_t seed): generator(ChunkGeneratorHell::fromSeed(seed)) {}
    explicit Context(int64_t seed, std::string&& cacheDir): generator(ChunkGeneratorHell::fromSeed(seed)), baritoneCache(cacheDir) {}
};

const Chunk& getOrGenChunk(Context& ctx, ChunkGenExec& executor, const ChunkPos& pos, FakeChunkMode fakeChunkMode = FakeChunkMode::GENERATE);

std::optional<Path> findPathFull(Context& ctx, const NodePos& start, const NodePos& goal, double fakeChunkCost);
std::optional<Path> findPathSegment(Context& ctx, const NodePos& start, const NodePos& goal, bool x4Min, int failTimeoutMs, bool airIfFake, double fakeChunkCost);

template<Size size>
NodePos findAir(Context& ctx, const BlockPos& start1x);
