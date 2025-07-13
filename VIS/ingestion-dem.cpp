/*======================================================================*
 |  ingest_demo.cpp – wiring it together                                |
 *======================================================================*/
#include "arena.hpp"
#include "ring_queue.hpp"
#include <thread>
#include <vector>
#include <iostream>

using namespace vis;

struct Tile {
    std::byte bytes[mem::kTileBytes];
};

int main() {
    mem::TileArena                 arena;
    concur::RingQueue<Tile*, 1 << 14> queue; // 16 384-slot queue
    constexpr int kProducers = 4;

    // ----- producer threads: read raw tiles from disk ---------------
    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&] {
            for (int i = 0; i < 10'000; ++i) {
                auto* tile = static_cast<Tile*>(arena.allocate());
                /* …fill tile->bytes with data… */
                while (!queue.push(tile)) {/* spin until space */}
            }
        });
    }

    // ----- consumer thread: encode + write to PostGIS ---------------
    std::thread consumer([&] {
        Tile* tile;
        while (true) {
            if (queue.pop(tile)) {
                /* encode + insert tile */
            }
        }
    });

    for (auto& t : producers) t.join();
    consumer.detach();
    std::cout << "Ingestion pipeline exited cleanly\n";
}
