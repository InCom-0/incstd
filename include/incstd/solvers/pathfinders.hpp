#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include <incstd/core/containers.hpp>
#include <incstd/polyfills/mdspan.hpp>
#include <incstd/solvers/directions.hpp>
#include <incstd/solvers/explorers.hpp>


// Jump Point Search family on 8-connected uniform-cost grids (no corner cutting).
// Positions are {row, col}. Internally everything lives in a grid padded with a one cell wall border,
// so that stepping never needs bounds checks. Costs are fixed point (c_costOrth == 1.0).
namespace incom::standard::solvers::pathfinders {
using namespace incom::standard;
using namespace incom::standard::solvers::directions;

using Pos_t  = directions::Pos<2uz>;
using Cost_t = std::uint64_t;
using Idx_t  = std::int32_t;

inline constexpr Cost_t c_costOrth       = 2378;
inline constexpr Cost_t c_costDiag       = 3363;  // 2378 * sqrt(2) within 6e-8 relative error
inline constexpr Cost_t c_costDiagExcess = c_costDiag - c_costOrth;
inline constexpr size_t c_maxExtent      = 32767; // Jump distances are stored as int16_t

static_assert(c_maxExtent * c_maxExtent <=
              (std::numeric_limits<Cost_t>::max() - (2uz * c_maxExtent * c_costDiag)) / c_costOrth);

// ####################################
// ### GRID
// ####################################

// Cell in the padded grid
struct Cell {
    std::uint16_t r;
    std::uint16_t c;

    friend constexpr bool
    operator==(Cell const &, Cell const &) = default;
};

[[nodiscard]] constexpr Cell
advanced(Cell c, Dir d, std::uint32_t steps = 1u) noexcept {
    return Cell{.r = static_cast<std::uint16_t>(static_cast<int>(c.r) + (row_step(d) * static_cast<int>(steps))),
                .c = static_cast<std::uint16_t>(static_cast<int>(c.c) + (col_step(d) * static_cast<int>(steps)))};
}

class Grid {
public:
    enum class Error {
        empty_extents,
        extent_too_large,
        cost_overflow
    };

    // f_walkable is a predicate taking Pos_t const & (same convention as explorers::FloodFill)
    template <typename F_Walkable>
    requires std::predicate<F_Walkable &, Pos_t const &>
    [[nodiscard]] static std::expected<Grid, Error>
    create(Pos_t extents, F_Walkable &&f_walkable) {
        auto const [rows, cols] = extents;
        if (rows == 0uz || cols == 0uz) { return std::unexpected(Error::empty_extents); }
        if (rows > c_maxExtent || cols > c_maxExtent) { return std::unexpected(Error::extent_too_large); }

        Grid res;
        res.m_rows   = rows;
        res.m_cols   = cols;
        res.m_stride = static_cast<Idx_t>(cols + 2uz);
        res.m_walk.assign((rows + 2uz) * (cols + 2uz), std::uint8_t{0});

        std::uint64_t walkableCount{};
        for (size_t r = 0uz; r < rows; ++r) {
            for (size_t c = 0uz; c < cols; ++c) {
                if (f_walkable(Pos_t{r, c})) {
                    res.m_walk[((r + 1uz) * (cols + 2uz)) + c + 1uz] = 1;
                    ++walkableCount;
                }
            }
        }

        res.label_regions();
        return res;
    }

    // Any rank-2 mdspan whose elements are convertible to bool (true == walkable)
    template <typename MDSPAN>
    requires(MDSPAN::rank() == 2uz)
    [[nodiscard]] static std::expected<Grid, Error>
    from_mdspan(MDSPAN const &view) {
        return create(Pos_t{static_cast<size_t>(view.extent(0)), static_cast<size_t>(view.extent(1))},
                      [&view](Pos_t const &p) { return static_cast<bool>(view[p[0], p[1]]); });
    }


    // ### Public coordinates ({row, col}, no padding)

    [[nodiscard]] size_t
    rows() const noexcept {
        return m_rows;
    }

    [[nodiscard]] size_t
    cols() const noexcept {
        return m_cols;
    }

    [[nodiscard]] bool
    in_bounds(Pos_t const &p) const noexcept {
        return p[0] < m_rows && p[1] < m_cols;
    }

    [[nodiscard]] bool
    is_walkable(Pos_t const &p) const noexcept {
        return in_bounds(p) && walk(index(to_cell(p)));
    }

    // Connected component of the position, 0 for non-walkable positions
    [[nodiscard]] std::uint32_t
    region_of(Pos_t const &p) const noexcept {
        return in_bounds(p) ? m_regions[static_cast<size_t>(index(to_cell(p)))] : 0u;
    }

    // Flips one cell and updates the regions locally. Returns false if nothing changed (or p is out of bounds).
    // Region ids stay unique per component but are not stable across calls.
    [[nodiscard]] bool
    set_walkable(Pos_t const &p, bool walkable) {
        if (not in_bounds(p)) { return false; }
        Idx_t const i = index(to_cell(p));
        if (walk(i) == walkable) { return false; }

        m_walk[static_cast<size_t>(i)] = walkable ? std::uint8_t{1} : std::uint8_t{0};
        if (walkable) { region_add(i); }
        else { region_remove(i); }

        // Every split / fresh region burns an id, a full relabel compacts them
        if (m_regionSize.size() > (2uz * m_walk.size()) + 16uz) { label_regions(); }
        return true;
    }


    // ### Padded coordinates (used by the search machinery)

    [[nodiscard]] size_t
    padded_size() const noexcept {
        return m_walk.size();
    }

    [[nodiscard]] Cell
    to_cell(Pos_t const &p) const noexcept {
        return Cell{.r = static_cast<std::uint16_t>(p[0] + 1uz), .c = static_cast<std::uint16_t>(p[1] + 1uz)};
    }

    [[nodiscard]] static constexpr Pos_t
    to_pos(Cell c) noexcept {
        return Pos_t{static_cast<size_t>(c.r) - 1uz, static_cast<size_t>(c.c) - 1uz};
    }

    [[nodiscard]] Idx_t
    index(Cell c) const noexcept {
        return (static_cast<Idx_t>(c.r) * m_stride) + static_cast<Idx_t>(c.c);
    }

    [[nodiscard]] bool
    walk(Idx_t i) const noexcept {
        return m_walk[static_cast<size_t>(i)] != 0;
    }

    [[nodiscard]] bool
    walk(Cell c) const noexcept {
        return walk(index(c));
    }

    // Index deltas of the 8 directions
    [[nodiscard]] std::array<Idx_t, 8>
    offsets() const noexcept {
        std::array<Idx_t, 8> res{};
        for (size_t d = 0uz; d < 8uz; ++d) { res[d] = (c_dRow[d] * m_stride) + c_dCol[d]; }
        return res;
    }

    // Visits indices of walkable cells so that the neighbour in direction d is always visited before the cell
    template <typename F>
    void
    for_each_walkable_toward(Dir d, F &&f) const {
        bool const rowsDesc = row_step(d) > 0;
        bool const colsDesc = col_step(d) > 0;
        for (size_t ri = 0uz; ri < m_rows; ++ri) {
            auto const r = static_cast<Idx_t>(rowsDesc ? m_rows - ri : ri + 1uz);
            for (size_t ci = 0uz; ci < m_cols; ++ci) {
                auto const c   = static_cast<Idx_t>(colsDesc ? m_cols - ci : ci + 1uz);
                auto const idx = (r * m_stride) + c;
                if (walk(idx)) { f(idx); }
            }
        }
    }

private:
    Grid() = default;

    size_t                     m_rows{};
    size_t                     m_cols{};
    Idx_t                      m_stride{};
    std::vector<std::uint8_t>  m_walk;
    std::vector<std::uint32_t> m_regions;
    std::vector<std::uint32_t> m_regionSize; // Indexed by region id, [0] unused
    std::vector<std::uint32_t> m_mark;       // Scratch of split_check, valid when >= m_epoch + 1
    std::uint32_t              m_epoch{};

    // Regions are 4-connected components, which is exactly the reachability under the no corner cutting rule
    static constexpr std::array<size_t, 4uz> c_orth{0uz, 2uz, 4uz, 6uz};

    [[nodiscard]] std::uint32_t
    new_region() {
        m_regionSize.push_back(0u);
        return static_cast<std::uint32_t>(m_regionSize.size() - 1uz);
    }

    // Relabels the component of seed from 'from' to 'to'. Cost is the size of that component.
    std::uint32_t
    relabel(Idx_t seed, std::uint32_t from, std::uint32_t to, std::array<Idx_t, 8> const &off) {
        std::vector<Idx_t> stack{seed};
        m_regions[static_cast<size_t>(seed)] = to;
        std::uint32_t count{};
        while (not stack.empty()) {
            Idx_t const i = stack.back();
            stack.pop_back();
            ++count;
            for (size_t const d : c_orth) {
                Idx_t const n = i + off[d];
                if (m_regions[static_cast<size_t>(n)] == from) {
                    m_regions[static_cast<size_t>(n)] = to;
                    stack.push_back(n);
                }
            }
        }
        return count;
    }

    // Cell i just became walkable: join / merge the regions of its orthogonal neighbours
    void
    region_add(Idx_t i) {
        auto const                     off = offsets();
        std::array<std::uint32_t, 4uz> ids{};
        std::array<Idx_t, 4uz>         seeds{};
        size_t                         count{};
        for (size_t const d : c_orth) {
            Idx_t const         n = i + off[d];
            std::uint32_t const r = m_regions[static_cast<size_t>(n)];
            if (r != 0u && std::find(ids.begin(), ids.begin() + static_cast<std::ptrdiff_t>(count), r) ==
                               ids.begin() + static_cast<std::ptrdiff_t>(count)) {
                ids[count]   = r;
                seeds[count] = n;
                ++count;
            }
        }

        if (count == 0uz) {
            std::uint32_t const fresh         = new_region();
            m_regions[static_cast<size_t>(i)] = fresh;
            m_regionSize[fresh]               = 1u;
            return;
        }

        size_t target = 0uz; // Keep the largest so that only the smaller ones get relabelled
        for (size_t k = 1uz; k < count; ++k) {
            if (m_regionSize[ids[k]] > m_regionSize[ids[target]]) { target = k; }
        }
        std::uint32_t const t             = ids[target];
        m_regions[static_cast<size_t>(i)] = t;
        ++m_regionSize[t];
        for (size_t k = 0uz; k < count; ++k) {
            if (k == target) { continue; }
            m_regionSize[t]      += relabel(seeds[k], ids[k], t, off);
            m_regionSize[ids[k]]  = 0u;
        }
    }

    // Cell i just became blocked: its region may have fallen apart
    void
    region_remove(Idx_t i) {
        std::uint32_t const r             = m_regions[static_cast<size_t>(i)];
        m_regions[static_cast<size_t>(i)] = 0u;
        --m_regionSize[r];

        auto const             off = offsets();
        std::array<Idx_t, 4uz> seeds{};
        size_t                 count{};
        for (size_t const d : c_orth) {
            if (walk(i + off[d])) { seeds[count++] = i + off[d]; }
        }
        if (count < 2uz || ring_connected(i, off)) { return; }
        split_check(seeds, count, r, off);
    }

    // True if all walkable orthogonal neighbours of i are linked through the walkable cells of its 8-ring alone.
    // Consecutive ring cells are 4-adjacent, so that is a single cyclic run of walkable ring cells.
    [[nodiscard]] bool
    ring_connected(Idx_t i, std::array<Idx_t, 8> const &off) const noexcept {
        std::array<bool, 8uz> w{};
        for (size_t k = 0uz; k < 8uz; ++k) { w[k] = walk(i + off[k]); }

        size_t first = 0uz;
        while (not w[first]) { first += 2uz; } // Callers guarantee two walkable orthogonal neighbours

        std::array<bool, 8uz> reached{};
        for (size_t const step : {1uz, 7uz}) {
            size_t k = first;
            for (size_t n = 0uz; n < 8uz && w[k]; ++n, k = (k + step) & 7uz) { reached[k] = true; }
        }
        return std::ranges::all_of(c_orth, [&](size_t k) { return not w[k] || reached[k]; });
    }

    // Lockstep flood from every seed. Floods that touch are the same component. A flood that runs dry while others
    // are still alive is a complete split-off component and gets a fresh id, so the cost is bounded by the smaller
    // side.
    void
    split_check(std::array<Idx_t, 4uz> const &seeds, size_t n, std::uint32_t region, std::array<Idx_t, 8> const &off) {
        if (m_mark.size() != m_walk.size() || m_epoch > std::numeric_limits<std::uint32_t>::max() - 16u) {
            m_mark.assign(m_walk.size(), 0u);
            m_epoch = 0u;
        }
        m_epoch                   += 8u;
        std::uint32_t const epoch  = m_epoch;

        struct Flood {
            std::vector<Idx_t> cells;    // Doubles as the BFS queue
            size_t             head{};
            size_t             parent{}; // Union-find over the flood indices
            bool               done{};
        };
        std::array<Flood, 4uz> fl{};
        for (size_t k = 0uz; k < n; ++k) {
            fl[k].cells                           = {seeds[k]};
            fl[k].parent                          = k;
            m_mark[static_cast<size_t>(seeds[k])] = epoch + 1u + static_cast<std::uint32_t>(k);
        }

        auto const find = [&](size_t k) {
            while (fl[k].parent != k) { k = fl[k].parent; }
            return k;
        };

        size_t groupsLeft = n;
        while (groupsLeft > 1uz) {
            for (size_t k = 0uz; k < n && groupsLeft > 1uz; ++k) {
                Flood &f = fl[k];
                if (f.done || f.head == f.cells.size()) { continue; }

                Idx_t const i = f.cells[f.head++];
                for (size_t const d : c_orth) {
                    Idx_t const nb = i + off[d];
                    if (not walk(nb)) { continue; }
                    std::uint32_t const m = m_mark[static_cast<size_t>(nb)];
                    if (m <= epoch) {
                        m_mark[static_cast<size_t>(nb)] = epoch + 1u + static_cast<std::uint32_t>(k);
                        f.cells.push_back(nb);
                    }
                    else {
                        size_t const a = find(k);
                        size_t const b = find(static_cast<size_t>(m - epoch - 1u));
                        if (a != b) {
                            fl[b].parent = a;
                            --groupsLeft;
                        }
                    }
                }
            }

            // A group is closed once all of its floods are exhausted
            for (size_t g = 0uz; g < n && groupsLeft > 1uz; ++g) {
                if (fl[g].done || find(g) != g) { continue; }
                bool exhausted = true;
                for (size_t k = 0uz; k < n; ++k) {
                    exhausted = exhausted && (fl[k].done || find(k) != g || fl[k].head == fl[k].cells.size());
                }
                if (not exhausted) { continue; }

                std::uint32_t const fresh = new_region();
                std::uint32_t       count{};
                for (size_t k = 0uz; k < n; ++k) {
                    if (fl[k].done || find(k) != g) { continue; }
                    for (Idx_t const c : fl[k].cells) { m_regions[static_cast<size_t>(c)] = fresh; }
                    count      += static_cast<std::uint32_t>(fl[k].cells.size());
                    fl[k].done  = true;
                }
                m_regionSize[fresh]   = count;
                m_regionSize[region] -= count;
                --groupsLeft;
            }
        }
    }

    // Padded border keeps the flood fill from ever probing outside of its area
    void
    label_regions() {
        m_regions.assign(m_walk.size(), 0u);
        std::uint32_t current{};

        auto const idxOf = [stride = static_cast<size_t>(m_stride)](Pos_t const &p) { return (p[0] * stride) + p[1]; };

        explorers::FloodFill fill(
            Pos_t{m_rows + 2uz, m_cols + 2uz}, [this, idxOf](Pos_t const &p) -> bool { return m_walk[idxOf(p)] != 0; },
            [this, idxOf, &current](Pos_t const &p) -> void { m_regions[idxOf(p)] = current; });

        for (size_t r = 1uz; r <= m_rows; ++r) {
            for (size_t c = 1uz; c <= m_cols; ++c) {
                Pos_t const p{r, c};
                if (m_walk[idxOf(p)] != 0 && not fill.is_alreadyVisited(p)) {
                    ++current;
                    fill.execute_fill(p);
                }
            }
        }

        m_regionSize.assign(static_cast<size_t>(current) + 1uz, 0u);
        for (std::uint32_t const r : m_regions) {
            if (r != 0u) { ++m_regionSize[r]; }
        }
    }
};


// ####################################
// ### PATH
// ####################################

enum class PathError {
    out_of_bounds,
    start_blocked,
    goal_blocked,
    unreachable
};

struct Path {
    std::vector<Pos_t> waypoints; // Start, jump points that were turned into path vertices, goal
    Cost_t             cost{};    // Fixed point

    [[nodiscard]] double
    length() const noexcept {
        return static_cast<double>(cost) / static_cast<double>(c_costOrth);
    }

    // Waypoints with the straight / diagonal segments between them filled in cell by cell
    [[nodiscard]] std::vector<Pos_t>
    cells() const {
        std::vector<Pos_t> res;
        if (waypoints.empty()) { return res; }

        auto const absDiff = [](size_t a, size_t b) { return a > b ? a - b : b - a; };
        auto const sign    = [](size_t a, size_t b) { return static_cast<std::ptrdiff_t>(b > a) - (b < a); };

        res.push_back(waypoints.front());
        for (size_t i = 1uz; i < waypoints.size(); ++i) {
            auto const &from  = waypoints[i - 1uz];
            auto const &to    = waypoints[i];
            auto const  steps = std::max(absDiff(from[0], to[0]), absDiff(from[1], to[1]));
            auto const  dr    = sign(from[0], to[0]);
            auto const  dc    = sign(from[1], to[1]);
            for (size_t k = 1uz; k <= steps; ++k) {
                auto const kk = static_cast<std::ptrdiff_t>(k);
                res.push_back(Pos_t{static_cast<size_t>(static_cast<std::ptrdiff_t>(from[0]) + (dr * kk)),
                                    static_cast<size_t>(static_cast<std::ptrdiff_t>(from[1]) + (dc * kk))});
            }
        }
        return res;
    }
};


// ####################################
// ### OPEN LISTS
// ####################################
namespace open_lists {
struct OpenEntry {
    Cost_t key; // f = g + h at the time of insertion
    Cell   cell;
};

// parentKey is f of the node being expanded. Superseded entries are not removed, the search skips stale ones.
template <typename T>
concept OpenList = std::default_initializable<T> && requires(T &t, OpenEntry e, Cost_t parentKey) {
    t.clear();
    t.push(e, parentKey);
    { t.pop() } -> std::same_as<std::optional<OpenEntry>>;
};


// Plain binary min-heap
class BinaryHeap {
public:
    void
    clear() {
        m_heap.clear();
    }

    void
    push(OpenEntry e, Cost_t) {
        m_heap.push_back(e);
        std::ranges::push_heap(m_heap, c_greater);
    }

    [[nodiscard]] std::optional<OpenEntry>
    pop() {
        if (m_heap.empty()) { return std::nullopt; }
        std::ranges::pop_heap(m_heap, c_greater);
        OpenEntry const res = m_heap.back();
        m_heap.pop_back();
        return res;
    }

private:
    static constexpr auto c_greater = [](OpenEntry const &a, OpenEntry const &b) { return a.key > b.key; };

    std::vector<OpenEntry> m_heap;
};

// Entries not worse than the node being expanded go to a LIFO stack, the rest is an unsorted array that is only
// scanned for its minimum once the stack runs dry. Very fast as long as the open list stays small (typical for JPS).
class FastStack {
public:
    void
    clear() {
        m_stack.clear();
        m_unsorted.clear();
    }

    void
    push(OpenEntry e, Cost_t parentKey) {
        (e.key <= parentKey ? m_stack : m_unsorted).push_back(e);
    }

    [[nodiscard]] std::optional<OpenEntry>
    pop() {
        if (not m_stack.empty()) {
            OpenEntry const res = m_stack.back();
            m_stack.pop_back();
            return res;
        }
        if (m_unsorted.empty()) { return std::nullopt; }

        auto const      it  = std::ranges::min_element(m_unsorted, {}, &OpenEntry::key);
        OpenEntry const res = *it;
        *it                 = m_unsorted.back();
        m_unsorted.pop_back();
        return res;
    }

private:
    std::vector<OpenEntry> m_stack;
    std::vector<OpenEntry> m_unsorted;
};

// Radix heap. Valid because f never decreases along a search with a consistent heuristic.
class Radix {
public:
    void
    clear() {
        m_heap.clear();
    }

    void
    push(OpenEntry e, Cost_t) {
        m_heap.push(e.key, e.cell);
    }

    [[nodiscard]] std::optional<OpenEntry>
    pop() {
        auto const top = m_heap.pop();
        if (not top) { return std::nullopt; }
        return OpenEntry{.key = top->key, .cell = top->value};
    }

private:
    containers::RadixHeap<Cost_t, Cell> m_heap;
};

static_assert(OpenList<BinaryHeap> && OpenList<FastStack> && OpenList<Radix>);
} // namespace open_lists


namespace jps {
struct Goal {
    Cell  cell;
    Idx_t idx;
};

// A jumper answers two questions for the search:
//  - which directions are worth exploring from a node that was reached while travelling in direction 'parent'
//  - how many steps lead from a cell to the next node of interest in a given direction (0 == none)
template <typename J>
concept Jumper = requires(J const &j, Cell c, Dir d, Goal const &g) {
    { j.grid() } -> std::same_as<Grid const &>;
    { j.explore_mask(c, d) } -> std::same_as<std::uint8_t>;
    { j.jump(c, d, g) } -> std::same_as<std::uint32_t>;
};

// A jumper that can flip single cells of its grid and keep its preprocessed data valid
template <typename J>
concept MutableJumper = Jumper<J> && std::copy_constructible<J> && requires(J &j, Pos_t p, bool walkable) {
    { j.set_walkable(p, walkable) } -> std::same_as<bool>;
};

namespace _detail {

struct ExploreRule {
    std::uint8_t natural; // Directions kept by plain pruning
    std::uint8_t forcedA; // Added when the cell 'behindA' is blocked
    std::uint8_t forcedB;
    Dir          behindA;
    Dir          behindB;
};

inline constexpr std::array<ExploreRule, 8uz> c_exploreRules = [] {
    auto const bit = [](int d) { return static_cast<std::uint8_t>(1u << (d & 7)); };
    auto const dir = [](int d) { return static_cast<Dir>(d & 7); };

    std::array<ExploreRule, 8uz> res{};
    for (int p = 0; p < 8; ++p) {
        res[static_cast<size_t>(p)] =
            (p & 1) ? ExploreRule{.natural = static_cast<std::uint8_t>(bit(p - 1) | bit(p) | bit(p + 1)),
                                  .forcedA = 0,
                                  .forcedB = 0,
                                  .behindA = dir(p),
                                  .behindB = dir(p)}
                    : ExploreRule{.natural = bit(p),
                                  .forcedA = static_cast<std::uint8_t>(bit(p - 2) | bit(p - 1)),
                                  .forcedB = static_cast<std::uint8_t>(bit(p + 2) | bit(p + 1)),
                                  .behindA = dir(p - 3),
                                  .behindB = dir(p + 3)};
    }
    return res;
}();

// Bit mask of directions to explore after arriving in direction 'parent'. is_blocked(Dir) tells whether the
// neighbouring cell of the current one in that direction is a wall. Blocked directions are left in the mask, the
// jump step rejects them.
template <typename F_IsBlocked>
[[nodiscard]] constexpr std::uint8_t
explore_mask(Dir parent, F_IsBlocked &&is_blocked) {
    auto const &rule = c_exploreRules[ix(parent)];
    return static_cast<std::uint8_t>(rule.natural | (is_blocked(rule.behindA) ? rule.forcedA : 0) |
                                     (is_blocked(rule.behindB) ? rule.forcedB : 0));
}

// Cell n (reached from n - off[d]) has a neighbour that can only be reached optimally through n
[[nodiscard]] inline bool
has_forced_neighbour(Grid const &grid, std::array<Idx_t, 8uz> const &off, Idx_t n, Dir d) noexcept {
    Idx_t const o  = off[ix(d)];
    Idx_t const pa = off[ix(rotated(d, -2))];
    Idx_t const pb = off[ix(rotated(d, 2))];
    return (grid.walk(n + pa) && not grid.walk(n + pa - o)) || (grid.walk(n + pb) && not grid.walk(n + pb - o));
}

// Converts a precomputed jump distance (positive: jump point, <= 0: wall) into the number of steps to take,
// short-circuiting to the goal (or to the cell aligned with it) when it lies within reach
[[nodiscard]] constexpr std::uint32_t
resolve_jump(std::int16_t jumpDist, Cell c, Dir d, Cell goal) noexcept {
    int const rr = (static_cast<int>(goal.r) - static_cast<int>(c.r)) * row_step(d);
    int const cc = (static_cast<int>(goal.c) - static_cast<int>(c.c)) * col_step(d);

    int toGoal = 0;
    if (is_diagonal(d)) { toGoal = (rr > 0 && cc > 0) ? std::min(rr, cc) : 0; }
    else if (row_step(d) != 0) { toGoal = (goal.c == c.c) ? rr : 0; }
    else { toGoal = (goal.r == c.r) ? cc : 0; }

    int const reach = jumpDist < 0 ? -static_cast<int>(jumpDist) : static_cast<int>(jumpDist);
    if (toGoal > 0 && toGoal <= reach) { return static_cast<std::uint32_t>(toGoal); }
    return jumpDist > 0 ? static_cast<std::uint32_t>(jumpDist) : 0u;
}
} // namespace _detail


// ####################################
// ### JUMPERS
// ####################################

// Plain Jump Point Search. No preprocessing, jump points are found by scanning the grid.
class Online {
public:
    explicit Online(Grid grid) : m_grid(std::move(grid)), m_off(m_grid.offsets()) {}

    [[nodiscard]] Grid const &
    grid() const noexcept {
        return m_grid;
    }

    [[nodiscard]] std::uint8_t
    explore_mask(Cell c, Dir parent) const noexcept {
        Idx_t const idx = m_grid.index(c);
        return _detail::explore_mask(parent, [&](Dir b) { return not m_grid.walk(idx + m_off[ix(b)]); });
    }

    [[nodiscard]] std::uint32_t
    jump(Cell c, Dir d, Goal const &goal) const noexcept {
        Idx_t const idx = m_grid.index(c);
        return is_diagonal(d) ? jump_diagonal(idx, d, goal.idx) : jump_straight(idx, d, goal.idx);
    }

private:
    Grid                 m_grid;
    std::array<Idx_t, 8> m_off;

    [[nodiscard]] std::uint32_t
    jump_straight(Idx_t from, Dir d, Idx_t goalIdx) const noexcept {
        Idx_t const   o = m_off[ix(d)];
        Idx_t         n = from;
        std::uint32_t k{};
        for (;;) {
            n += o;
            ++k;
            if (not m_grid.walk(n)) { return 0u; }
            if (n == goalIdx || _detail::has_forced_neighbour(m_grid, m_off, n, d)) { return k; }
        }
    }

    [[nodiscard]] std::uint32_t
    jump_diagonal(Idx_t from, Dir d, Idx_t goalIdx) const noexcept {
        Dir const     vd = vertical_of(d);
        Dir const     hd = horizontal_of(d);
        Idx_t const   o  = m_off[ix(d)];
        Idx_t         n  = from;
        std::uint32_t k{};
        for (;;) {
            if (not(m_grid.walk(n + o) && m_grid.walk(n + m_off[ix(vd)]) && m_grid.walk(n + m_off[ix(hd)]))) {
                return 0u;
            }
            n += o;
            ++k;
            if (n == goalIdx || jump_straight(n, vd, goalIdx) != 0u || jump_straight(n, hd, goalIdx) != 0u) {
                return k;
            }
        }
    }
};

struct alignas(16) JumpCell {
    std::array<std::int16_t, 8uz>
        dist{}; // Per direction: > 0 steps to a jump point, <= 0 minus steps to the last cell before a wall
};

// JPS+. For every cell and direction the distance to the next jump point is precomputed in O(cells).
class Precomputed {
public:
    explicit Precomputed(Grid grid) : m_grid(std::move(grid)), m_off(m_grid.offsets()), m_jump(m_grid.padded_size()) {
        // Diagonals look at the cardinal distances of their neighbours, so those go first
        for (Dir const d : {Dir::Down, Dir::Right, Dir::Up, Dir::Left}) {
            m_grid.for_each_walkable_toward(
                d, [&](Idx_t i) { m_jump[static_cast<size_t>(i)].dist[ix(d)] = compute_cardinal(i, d); });
        }
        for (Dir const d : {Dir::DownRight, Dir::DownLeft, Dir::UpRight, Dir::UpLeft}) {
            m_grid.for_each_walkable_toward(
                d, [&](Idx_t i) { m_jump[static_cast<size_t>(i)].dist[ix(d)] = compute_diagonal(i, d); });
        }
    }

    // Flips one cell and repairs the jump table locally. Returns false if nothing changed.
    // Must not run concurrently with queries on this object.
    bool
    set_walkable(Pos_t p, bool walkable) {
        if (not m_grid.set_walkable(p, walkable)) { return false; }
        repair_around(m_grid.index(m_grid.to_cell(p)));
        return true;
    }

    [[nodiscard]] Grid const &
    grid() const noexcept {
        return m_grid;
    }

    [[nodiscard]] std::int16_t
    distance(Cell c, Dir d) const noexcept {
        return m_jump[static_cast<size_t>(m_grid.index(c))].dist[ix(d)];
    }

    [[nodiscard]] std::uint8_t
    explore_mask(Cell c, Dir parent) const noexcept {
        auto const &dist = m_jump[static_cast<size_t>(m_grid.index(c))].dist;
        return _detail::explore_mask(parent, [&](Dir b) { return dist[ix(b)] == 0; });
    }

    [[nodiscard]] std::uint32_t
    jump(Cell c, Dir d, Goal const &goal) const noexcept {
        return _detail::resolve_jump(distance(c, d), c, d, goal.cell);
    }

private:
    Grid                  m_grid;
    std::array<Idx_t, 8>  m_off;
    std::vector<JumpCell> m_jump;

    // Value of dist[d] for cell i given the current grid and the already valid dist[d] of its successor
    [[nodiscard]] std::int16_t
    compute_cardinal(Idx_t i, Dir d) const noexcept {
        if (not m_grid.walk(i)) { return 0; }
        Idx_t const n = i + m_off[ix(d)];
        if (not m_grid.walk(n)) { return 0; }
        if (_detail::has_forced_neighbour(m_grid, m_off, n, d)) { return 1; }
        return extended(m_jump[static_cast<size_t>(n)].dist[ix(d)]);
    }

    [[nodiscard]] std::int16_t
    compute_diagonal(Idx_t i, Dir d) const noexcept {
        if (not m_grid.walk(i)) { return 0; }
        Dir const   vd = vertical_of(d);
        Dir const   hd = horizontal_of(d);
        Idx_t const n  = i + m_off[ix(d)];
        if (not(m_grid.walk(n) && m_grid.walk(i + m_off[ix(vd)]) && m_grid.walk(i + m_off[ix(hd)]))) { return 0; }
        auto const &dn = m_jump[static_cast<size_t>(n)].dist;
        if (dn[ix(vd)] > 0 || dn[ix(hd)] > 0) { return 1; }
        return extended(dn[ix(d)]);
    }

    // Every entry reads the grid within one cell of its own cell, so only the 3x3 around x has stale inputs.
    // From there a change travels backwards along the direction and stops at the first unchanged entry.
    void
    repair_around(Idx_t x) {
        std::array<Idx_t, 9uz> seeds{x};
        for (size_t d = 0uz; d < 8uz; ++d) { seeds[d + 1uz] = x + m_off[d]; }

        std::vector<Idx_t> work;
        std::vector<Idx_t> changedCardinal;
        auto const         propagate = [&](Dir d, bool cardinal) {
            while (not work.empty()) {
                Idx_t const i = work.back();
                work.pop_back();
                std::int16_t const v    = cardinal ? compute_cardinal(i, d) : compute_diagonal(i, d);
                auto              &slot = m_jump[static_cast<size_t>(i)].dist[ix(d)];
                if (v == slot) { continue; }
                slot = v;
                if (cardinal) { changedCardinal.push_back(i); }
                work.push_back(i - m_off[ix(d)]);
            }
        };

        for (Dir const d : {Dir::Down, Dir::Right, Dir::Up, Dir::Left}) {
            work.assign(seeds.begin(), seeds.end());
            propagate(d, true);
        }
        for (Dir const d : {Dir::DownRight, Dir::DownLeft, Dir::UpRight, Dir::UpLeft}) {
            work.assign(seeds.begin(), seeds.end());
            for (Idx_t const c : changedCardinal) {
                work.push_back(c - m_off[ix(d)]);
            } // Diagonal reads cardinals of its successor
            propagate(d, false);
        }
    }

    // One more step away from the same jump point / wall
    [[nodiscard]] static constexpr std::int16_t
    extended(std::int16_t prev) noexcept {
        return static_cast<std::int16_t>(prev > 0 ? prev + 1 : prev - 1);
    }
};

// Axis aligned bounding box of the cells that are optimally reachable by moving off in a certain direction
struct Box {
    std::uint16_t minR{std::numeric_limits<std::uint16_t>::max()};
    std::uint16_t maxR{0};
    std::uint16_t minC{std::numeric_limits<std::uint16_t>::max()};
    std::uint16_t maxC{0};

    [[nodiscard]] constexpr bool
    contains(Cell c) const noexcept {
        return c.r >= minR && c.r <= maxR && c.c >= minC && c.c <= maxC;
    }

    constexpr void
    extend(Cell c) noexcept {
        minR = std::min(minR, c.r);
        maxR = std::max(maxR, c.r);
        minC = std::min(minC, c.c);
        maxC = std::max(maxC, c.c);
    }
};

// One cache line per cell
struct alignas(64) BoundsCell {
    std::array<Box, 8uz> box{};
};

// JPS+ with Goal Bounding. The O(cells^2) preprocessing is distributed over threads.
class GoalBounded {
public:
    // threads == 0 means hardware concurrency
    explicit GoalBounded(Grid grid, size_t threads = 0uz)
        : m_base(std::move(grid)), m_bounds(m_base.grid().padded_size()) {
        auto const &g = m_base.grid();
        if (threads == 0uz) { threads = std::max(1uz, static_cast<size_t>(std::thread::hardware_concurrency())); }
        threads = std::min(threads, g.rows());

        std::atomic<size_t> nextRow{0uz};
        auto const          worker = [&] {
            Flooder flooder(m_base);
            for (size_t r = nextRow.fetch_add(1uz); r < g.rows(); r = nextRow.fetch_add(1uz)) {
                for (size_t c = 0uz; c < g.cols(); ++c) {
                    Cell const start = g.to_cell(Pos_t{r, c});
                    if (g.walk(start)) { flooder.flood(start, m_bounds[static_cast<size_t>(g.index(start))]); }
                }
            }
        };

        std::vector<std::jthread> pool;
        pool.reserve(threads - 1uz);
        for (size_t i = 1uz; i < threads; ++i) { pool.emplace_back(worker); }
        worker();
    }

    [[nodiscard]] Grid const &
    grid() const noexcept {
        return m_base.grid();
    }

    [[nodiscard]] std::uint8_t
    explore_mask(Cell c, Dir parent) const noexcept {
        return m_base.explore_mask(c, parent);
    }

    [[nodiscard]] std::uint32_t
    jump(Cell c, Dir d, Goal const &goal) const noexcept {
        if (not m_bounds[static_cast<size_t>(m_base.grid().index(c))].box[ix(d)].contains(goal.cell)) { return 0u; }
        return m_base.jump(c, d, goal);
    }

private:
    Precomputed             m_base;
    std::vector<BoundsCell> m_bounds;

    // Dijkstra flood from a start cell that expands like the JPS+ search does (pruned neighbours), but one cell at a
    // time. Every reached cell is labelled with the set of first-step directions of all optimal paths towards it.
    class Flooder {
    public:
        explicit Flooder(Precomputed const &pre) : m_pre(pre), m_nodes(pre.grid().padded_size()) {}

        void
        flood(Cell start, BoundsCell &out) {
            ++m_stamp;
            m_heap.clear();
            out = BoundsCell{};

            m_nodes[idx(start)] = Node{.g = 0, .stamp = m_stamp, .startMask = 0, .parentMask = 0};
            relax_from(start, 0, 0, std::uint8_t{0xFF}, true);

            while (auto const top = m_heap.pop()) {
                Cell const cell = top->value;
                Node const n    = m_nodes[idx(cell)];
                if (n.g != top->key) { continue; } // Superseded by a cheaper entry

                std::uint8_t expand{};
                for (std::uint8_t s = n.startMask; s != 0; s = static_cast<std::uint8_t>(s & (s - 1))) {
                    out.box[static_cast<size_t>(std::countr_zero(s))].extend(cell);
                }
                for (std::uint8_t p = n.parentMask; p != 0; p = static_cast<std::uint8_t>(p & (p - 1))) {
                    expand |= m_pre.explore_mask(cell, static_cast<Dir>(std::countr_zero(p)));
                }
                relax_from(cell, n.g, n.startMask, expand, false);
            }
        }

    private:
        struct Node {
            Cost_t        g;
            std::uint32_t stamp;
            std::uint8_t  startMask;  // Bit per first-step direction of optimal paths
            std::uint8_t  parentMask; // Bit per direction of the last step of optimal paths
        };

        Precomputed const                  &m_pre;
        std::vector<Node>                   m_nodes;
        containers::RadixHeap<Cost_t, Cell> m_heap;
        std::uint32_t                       m_stamp{};

        [[nodiscard]] size_t
        idx(Cell c) const noexcept {
            return static_cast<size_t>(m_pre.grid().index(c));
        }

        void
        relax_from(Cell cell, Cost_t g, std::uint8_t startMask, std::uint8_t mask, bool isStart) {
            for (; mask != 0; mask = static_cast<std::uint8_t>(mask & (mask - 1))) {
                Dir const d = static_cast<Dir>(std::countr_zero(mask));
                if (m_pre.distance(cell, d) == 0) { continue; }

                Cell const         next = advanced(cell, d);
                Cost_t const       ng   = g + (is_diagonal(d) ? c_costDiag : c_costOrth);
                std::uint8_t const sm   = isStart ? static_cast<std::uint8_t>(1u << ix(d)) : startMask;
                std::uint8_t const pm   = static_cast<std::uint8_t>(1u << ix(d));
                Node              &nn   = m_nodes[idx(next)];
                if (nn.stamp != m_stamp || ng < nn.g) {
                    nn = Node{.g = ng, .stamp = m_stamp, .startMask = sm, .parentMask = pm};
                    m_heap.push(ng, next);
                }
                else if (ng == nn.g) {
                    nn.startMask  = static_cast<std::uint8_t>(nn.startMask | sm);
                    nn.parentMask = static_cast<std::uint8_t>(nn.parentMask | pm);
                }
            }
        }
    };
};

static_assert(Jumper<Online> && Jumper<Precomputed> && Jumper<GoalBounded>);
static_assert(MutableJumper<Precomputed> && not MutableJumper<Online> && not MutableJumper<GoalBounded>);


// ####################################
// ### SEARCH
// ####################################

// A* over jump points. The preprocessed data lives in the (immutable) jumper which is shared between copies of the
// searcher. Every copy owns its own scratch memory, so copies can search concurrently, one searcher cannot.
template <jps::Jumper J, open_lists::OpenList OL = open_lists::FastStack>
class JumpPointSearch {
public:
    // Constructor arguments after the grid are forwarded to the jumper
    template <typename... Args>
    explicit JumpPointSearch(Grid grid, Args &&...args)
        : m_jumper(std::make_shared<J>(std::move(grid), std::forward<Args>(args)...)),
          m_nodes(m_jumper->grid().padded_size()) {}

    [[nodiscard]] Grid const &
    grid() const noexcept {
        return m_jumper->grid();
    }

    // Returns false if the cell was already in that state. Copies of this searcher keep their own snapshot.
    bool
    set_walkable(Pos_t p, bool walkable)
    requires jps::MutableJumper<J>
    {
        if (m_jumper.use_count() > 1) { m_jumper = std::make_shared<J>(*m_jumper); }
        return m_jumper->set_walkable(p, walkable);
    }

    // Number of nodes expanded by the last search
    [[nodiscard]] size_t
    last_expanded() const noexcept {
        return m_expanded;
    }

    [[nodiscard]] std::expected<Path, PathError>
    find_path(Pos_t start, Pos_t goal) {
        Grid const &g = m_jumper->grid();
        if (not g.in_bounds(start) || not g.in_bounds(goal)) { return std::unexpected(PathError::out_of_bounds); }
        if (not g.is_walkable(start)) { return std::unexpected(PathError::start_blocked); }
        if (not g.is_walkable(goal)) { return std::unexpected(PathError::goal_blocked); }
        if (g.region_of(start) != g.region_of(goal)) { return std::unexpected(PathError::unreachable); }

        m_expanded = 0uz;
        if (start == goal) { return Path{.waypoints = {start}, .cost = 0}; }

        Cell const startCell = g.to_cell(start);
        Goal const goalInfo{.cell = g.to_cell(goal), .idx = g.index(g.to_cell(goal))};

        begin_search();
        m_nodes[idx(startCell)] = Node{.g = 0, .parent = startCell, .stamp = m_stamp, .dir = Dir::Down};
        expand(startCell, 0, heuristic(startCell, goalInfo.cell), std::uint8_t{0xFF}, goalInfo);

        while (auto const top = m_open.pop()) {
            Cell const cell = top->cell;
            Node const n    = m_nodes[idx(cell)];
            if (top->key != n.g + heuristic(cell, goalInfo.cell)) { continue; } // Superseded by a cheaper entry

            if (cell == goalInfo.cell) { return make_path(startCell, goalInfo.cell); }
            ++m_expanded;
            expand(cell, n.g, top->key, m_jumper->explore_mask(cell, n.dir), goalInfo);
        }
        return std::unexpected(PathError::unreachable);
    }

private:
    struct Node {
        Cost_t        g;
        Cell          parent;
        std::uint32_t stamp;
        Dir           dir;
    };

    // template <typename T>
    // struct m_jumper_t {
    //     using type = std::shared_ptr<J> const;
    // };

    // template <typename T>
    // requires jps::MutableJumper<T>
    // struct m_jumper_t<T> {
    //     using type = std::shared_ptr<J>;
    // };


    // TODO: Why exactly are we having a shared pointer here? Is that really necessary / good for the anticipated
    // usecase?
    std::shared_ptr<J> m_jumper; // Non-const so set_walkable can mutate it
    std::vector<Node>  m_nodes;
    OL                 m_open;
    std::uint32_t      m_stamp{};
    size_t             m_expanded{};

    [[nodiscard]] size_t
    idx(Cell c) const noexcept {
        return static_cast<size_t>(m_jumper->grid().index(c));
    }

    void
    begin_search() {
        if (++m_stamp == 0u) { // Wrapped around, stale stamps could alias
            for (auto &n : m_nodes) { n.stamp = 0u; }
            m_stamp = 1u;
        }
        m_open.clear();
    }

    // Octile distance, exactly consistent with the step costs
    [[nodiscard]] static Cost_t
    heuristic(Cell c, Cell goal) noexcept {
        auto const dr = static_cast<Cost_t>(c.r > goal.r ? c.r - goal.r : goal.r - c.r);
        auto const dc = static_cast<Cost_t>(c.c > goal.c ? c.c - goal.c : goal.c - c.c);
        return (std::min(dr, dc) * c_costDiagExcess) + (std::max(dr, dc) * c_costOrth);
    }

    void
    expand(Cell from, Cost_t g, Cost_t key, std::uint8_t mask, Goal const &goal) {
        for (; mask != 0; mask = static_cast<std::uint8_t>(mask & (mask - 1))) {
            Dir const  d     = static_cast<Dir>(std::countr_zero(mask));
            auto const steps = m_jumper->jump(from, d, goal);
            if (steps == 0u) { continue; }

            Cell const   to = advanced(from, d, steps);
            Cost_t const ng = g + (steps * (is_diagonal(d) ? c_costDiag : c_costOrth));
            Node        &nn = m_nodes[idx(to)];
            if (nn.stamp != m_stamp || ng < nn.g) {
                nn = Node{.g = ng, .parent = from, .stamp = m_stamp, .dir = d};
                m_open.push(open_lists::OpenEntry{.key = ng + heuristic(to, goal.cell), .cell = to}, key);
            }
        }
    }

    [[nodiscard]] Path
    make_path(Cell start, Cell goal) const {
        Path res{.waypoints = {}, .cost = m_nodes[idx(goal)].g};
        for (Cell cur = goal;; cur = m_nodes[idx(cur)].parent) {
            res.waypoints.push_back(Grid::to_pos(cur));
            if (cur == start) { break; }
        }
        std::ranges::reverse(res.waypoints);
        return res;
    }
};
} // namespace jps

template <jps::Jumper J, open_lists::OpenList OL = open_lists::FastStack>
using JumpPointSearch = jps::JumpPointSearch<J, OL>;

using Jps       = JumpPointSearch<jps::Online>;
using JpsPlus   = JumpPointSearch<jps::Precomputed>;
using JpsPlusGB = JumpPointSearch<jps::GoalBounded>;

} // namespace incom::standard::solvers::pathfinders
