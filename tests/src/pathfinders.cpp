#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <incstd/polyfills/mdspan.hpp>
#include <incstd/solvers/pathfinders.hpp>

namespace pf  = incom::standard::solvers::pathfinders;
namespace dir = incom::standard::solvers::directions;

using pf::Cell;
using pf::Cost_t;
using pf::Grid;
using pf::Path;
using pf::PathError;
using pf::Pos_t;

namespace {

// '#' is a wall, everything else is walkable
Grid
make_grid(std::vector<std::string> const &rows) {
    auto const res = Grid::create(Pos_t{rows.size(), rows.front().size()},
                                  [&rows](Pos_t const &p) { return rows[p[0]][p[1]] != '#'; });
    return res.value();
}

Grid
make_random_grid(std::mt19937 &rng, size_t rows, size_t cols, double wallDensity) {
    std::bernoulli_distribution wall(wallDensity);
    std::vector<std::uint8_t>   cells(rows * cols);
    for (auto &c : cells) { c = wall(rng) ? 0 : 1; }
    return Grid::create(Pos_t{rows, cols}, [&](Pos_t const &p) { return cells[(p[0] * cols) + p[1]] != 0; }).value();
}

// Dijkstra over the same movement rules (8-connected, no corner cutting). Unreachable cells keep the max value.
std::vector<Cost_t>
reference_costs(Grid const &g, Pos_t start) {
    using Entry    = std::pair<Cost_t, size_t>;
    auto const idx = [&g](Pos_t const &p) { return (p[0] * g.cols()) + p[1]; };

    std::vector<Cost_t>                                            best(g.rows() * g.cols(),
                                                                        std::numeric_limits<Cost_t>::max());
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    best[idx(start)] = 0;
    open.emplace(0, idx(start));

    while (not open.empty()) {
        auto const [cost, i] = open.top();
        open.pop();
        if (cost != best[i]) { continue; }
        Pos_t const cur{i / g.cols(), i % g.cols()};

        for (size_t d = 0uz; d < 8uz; ++d) {
            auto const nr = static_cast<std::ptrdiff_t>(cur[0]) + dir::c_dRow[d];
            auto const nc = static_cast<std::ptrdiff_t>(cur[1]) + dir::c_dCol[d];
            if (nr < 0 || nc < 0) { continue; }
            Pos_t const next{static_cast<size_t>(nr), static_cast<size_t>(nc)};
            if (not g.is_walkable(next)) { continue; }

            bool const diagonal = (d & 1uz) != 0uz;
            if (diagonal && not(g.is_walkable(Pos_t{next[0], cur[1]}) && g.is_walkable(Pos_t{cur[0], next[1]}))) {
                continue;
            }
            Cost_t const ng = cost + (diagonal ? pf::c_costDiag : pf::c_costOrth);
            if (ng < best[idx(next)]) {
                best[idx(next)] = ng;
                open.emplace(ng, idx(next));
            }
        }
    }
    return best;
}

std::optional<Cost_t>
reference_cost(Grid const &g, Pos_t start, Pos_t goal) {
    auto const cost = reference_costs(g, start)[(goal[0] * g.cols()) + goal[1]];
    return cost == std::numeric_limits<Cost_t>::max() ? std::nullopt : std::optional<Cost_t>{cost};
}

// Walks the path cell by cell and checks every step against the movement rules
void
expect_valid_path(Grid const &g, Path const &p, Pos_t start, Pos_t goal) {
    auto const cells = p.cells();
    ASSERT_FALSE(cells.empty());
    EXPECT_EQ(cells.front(), start);
    EXPECT_EQ(cells.back(), goal);
    EXPECT_EQ(p.waypoints.front(), start);
    EXPECT_EQ(p.waypoints.back(), goal);

    Cost_t sum{};
    for (size_t i = 1uz; i < cells.size(); ++i) {
        auto const &a  = cells[i - 1uz];
        auto const &b  = cells[i];
        auto const  dr = static_cast<std::ptrdiff_t>(b[0]) - static_cast<std::ptrdiff_t>(a[0]);
        auto const  dc = static_cast<std::ptrdiff_t>(b[1]) - static_cast<std::ptrdiff_t>(a[1]);
        ASSERT_TRUE(std::abs(dr) <= 1 && std::abs(dc) <= 1 && (dr != 0 || dc != 0));
        ASSERT_TRUE(g.is_walkable(b));
        if (dr != 0 && dc != 0) {
            ASSERT_TRUE(g.is_walkable(Pos_t{b[0], a[1]}) && g.is_walkable(Pos_t{a[0], b[1]})) << "corner cut";
            sum += pf::c_costDiag;
        }
        else { sum += pf::c_costOrth; }
    }
    EXPECT_EQ(sum, p.cost);
}

std::uint8_t
bit(dir::Dir d) {
    return static_cast<std::uint8_t>(1u << dir::ix(d));
}

} // namespace


// ####################################
// ### GRID
// ####################################

TEST(PathfindersGrid, RejectsEmptyExtents) {
    auto const always = [](Pos_t const &) { return true; };
    EXPECT_EQ(Grid::create(Pos_t{0uz, 5uz}, always).error(), Grid::Error::empty_extents);
    EXPECT_EQ(Grid::create(Pos_t{5uz, 0uz}, always).error(), Grid::Error::empty_extents);
}

TEST(PathfindersGrid, RejectsTooLargeExtents) {
    auto const always = [](Pos_t const &) { return true; };
    EXPECT_EQ(Grid::create(Pos_t{pf::c_maxExtent + 1uz, 1uz}, always).error(), Grid::Error::extent_too_large);
    EXPECT_EQ(Grid::create(Pos_t{1uz, pf::c_maxExtent + 1uz}, always).error(), Grid::Error::extent_too_large);
}

TEST(PathfindersGrid, ReportsExtentsAndWalkability) {
    auto const g = make_grid({"..#",
                              "#..",
                              "..."});
    EXPECT_EQ(g.rows(), 3uz);
    EXPECT_EQ(g.cols(), 3uz);
    EXPECT_TRUE(g.is_walkable({0, 0}));
    EXPECT_FALSE(g.is_walkable({0, 2}));
    EXPECT_FALSE(g.is_walkable({1, 0}));
    EXPECT_TRUE(g.is_walkable({2, 2}));
}

TEST(PathfindersGrid, OutOfBoundsIsNeitherWalkableNorInBounds) {
    auto const g = make_grid({"...", "..."});
    EXPECT_TRUE(g.in_bounds({1, 2}));
    EXPECT_FALSE(g.in_bounds({2, 0}));
    EXPECT_FALSE(g.in_bounds({0, 3}));
    EXPECT_FALSE(g.is_walkable({2, 0}));
    EXPECT_FALSE(g.is_walkable({0, 3}));
    EXPECT_EQ(g.region_of({2, 0}), 0u);
}

TEST(PathfindersGrid, PredicateReceivesRowColPositions) {
    std::vector<Pos_t> seen;
    auto const         g = Grid::create(Pos_t{2uz, 3uz}, [&seen](Pos_t const &p) {
        seen.push_back(p);
        return true;
    });
    ASSERT_TRUE(g.has_value());
    EXPECT_EQ(seen.size(), 6uz);
    EXPECT_NE(std::ranges::find(seen, Pos_t{1uz, 2uz}), seen.end());
}

TEST(PathfindersGrid, FromMdspanMatchesLayout) {
    std::vector<int> const data{1, 0, 1,
                                1, 1, 0};
    incom::standard::polyfills::mdspan<int const, incom::standard::polyfills::dextents<size_t, 2uz>> const view(
        data.data(), 2uz, 3uz);

    auto const g = Grid::from_mdspan(view);
    ASSERT_TRUE(g.has_value());
    EXPECT_EQ(g->rows(), 2uz);
    EXPECT_EQ(g->cols(), 3uz);
    for (size_t r = 0uz; r < 2uz; ++r) {
        for (size_t c = 0uz; c < 3uz; ++c) { EXPECT_EQ(g->is_walkable({r, c}), data[(r * 3uz) + c] != 0); }
    }
}

TEST(PathfindersGrid, LabelsConnectedRegions) {
    auto const g = make_grid({"..#..",
                              "..#..",
                              "..#.."});
    EXPECT_EQ(g.region_of({1, 2}), 0u);
    EXPECT_NE(g.region_of({0, 0}), 0u);
    EXPECT_NE(g.region_of({0, 4}), 0u);
    EXPECT_EQ(g.region_of({0, 0}), g.region_of({2, 1}));
    EXPECT_EQ(g.region_of({0, 3}), g.region_of({2, 4}));
    EXPECT_NE(g.region_of({0, 0}), g.region_of({0, 4}));
}

TEST(PathfindersGrid, PaddedCoordinatesRoundTrip) {
    auto const g = make_grid({"...", "..."});
    EXPECT_EQ(g.padded_size(), 4uz * 5uz);
    EXPECT_EQ(g.to_cell({0, 0}), (Cell{.r = 1, .c = 1}));
    EXPECT_EQ(Grid::to_pos(g.to_cell({1, 2})), (Pos_t{1uz, 2uz}));
    // Border is a wall
    EXPECT_FALSE(g.walk(Cell{.r = 0, .c = 0}));
    EXPECT_FALSE(g.walk(Cell{.r = 3, .c = 4}));
    EXPECT_TRUE(g.walk(g.to_cell({1, 2})));
}

TEST(PathfindersGrid, OffsetsMatchDirections) {
    auto const g   = make_grid({"...", "..."});
    auto const off = g.offsets();
    auto const c   = g.to_cell({0, 0});
    for (size_t d = 0uz; d < 8uz; ++d) {
        auto const n = pf::advanced(c, static_cast<dir::Dir>(d));
        EXPECT_EQ(g.index(c) + off[d], g.index(n)) << "dir " << d;
    }
}

TEST(PathfindersGrid, ForEachWalkableTowardVisitsNeighbourFirst) {
    auto const g = make_grid({"..#.",
                              "....",
                              ".#.."});
    auto const off = g.offsets();
    for (size_t di = 0uz; di < 8uz; ++di) {
        auto const        d = static_cast<dir::Dir>(di);
        std::vector<bool> visited(g.padded_size(), false);
        size_t            count{};
        g.for_each_walkable_toward(d, [&](pf::Idx_t i) {
            ++count;
            auto const n = i + off[di];
            if (g.walk(n)) { EXPECT_TRUE(visited[static_cast<size_t>(n)]) << "dir " << di; }
            visited[static_cast<size_t>(i)] = true;
        });
        EXPECT_EQ(count, 10uz);
    }
}

TEST(PathfindersCell, AdvancedMovesInDirection) {
    Cell const c{.r = 5, .c = 5};
    EXPECT_EQ(pf::advanced(c, dir::Dir::Down), (Cell{.r = 6, .c = 5}));
    EXPECT_EQ(pf::advanced(c, dir::Dir::UpLeft, 3u), (Cell{.r = 2, .c = 2}));
    EXPECT_EQ(pf::advanced(c, dir::Dir::Right, 2u), (Cell{.r = 5, .c = 7}));
    EXPECT_EQ(pf::advanced(c, dir::Dir::DownLeft, 2u), (Cell{.r = 7, .c = 3}));
}


// ####################################
// ### PATH
// ####################################

TEST(PathfindersPath, LengthIsInOrthogonalUnits) {
    Path const p{.waypoints = {}, .cost = 3uz * pf::c_costOrth};
    EXPECT_DOUBLE_EQ(p.length(), 3.0);
}

TEST(PathfindersPath, CellsOfEmptyPathIsEmpty) {
    EXPECT_TRUE(Path{}.cells().empty());
}

TEST(PathfindersPath, CellsFillsStraightAndDiagonalSegments) {
    Path const p{.waypoints = {Pos_t{0uz, 0uz}, Pos_t{0uz, 2uz}, Pos_t{2uz, 0uz}, Pos_t{2uz, 0uz}}, .cost = 0};
    std::vector<Pos_t> const expected{{0, 0}, {0, 1}, {0, 2}, {1, 1}, {2, 0}};
    EXPECT_EQ(p.cells(), expected);
}

TEST(PathfindersPath, CellsOfSingleWaypoint) {
    Path const p{.waypoints = {Pos_t{4uz, 7uz}}, .cost = 0};
    EXPECT_EQ(p.cells(), std::vector<Pos_t>{(Pos_t{4uz, 7uz})});
}


// ####################################
// ### OPEN LISTS
// ####################################

template <typename OL>
class OpenListTest : public ::testing::Test {};

using OpenListTypes = ::testing::Types<pf::open_lists::BinaryHeap, pf::open_lists::FastStack, pf::open_lists::Radix>;
TYPED_TEST_SUITE(OpenListTest, OpenListTypes);

TYPED_TEST(OpenListTest, PopOnEmptyReturnsNullopt) {
    TypeParam ol;
    EXPECT_FALSE(ol.pop().has_value());
}

TYPED_TEST(OpenListTest, PopsInKeyOrder) {
    TypeParam                 ol;
    std::vector<Cost_t> const keys{50, 30, 90, 10, 70, 30, 20};
    for (size_t i = 0uz; i < keys.size(); ++i) {
        ol.push(pf::open_lists::OpenEntry{.key = keys[i], .cell = Cell{.r = static_cast<std::uint16_t>(i), .c = 0}}, 0);
    }

    std::vector<Cost_t> popped;
    while (auto const e = ol.pop()) { popped.push_back(e->key); }

    auto sorted = keys;
    std::ranges::sort(sorted);
    EXPECT_EQ(popped, sorted);
}

TYPED_TEST(OpenListTest, KeepsCellWithKey) {
    TypeParam ol;
    ol.push(pf::open_lists::OpenEntry{.key = 8, .cell = Cell{.r = 3, .c = 4}}, 0);
    auto const e = ol.pop();
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->key, 8u);
    EXPECT_EQ(e->cell, (Cell{.r = 3, .c = 4}));
    EXPECT_FALSE(ol.pop().has_value());
}

TYPED_TEST(OpenListTest, InterleavedPushesStayMonotone) {
    TypeParam ol;
    ol.push(pf::open_lists::OpenEntry{.key = 10, .cell = {}}, 0);
    ol.push(pf::open_lists::OpenEntry{.key = 20, .cell = {}}, 0);
    EXPECT_EQ(ol.pop()->key, 10u);
    ol.push(pf::open_lists::OpenEntry{.key = 15, .cell = {}}, 10);
    ol.push(pf::open_lists::OpenEntry{.key = 40, .cell = {}}, 10);
    EXPECT_EQ(ol.pop()->key, 15u);
    EXPECT_EQ(ol.pop()->key, 20u);
    EXPECT_EQ(ol.pop()->key, 40u);
    EXPECT_FALSE(ol.pop().has_value());
}

TYPED_TEST(OpenListTest, ClearEmptiesAndAllowsReuse) {
    TypeParam ol;
    ol.push(pf::open_lists::OpenEntry{.key = 100, .cell = {}}, 0);
    ol.push(pf::open_lists::OpenEntry{.key = 200, .cell = {}}, 0);
    ol.clear();
    EXPECT_FALSE(ol.pop().has_value());

    ol.push(pf::open_lists::OpenEntry{.key = 1, .cell = {}}, 0);
    EXPECT_EQ(ol.pop()->key, 1u);
}

TEST(PathfindersFastStack, EntriesNotWorseThanParentAreLifo) {
    pf::open_lists::FastStack ol;
    ol.push(pf::open_lists::OpenEntry{.key = 100, .cell = {}}, 100);
    ol.push(pf::open_lists::OpenEntry{.key = 90, .cell = {}}, 100);
    ol.push(pf::open_lists::OpenEntry{.key = 500, .cell = {}}, 100);
    EXPECT_EQ(ol.pop()->key, 90u);
    EXPECT_EQ(ol.pop()->key, 100u);
    EXPECT_EQ(ol.pop()->key, 500u);
}


// ####################################
// ### JUMPERS
// ####################################

TEST(PathfindersJumpers, ExploreMaskNaturalNeighboursOnOpenGrid) {
    auto const g = make_grid({".....",
                              ".....",
                              "....."});
    pf::jps::Online const      online(g);
    pf::jps::Precomputed const pre(g);
    Cell const                     c = g.to_cell({1, 2});

    // Straight move keeps only straight, diagonal move keeps the diagonal and both of its components
    EXPECT_EQ(online.explore_mask(c, dir::Dir::Right), bit(dir::Dir::Right));
    EXPECT_EQ(pre.explore_mask(c, dir::Dir::Right), bit(dir::Dir::Right));
    auto const diag = static_cast<std::uint8_t>(bit(dir::Dir::Down) | bit(dir::Dir::DownRight) | bit(dir::Dir::Right));
    EXPECT_EQ(online.explore_mask(c, dir::Dir::DownRight), diag);
    EXPECT_EQ(pre.explore_mask(c, dir::Dir::DownRight), diag);
}

TEST(PathfindersJumpers, ExploreMaskAddsForcedNeighbours) {
    auto const g = make_grid({"...",
                              ".#.",
                              "..."});
    pf::jps::Online const      online(g);
    pf::jps::Precomputed const pre(g);
    Cell const                     c = g.to_cell({0, 2}); // Wall below-left, arrived travelling Right

    auto const expected =
        static_cast<std::uint8_t>(bit(dir::Dir::Right) | bit(dir::Dir::DownRight) | bit(dir::Dir::Down));
    EXPECT_EQ(online.explore_mask(c, dir::Dir::Right) & expected, expected);
    EXPECT_EQ(pre.explore_mask(c, dir::Dir::Right) & expected, expected);
    EXPECT_EQ(online.explore_mask(c, dir::Dir::Right), pre.explore_mask(c, dir::Dir::Right));
}

TEST(PathfindersJumpers, PrecomputedDistancesToWallsAreNegative) {
    auto const                     g = make_grid({"....."});
    pf::jps::Precomputed const pre(g);
    EXPECT_EQ(pre.distance(g.to_cell({0, 0}), dir::Dir::Right), -4);
    EXPECT_EQ(pre.distance(g.to_cell({0, 4}), dir::Dir::Left), -4);
    EXPECT_EQ(pre.distance(g.to_cell({0, 2}), dir::Dir::Down), 0);
    EXPECT_EQ(pre.distance(g.to_cell({0, 2}), dir::Dir::DownRight), 0);
}

TEST(PathfindersJumpers, PrecomputedDistanceToForcedJumpPoint) {
    auto const g = make_grid({"...",
                              ".#.",
                              "..."});
    pf::jps::Precomputed const pre(g);
    // Moving right along the top row, (0,2) has a forced neighbour below it
    EXPECT_EQ(pre.distance(g.to_cell({0, 0}), dir::Dir::Right), 2);
    EXPECT_EQ(pre.distance(g.to_cell({0, 1}), dir::Dir::Right), 1);
}

TEST(PathfindersJumpers, OnlineAndPrecomputedAgreeOnJumpPoints) {
    auto const g = make_grid({"...",
                              ".#.",
                              "..."});
    pf::jps::Online const      online(g);
    pf::jps::Precomputed const pre(g);
    pf::jps::Goal const                 goal{.cell = g.to_cell({2, 0}), .idx = g.index(g.to_cell({2, 0}))};

    Cell const c = g.to_cell({0, 0});
    EXPECT_EQ(online.jump(c, dir::Dir::Right, goal), 2u);
    EXPECT_EQ(pre.jump(c, dir::Dir::Right, goal), 2u);
    EXPECT_EQ(online.jump(c, dir::Dir::Up, goal), 0u);
    EXPECT_EQ(pre.jump(c, dir::Dir::Up, goal), 0u);
}

TEST(PathfindersJumpers, JumpStopsAtGoalWhenInReach) {
    auto const g = make_grid({"......"});
    pf::jps::Online const      online(g);
    pf::jps::Precomputed const pre(g);
    pf::jps::Goal const                 goal{.cell = g.to_cell({0, 3}), .idx = g.index(g.to_cell({0, 3}))};

    Cell const c = g.to_cell({0, 0});
    EXPECT_EQ(online.jump(c, dir::Dir::Right, goal), 3u);
    EXPECT_EQ(pre.jump(c, dir::Dir::Right, goal), 3u);
    EXPECT_EQ(online.jump(c, dir::Dir::Left, goal), 0u);
}

TEST(PathfindersJumpers, DiagonalJumpDoesNotCutCorners) {
    auto const g = make_grid({".#",
                              "..",
                              "#."});
    pf::jps::Online const      online(g);
    pf::jps::Precomputed const pre(g);
    pf::jps::Goal const                 goal{.cell = g.to_cell({2, 1}), .idx = g.index(g.to_cell({2, 1}))};

    // (0,0) -> (1,1) would squeeze past the wall at (0,1)
    EXPECT_EQ(online.jump(g.to_cell({0, 0}), dir::Dir::DownRight, goal), 0u);
    EXPECT_EQ(pre.jump(g.to_cell({0, 0}), dir::Dir::DownRight, goal), 0u);
}


// ####################################
// ### SEARCH
// ####################################

template <typename S>
class SearchTest : public ::testing::Test {};

using SearchTypes = ::testing::Types<
    pf::Jps, pf::JpsPlus, pf::JpsPlusGB, pf::JumpPointSearch<pf::jps::Online, pf::open_lists::BinaryHeap>,
    pf::JumpPointSearch<pf::jps::Online, pf::open_lists::Radix>,
    pf::JumpPointSearch<pf::jps::Precomputed, pf::open_lists::BinaryHeap>,
    pf::JumpPointSearch<pf::jps::GoalBounded, pf::open_lists::Radix>>;
TYPED_TEST_SUITE(SearchTest, SearchTypes);

TYPED_TEST(SearchTest, StartEqualsGoal) {
    TypeParam  s(make_grid({"..."}));
    auto const res = s.find_path({0, 1}, {0, 1});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->cost, 0u);
    EXPECT_EQ(res->waypoints, std::vector<Pos_t>{(Pos_t{0uz, 1uz})});
    EXPECT_EQ(s.last_expanded(), 0uz);
}

TYPED_TEST(SearchTest, StraightLine) {
    TypeParam  s(make_grid({"..........",
                            ".........."}));
    auto const res = s.find_path({0, 0}, {0, 9});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->cost, 9uz * pf::c_costOrth);
    EXPECT_EQ(res->waypoints.size(), 2uz);
    EXPECT_DOUBLE_EQ(res->length(), 9.0);
}

TYPED_TEST(SearchTest, PureDiagonal) {
    TypeParam  s(make_grid({".....",
                            ".....",
                            ".....",
                            ".....",
                            "....."}));
    auto const res = s.find_path({0, 0}, {4, 4});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->cost, 4uz * pf::c_costDiag);
    EXPECT_EQ(res->waypoints.size(), 2uz);
}

TYPED_TEST(SearchTest, MixedDiagonalAndStraight) {
    TypeParam  s(make_grid({"......",
                            "......",
                            "......"}));
    auto const res = s.find_path({0, 0}, {2, 5});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->cost, (2uz * pf::c_costDiag) + (3uz * pf::c_costOrth));
}

TYPED_TEST(SearchTest, PathIsSymmetric) {
    auto const g = make_grid({"........",
                              ".####...",
                              "...#..#.",
                              ".#.#.##.",
                              ".#......"});
    TypeParam  s(g);
    auto const ab = s.find_path({0, 0}, {4, 7});
    auto const ba = s.find_path({4, 7}, {0, 0});
    ASSERT_TRUE(ab.has_value());
    ASSERT_TRUE(ba.has_value());
    EXPECT_EQ(ab->cost, ba->cost);
    expect_valid_path(g, *ab, {0, 0}, {4, 7});
    expect_valid_path(g, *ba, {4, 7}, {0, 0});
}

TYPED_TEST(SearchTest, GoesAroundWall) {
    auto const g = make_grid({".....",
                              ".###.",
                              ".###.",
                              "....."});
    TypeParam  s(g);
    auto const res = s.find_path({1, 0}, {1, 4});
    ASSERT_TRUE(res.has_value());
    expect_valid_path(g, *res, {1, 0}, {1, 4});
    EXPECT_EQ(res->cost, reference_cost(g, {1, 0}, {1, 4}).value());
    EXPECT_GT(res->waypoints.size(), 2uz);
}

TYPED_TEST(SearchTest, SqueezesThroughGap) {
    auto const g = make_grid({"..#..",
                              "..#..",
                              ".....",
                              "..#..",
                              "..#.."});
    TypeParam  s(g);
    auto const res = s.find_path({0, 0}, {4, 4});
    ASSERT_TRUE(res.has_value());
    expect_valid_path(g, *res, {0, 0}, {4, 4});
    EXPECT_EQ(res->cost, reference_cost(g, {0, 0}, {4, 4}).value());
}

TYPED_TEST(SearchTest, DoesNotCutCorners) {
    auto const g = make_grid({".#.",
                              "...",
                              ".#."});
    TypeParam  s(g);
    auto const res = s.find_path({0, 0}, {0, 2});
    ASSERT_TRUE(res.has_value());
    expect_valid_path(g, *res, {0, 0}, {0, 2});
    // Both diagonal shortcuts squeeze past a wall, so the detour is 4 orthogonal steps
    EXPECT_EQ(res->cost, 4uz * pf::c_costOrth);
}

TYPED_TEST(SearchTest, DiagonalBetweenTouchingWallsIsRejected) {
    TypeParam  s(make_grid({".#",
                            "#."}));
    auto const res = s.find_path({0, 0}, {1, 1});
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), PathError::unreachable);
}

TYPED_TEST(SearchTest, OutOfBounds) {
    TypeParam s(make_grid({"...", "..."}));
    EXPECT_EQ(s.find_path({2, 0}, {0, 0}).error(), PathError::out_of_bounds);
    EXPECT_EQ(s.find_path({0, 0}, {0, 3}).error(), PathError::out_of_bounds);
}

TYPED_TEST(SearchTest, BlockedEndpoints) {
    TypeParam s(make_grid({".#.",
                           "..."}));
    EXPECT_EQ(s.find_path({0, 1}, {0, 2}).error(), PathError::start_blocked);
    EXPECT_EQ(s.find_path({0, 0}, {0, 1}).error(), PathError::goal_blocked);
    EXPECT_EQ(s.find_path({0, 1}, {0, 1}).error(), PathError::start_blocked);
}

TYPED_TEST(SearchTest, UnreachableAcrossRegions) {
    TypeParam  s(make_grid({"..#..",
                            "..#..",
                            "..#.."}));
    auto const res = s.find_path({0, 0}, {2, 4});
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), PathError::unreachable);
}

TYPED_TEST(SearchTest, SingleCellGrid) {
    TypeParam  s(make_grid({"."}));
    auto const res = s.find_path({0, 0}, {0, 0});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->cost, 0u);
}

TYPED_TEST(SearchTest, RepeatedSearchesAreIndependent) {
    auto const g = make_grid({"........",
                              ".######.",
                              ".#....#.",
                              ".#.##.#.",
                              ".#.#..#.",
                              "........"});
    TypeParam  s(g);
    auto const first = s.find_path({0, 0}, {4, 4});
    ASSERT_TRUE(first.has_value());
    EXPECT_FALSE(s.find_path({0, 0}, {1, 1}).has_value());
    auto const second = s.find_path({0, 0}, {4, 4});
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->cost, second->cost);
    EXPECT_EQ(first->waypoints, second->waypoints);
}

TYPED_TEST(SearchTest, ExposesGrid) {
    TypeParam s(make_grid({"...", "..."}));
    EXPECT_EQ(s.grid().rows(), 2uz);
    EXPECT_EQ(s.grid().cols(), 3uz);
}

TYPED_TEST(SearchTest, CopiesSearchIndependently) {
    auto const g = make_grid({"........",
                              ".####...",
                              "...#..#.",
                              ".#.#.##.",
                              ".#......"});
    TypeParam  a(g);
    TypeParam  b  = a;
    auto const ra = a.find_path({0, 0}, {4, 7});
    auto const rb = b.find_path({4, 7}, {0, 0});
    ASSERT_TRUE(ra.has_value());
    ASSERT_TRUE(rb.has_value());
    EXPECT_EQ(ra->cost, rb->cost);
}

TYPED_TEST(SearchTest, MatchesReferenceOnRandomGrids) {
    std::mt19937                      rng(12345u);
    constexpr std::array<double, 5uz> densities{0.0, 0.1, 0.2, 0.3, 0.45};

    for (int round = 0; round < 60; ++round) {
        auto const rows = std::uniform_int_distribution<size_t>(1uz, 24uz)(rng);
        auto const cols = std::uniform_int_distribution<size_t>(1uz, 24uz)(rng);
        auto const g    = make_random_grid(rng, rows, cols, densities[static_cast<size_t>(round) % densities.size()]);
        TypeParam  s(g);

        std::uniform_int_distribution<size_t> pickR(0uz, rows - 1uz);
        std::uniform_int_distribution<size_t> pickC(0uz, cols - 1uz);
        for (int q = 0; q < 25; ++q) {
            Pos_t const start{pickR(rng), pickC(rng)};
            Pos_t const goal{pickR(rng), pickC(rng)};
            auto const  res = s.find_path(start, goal);
            SCOPED_TRACE(testing::Message() << "round " << round << " " << rows << "x" << cols << " start {"
                                            << start[0] << "," << start[1] << "} goal {" << goal[0] << ","
                                            << goal[1] << "}");

            if (not g.is_walkable(start)) {
                ASSERT_FALSE(res.has_value());
                EXPECT_EQ(res.error(), PathError::start_blocked);
                continue;
            }
            if (not g.is_walkable(goal)) {
                ASSERT_FALSE(res.has_value());
                EXPECT_EQ(res.error(), PathError::goal_blocked);
                continue;
            }

            auto const expected = reference_cost(g, start, goal);
            if (not expected) {
                ASSERT_FALSE(res.has_value());
                EXPECT_EQ(res.error(), PathError::unreachable);
                continue;
            }
            ASSERT_TRUE(res.has_value());
            EXPECT_EQ(res->cost, *expected);
            expect_valid_path(g, *res, start, goal);
        }
    }
}

TEST(PathfindersSearch, GoalBoundedIsIndependentOfThreadCount) {
    std::mt19937 rng(777u);
    auto const   g = make_random_grid(rng, 20uz, 20uz, 0.25);

    pf::JpsPlusGB single(g, 1uz);
    pf::JpsPlusGB several(g, 4uz);
    pf::JpsPlusGB automatic(g);

    std::uniform_int_distribution<size_t> pick(0uz, 19uz);
    for (int q = 0; q < 60; ++q) {
        Pos_t const start{pick(rng), pick(rng)};
        Pos_t const goal{pick(rng), pick(rng)};
        auto const  a = single.find_path(start, goal);
        auto const  b = several.find_path(start, goal);
        auto const  c = automatic.find_path(start, goal);
        ASSERT_EQ(a.has_value(), b.has_value());
        ASSERT_EQ(a.has_value(), c.has_value());
        if (a) {
            EXPECT_EQ(a->cost, b->cost);
            EXPECT_EQ(a->cost, c->cost);
        }
        else {
            EXPECT_EQ(a.error(), b.error());
            EXPECT_EQ(a.error(), c.error());
        }
    }
}

TEST(PathfindersSearch, LargeOpenGridDiagonal) {
    auto const g = Grid::create(Pos_t{500uz, 700uz}, [](Pos_t const &) { return true; });
    ASSERT_TRUE(g.has_value());
    pf::JpsPlus s(*g);
    auto const  res = s.find_path({0, 0}, {499, 699});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->cost, (499uz * pf::c_costDiag) + (200uz * pf::c_costOrth));
}


// ####################################
// ### BIGGER SAMPLES
// ####################################

namespace {

std::vector<Pos_t>
walkable_cells(Grid const &g) {
    std::vector<Pos_t> res;
    for (size_t r = 0uz; r < g.rows(); ++r) {
        for (size_t c = 0uz; c < g.cols(); ++c) {
            if (g.is_walkable({r, c})) { res.push_back(Pos_t{r, c}); }
        }
    }
    return res;
}

// Random sources, each against random targets (all walkable cells when targetsPerSource == 0)
template <typename S>
void
check_against_reference(Grid const &g, S &searcher, std::mt19937 &rng, int sources, int targetsPerSource) {
    auto const cells = walkable_cells(g);
    ASSERT_FALSE(cells.empty());
    std::uniform_int_distribution<size_t> pick(0uz, cells.size() - 1uz);

    for (int si = 0; si < sources; ++si) {
        Pos_t const start = cells[pick(rng)];
        auto const  costs = reference_costs(g, start);

        auto const check = [&](Pos_t const &goal) {
            auto const expected = costs[(goal[0] * g.cols()) + goal[1]];
            auto const res      = searcher.find_path(start, goal);
            SCOPED_TRACE(testing::Message() << "start {" << start[0] << "," << start[1] << "} goal {" << goal[0]
                                            << "," << goal[1] << "}");
            if (expected == std::numeric_limits<Cost_t>::max()) {
                ASSERT_FALSE(res.has_value());
                EXPECT_EQ(res.error(), PathError::unreachable);
                return;
            }
            ASSERT_TRUE(res.has_value());
            EXPECT_EQ(res->cost, expected);
            expect_valid_path(g, *res, start, goal);
        };

        if (targetsPerSource == 0) {
            for (auto const &goal : cells) { check(goal); }
        }
        else {
            for (int t = 0; t < targetsPerSource; ++t) { check(cells[pick(rng)]); }
        }
        if (::testing::Test::HasFatalFailure()) { return; }
    }
}

// Perfect maze (recursive backtracker) in a (2 * cells + 1) square grid. 'braid' is the chance of knocking out
// additional walls to create loops.
Grid
make_maze(std::mt19937 &rng, size_t mazeCells, double braid) {
    size_t const              n = (2uz * mazeCells) + 1uz;
    std::vector<std::uint8_t> open(n * n, 0);
    std::vector<bool>         seen(mazeCells * mazeCells, false);

    std::vector<Pos_t> stack{Pos_t{0uz, 0uz}};
    seen[0]      = true;
    open[n + 1uz] = 1;
    while (not stack.empty()) {
        auto const cur = stack.back();
        std::vector<Pos_t> next;
        if (cur[0] > 0uz && not seen[((cur[0] - 1uz) * mazeCells) + cur[1]]) { next.push_back({cur[0] - 1uz, cur[1]}); }
        if (cur[0] + 1uz < mazeCells && not seen[((cur[0] + 1uz) * mazeCells) + cur[1]]) {
            next.push_back({cur[0] + 1uz, cur[1]});
        }
        if (cur[1] > 0uz && not seen[(cur[0] * mazeCells) + cur[1] - 1uz]) { next.push_back({cur[0], cur[1] - 1uz}); }
        if (cur[1] + 1uz < mazeCells && not seen[(cur[0] * mazeCells) + cur[1] + 1uz]) {
            next.push_back({cur[0], cur[1] + 1uz});
        }
        if (next.empty()) {
            stack.pop_back();
            continue;
        }
        auto const nx = next[std::uniform_int_distribution<size_t>(0uz, next.size() - 1uz)(rng)];
        seen[(nx[0] * mazeCells) + nx[1]] = true;
        open[((2uz * nx[0] + 1uz) * n) + (2uz * nx[1]) + 1uz] = 1;
        // Wall between the two maze cells
        open[((cur[0] + nx[0] + 1uz) * n) + cur[1] + nx[1] + 1uz] = 1;
        stack.push_back(nx);
    }

    std::bernoulli_distribution knock(braid);
    for (size_t r = 1uz; r + 1uz < n; ++r) {
        for (size_t c = 1uz; c + 1uz < n; ++c) {
            if (((r + c) & 1uz) != 0uz && knock(rng)) { open[(r * n) + c] = 1; }
        }
    }
    return Grid::create(Pos_t{n, n}, [&](Pos_t const &p) { return open[(p[0] * n) + p[1]] != 0; }).value();
}

// Square rooms separated by walls, every wall segment has one random door
Grid
make_rooms(std::mt19937 &rng, size_t roomsPerSide, size_t roomSize) {
    size_t const              n = (roomsPerSide * (roomSize + 1uz)) + 1uz;
    std::vector<std::uint8_t> open(n * n, 0);
    for (size_t r = 0uz; r < n; ++r) {
        for (size_t c = 0uz; c < n; ++c) {
            open[(r * n) + c] = (r % (roomSize + 1uz) != 0uz && c % (roomSize + 1uz) != 0uz) ? 1 : 0;
        }
    }
    std::uniform_int_distribution<size_t> door(0uz, roomSize - 1uz);
    for (size_t i = 0uz; i <= roomsPerSide; ++i) {
        for (size_t j = 0uz; j < roomsPerSide; ++j) {
            size_t const line = i * (roomSize + 1uz);
            size_t const pos  = (j * (roomSize + 1uz)) + 1uz + door(rng);
            if (i > 0uz && i < roomsPerSide) {
                open[(line * n) + pos] = 1;
                open[(pos * n) + line] = 1;
            }
        }
    }
    return Grid::create(Pos_t{n, n}, [&](Pos_t const &p) { return open[(p[0] * n) + p[1]] != 0; }).value();
}

} // namespace

TEST(PathfindersBig, JpsPlusRandomGrids150x150) {
    std::mt19937 rng(2024u);
    for (double const density : {0.05, 0.15, 0.25, 0.35, 0.45}) {
        SCOPED_TRACE(testing::Message() << "density " << density);
        auto const  g = make_random_grid(rng, 150uz, 150uz, density);
        pf::JpsPlus s(g);
        check_against_reference(g, s, rng, 4, 150);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
    }
}

TEST(PathfindersBig, JpsPlusNonSquareGrids) {
    std::mt19937 rng(99u);
    for (auto const [rows, cols] : {std::pair{1uz, 600uz}, std::pair{600uz, 1uz}, std::pair{20uz, 400uz},
                                    std::pair{400uz, 20uz}, std::pair{97uz, 211uz}}) {
        SCOPED_TRACE(testing::Message() << rows << "x" << cols);
        auto const  g = make_random_grid(rng, rows, cols, 0.15);
        pf::JpsPlus s(g);
        check_against_reference(g, s, rng, 3, 100);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
    }
}

TEST(PathfindersBig, JpsPlusExhaustiveFromSourceOn40x40) {
    std::mt19937 rng(5u);
    auto const   g = make_random_grid(rng, 40uz, 40uz, 0.3);
    pf::JpsPlus  s(g);
    check_against_reference(g, s, rng, 3, 0);
}

TEST(PathfindersBig, JpsPlusPerfectMaze) {
    std::mt19937 rng(31337u);
    auto const   g = make_maze(rng, 100uz, 0.0); // 201x201
    pf::JpsPlus  s(g);
    check_against_reference(g, s, rng, 3, 60);
}

TEST(PathfindersBig, JpsPlusBraidedMaze) {
    std::mt19937 rng(8080u);
    for (double const braid : {0.05, 0.2, 0.5}) {
        SCOPED_TRACE(testing::Message() << "braid " << braid);
        auto const  g = make_maze(rng, 60uz, braid); // 121x121
        pf::JpsPlus s(g);
        check_against_reference(g, s, rng, 3, 80);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
    }
}

TEST(PathfindersBig, JpsPlusRoomsAndDoors) {
    std::mt19937 rng(777u);
    auto const   g = make_rooms(rng, 8uz, 15uz); // 129x129
    pf::JpsPlus  s(g);
    check_against_reference(g, s, rng, 4, 120);
}

TEST(PathfindersBig, JpsPlusSerpentineCorridor) {
    // Alternating full walls with a single gap at alternating ends force a long snaking route
    constexpr size_t rows = 101uz;
    constexpr size_t cols = 200uz;
    auto const       g    = Grid::create(Pos_t{rows, cols}, [](Pos_t const &p) {
        if (p[0] % 2uz == 0uz) { return true; }
        size_t const gap = ((p[0] / 2uz) % 2uz == 0uz) ? cols - 1uz : 0uz;
        return p[1] == gap;
    }).value();

    pf::JpsPlus s(g);
    auto const  res = s.find_path({0, 0}, {rows - 1uz, 0});
    ASSERT_TRUE(res.has_value());
    expect_valid_path(g, *res, {0, 0}, {rows - 1uz, 0});
    EXPECT_EQ(res->cost, reference_cost(g, {0, 0}, {rows - 1uz, 0}).value());
    EXPECT_GT(res->length(), 50.0 * static_cast<double>(cols) * 0.9);
}

TEST(PathfindersBig, JpsPlusSparseObstaclesLargeGrid) {
    std::mt19937 rng(1234u);
    auto const   g = make_random_grid(rng, 600uz, 600uz, 0.02);
    pf::JpsPlus  s(g);
    check_against_reference(g, s, rng, 2, 40);
}

TEST(PathfindersBig, JpsPlusLongestSupportedCorridor) {
    // Jump distances are stored as int16_t, this is the longest representable run
    auto const  g = Grid::create(Pos_t{1uz, pf::c_maxExtent}, [](Pos_t const &) { return true; });
    ASSERT_TRUE(g.has_value());
    pf::JpsPlus s(*g);

    auto const fwd = s.find_path({0, 0}, {0, pf::c_maxExtent - 1uz});
    ASSERT_TRUE(fwd.has_value());
    EXPECT_EQ(fwd->cost, (pf::c_maxExtent - 1uz) * pf::c_costOrth);
    EXPECT_EQ(fwd->waypoints.size(), 2uz);

    auto const bwd = s.find_path({0, pf::c_maxExtent - 1uz}, {0, 0});
    ASSERT_TRUE(bwd.has_value());
    EXPECT_EQ(bwd->cost, fwd->cost);
}

TEST(PathfindersBig, AllSearchersAgreeOnMazeAndRandomGrid) {
    std::mt19937 rng(4711u);
    auto const   maze   = make_maze(rng, 25uz, 0.15); // 51x51
    auto const   random = make_random_grid(rng, 60uz, 60uz, 0.25);

    for (Grid const &g : {maze, random}) {
        pf::Jps       jps(g);
        pf::JpsPlus   plus(g);
        pf::JpsPlusGB bounded(g);

        auto const                            cells = walkable_cells(g);
        std::uniform_int_distribution<size_t> pick(0uz, cells.size() - 1uz);
        for (int q = 0; q < 150; ++q) {
            auto const &start = cells[pick(rng)];
            auto const &goal  = cells[pick(rng)];
            auto const  a     = jps.find_path(start, goal);
            auto const  b     = plus.find_path(start, goal);
            auto const  c     = bounded.find_path(start, goal);
            ASSERT_EQ(a.has_value(), b.has_value());
            ASSERT_EQ(a.has_value(), c.has_value());
            if (a) {
                EXPECT_EQ(a->cost, b->cost);
                EXPECT_EQ(a->cost, c->cost);
            }
        }
    }
}

TEST(PathfindersBig, JpsPlusConcurrentCopies) {
    std::mt19937 rng(606u);
    auto const   g = make_random_grid(rng, 120uz, 120uz, 0.2);
    pf::JpsPlus  base(g);

    auto const                            cells = walkable_cells(g);
    std::uniform_int_distribution<size_t> pick(0uz, cells.size() - 1uz);
    std::vector<std::pair<Pos_t, Pos_t>> queries;
    for (int q = 0; q < 200; ++q) { queries.emplace_back(cells[pick(rng)], cells[pick(rng)]); }

    std::vector<std::optional<Cost_t>> expected;
    for (auto const &[a, b] : queries) { expected.push_back(reference_cost(g, a, b)); }

    constexpr size_t                   threads = 4uz;
    std::vector<std::vector<std::optional<Cost_t>>> results(threads);
    {
        std::vector<std::jthread> pool;
        for (size_t t = 0uz; t < threads; ++t) {
            pool.emplace_back([&, t] {
                pf::JpsPlus local = base; // Shares the immutable jumper, owns its scratch memory
                for (auto const &[a, b] : queries) {
                    auto const res = local.find_path(a, b);
                    results[t].push_back(res ? std::optional<Cost_t>{res->cost} : std::nullopt);
                }
            });
        }
    }
    for (size_t t = 0uz; t < threads; ++t) { EXPECT_EQ(results[t], expected) << "thread " << t; }
}