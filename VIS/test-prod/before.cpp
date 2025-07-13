// label_layout_slow.cpp  (simplified)
void placeLabels(std::vector<Label>& labels) {
    for (auto& a : labels) {
        for (auto& b : labels) {            // ← N² double loop
            if (&a == &b) continue;
            if (boxesOverlap(a.box, b.box)) {
                nudge(b);                   // shift until they no longer collide
            }
        }
    }
}
