// =====================================================================
//  Data Lab                                         [PL CO4, CO5]
//  The data structures the simulator runs on, animated with the
//  program's own data:
//   * Hashing   - the compiler's symbols + assembler labels inserted with
//                 linear probing / quadratic probing / chaining
//   * Sorting   - the profiler's instruction counts (how often each
//                 mnemonic executed) sorted by bubble / insertion / quick
//   * Searching - the values in data memory searched with linear /
//                 binary / Fibonacci search
// =====================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/gfx/Scene3D.h"
#include "app/Views.h"

namespace vas::ui {

using namespace gfx;
namespace D = gfx::draw;

static void barCompare(const Rect& r, const std::vector<std::pair<std::string, int>>& v, int highlight, const Color& c,
                       const std::string& unit) {
    int mx = 1;
    for (auto& p : v) mx = std::max(mx, p.second);
    float rowH = std::min(30.f, r.h / std::max<std::size_t>(1, v.size()));
    for (int i = 0; i < int(v.size()); ++i) {
        Rect row(r.x, r.y + i * rowH, r.w, rowH - 6);
        D::text(Face::Semibold, 12, row.x, row.y + (row.h - 14) / 2, v[i].first, i == highlight ? theme::text : theme::textDim);
        Rect bar(row.x + 110, row.y, (row.w - 170) * v[i].second / mx, row.h);
        Color col = i == highlight ? c : theme::textMute;
        D::roundRectGrad(Rect(row.x + 110, row.y, row.w - 170, row.h), 5, theme::bg1, theme::bg1);
        D::roundRectGrad(bar, 5, col.alpha(0.75f), col.alpha(0.45f));
        D::text(Face::MonoBold, 12, row.r(), row.y + (row.h - 14) / 2, std::to_string(v[i].second) + " " + unit,
                i == highlight ? theme::text : theme::textMute, Align::Right);
    }
}

void DataLabView::draw(const Rect& area, Sim& sim, UI& ui) {
    Rect a = area;
    Rect head = a.cutTop(66);
    a.cutTop(theme::gap);
    D::shadow(head, 14, 14, Color(0, 0, 0, 1));
    D::roundRectGrad(head, 14, theme::panel.lighten(0.03f), theme::panel);
    D::roundRectOutline(head, 14, theme::border);
    int nm = ui.segmented("dlMode", Rect(head.x + 14, head.y + 15, 420, 36), {"Hashing", "Sorting", "Searching"}, mode_,
                          theme::pink);
    if (nm != mode_) mode_ = nm;
    const char* blurb[3] = {
        "Symbol & label tables of the compiler and assembler - collision handling compared live (PL CO5)",
        "The profiler counted how often each instruction ran - sorted with three algorithms (PL CO4)",
        "Values in data memory searched with linear, binary and Fibonacci search (PL CO4)"};
    D::textIn(Face::Regular, 13, Rect(head.x + 450, head.y, head.w - 460, head.h), blurb[mode_], theme::textDim);

    if (mode_ == 0) drawHashing(a, sim, ui);
    else if (mode_ == 1) drawSorting(a, sim, ui);
    else drawSearching(a, sim, ui);
}

// ================================================================= hashing
void DataLabView::drawHashing(const Rect& area, Sim& sim, UI& ui) {
    // key set: the program's own symbols and labels, topped up with filler names
    std::vector<std::string> keys;
    if (sim.compiledOk) for (const auto& s : sim.compiled.symbols) keys.push_back(s.name);
    if (sim.program) for (const auto& l : sim.program->labels) keys.push_back(l.first);
    if (sim.program && sim.asmMode) for (const auto& d : sim.program->data) keys.push_back(d.name);
    const char* fillerNames[] = {"count", "total", "i", "n", "max", "temp", "flag", "avg", "x", "y", "key", "ptr"};
    for (const char* d : fillerNames) {
        if (keys.size() >= 11) break;
        if (std::find(keys.begin(), keys.end(), d) == keys.end()) keys.push_back(d);
    }
    std::string key = std::to_string(hashMethod_);
    for (auto& k : keys) key += "|" + k;
    if (key != hashKey_) {
        hashKey_ = key;
        keys_ = keys;
        table_.reset(ds::makeHashTable(ds::HashMethod(hashMethod_), 7));
        traces_.clear();
        snapshots_.clear();
        snapshots_.push_back(table_->buckets());
        for (int i = 0; i < int(keys_.size()); ++i) {
            ds::ProbeTrace t;
            table_->insert(keys_[i], i, &t);
            traces_.push_back(t);
            snapshots_.push_back(table_->buckets());
        }
        hashShown_ = 0;
        hashT_ = 0;
    }
    const float perKey = 1.8f;
    hashT_ += ui.dt * std::max(0.5f, sim.speed) / perKey;
    if (hashT_ >= 1 && hashShown_ < int(keys_.size())) { hashT_ = 0; ++hashShown_; }
    int cur = std::min(hashShown_, int(keys_.size()) - 1);
    bool doneAll = hashShown_ >= int(keys_.size());
    float t = doneAll ? 1.f : hashT_;

    Rect a = area;
    Rect left = a.cutLeft(210);
    a.cutLeft(theme::gap);
    Rect right = a.cutRight(std::min(390.f, a.w * 0.36f));
    a.cutRight(theme::gap);

    // --- key queue
    Rect qc = ui.card(left, "KEYS TO INSERT", theme::pink, std::to_string(keys_.size()));
    for (int i = 0; i < int(keys_.size()); ++i) {
        Rect kr(qc.x, qc.y + i * 32, qc.w, 28);
        if (kr.b() > qc.b()) break;
        Color col = i < hashShown_ ? theme::green : (i == cur && !doneAll ? theme::pink : theme::textMute);
        D::roundRect(kr, 7, col.alpha(i == cur && !doneAll ? 0.2f : 0.07f));
        D::roundRectOutline(kr, 7, col.alpha(0.5f));
        D::textIn(Face::MonoBold, 13, kr, keys_[i], i < hashShown_ ? theme::textDim : theme::text, Align::Left, 10);
        if (i < hashShown_) D::icon(Icon::Check, Rect(kr.r() - 24, kr.y + 6, 16, 16), theme::green);
    }
    Rect ctl(qc.x, qc.b() - 34, qc.w, 34);
    if (ui.button("hReplay", ctl, "Replay", Style::Secondary, int(Icon::Reset))) { hashShown_ = 0; hashT_ = 0; }

    // --- table
    const auto& before = snapshots_[std::min(int(snapshots_.size()) - 1, cur)];
    const auto& after = snapshots_[std::min(int(snapshots_.size()) - 1, cur + 1)];
    bool settled = doneAll || t > 0.82f;
    const auto& buckets = settled ? after : before;
    int cap = int(buckets.size());
    std::string title = std::string("HASH TABLE  \xC2\xB7  ") + table_->name() + "  \xC2\xB7  m = " + std::to_string(cap);
    Rect tc = ui.card(a, title, theme::pink);
    int nh = ui.segmented("hMethod", Rect(tc.r() - 330, tc.y - 40 + 4, 330, 30), {"Linear", "Quadratic", "Chaining"},
                          hashMethod_, theme::pink);
    if (nh != hashMethod_) hashMethod_ = nh;
    int cols = cap > 13 ? 2 : 1;
    int perCol = (cap + cols - 1) / cols;
    float rowH = std::min(40.f, tc.h / perCol);
    float colW = tc.w / cols;
    auto slotRect = [&](int i) {
        int col = i / perCol, row = i % perCol;
        return Rect(tc.x + col * colW, tc.y + row * rowH, colW - 10, rowH - 6);
    };
    const ds::ProbeTrace* tr = (!doneAll && cur < int(traces_.size())) ? &traces_[cur] : nullptr;
    // probe progress: hops happen between t = 0.25 and 0.8
    int nProbe = tr ? int(tr->probes.size()) : 0;
    float hopT = clampf((t - 0.25f) / 0.55f, 0, 1) * nProbe;
    int hop = std::min(nProbe - 1, int(hopT));
    for (int i = 0; i < cap; ++i) {
        Rect sr = slotRect(i);
        D::roundRect(sr, 8, theme::bg1);
        D::roundRectOutline(sr, 8, theme::border);
        D::textIn(Face::MonoBold, 12, Rect(sr.x, sr.y, 34, sr.h), std::to_string(i), theme::textMute, Align::Center);
        D::rect(Rect(sr.x + 34, sr.y + 6, 1, sr.h - 12), theme::border);
        if (tr && !settled && tr->capacity == cap) {
            for (int k = 0; k <= hop && k < nProbe; ++k) {
                if (tr->probes[k] != i) continue;
                bool last = k == nProbe - 1;
                Color pc = last ? theme::green : theme::red;
                float fresh = k == hop ? 1.f : 0.4f;
                D::roundRect(sr, 8, pc.alpha(0.12f + 0.15f * fresh));
                D::roundRectOutline(sr, 8, pc.alpha(0.5f + 0.5f * fresh), 1.5f);
                if (!last) D::text(Face::Bold, 10, sr.r() - 8, sr.y + 4, "collision", theme::red, Align::Right);
            }
        }
        float x = sr.x + 44;
        for (int k = 0; k < int(buckets[i].size()); ++k) {
            const std::string& kk = buckets[i][k].first;
            float w = D::textWidth(Face::MonoBold, 13, kk) + 22;
            if (k > 0) {
                D::line(Vec2(x - 12, sr.center().y), Vec2(x - 3, sr.center().y), theme::textMute, 1.5f);
                D::arrowHead(Vec2(x - 1, sr.center().y), Vec2(1, 0), 6, theme::textMute);
            }
            bool fresh = tr && kk == tr->key && settled;
            Rect kr(x, sr.y + 4, w, sr.h - 8);
            if (fresh) D::glow(kr, 7, 8, theme::green);
            D::roundRectGrad(kr, 7, (fresh ? theme::green : theme::pink).alpha(0.3f), (fresh ? theme::green : theme::pink).alpha(0.15f));
            D::roundRectOutline(kr, 7, (fresh ? theme::green : theme::pink).alpha(0.7f));
            D::textIn(Face::MonoBold, 13, kr, kk, theme::text, Align::Center);
            x += w + 16;
        }
    }
    // flying key card following the probe sequence
    if (tr && !settled && nProbe > 0 && tr->capacity == cap) {
        Rect from = slotRect(tr->probes[std::max(0, hop - 1)]);
        Rect to = slotRect(tr->probes[hop]);
        float f = hop == 0 ? easeOutCubic(clampf(t / 0.25f, 0, 1)) : easeInOutCubic(hopT - hop);
        Vec2 start = hop == 0 ? Vec2(tc.x - 40, tc.y - 20) : Vec2(from.r() - 120, from.y);
        Vec2 end(to.r() - 120, to.y);
        Vec2 p = start + (end - start) * f;
        float w = D::textWidth(Face::MonoBold, 14, tr->key) + 26;
        Rect kr(p.x, p.y + 3, w, std::min(rowH - 12, 30.f));
        D::shadow(kr, 8, 10, Color(0, 0, 0, 1));
        D::roundRectGrad(kr, 8, theme::pink.lighten(0.15f), theme::pink);
        D::textIn(Face::MonoBold, 14, kr, tr->key, theme::onAccent, Align::Center);
    }

    // --- explanation
    Rect r1 = right.cutTop(right.h * 0.52f);
    right.cutTop(theme::gap);
    Rect ec = ui.card(r1, "HASH COMPUTATION", theme::pink);
    D::text(Face::Regular, 12, ec.x, ec.y, "h(k) = (k[0]*31^(n-1) + ... + k[n-1])  mod  m", theme::textDim);
    D::text(Face::Regular, 12, ec.x, ec.y + 20, table_->formula(), theme::textDim);
    float y = ec.y + 50;
    if (tr) {
        char b[160];
        std::snprintf(b, sizeof b, "h(\"%s\") = %llu", tr->key.c_str(), static_cast<unsigned long long>(tr->rawHash));
        D::text(Face::MonoBold, 14, ec.x, y, D::ellipsize(Face::MonoBold, 14, b, ec.w), theme::pink.lighten(0.3f));
        std::snprintf(b, sizeof b, "%llu mod %d = %d", static_cast<unsigned long long>(tr->rawHash), tr->capacity, tr->home);
        D::text(Face::MonoBold, 14, ec.x, y + 24, b, theme::text);
        y += 58;
        if (tr->rehashed) {
            D::roundRect(Rect(ec.x, y, ec.w, 26), 6, theme::amber.alpha(0.15f));
            D::textIn(Face::Semibold, 12, Rect(ec.x, y, ec.w, 26),
                      "Load factor too high - rehashed " + std::to_string(tr->oldCapacity) + " \xE2\x86\x92 " +
                          std::to_string(tr->capacity) + " slots",
                      theme::amber, Align::Left, 8);
            y += 34;
        }
        float x = ec.x;
        for (int k = 0; k < nProbe; ++k) {
            bool last = k == nProbe - 1;
            std::string s = "slot " + std::to_string(tr->probes[k]);
            float w = D::textWidth(Face::MonoBold, 12, s) + 32;
            if (x + w > ec.r()) { x = ec.x; y += 30; }
            Color pc = last ? theme::green : theme::red;
            bool reached = k <= hop || settled;
            D::roundRect(Rect(x, y, w, 24), 6, pc.alpha(reached ? 0.2f : 0.05f));
            D::textIn(Face::MonoBold, 12, Rect(x, y, w, 24), s, reached ? pc.lighten(0.3f) : theme::textMute, Align::Left, 8);
            D::icon(last ? Icon::Check : Icon::Cross, Rect(x + w - 20, y + 5, 14, 14), reached ? pc : theme::textMute);
            x += w + 6;
        }
        y += 34;
    } else {
        D::text(Face::Semibold, 13, ec.x, y, "All keys inserted.", theme::green);
        y += 30;
    }
    char st[160];
    std::snprintf(st, sizeof st, "final table:  load factor %.2f   \xC2\xB7   %d probes   \xC2\xB7   %d collisions", table_->loadFactor(),
                  table_->totalProbes(), table_->collisions());
    D::text(Face::Regular, 12, ec.x, ec.b() - 16, st, theme::textMute);

    // --- comparison of the three methods on the same keys
    Rect cc = ui.card(right, "WHICH METHOD IS BEST HERE?", theme::green, "total probes");
    std::vector<std::pair<std::string, int>> cmp;
    for (int m = 0; m < 3; ++m) {
        std::unique_ptr<ds::HashTable> h(ds::makeHashTable(ds::HashMethod(m), 7));
        for (int i = 0; i < int(keys_.size()); ++i) h->insert(keys_[i], i);
        for (const auto& k : keys_) { int v; h->find(k, v); }
        cmp.push_back({h->name(), h->totalProbes()});
    }
    barCompare(Rect(cc.x, cc.y, cc.w, std::min(cc.h - 40, 110.f)), cmp, hashMethod_, theme::pink, "");
    D::paragraph(Face::Regular, 11, Rect(cc.x, cc.b() - 34, cc.w, 34),
                 "Probes for inserting and then looking up every key - fewer probes means faster symbol lookups.",
                 theme::textMute);
}

// ================================================================= sorting
void DataLabView::drawSorting(const Rect& area, Sim& sim, UI& ui) {
    std::vector<std::string> labels;
    std::vector<int> data;
    std::string source;
    auto counts = sim.profiler.counts();
    if (counts.size() >= 3) {
        for (auto& c : counts) { labels.push_back(c.first); data.push_back(c.second); }
        source = "instruction counts from the profiler (CPU observer)";
    } else if (sim.program && sim.program->data.size() >= 3) {
        for (auto& d : sim.program->data) { labels.push_back(d.name); data.push_back(int(sim.cpu.peek(d.addr))); }
        source = "values of the program's variables";
    } else {
        labels = {"mov", "add", "cmp", "jle", "imul", "sub", "jmp", "hlt", "push", "pop"};
        data = {14, 6, 9, 9, 4, 5, 8, 1, 3, 3};
        source = "sample data - run a program to sort its real instruction counts";
    }
    std::string key = std::to_string(sortAlgo_);
    for (std::size_t i = 0; i < data.size(); ++i) key += "|" + labels[i] + ":" + std::to_string(data[i]);
    std::unique_ptr<ds::Sorter> algos[3] = {std::make_unique<ds::BubbleSort>(), std::make_unique<ds::InsertionSort>(),
                                            std::make_unique<ds::QuickSort>()};
    if (key != sortKey_) {
        sortKey_ = key;
        sortLabels_ = labels;
        sortData_ = data;
        sortWork_ = data;
        sortSteps_.clear();
        std::vector<int> tmp = data;
        algos[sortAlgo_]->sort(tmp, sortSteps_);
        sortPos_ = 0;
        sortT_ = 0;
        sortPlaying_ = true;
    }
    const float stepDur = 0.32f / sortSpeed_;
    if (sortPlaying_ && sortPos_ < int(sortSteps_.size())) {
        sortT_ += ui.dt;
        while (sortT_ >= stepDur && sortPos_ < int(sortSteps_.size())) { sortT_ -= stepDur; ++sortPos_; }
    }
    // replay the steps up to sortPos_ to know the current arrangement
    std::vector<int> arr = sortData_;
    std::vector<int> idx(arr.size());
    for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = int(i);
    std::vector<char> sorted(arr.size(), 0);
    int comps = 0, swaps = 0, pivot = -1;
    ds::SortStep curStep{ds::SortStep::Done};
    for (int k = 0; k < sortPos_ && k < int(sortSteps_.size()); ++k) {
        const auto& s = sortSteps_[k];
        if (s.type == ds::SortStep::Swap) { std::swap(arr[s.i], arr[s.j]); std::swap(idx[s.i], idx[s.j]); ++swaps; }
        if (s.type == ds::SortStep::Compare) ++comps;
        if (s.type == ds::SortStep::Sorted && s.i >= 0) sorted[s.i] = 1;
        if (s.type == ds::SortStep::Pivot) pivot = s.i;
        curStep = s;
    }
    bool finished = sortPos_ >= int(sortSteps_.size());
    if (finished) std::fill(sorted.begin(), sorted.end(), 1);

    Rect a = area;
    Rect right = a.cutRight(std::min(380.f, a.w * 0.34f));
    a.cutRight(theme::gap);
    Rect c = ui.card(a, "SORTING  \xC2\xB7  " + algos[sortAlgo_]->name(), theme::amber, source);
    Rect ctl = c.cutTop(36);
    c.cutTop(12);
    int na = ui.segmented("sAlgo", Rect(ctl.x, ctl.y, 380, 34), {"Bubble", "Insertion", "Quick"}, sortAlgo_, theme::amber);
    if (na != sortAlgo_) { sortAlgo_ = na; sortKey_.clear(); }
    float bx = ctl.x + 396;
    if (ui.iconButton("sPlay", Rect(bx, ctl.y, 36, 34), sortPlaying_ && !finished ? Icon::Pause : Icon::Play, "Play / pause")) {
        if (finished) { sortPos_ = 0; sortT_ = 0; sortPlaying_ = true; }
        else sortPlaying_ = !sortPlaying_;
    }
    if (ui.iconButton("sStep", Rect(bx + 40, ctl.y, 36, 34), Icon::Step, "Next step")) {
        sortPlaying_ = false;
        if (sortPos_ < int(sortSteps_.size())) ++sortPos_;
    }
    if (ui.iconButton("sRestart", Rect(bx + 80, ctl.y, 36, 34), Icon::Reset, "Restart")) { sortPos_ = 0; sortT_ = 0; sortPlaying_ = true; }
    ui.slider("sSpeed", Rect(bx + 132, ctl.y + 5, 120, 24), sortSpeed_, 0.25f, 6.f);

    // bars
    int n = int(arr.size());
    int mx = 1;
    for (int v : arr) mx = std::max(mx, v);
    Rect plot = c.inset(-6, -4);
    plot.cutBottom(26);
    float swapT = (curStep.type == ds::SortStep::Swap && !finished) ? easeInOutCubic(sortT_ / stepDur) : 1.f;
    // ---- 3D bar chart: lit columns on a floor; swapped bars slide past each other   [CG CO2, CO3, CO6]
    const float gapX = 1.25f, maxH = 5.0f;
    float spanX = (n - 1) * gapX;
    s3d::Camera cam;
    cam.yaw = -18 + std::sin(ui.time * 0.25f) * 6;
    cam.pitch = 20;
    cam.dist = std::max(11.f, spanX * 1.35f + 4);
    cam.ty = maxH * 0.42f;
    s3d::begin(plot, cam, theme::light);
    s3d::floorQuad(-spanX / 2 - 1.2f, -1.4f, spanX / 2 + 1.2f, 1.4f, 0, theme::border.alpha(0.6f));
    struct Label { Vec2 top, base; int i; Color col; bool hot; };
    std::vector<Label> barLabels;
    for (int i = 0; i < n; ++i) {
        float h = std::max(0.15f, maxH * arr[i] / mx);
        float pos = float(i);
        bool sw = !finished && curStep.type == ds::SortStep::Swap && (i == curStep.i || i == curStep.j);
        float lift = 0;
        if (sw) {
            int other = i == curStep.i ? curStep.j : curStep.i;
            pos = other + (i - other) * swapT;
            lift = std::sin(swapT * float(M_PI)) * (i == curStep.i ? 0.9f : -0.9f);   // pass in front / behind
        }
        float x = -spanX / 2 + pos * gapX;
        Color col = theme::cyan;
        bool cmp = !finished && curStep.type == ds::SortStep::Compare && (i == curStep.i || i == curStep.j);
        if (sorted[i]) col = theme::green;
        if (i == pivot && !finished && !sorted[i]) col = theme::violet;
        if (cmp) col = theme::amber;
        if (sw) col = theme::pink;
        if (cmp || sw) s3d::floorQuad(x - 0.7f, -0.7f + lift, x + 0.7f, 0.7f + lift, 0.01f, col.alpha(0.35f));
        s3d::material(col, (cmp || sw) ? 0.35f : 0.06f);
        s3d::box(x - 0.42f, 0, -0.42f + lift, x + 0.42f, h, 0.42f + lift);
        Label L;
        L.i = i;
        L.col = col;
        L.hot = cmp || sw;
        s3d::project(x, h + 0.35f, lift, L.top);
        s3d::project(x, -0.2f, 1.0f, L.base);
        barLabels.push_back(L);
    }
    s3d::end();
    for (const auto& L : barLabels) {
        D::text(Face::MonoBold, L.hot ? 15 : 13, L.top.x, L.top.y - 18, std::to_string(arr[L.i]),
                L.hot ? L.col : theme::text, Align::Center);
        D::text(Face::MonoBold, 12, L.base.x, L.base.y, D::ellipsize(Face::MonoBold, 12, sortLabels_[idx[L.i]], 70),
                theme::textDim, Align::Center);
    }
    // legend
    float lx = c.x;
    const std::pair<const char*, Color> leg[] = {{"compare", theme::amber}, {"swap", theme::pink}, {"pivot", theme::violet},
                                                 {"in final place", theme::green}};
    for (auto& l : leg) {
        D::roundRect(Rect(lx, c.b() - 12, 10, 10), 3, l.second);
        lx += D::text(Face::Regular, 11, lx + 15, c.b() - 14, l.first, theme::textMute) + 30;
    }

    // stats
    Rect s1 = right.cutTop(200);
    right.cutTop(theme::gap);
    Rect sc = ui.card(s1, "THIS RUN", theme::amber, algos[sortAlgo_]->complexity());
    auto big = [&](float x, const std::string& label, const std::string& v, const Color& col) {
        D::text(Face::Bold, 11, x, sc.y, label, theme::textMute);
        D::text(Face::Bold, 30, x, sc.y + 18, v, col);
    };
    big(sc.x, "COMPARISONS", std::to_string(comps), theme::amber);
    big(sc.x + sc.w / 2, "SWAPS", std::to_string(swaps), theme::pink);
    D::text(Face::Regular, 12, sc.x, sc.y + 70, "step " + std::to_string(sortPos_) + " of " + std::to_string(sortSteps_.size()),
            theme::textDim);
    Rect pb(sc.x, sc.y + 94, sc.w, 6);
    D::roundRect(pb, 3, theme::bg1);
    D::roundRect(Rect(pb.x, pb.y, pb.w * (sortSteps_.empty() ? 0 : float(sortPos_) / sortSteps_.size()), pb.h), 3, theme::amber);
    D::paragraph(Face::Regular, 12, Rect(sc.x, sc.y + 112, sc.w, 50),
                 finished ? "Sorted - the most executed instruction is on the right." : "Bars are instructions; height = times executed.",
                 theme::textDim);

    Rect cc = ui.card(right, "ALL THREE ON THIS DATA", theme::green, "comparisons");
    std::vector<std::pair<std::string, int>> cmpv;
    for (int k = 0; k < 3; ++k) {
        std::vector<int> tmp = sortData_;
        std::vector<ds::SortStep> st;
        algos[k]->sort(tmp, st);
        int cnt = 0;
        for (auto& s : st) if (s.type == ds::SortStep::Compare) ++cnt;
        cmpv.push_back({algos[k]->name(), cnt});
    }
    barCompare(Rect(cc.x, cc.y, cc.w, std::min(cc.h, 110.f)), cmpv, sortAlgo_, theme::amber, "");
}

// =============================================================== searching
void DataLabView::drawSearching(const Rect& area, Sim& sim, UI& ui) {
    std::vector<int> data;
    std::vector<std::string> labels;
    if (sim.program)
        for (auto& d : sim.program->data) { data.push_back(int(sim.cpu.peek(d.addr))); labels.push_back(d.name); }
    const int filler[] = {3, 8, 12, 17, 21, 26, 34, 42, 55, 63, 71, 88, 97};
    for (int f : filler) {
        if (data.size() >= 15) break;
        if (std::find(data.begin(), data.end(), f) == data.end()) { data.push_back(f); labels.push_back(""); }
    }
    std::unique_ptr<ds::Searcher> algos[3] = {std::make_unique<ds::LinearSearch>(), std::make_unique<ds::BinarySearch>(),
                                              std::make_unique<ds::FibonacciSearch>()};
    bool needSorted = algos[searchAlgo_]->needsSorted();
    // keep labels attached while sorting (quick sort on pairs)                [PL CO4]
    std::vector<std::pair<int, std::string>> pairs;
    for (std::size_t i = 0; i < data.size(); ++i) pairs.push_back({data[i], labels[i]});
    if (needSorted) ds::quickSort(pairs, [](const auto& x, const auto& y) { return x.first < y.first; });
    data.clear();
    labels.clear();
    for (auto& p : pairs) { data.push_back(p.first); labels.push_back(p.second); }

    if (searchKey_.empty() && !data.empty()) {
        // default target: the first program variable's value, if any
        target_ = sim.program && !sim.program->data.empty() ? int(sim.cpu.peek(sim.program->data.back().addr)) : data[data.size() / 2];
    }
    std::string key = std::to_string(searchAlgo_) + "|" + std::to_string(target_);
    for (int v : data) key += "," + std::to_string(v);
    if (key != searchKey_) {
        searchKey_ = key;
        searchData_ = data;
        searchLabels_ = labels;
        searchSteps_.clear();
        algos[searchAlgo_]->search(searchData_, target_, searchSteps_);
        searchPos_ = 0;
        searchT_ = 0;
    }
    searchT_ += ui.dt * std::max(0.5f, sim.speed);
    if (searchT_ > 0.9f && searchPos_ < int(searchSteps_.size())) { searchT_ = 0; ++searchPos_; }
    bool finished = searchPos_ >= int(searchSteps_.size());
    bool found = !searchSteps_.empty() && searchSteps_.back().found;

    Rect a = area;
    Rect right = a.cutRight(std::min(380.f, a.w * 0.34f));
    a.cutRight(theme::gap);
    Rect c = ui.card(a, "SEARCHING  \xC2\xB7  " + algos[searchAlgo_]->name(), theme::cyan,
                     needSorted ? "data sorted first (quick sort)" : "works on unsorted data");
    Rect ctl = c.cutTop(36);
    c.cutTop(16);
    int na = ui.segmented("qAlgo", Rect(ctl.x, ctl.y, 380, 34), {"Linear", "Binary", "Fibonacci"}, searchAlgo_, theme::cyan);
    if (na != searchAlgo_) { searchAlgo_ = na; searchKey_.clear(); }
    int tv = target_;
    if (ui.stepper("qTarget", Rect(ctl.x + 396, ctl.y, 210, 34), tv, -999, 999, "Find")) { target_ = tv; }
    if (ui.iconButton("qReplay", Rect(ctl.x + 620, ctl.y, 36, 34), Icon::Reset, "Replay")) { searchPos_ = 0; searchT_ = 0; }

    int n = int(searchData_.size());
    float cw = std::min(78.f, c.w / std::max(1, n));
    float x0 = c.x + (c.w - cw * n) / 2;
    float y0 = c.y + 60;
    const ds::SearchStep* st = searchPos_ > 0 ? &searchSteps_[searchPos_ - 1] : nullptr;
    for (int i = 0; i < n; ++i) {
        Rect cell(x0 + i * cw + 3, y0, cw - 6, 58);
        bool inRange = !st || (i >= st->lo && i <= st->hi);
        bool probe = st && st->probe == i;
        bool visited = false;
        for (int k = 0; k < searchPos_; ++k) if (searchSteps_[k].probe == i) visited = true;
        Color col = probe ? (st->found ? theme::green : theme::amber) : (visited ? theme::red : theme::cyan);
        float al = inRange ? 1.f : 0.25f;
        if (probe) D::glow(cell, 8, 10, col);
        D::roundRectGrad(cell, 8, col.alpha((probe ? 0.4f : 0.14f) * al), col.alpha((probe ? 0.25f : 0.06f) * al));
        D::roundRectOutline(cell, 8, col.alpha((probe ? 1.f : 0.45f) * al), probe ? 2.f : 1.f);
        D::textIn(Face::MonoBold, 18, cell, std::to_string(searchData_[i]), theme::text.alpha(al), Align::Center);
        D::text(Face::Mono, 10, cell.center().x, y0 - 18, "[" + std::to_string(i) + "]", theme::textMute, Align::Center);
        if (!searchLabels_[i].empty())
            D::text(Face::MonoBold, 11, cell.center().x, cell.b() + 6, D::ellipsize(Face::MonoBold, 11, searchLabels_[i], cw),
                    theme::pink, Align::Center);
        if (ui.clicked(cell)) target_ = searchData_[i];
    }
    if (st) {
        auto marker = [&](int i, const char* lbl, const Color& col, float dy) {
            if (i < 0 || i >= n) return;
            Vec2 tip(x0 + i * cw + cw / 2, y0 + 58 + 26 + dy);
            D::arrowHead(tip - Vec2(0, 2), Vec2(0, -1), 9, col);
            D::text(Face::Bold, 11, tip.x, tip.y + 2, lbl, col, Align::Center);
        };
        if (searchAlgo_ != 0) { marker(st->lo, "lo", theme::textDim, 0); marker(st->hi, "hi", theme::textDim, 0); }
        marker(st->probe, searchAlgo_ == 1 ? "mid" : "probe", theme::amber, 22);
    }
    // step log
    float ly = y0 + 150;
    D::text(Face::Bold, 11, c.x, ly, "STEPS", theme::textMute);
    ly += 20;
    for (int k = 0; k < searchPos_; ++k) {
        const auto& s = searchSteps_[k];
        char b[160];
        if (searchAlgo_ == 0) std::snprintf(b, sizeof b, "%d.  a[%d] = %d  %s", k + 1, s.probe, searchData_[s.probe], s.found ? "== target  FOUND" : "!= target");
        else
            std::snprintf(b, sizeof b, "%d.  range [%d..%d]  probe a[%d] = %d  %s", k + 1, s.lo, s.hi, s.probe,
                          searchData_[s.probe],
                          s.found ? "== target  FOUND" : (searchData_[s.probe] < target_ ? "< target  \xE2\x86\x92 go right" : "> target  \xE2\x86\x92 go left"));
        D::text(Face::Mono, 13, c.x, ly, b, s.found ? theme::green : theme::textDim);
        ly += 22;
        if (ly > c.b() - 10) break;
    }
    if (finished && !found) D::text(Face::Semibold, 14, c.x, ly + 4, std::to_string(target_) + " is not in memory", theme::red);

    Rect s1 = right.cutTop(170);
    right.cutTop(theme::gap);
    Rect sc = ui.card(s1, "RESULT", theme::cyan, algos[searchAlgo_]->complexity());
    D::text(Face::Bold, 11, sc.x, sc.y, "COMPARISONS", theme::textMute);
    D::text(Face::Bold, 34, sc.x, sc.y + 18, std::to_string(searchPos_), theme::cyan);
    std::string res = !finished ? "searching..." : (found ? "found at index " + std::to_string(searchSteps_.back().probe) : "not found");
    D::text(Face::Semibold, 15, sc.x + 120, sc.y + 30, res, finished ? (found ? theme::green : theme::red) : theme::textDim);
    D::paragraph(Face::Regular, 12, Rect(sc.x, sc.y + 74, sc.w, 40), "Click any cell to search for its value.", theme::textMute);

    Rect cc = ui.card(right, "ALL THREE FOR THIS TARGET", theme::green, "comparisons");
    std::vector<std::pair<std::string, int>> cmpv;
    std::vector<int> sortedData;
    for (auto& p : pairs) sortedData.push_back(p.first);
    if (!needSorted) ds::quickSort(sortedData, [](int x, int y) { return x < y; });
    for (int k = 0; k < 3; ++k) {
        std::vector<ds::SearchStep> st2;
        std::vector<int> d = algos[k]->needsSorted() ? sortedData : searchData_;
        algos[k]->search(d, target_, st2);
        cmpv.push_back({algos[k]->name(), int(st2.size())});
    }
    barCompare(Rect(cc.x, cc.y, cc.w, std::min(cc.h - 30, 110.f)), cmpv, searchAlgo_, theme::cyan, "");
    D::paragraph(Face::Regular, 11, Rect(cc.x, cc.b() - 30, cc.w, 30),
                 "Binary and Fibonacci search need sorted data but only ~log2(n) comparisons.", theme::textMute);
}

} // namespace vas::ui
