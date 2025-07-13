/*======================================================================*
 |  arena.hpp – fixed-size tile allocator                               |
 *======================================================================*/
#pragma once
#include <cstddef>
#include <memory_resource>
#include <vector>
#include <new>
#include <cassert>

namespace vis::mem {

// Assume all LiDAR / imagery tiles are exactly 128 KiB.
constexpr std::size_t kTileBytes = 128 * 1024;
constexpr std::size_t kCacheLine  = 64;

/// Simple bump-pointer arena backed by a monotonic_buffer_resource.
/// One arena block (default = 8 MiB) holds 64 tiles → avoids heap churn.
class TileArena {
public:
    explicit TileArena(std::size_t block_bytes = 8 * 1024 * 1024)
        : upstream_(block_bytes), pool_(&upstream_) {}

    void* allocate() noexcept {
        return pool_.allocate(kTileBytes, kCacheLine);
    }
    void deallocate(void* /*p*/) noexcept {
        // No-op: full arena freed on destruction.
    }

private:
    std::pmr::monotonic_buffer_resource upstream_;
    std::pmr::memory_resource          pool_;
};

} // namespace vis::mem
