// -- amie_rtree.cpp -------------------------------------------------
#include "postgres.h"
#include "fmgr.h"
#include <immintrin.h>          // AVX2 intrinsics 🔥
#include <cstdint>
#include <array>

/* ----- 1. Morton-key (Z-curve) encoder --------------------------- */
static inline uint64_t morton2D(uint32_t x, uint32_t y) noexcept {
    auto part = [](uint32_t v) {
        v = (v | (v << 16)) & 0x0000FFFF0000FFFFu;
        v = (v | (v <<  8)) & 0x00FF00FF00FF00FFu;
        v = (v | (v <<  4)) & 0x0F0F0F0F0F0F0F0Fu;
        v = (v | (v <<  2)) & 0x3333333333333333u;
        v = (v | (v <<  1)) & 0x5555555555555555u;
        return v;
    };
    return (part(y) << 1) | part(x);
}

/* ----- 2. Hilbert-bulk-load R-tree node -------------------------- */
struct Node {
    std::array<BOX3D, 32> mbrs;        // 32-ary Quadratic split
    std::array<uint64_t, 32> keys;     // Z-curve spatial keys
    int count = 0;
};

PG_FUNCTION_INFO_V1(amie_rtree_consistent);

Datum
amie_rtree_consistent(PG_FUNCTION_ARGS)
{
    auto *query = (BOX3D*) PG_GETARG_POINTER(0);
    auto *node  = (Node*)  PG_GETARG_POINTER(1);

    /* ----- 3. SIMD AVX2 intersection test, 8 boxes per batch ----- */
    __m256d qminx = _mm256_set1_pd(query->xmin);
    __m256d qmaxx = _mm256_set1_pd(query->xmax);
    /* … repeat for y/z … */

    bool hit = false;
    for (int i = 0; i < node->count; i += 4) {
        __m256d nminx = _mm256_load_pd(&node->mbrs[i].xmin);
        __m256d nmaxx = _mm256_load_pd(&node->mbrs[i].xmax);
        auto xok = _mm256_cmp_pd(qmaxx, nminx, _CMP_GE_OQ) &
                   _mm256_cmp_pd(nmaxx, qminx, _CMP_GE_OQ);
        /* … y & z … */
        if (_mm256_movemask_pd(xok /* & y & z */)) { hit = true; break; }
    }
    PG_RETURN_BOOL(hit);
}
