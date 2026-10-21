#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <incstd/solvers/pathfinders.hpp>

namespace pf = incom::standard::solvers::pathfinders;

using pf::Grid;
using pf::Pos_t;

namespace {

// ####################################
// ### SAMPLE GENERATORS
// ####################################

using Query = std::pair<Pos_t, Pos_t>;

Grid
from_open(size_t rows, size_t cols, std::vector<std::uint8_t> const &open) {
    return Grid::create(Pos_t{rows, cols}, [&](Pos_t const &p) { return open[(p[0] * cols) + p[1]] != 0; }).value();
}

Grid
make_open(size_t rows, size_t cols) {
    return Grid::create(Pos_t{rows, cols}, [](Pos_t const &) { return true; }).value();
}

Grid
make_random(std::mt19937 &rng, size_t n, double density) {
    std::bernoulli_distribution wall(density);
    std::vector<std::uint8_t>   open(n * n);
    for (auto &c : open) { c = wall(rng) ? 0 : 1; }
    return from_open(n, n, open);
}

// Random rectangular obstacles of 2..maxSide cells on an otherwise open grid
Grid
make_blocks(std::mt19937 &rng, size_t n, size_t count, size_t maxSide) {
    std::vector<std::uint8_t>             open(n * n, 1);
    std::uniform_int_distribution<size_t> pos(0uz, n - 1uz);
    std::uniform_int_distribution<size_t> side(2uz, maxSide);
    for (size_t i = 0uz; i < count; ++i) {
        size_t const r = pos(rng);
        size_t const c = pos(rng);
        size_t const h = side(rng);
        size_t const w = side(rng);
        for (size_t y = r; y < std::min(n, r + h); ++y) {
            for (size_t x = c; x < std::min(n, c + w); ++x) { open[(y * n) + x] = 0; }
        }
    }
    return from_open(n, n, open);
}

// Recursive backtracker maze in a (2 * mazeCells + 1) square grid, 'braid' is the chance to knock out extra walls
Grid
make_maze(std::mt19937 &rng, size_t mazeCells, double braid) {
    size_t const              n = (2uz * mazeCells) + 1uz;
    std::vector<std::uint8_t> open(n * n, 0);
    std::vector<bool>         seen(mazeCells * mazeCells, false);

    std::vector<Pos_t> stack{Pos_t{0uz, 0uz}};
    seen[0]       = true;
    open[n + 1uz] = 1;
    while (not stack.empty()) {
        Pos_t const        cur = stack.back();
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
        Pos_t const nx                    = next[std::uniform_int_distribution<size_t>(0uz, next.size() - 1uz)(rng)];
        seen[(nx[0] * mazeCells) + nx[1]] = true;
        open[((2uz * nx[0] + 1uz) * n) + (2uz * nx[1]) + 1uz]     = 1;
        open[((cur[0] + nx[0] + 1uz) * n) + cur[1] + nx[1] + 1uz] = 1; // Wall between the two maze cells
        stack.push_back(nx);
    }

    std::bernoulli_distribution knock(braid);
    for (size_t r = 1uz; r + 1uz < n; ++r) {
        for (size_t c = 1uz; c + 1uz < n; ++c) {
            if ((r + c) % 2uz == 1uz && knock(rng)) { open[(r * n) + c] = 1; }
        }
    }
    return from_open(n, n, open);
}

// Square rooms separated by walls, every wall segment has one random door
Grid
make_rooms(std::mt19937 &rng, size_t roomsPerSide, size_t roomSize) {
    size_t const              n = (roomsPerSide * (roomSize + 1uz)) + 1uz;
    std::vector<std::uint8_t> open(n * n, 1);
    for (size_t i = 0uz; i <= roomsPerSide; ++i) {
        size_t const w = i * (roomSize + 1uz);
        for (size_t k = 0uz; k < n; ++k) {
            open[(w * n) + k] = 0;
            open[(k * n) + w] = 0;
        }
    }
    std::uniform_int_distribution<size_t> door(0uz, roomSize - 1uz);
    for (size_t i = 0uz; i <= roomsPerSide; ++i) {
        size_t const w = i * (roomSize + 1uz);
        for (size_t j = 0uz; j < roomsPerSide; ++j) {
            size_t const d    = (j * (roomSize + 1uz)) + 1uz + door(rng);
            open[(w * n) + d] = 1;
            open[(d * n) + w] = 1;
        }
    }
    return from_open(n, n, open);
}

// Full walls with a single gap at alternating ends, forces one long snaking route
Grid
make_serpentine(size_t rows, size_t cols) {
    return Grid::create(Pos_t{rows, cols},
                        [=](Pos_t const &p) {
                            if (p[0] % 2uz == 0uz) { return true; }
                            return p[1] == (((p[0] / 2uz) % 2uz == 0uz) ? cols - 1uz : 0uz);
                        })
        .value();
}

// Random connected start/goal pairs that are far apart (at least a third of the larger extent)
std::vector<Query>
make_queries(Grid const &g, std::mt19937 &rng, size_t count) {
    std::vector<Pos_t> cells;
    for (size_t r = 0uz; r < g.rows(); ++r) {
        for (size_t c = 0uz; c < g.cols(); ++c) {
            if (g.is_walkable({r, c})) { cells.push_back({r, c}); }
        }
    }

    size_t const                          minDist = std::max(g.rows(), g.cols()) / 3uz;
    std::uniform_int_distribution<size_t> pick(0uz, cells.size() - 1uz);
    std::vector<Query>                    res;
    for (size_t attempts = 0uz; res.size() < count && attempts < count * 1000uz; ++attempts) {
        Pos_t const a = cells[pick(rng)];
        Pos_t const b = cells[pick(rng)];
        auto const  d = std::max(a[0] > b[0] ? a[0] - b[0] : b[0] - a[0], a[1] > b[1] ? a[1] - b[1] : b[1] - a[1]);
        if (g.region_of(a) == g.region_of(b) && d >= minDist) { res.emplace_back(a, b); }
    }
    return res;
}

struct Sample {
    std::string        name;
    Grid               grid;
    std::vector<Query> queries;
};

Sample
make_sample(std::string name, Grid grid, std::mt19937 &rng, size_t queries = 50uz) {
    auto q = make_queries(grid, rng, queries);
    return Sample{.name = std::move(name), .grid = std::move(grid), .queries = std::move(q)};
}

std::string
describe(Sample const &s) {
    return s.name + " (" + std::to_string(s.grid.rows()) + "x" + std::to_string(s.grid.cols()) + ")";
}

// ####################################
// ### BENCHMARKS
// ####################################

// GoalBounded preprocessing is O(cells^2), only feasible on small grids
constexpr size_t c_maxCellsGoalBounding = 250uz * 250uz;
constexpr size_t c_maxCellsBuildBench   = 1000uz * 1000uz;

template <typename S>
void
bench_search(ankerl::nanobench::Bench &bench, std::string const &name, Sample const &sample) {
    S searcher(sample.grid);
    bench.run(name, [&] {
        for (auto const &[a, b] : sample.queries) {
            auto res = searcher.find_path(a, b);
            ankerl::nanobench::doNotOptimizeAway(res);
        }
    });
}

template <typename S>
void
bench_build(ankerl::nanobench::Bench &bench, std::string const &name, Sample const &sample) {
    bench.run(name, [&] {
        S searcher(sample.grid);
        ankerl::nanobench::doNotOptimizeAway(searcher);
    });
}

void
run_queries(Sample const &sample) {
    ankerl::nanobench::Bench bench;
    bench.title("find_path: " + describe(sample))
        .unit("query")
        .batch(sample.queries.size())
        .warmup(2)
        .relative(true)
        .minEpochIterations(3);

    bench_search<pf::Jps>(bench, "Jps", sample);
    bench_search<pf::JpsPlus>(bench, "JpsPlus", sample);
    if (sample.grid.rows() * sample.grid.cols() <= c_maxCellsGoalBounding) {
        bench_search<pf::JpsPlusGB>(bench, "JpsPlusGB", sample);
    }
}

void
run_construction(Sample const &sample) {
    ankerl::nanobench::Bench bench;
    bench.title("construction: " + describe(sample)).unit("build").warmup(1).relative(true).minEpochIterations(2);

    bench_build<pf::Jps>(bench, "Jps", sample);
    bench_build<pf::JpsPlus>(bench, "JpsPlus", sample);
    bench_build<pf::JpsPlusGB>(bench, "JpsPlusGB", sample);
}

} // namespace

int
main() {
    std::mt19937 rng(20261021u);

    std::vector<Sample> samples;
    // Small, GoalBounded friendly
    samples.push_back(make_sample("open", make_open(64uz, 64uz), rng));
    samples.push_back(make_sample("random 25%", make_random(rng, 64uz, 0.25), rng));
    samples.push_back(make_sample("perfect maze", make_maze(rng, 32uz, 0.0), rng));
    // Medium
    samples.push_back(make_sample("open", make_open(200uz, 200uz), rng));
    samples.push_back(make_sample("random 10%", make_random(rng, 200uz, 0.10), rng));
    samples.push_back(make_sample("random 35%", make_random(rng, 200uz, 0.35), rng));
    samples.push_back(make_sample("sparse blocks", make_blocks(rng, 200uz, 150uz, 12uz), rng));
    samples.push_back(make_sample("perfect maze", make_maze(rng, 100uz, 0.0), rng));
    samples.push_back(make_sample("braided maze", make_maze(rng, 100uz, 0.2), rng));
    samples.push_back(make_sample("rooms and doors", make_rooms(rng, 12uz, 15uz), rng));
    samples.push_back(make_sample("serpentine", make_serpentine(101uz, 200uz), rng, 5uz));
    // Large, GoalBounded is skipped
    samples.push_back(make_sample("open", make_open(1024uz, 1024uz), rng, 20uz));
    samples.push_back(make_sample("random 5%", make_random(rng, 1024uz, 0.05), rng, 20uz));
    samples.push_back(make_sample("sparse blocks", make_blocks(rng, 1024uz, 2500uz, 20uz), rng, 20uz));
    samples.push_back(make_sample("perfect maze", make_maze(rng, 500uz, 0.0), rng, 20uz));
    samples.push_back(make_sample("rooms and doors", make_rooms(rng, 32uz, 31uz), rng, 20uz));

    for (auto const &s : samples) {
        if (s.queries.empty()) {
            std::cerr << "No usable queries for sample " << s.name << "\n";
            continue;
        }
        run_queries(s);
    }

    for (auto const &s : samples) {
        if (s.grid.rows() * s.grid.cols() <= c_maxCellsBuildBench) { run_construction(s); }
    }
    return 0;
}
