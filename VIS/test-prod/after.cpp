// label_layout_fast.cpp
#include <boost/geometry/index/rtree.hpp>

using Box   = boost::geometry::model::box<Point>;
using Value = std::pair<Box, std::size_t>;          // (bounding box, label-id)
namespace bgi  = boost::geometry::index;

// 1. Build an R-tree once per tile  ───────────────────────────────
bgi::rtree<Value, bgi::quadratic<16>> buildSpatialIndex(
        const std::vector<Label>& labels)
{
    std::vector<Value> items;
    items.reserve(labels.size());
    for (std::size_t i = 0; i < labels.size(); ++i)
        items.emplace_back(labels[i].box, i);
    return bgi::rtree<Value, bgi::quadratic<16>>(items);
}

// 2. Sweep from left → right, querying only *nearby* boxes ─────────
void placeLabelsFast(std::vector<Label>& labels)
{
    auto rtree = buildSpatialIndex(labels);

    std::sort(labels.begin(), labels.end(),
              [](const Label& a, const Label& b) { return a.box.min_corner().x() <
                                                          b.box.min_corner().x(); });

    for (auto& lbl : labels) {
        std::vector<Value> hits;
        rtree.query(bgi::intersects(lbl.box), std::back_inserter(hits));

        for (auto& [box, id] : hits) {
            if (&lbl == &labels[id]) continue;
            if (boxesOverlap(lbl.box, box))
                nudge(lbl);                       // minimal collisions to resolve
        }

        rtree.insert({lbl.box, &lbl - &labels[0]});
    }
}
