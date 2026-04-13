#pragma once

#include <cassert>
#include <cmath>
#include <format>
#include <limits>
#include <ranges>


#include <ankerl/unordered_dense.h>
#include <more_concepts/more_concepts.hpp>

#include <incstd/core/hashing.hpp>
#include <incstd/core/matrix.hpp>


namespace incom::standard::solvers {
using namespace incom::standard;

namespace packing {
namespace {
struct FastPseudoRandom {
    uint64_t m_state = 0x9e3779b97f4a7c15ull;

    FastPseudoRandom() = default;

    explicit FastPseudoRandom(uint64_t seed) { setSeed(seed); }

    void
    setSeed(uint64_t seed) {
        m_state = seed;
        if (m_state == 0ull) { m_state = 0x9e3779b97f4a7c15ull; }
    }

    uint64_t
    nextRandomWord() {
        uint64_t z = (m_state += 0x9e3779b97f4a7c15ull);
        z          = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z          = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return z ^ (z >> 31);
    }

    static uint64_t
    multiplyHigh64(uint64_t lhs, uint64_t rhs) {
#if defined(_MSC_VER) && ! defined(__clang__)
        uint64_t high = 0;
        _umul128(lhs, rhs, &high);
        return high;
#else
        return static_cast<uint64_t>((static_cast<unsigned __int128>(lhs) * rhs) >> 64);
#endif
    }

    size_t
    pseudoRandom_0_to(size_t maxInclusive) {
        if (maxInclusive == std::numeric_limits<size_t>::max()) { return static_cast<size_t>(nextRandomWord()); }

        auto const bound = static_cast<uint64_t>(maxInclusive) + 1ull;
        return static_cast<size_t>(multiplyHigh64(nextRandomWord(), bound));
    }
};
} // namespace


template <size_t SQSZ>
requires(SQSZ > 2)
class BoxPacker_2D {
public:
    struct Pos {
        long long y = 0;
        long long x = 0;
    };
    struct Shape {
        struct OverlayRes {
            Shape res_shp;

            size_t pointsAdded;
            size_t pointsOverlaid;
            size_t bordersTouching;
            size_t bordersNotTouching;

            double surfaceCovered_relative;
            double surfaceOpened_relative;
        };

        std::array<std::array<bool, SQSZ>, SQSZ> m_matrix = {};

        // Construction
        Shape()                 = default;
        Shape(Shape const &src) = default;
        Shape(Shape &&src)      = default;
        ~Shape()                = default;

        Shape &
        operator=(Shape const &) = default;
        Shape &
        operator=(Shape &&) = default;

        auto
        operator<=>(Shape const &other) const = default;


        Shape(std::array<std::array<bool, SQSZ - 2>, SQSZ - 2> const &src) {
            for (auto r = 1uz; auto const &line : src) {
                for (auto c = 1uz; bool one : line) { m_matrix[r][c++] = one; }
                r++;
            }
        }
        Shape(std::array<std::array<bool, SQSZ>, SQSZ> const &src) : m_matrix(src) {}

        OverlayRes
        compute_overlayWith(Shape const &other) const {
            OverlayRes res{{}, 0uz, 0uz, 0uz};
            for (size_t r = 0; r < SQSZ; ++r) {
                for (size_t c = 0; c < SQSZ; ++c) {
                    res.pointsOverlaid         += (m_matrix[r][c] and other.m_matrix[r][c]);
                    res.pointsAdded            += (not m_matrix[r][c]) and other.m_matrix[r][c];
                    res.res_shp.m_matrix[r][c]  = m_matrix[r][c] | other.m_matrix[r][c];
                }
            }
            for (size_t r = 1; r < SQSZ - 1; ++r) {
                for (size_t c = 1; c < SQSZ - 1; ++c) {
                    if (other.m_matrix[r][c]) {
                        res.bordersTouching += m_matrix[r - 1][c] & (not other.m_matrix[r - 1][c]);
                        res.bordersTouching += m_matrix[r][c - 1] & (not other.m_matrix[r][c - 1]);
                        res.bordersTouching += m_matrix[r][c + 1] & (not other.m_matrix[r][c + 1]);
                        res.bordersTouching += m_matrix[r + 1][c] & (not other.m_matrix[r + 1][c]);

                        res.bordersNotTouching += (not m_matrix[r - 1][c]) & (not other.m_matrix[r - 1][c]);
                        res.bordersNotTouching += (not m_matrix[r][c - 1]) & (not other.m_matrix[r][c - 1]);
                        res.bordersNotTouching += (not m_matrix[r][c + 1]) & (not other.m_matrix[r][c + 1]);
                        res.bordersNotTouching += (not m_matrix[r + 1][c]) & (not other.m_matrix[r + 1][c]);
                    }
                }
            }

            res.surfaceCovered_relative = (res.bordersTouching / static_cast<double>(res.pointsAdded));
            res.surfaceOpened_relative  = (res.bordersNotTouching / static_cast<double>(res.pointsAdded));

            return res;
        }

        std::vector<Shape>
        compute_alternsRotFlip() const {
            namespace incmatrix = incom::standard::matrix;

            auto                                                                                m_matrix_cpy = m_matrix;
            ankerl::unordered_dense::set<decltype(m_matrix_cpy), standard::hashing::XXH3Hasher> hlprMP;

            hlprMP.insert(m_matrix_cpy);
            for (int rot_i = 0; rot_i < 3; ++rot_i) {
                incmatrix::matrixRotateLeft(m_matrix_cpy);
                hlprMP.insert(m_matrix_cpy);
            }

            // Flip vertically
            for (size_t i = 0uz; i < (m_matrix_cpy.size() / 2); ++i) {
                std::swap(m_matrix_cpy.at(i), m_matrix_cpy.at(m_matrix_cpy.size() - 1 - i));
            }

            hlprMP.insert(m_matrix_cpy);
            for (int rot_i = 0; rot_i < 3; ++rot_i) {
                incmatrix::matrixRotateLeft(m_matrix_cpy);
                hlprMP.insert(m_matrix_cpy);
            }

            return std::vector<Shape>(hlprMP.begin(), hlprMP.end());
        }

        // ADL for hashing using XXH3Hasher
        friend constexpr void
        XXH3Hash(Shape const &input, XXH3_state_t *state) {
            XXH3_64bits_update(state, input.m_matrix.data(), sizeof(decltype(input.m_matrix)));
        }
    };


    using Shape_t   = Shape;
    using PastRes_t = std::pair<Pos, typename Shape_t::OverlayRes>; // Pos == [][] positions in 'm_shapes_alterns'
    using pastResMap_t =
        ankerl::unordered_dense::segmented_map<Shape_t, std::vector<PastRes_t>, incom::standard::hashing::XXH3Hasher>;


    // 1) Position (top left) in matrix, 2) What shape currently is at that position, 3) Possibilities
    std::vector<std::tuple<Pos, std::reference_wrapper<Shape_t>, std::reference_wrapper<std::vector<PastRes_t>>>>
        m_frontierTiles;

private:
    std::vector<std::vector<char>> m_area;
    Pos                            m_firstTilePos;

    std::vector<std::vector<Shape_t>> m_shapes_alterns;
    std::vector<size_t>               m_useableCount_perShape;
    std::vector<double>               m_shapesRatios_orig;
    FastPseudoRandom                  m_fprng;

    // Memoization of what 'shapes_alterns'
    pastResMap_t m_pastComputed;

    void
    set_pseudoRandomSeed(uint64_t seed) {
        m_fprng.setSeed(seed);
    }


    std::size_t
    hash_stateOfSelf() {
        XXH3_state_t *state = XXH3_createState();
        XXH3_64bits_reset_withSeed(state, 0);

        size_t const m_area_ySz = m_area.size();
        size_t const m_area_xSz = m_area.empty() ? 0uz : m_area.front().size();
        XXH3_64bits_update(state, &m_area_ySz, sizeof(size_t));
        XXH3_64bits_update(state, &m_area_xSz, sizeof(size_t));

        if (m_frontierTiles.size() > 0) {
            XXH3_64bits_update(state, &std::get<0>(m_frontierTiles.front()).y, sizeof(long long));
            XXH3_64bits_update(state, &std::get<0>(m_frontierTiles.front()).x, sizeof(long long));
        }

        for (auto const &alternsLine : m_shapes_alterns) {
            for (auto const &shp : alternsLine) { XXH3Hash(shp, state); }
        }
        // XXH3_64bits_update(state, m_useableCount_perShape.data(),
        //                    sizeof(typename std::remove_cvref_t<decltype(m_useableCount_perShape)>::value_type) *
        //                        m_useableCount_perShape.size());

        XXH64_hash_t result = XXH3_64bits_digest(state);
        XXH3_freeState(state);
        return result;
    }

    std::optional<std::tuple<Shape_t &, std::vector<PastRes_t> &>>
    get_possibsFor(Shape_t const &tile) {
        if (auto found = m_pastComputed.find(tile); found != m_pastComputed.end()) {
            return std::tie(found->first, found->second);
        }
        return std::nullopt;
    }

    std::tuple<Shape_t &, std::vector<PastRes_t> &>
    compute_possibsFor(Shape_t const &tile) {
        std::vector<PastRes_t> resToMap;
        auto allowed = [](PastRes_t const &toCheck) -> bool { return (toCheck.second.pointsOverlaid == 0uz); };

        for (long long shpID = 0; shpID < m_shapes_alterns.size(); ++shpID) {
            for (long long alternID = 0; alternID < m_shapes_alterns.at(shpID).size(); ++alternID) {
                auto rs =
                    PastRes_t{Pos{shpID, alternID}, tile.compute_overlayWith(m_shapes_alterns.at(shpID).at(alternID))};
                if (allowed(rs)) { resToMap.push_back(rs); }
            }
        }
        std::ranges::sort(resToMap, [](auto const &l, auto const &r) -> bool {
            double const soDif = r.second.surfaceOpened_relative - l.second.surfaceOpened_relative;
            if (soDif == 0.0) { return l.second.pointsAdded > r.second.pointsAdded; }
            else { return std::abs(soDif) + soDif; }
        });
        auto insRes = m_pastComputed.insert({tile, resToMap});
        return std::tie(insRes.first->first, insRes.first->second);
    }

    std::vector<Pos>
    get_surrOverlappingPoss(Pos const &shapePos) {
        std::vector<Pos> res;

        size_t const rows = m_area.size();
        size_t const cols = m_area.size() > 0 ? m_area.front().size() : 0;

        constexpr long long SQSZcpy = static_cast<long long>(SQSZ);

        // shapePos needs to be the Pos of some valid window in our area
        if (shapePos.y <= (rows - SQSZ) && shapePos.x <= (cols - SQSZ)) {
            for (int rc = -(SQSZcpy - 2); rc < (SQSZcpy - 1); ++rc) {
                for (int cc = -(SQSZcpy - 2); cc < (SQSZcpy - 1); ++cc) {
                    long long const r_loc = shapePos.y + rc;
                    long long const c_loc = shapePos.x + cc;
                    // if (rc != 0 && cc != 0) {
                    //     if (r_loc >= 0 && r_loc <= (rows - SQSZ) && c_loc >= 0 && c_loc <= (cols - SQSZ)) {
                    //         res.push_back(Pos{.y = r_loc, .x = c_loc});
                    //     }
                    // }
                    if (r_loc >= 0 && r_loc <= (rows - SQSZ) && c_loc >= 0 && c_loc <= (cols - SQSZ)) {
                        res.push_back(Pos{.y = r_loc, .x = c_loc});
                    }
                }
            }
        }
        return res;
    }

    std::expected<Shape_t, int>
    get_windowAtPos(Pos const &shapePos) {
        size_t const rows = m_area.size();
        size_t const cols = m_area.size() > 0 ? m_area.front().size() : 0;

        std::expected<Shape_t, int> res{std::unexpected(0)};

        if (shapePos.y >= 0 && shapePos.y <= (rows - SQSZ) && shapePos.x >= 0 && shapePos.x <= (cols - SQSZ)) {
            res             = Shape_t{};
            Shape_t &shpRes = res.value();

            for (int row = shapePos.y; row < shapePos.y + SQSZ; ++row) {
                for (int col = shapePos.x; col < (shapePos.x + SQSZ); ++col) {
                    shpRes.m_matrix[row - shapePos.y][col - shapePos.x] = m_area[row][col];
                }
            }
        }
        return res;
    }

    bool
    is_posValid(Pos const &p) {
        const long long py = p.y;
        const long long px = p.x;
        if (py < 0ll || py > (static_cast<long long>(m_area.size()) - SQSZ) || px < 0ll ||
            px > (m_area.size() == 0 ? 0 : m_area.front().size() - SQSZ)) {
            return false;
        }
        return true;
    }

    bool
    set_windowAtPos(Pos const &shapePos, PastRes_t const &pr) {
        if (not is_posValid(shapePos)) { return false; }
        for (long long r = shapePos.y; r < (shapePos.y + SQSZ); ++r) {
            for (long long c = shapePos.x; c < (shapePos.x + SQSZ); ++c) {
                m_area[r][c] = pr.second.res_shp.m_matrix[r - shapePos.y][c - shapePos.x];
            }
        }
        return true;
    }


    size_t
    erase_fromFrontier(std::vector<Pos> const &shapePoss) {
        auto const [ite_first, ite_last] = std::ranges::remove_if(m_frontierTiles, [&](auto &tpl) {
            return std::ranges::find_if(shapePoss, [&](Pos const &onePos) {
                       return ((std::get<0>(tpl).y == onePos.y) && (std::get<0>(tpl).x == onePos.x));
                   }) != shapePoss.end();
        });
        size_t const removed             = ite_last - ite_first;
        m_frontierTiles.erase(ite_first, ite_last);
        return removed;
    }

    size_t
    add_toFrontier(std::vector<Pos> const &shapePoss) {
        size_t resCount = 0uz;
        for (auto const &onePos : shapePoss) {
            auto window = get_windowAtPos(onePos);
            if (not window.has_value()) { continue; }

            auto possibsForWindow = getOrCompute_possibsFor(window.value());
            if (std::get<1>(possibsForWindow).size() > 0) {
                m_frontierTiles.push_back(std::tuple_cat(std::make_tuple(onePos), possibsForWindow));
            }
            resCount++;
        }
        return resCount;
    }

    auto
    compute_perShapeScoringAdjustments() {
        auto ratiosHlprView = std::views::transform(
            m_useableCount_perShape,
            [&, id = 0uz,
             sum = static_cast<double>(std::ranges::fold_left_first(m_useableCount_perShape, std::plus{}).value_or(0))](
                size_t oneCount) mutable {
                return (oneCount == 0uz ? std::numeric_limits<double>::max() : (sum / oneCount)) *
                       m_shapesRatios_orig.at(id++);
            });

        // This wierd adjustment is to prevent the solver f
        return decltype(m_shapesRatios_orig)(ratiosHlprView.begin(), ratiosHlprView.end());
    }


public:
    // Construction
    BoxPacker_2D()                  = delete;
    BoxPacker_2D(BoxPacker_2D const &src) = default;
    BoxPacker_2D(BoxPacker_2D &&src)      = default;
    ~BoxPacker_2D()                 = default;

    BoxPacker_2D &
    operator=(BoxPacker_2D const &) = default;
    BoxPacker_2D &
    operator=(BoxPacker_2D &&) = default;

    BoxPacker_2D(size_t const area_ySize, size_t const area_xSize, std::vector<std::vector<Shape_t>> const &shps_alterns,
           std::vector<size_t> const &shps_counts, size_t const firstTile_yPos = 0uz, size_t const firstTile_xPos = 0uz,
           pastResMap_t const &pastReslts = {})
        : m_useableCount_perShape(shps_counts),
          m_area(std::vector(area_ySize + 2, std::vector<char>(area_xSize + 2, 0))),
          m_firstTilePos(Pos{.y = static_cast<long long>(firstTile_yPos), .x = static_cast<long long>(firstTile_xPos)}),
          m_shapes_alterns(shps_alterns), m_pastComputed(pastReslts) {

        std::ranges::fill(m_area.front(), 1);
        for (auto &line : std::views::take(m_area, m_area.size() - 1) | std::views::drop(1)) {
            line.front() = 1;
            line.back()  = 1;
        }
        std::ranges::fill(m_area.back(), 1);

        m_useableCount_perShape.resize(m_shapes_alterns.size(), 0uz);
        auto ratiosHlprView = std::views::transform(
            m_useableCount_perShape,
            [sum = static_cast<double>(
                 std::ranges::fold_left_first(m_useableCount_perShape, std::plus{}).value_or(nextafter(0.0, 1.0)))](
                size_t oneCount) { return oneCount / sum; });

        m_shapesRatios_orig = decltype(m_shapesRatios_orig)(ratiosHlprView.begin(), ratiosHlprView.end());

        auto const ftPos     = Pos{.y = static_cast<long long>(std::min(firstTile_yPos, area_ySize - SQSZ)),
                                   .x = static_cast<long long>(std::min(firstTile_xPos, area_xSize - SQSZ))};
        auto       firstTile = get_windowAtPos(ftPos).value();

        m_frontierTiles.push_back(std::tuple_cat(std::make_tuple(ftPos), getOrCompute_possibsFor(firstTile)));
    }

    BoxPacker_2D(size_t const &area_ySize, size_t const &area_xSize,
           std::vector<std::array<std::array<bool, SQSZ - 2>, SQSZ - 2>> const &shps,
           std::vector<size_t> const &shps_counts, size_t const firstTile_yPos = 0uz, size_t const firstTile_xPos = 0uz,
           pastResMap_t const &pastReslts = {})
        : BoxPacker_2D(area_ySize, area_xSize,
                 std::views::transform(
                     shps, [](auto const &smallerShp) { return Shape_t(smallerShp).compute_alternsRotFlip(); }) |
                     std::ranges::to<std::vector>(),
                 shps_counts, firstTile_yPos, firstTile_xPos, pastReslts) {}

    // 'Primes' random number generator used by the solver instance with a seed based on hash of the solver state
    // Returns the the seed it used
    size_t
    prime_fprng() {
        size_t const res = hash_stateOfSelf();
        set_pseudoRandomSeed(res);
        return res;
    }

    size_t
    get_useableShapeCountRemaining() {
        return std::ranges::fold_left_first(m_useableCount_perShape, std::plus()).value_or(0uz);
    }
    size_t
    get_pastResSize() {
        return m_pastComputed.size();
    }

    size_t
    get_pseudoRandom_0_to(size_t maxInclusive) {
        return m_fprng.pseudoRandom_0_to(maxInclusive);
    }

    // Returns true if this shape is actually new, False if it is the same as some other existing shape
    bool
    add_shape(Shape_t const &toAdd) {
        return true;
    }

    BoxPacker_2D
    clone_keepShapeData(std::vector<size_t> const &shps_counts) {
        return BoxPacker_2D(m_area.size(), m_area.size() > 0 ? m_area.front().size() : 0uz, m_shapes_alterns, shps_counts,
                      m_firstTilePos.y, m_firstTilePos.x, m_pastComputed);
    }
    BoxPacker_2D
    clone_keepShapeData(size_t const area_ySize, size_t const area_xSize, std::vector<size_t> const &shps_counts) {
        return BoxPacker_2D(area_ySize, area_xSize, m_shapes_alterns, shps_counts, m_firstTilePos.y, m_firstTilePos.x, {});
    }

    void
    reset_allButNotPastComputed(std::vector<size_t> const &shps_counts) {
        reset_area();
        reset_frontier();
        reset_useableShapeCounts(shps_counts);
    }

    void
    reset_allButNotPastComputed(size_t area_ySize, size_t area_xSize, std::vector<size_t> const &shps_counts) {
        reset_area(area_ySize, area_xSize);
        reset_frontier();
        reset_useableShapeCounts(shps_counts);
    }

    void
    reset_allButNotPastComputed(size_t area_ySize, size_t area_xSize, std::vector<size_t> const &shps_counts,
                                Pos const &p) {
        reset_area(area_ySize, area_xSize);
        reset_frontier(p);
        reset_useableShapeCounts(shps_counts);
    }

    void
    reset_area() {
        std::ranges::fill(m_area.front(), 1);
        for (auto &line : std::views::take(m_area, m_area.size() - 1) | std::views::drop(1)) {
            std::ranges::fill(line, 0);
            line.front() = 1;
            line.back()  = 1;
        }
        std::ranges::fill(m_area.back(), 1);
    }

    void
    reset_area(size_t area_ySize, size_t area_xSize) {
        m_area.resize(area_ySize);
        for (auto &areaLine : m_area) { areaLine.resize(area_xSize); }
        reset_area();
    }

    void
    reset_frontier() {
        auto firstTile = get_windowAtPos(m_firstTilePos).value();
        m_frontierTiles.clear();
        m_frontierTiles.push_back(std::tuple_cat(std::make_tuple(m_firstTilePos), getOrCompute_possibsFor(firstTile)));
    }

    void
    reset_frontier(Pos const &p) {
        auto const ftPos =
            Pos{.y = static_cast<long long>(std::min(p.y, m_area.size() - SQSZ)),
                .x = static_cast<long long>(std::min(p.y, (m_area.size() > 0 ? m_area.front().size() : 0) - SQSZ))};
        m_firstTilePos = ftPos;
        reset_frontier();
    }

    void
    reset_useableShapeCounts(std::vector<size_t> const &shps_counts) {
        m_useableCount_perShape = shps_counts;
        m_useableCount_perShape.resize(m_shapes_alterns.size(), 0);
    }

    void
    reset_pastComputed() {
        m_pastComputed.clear();
    }


    std::tuple<Shape_t &, std::vector<PastRes_t> &>
    getOrCompute_possibsFor(Shape_t const &tile) {
        if (auto comp = get_possibsFor(tile); comp.has_value()) { return comp.value(); }
        return compute_possibsFor(tile);
    }

    std::string
    get_areaState() {
        std::string toPrint{};
        for (auto const &line : m_area) {
            constexpr std::array<char, 2> map{46, 35};

            auto r = std::views::transform(line, [&](char oneCh) -> char { return map[oneCh]; });
            toPrint.append(std::format("{:s}\n", r));
        }
        return toPrint;
    }


    std::optional<std::tuple<Pos, PastRes_t>>
    solve_oneStep() {
        std::vector<std::array<size_t, 4>> toConsider;

        std::vector<std::vector<std::array<size_t, 4>>> toConsider2(m_shapes_alterns.size());
        std::vector<double> lastSORs(m_shapes_alterns.size(), std::numeric_limits<double>::max());

        // This 'wierd' adjustment is to make sure the solver selects shapes more evenly
        auto perShape_adjustments = compute_perShapeScoringAdjustments();

        for (size_t ft_i = 0uz; auto const &oneFT : m_frontierTiles) {

            std::vector<char> tracker(m_shapes_alterns.size(), 0);
            for (size_t alt_i = 0uz; alt_i < std::get<2>(oneFT).get().size(); ++alt_i) {
                auto const &[alternsPos, overlay] = std::get<2>(oneFT).get().at(alt_i);

                if (std::ranges::fold_left_first(tracker, std::bit_and{}).value_or(0) == 1) { break; }
                else if (m_useableCount_perShape.at(alternsPos.y) == 0 ||
                         overlay.surfaceOpened_relative * perShape_adjustments.at(alternsPos.y) >
                             lastSORs.at(alternsPos.y)) {
                    continue;
                }
                else if (overlay.surfaceOpened_relative * perShape_adjustments.at(alternsPos.y) <
                         lastSORs.at(alternsPos.y)) {
                    lastSORs.at(alternsPos.y) = overlay.surfaceOpened_relative * perShape_adjustments.at(alternsPos.y);
                    toConsider2.at(alternsPos.y).clear();
                }
                tracker.at(alternsPos.y) = 1;
                toConsider2.at(alternsPos.y)
                    .push_back({ft_i, alt_i, static_cast<size_t>(alternsPos.y), static_cast<size_t>(alternsPos.x)});
            }
            ft_i++;
        }
        //  There are none viable overlays ... can't solve any more
        if (std::ranges::all_of(toConsider2, [](auto const &toConsLine) { return toConsLine.empty(); })) {
            return std::nullopt;
        }

        size_t const selEleToConsider  = std::ranges::min_element(lastSORs, std::less()) - lastSORs.begin();
        size_t const selEleAnternative = m_fprng.pseudoRandom_0_to(toConsider2.at(selEleToConsider).size() - 1);

        std::tuple<Pos, PastRes_t> res{
            std::get<0>(m_frontierTiles.at(toConsider2.at(selEleToConsider).at(selEleAnternative).at(0))),
            std::get<2>(m_frontierTiles.at(toConsider2.at(selEleToConsider).at(selEleAnternative).at(0)))
                .get()
                .at(toConsider2.at(selEleToConsider).at(selEleAnternative).at(1))};


        auto const surrPoss = get_surrOverlappingPoss(std::get<0>(res));
        erase_fromFrontier(surrPoss);
        set_windowAtPos(std::get<0>(res), std::get<1>(res));
        add_toFrontier(surrPoss);

        // We used one
        m_useableCount_perShape[std::get<1>(res).first.y]--;
        return res;
    }
    std::vector<std::tuple<Pos, PastRes_t>>
    solve_XSteps(size_t numOfSteps = std::numeric_limits<size_t>::max()) {
        std::vector<std::tuple<Pos, PastRes_t>> res;
        while (numOfSteps-- > 0) {
            if (auto oneStepRes = solve_oneStep()) { res.push_back(std::move(oneStepRes.value())); }
            else { break; }
        }
        return res;
    }
};
} // namespace packing

} // namespace incom::standard::solvers