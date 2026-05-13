#pragma once

#include <cassert>
#include <cmath>
#include <deque>
#include <format>
#include <limits>
#include <mdspan>
#include <ranges>


#include <ankerl/unordered_dense.h>

#include <incstd/core/explorers.hpp>
#include <incstd/core/hashing.hpp>
#include <incstd/core/matrix.hpp>
#include <incstd/core/random.hpp>


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
    // inline static constexpr size_t insideBlockRings = SQSZ-2

    struct Pos {
        long long y = 0;
        long long x = 0;
    };

    struct AlternID {
        size_t shpID;
        size_t alternID;
    };

    struct Shape {
        struct OverlayRes {
            Shape res_shp;

            size_t pointsAdded;
            size_t pointsOverlaid;
            size_t bordersTouching;
            size_t bordersNotTouching;

            size_t pointsTouching;
            size_t pointsNotTouching;

            // The best overlays is where the 'empty pixels' form a continuous area, Gaps count measures how many such
            // areas there (ideal cases == 1)
            size_t gapsCount;

            // It is conveivable that overlay may produce a shape where there isn't just one contiguous 'filled pixels'
            // area
            // This variable measure how many such areas there are (ideal case == 1)
            size_t shapesCount;

            double surfacePointsCovered_relative = 0.0;
            double surfacePointsOpened_relative  = std::numeric_limits<double>::max();

            double surfaceCovered_relative = 0.0;
            double surfaceOpened_relative  = std::numeric_limits<double>::max();
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

        int
        is_emptyOrFilled() {
            size_t const count = count_filled();
            if (count == 0) { return -1; }
            else if (count == (SQSZ * SQSZ)) { return 1; }
            return 0;
        }

        size_t
        count_filled() {
            size_t count = 0uz;
            for (int r = 0; r < SQSZ; ++r) {
                for (int c = 0; c < SQSZ; ++c) { count += m_matrix[r][c]; }
            }
            return count;
        }

        size_t
        count_filledBorderLess() {
            size_t count = 0uz;
            for (int r = 1; r < (SQSZ - 1); ++r) {
                for (int c = 1; c < (SQSZ - 1); ++c) { count += m_matrix[r][c]; }
            }
            return count;
        }

        OverlayRes
        compute_overlayWith(Shape const &other) const {
            OverlayRes res{};
            for (size_t r = 0; r < SQSZ; ++r) {
                for (size_t c = 0; c < SQSZ; ++c) {
                    res.pointsOverlaid         += (m_matrix[r][c] and other.m_matrix[r][c]);
                    res.pointsAdded            += (not m_matrix[r][c]) and other.m_matrix[r][c];
                    res.res_shp.m_matrix[r][c]  = m_matrix[r][c] | other.m_matrix[r][c];
                }
            }

            Shape Touch{};
            Shape NotTouch{};

            for (size_t r = 1; r < SQSZ - 1; ++r) {
                for (size_t c = 1; c < SQSZ - 1; ++c) {
                    if (other.m_matrix[r][c]) {
                        res.bordersTouching += m_matrix[r - 1][c] & (not other.m_matrix[r - 1][c]);
                        res.bordersTouching += m_matrix[r][c - 1] & (not other.m_matrix[r][c - 1]);
                        res.bordersTouching += m_matrix[r][c + 1] & (not other.m_matrix[r][c + 1]);
                        res.bordersTouching += m_matrix[r + 1][c] & (not other.m_matrix[r + 1][c]);

                        Touch.m_matrix[r - 1][c] |= m_matrix[r - 1][c] & (not other.m_matrix[r - 1][c]);
                        Touch.m_matrix[r][c - 1] |= m_matrix[r][c - 1] & (not other.m_matrix[r][c - 1]);
                        Touch.m_matrix[r][c + 1] |= m_matrix[r][c + 1] & (not other.m_matrix[r][c + 1]);
                        Touch.m_matrix[r + 1][c] |= m_matrix[r + 1][c] & (not other.m_matrix[r + 1][c]);

                        res.bordersNotTouching += (not m_matrix[r - 1][c]) & (not other.m_matrix[r - 1][c]);
                        res.bordersNotTouching += (not m_matrix[r][c - 1]) & (not other.m_matrix[r][c - 1]);
                        res.bordersNotTouching += (not m_matrix[r][c + 1]) & (not other.m_matrix[r][c + 1]);
                        res.bordersNotTouching += (not m_matrix[r + 1][c]) & (not other.m_matrix[r + 1][c]);

                        NotTouch.m_matrix[r - 1][c] |= not m_matrix[r - 1][c] & (not other.m_matrix[r - 1][c]);
                        NotTouch.m_matrix[r][c - 1] |= not m_matrix[r][c - 1] & (not other.m_matrix[r][c - 1]);
                        NotTouch.m_matrix[r][c + 1] |= not m_matrix[r][c + 1] & (not other.m_matrix[r][c + 1]);
                        NotTouch.m_matrix[r + 1][c] |= not m_matrix[r + 1][c] & (not other.m_matrix[r + 1][c]);
                    }
                }
            }

            Shape gapPastMemo;
            Shape filledPastMemo;
            Shape curMemo;

            Pos curPos{.y = 0ll, .x = 0ll};

            auto gapsRecLambda = [&](this auto const &self) -> bool {
                if (res.res_shp.m_matrix[curPos.y][curPos.x] == true) { return true; }
                if (curMemo.m_matrix[curPos.y][curPos.x] == true) { return true; }
                curMemo.m_matrix[curPos.y][curPos.x] = true;

                if (gapPastMemo.m_matrix[curPos.y][curPos.x] == true) { return false; } // We were there already
                gapPastMemo.m_matrix[curPos.y][curPos.x] = true;

                for (long long row : {-1ll, 1ll}) {
                    if (curPos.y + row < 0 || curPos.y + row >= SQSZ) { continue; }
                    curPos.y += row;
                    if (not self()) { return false; }
                    curPos.y -= row;
                }
                for (long long col : {-1ll, 1ll}) {
                    if (curPos.x + col < 0 || curPos.x + col >= SQSZ) { continue; }
                    curPos.x += col;
                    if (not self()) { return false; };
                    curPos.x -= col;
                }
                return true;
            };
            auto filledRecLambda = [&](this auto const &self) -> bool {
                if (res.res_shp.m_matrix[curPos.y][curPos.x] == false) { return true; }
                if (curMemo.m_matrix[curPos.y][curPos.x] == true) { return true; }
                curMemo.m_matrix[curPos.y][curPos.x] = true;

                if (filledPastMemo.m_matrix[curPos.y][curPos.x] == true) { return false; } // We were there already
                filledPastMemo.m_matrix[curPos.y][curPos.x] = true;

                for (long long row : {-1ll, 1ll}) {
                    if (curPos.y + row < 0 || curPos.y + row >= SQSZ) { continue; }
                    curPos.y += row;
                    if (not self()) { return false; }
                    curPos.y -= row;
                }
                for (long long col : {-1ll, 1ll}) {
                    if (curPos.x + col < 0 || curPos.x + col >= SQSZ) { continue; }
                    curPos.x += col;
                    if (not self()) { return false; };
                    curPos.x -= col;
                }
                return true;
            };

            for (size_t r = 0; r < SQSZ; ++r) {
                for (size_t c = 0; c < SQSZ; ++c) {
                    if (res.res_shp.m_matrix[r][c] == false && gapPastMemo.m_matrix[r][c] == false) {
                        curPos.y       = r;
                        curPos.x       = c;
                        curMemo        = Shape{};
                        res.gapsCount += gapsRecLambda();
                    }
                    if (res.res_shp.m_matrix[r][c] == true && filledPastMemo.m_matrix[r][c] == false) {
                        curPos.y         = r;
                        curPos.x         = c;
                        curMemo          = Shape{};
                        res.shapesCount += filledRecLambda();
                    }
                }
            }


            res.pointsTouching    = Touch.count_filled();
            res.pointsNotTouching = NotTouch.count_filled();
            double const denomP   = std::max(static_cast<double>(res.pointsTouching + res.pointsNotTouching), 1.0);
            double const denomB   = std::max(static_cast<double>(res.bordersTouching + res.bordersNotTouching), 1.0);

            res.surfacePointsCovered_relative = res.pointsTouching / denomP;
            res.surfacePointsOpened_relative  = res.pointsNotTouching / denomP;

            res.surfaceCovered_relative = res.bordersTouching / denomB;
            res.surfaceOpened_relative  = res.bordersNotTouching / denomB;

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

    struct PastRes {
        struct AlternID {
            size_t shpID;
            size_t alternID;
        };
        size_t            uncoveredBySurr = 0uz;
        AlternID          ol_shpID;
        Shape::OverlayRes ol_res;
    };

    struct ConsideredShapeOption {
        enum class Type : uint8_t {
            Gapless = 1,
            Dividing,
            Gapcreating
        };

        Pos     p;
        PastRes pr_option = PastRes{.ol_shpID = {}, .ol_res = {}};

        Type type = Type::Gapcreating;
    };

    using possibilitiesByShape_t     = std::vector<std::vector<PastRes>>;
    using frontierTilePossibs_t      = std::optional<std::reference_wrapper<possibilitiesByShape_t>>;
    using consideredOptionsByShape_t = std::vector<std::vector<ConsideredShapeOption>>;
    using pastResMap_t =
        ankerl::unordered_dense::segmented_map<Shape, possibilitiesByShape_t, incom::standard::hashing::XXH3Hasher>;

    struct SolverPolicy {
        struct SelectionState {
            double lowestSOR = std::numeric_limits<double>::max();

            [[nodiscard]] bool
            shouldStopOn(double const curAdjSOR) const {
                return lowestSOR < curAdjSOR;
            }

            [[nodiscard]] bool
            hasNewBest(double const curAdjSOR) const {
                return lowestSOR > curAdjSOR;
            }

            void
            reset(consideredOptionsByShape_t &toConsider, double const curAdjSOR) {
                lowestSOR = curAdjSOR;
                for (auto &toConsLine : toConsider) { toConsLine.clear(); }
            }
        };

        [[nodiscard]] static bool
        allows(PastRes const &toCheck) {
            return toCheck.ol_res.pointsOverlaid == 0uz;
        }

        [[nodiscard]] static bool
        prefer_precomputed(PastRes const &l, PastRes const &r) {
            double const soDif = r.ol_res.surfaceOpened_relative - l.ol_res.surfaceOpened_relative;
            if (soDif == 0.0) { return l.ol_res.pointsAdded > r.ol_res.pointsAdded; }
            else { return std::abs(soDif) + soDif; }
        }
    };

    std::optional<std::tuple<Pos, PastRes>>
    solve_oneStep() {
        auto selOpt = findNextStep_covering()
                          .or_else([this]() { return findNextStep_regular(); })
                          .or_else([this]() { return findNextStep_withGap(); })
                          .and_then([this](auto const &VofV_csos) { return select_oneCSO(VofV_csos); });

        if (not selOpt.has_value()) { return std::nullopt; }

        ConsideredShapeOption const   &selCSO = selOpt.value();
        std::tuple<Pos, PastRes> const res{selCSO.p, selCSO.pr_option};

        auto const surrPoss = get_surrOverlappingPoss_forWindowsAt<shapeOLCount_border>(std::get<0>(res));
        erase_fromFrontier(surrPoss);
        set_windowAtPos(selCSO.p, selCSO.pr_option);
        add_toFrontier(surrPoss);

        // We used one
        m_useableCount_perShape[std::get<1>(res).ol_shpID.shpID]--;

        // Figure out which are uncoverable and input them
        for (Pos const &uncov : verify_uncoverable(selCSO)) {
            if (std::ranges::find_if(m_uncoverableFrontierPoss, [&](auto const &item) {
                    return (item.y == uncov.y && item.x == uncov.x);
                }) == m_uncoverableFrontierPoss.end()) {
                m_uncoverableFrontierPoss.push_back(uncov);
            }
        }

        return res;
    }

    std::vector<std::tuple<Pos, PastRes>>
    solve_XSteps(size_t numOfSteps = std::numeric_limits<size_t>::max()) {
        std::vector<std::tuple<Pos, PastRes>> res;
        while (numOfSteps-- > 0) {
            if (auto oneStepRes = solve_oneStep()) { res.push_back(std::move(oneStepRes.value())); }
            else { break; }
        }
        return res;
    }

    // #####################################################################
    // ### CONSTRUCTION ###
    // #####################################################################
public:
    // MAIN Constructor, SQSZ is deduced automatically using a deduction guide
    BoxPacker_2D(size_t area_ySize, size_t area_xSize,
                 std::vector<std::array<std::array<bool, SQSZ - 2>, SQSZ - 2>> const &shps,
                 std::vector<size_t> const &shps_counts, size_t const firstTile_yPos = 0uz,
                 size_t const firstTile_xPos = 0uz, pastResMap_t const &pastReslts = {})
        : BoxPacker_2D(area_ySize, area_xSize,
                       std::views::transform(
                           shps, [](auto const &smallerShp) { return Shape(smallerShp).compute_alternsRotFlip(); }) |
                           std::ranges::to<std::vector>(),
                       shps_counts, firstTile_yPos, firstTile_xPos, pastReslts) {}

    //    1. NOT default constructible (makes no sense to do that)
    //    2. NOT copiable (or copy assignable) because some member vars point to other member vars
    BoxPacker_2D()                        = delete;
    BoxPacker_2D(BoxPacker_2D const &src) = delete;
    BoxPacker_2D(BoxPacker_2D &&src)      = default;
    ~BoxPacker_2D()                       = default;

    BoxPacker_2D &
    operator=(BoxPacker_2D const &) = delete;
    BoxPacker_2D &
    operator=(BoxPacker_2D &&) = default;

private:
    BoxPacker_2D(size_t const area_ySize, size_t const area_xSize, std::vector<std::vector<Shape>> const &shps_alterns,
                 std::vector<size_t> const &shps_counts, size_t const firstTile_yPos = 0uz,
                 size_t const firstTile_xPos = 0uz, pastResMap_t const &pastReslts = {})
        : m_useableCount_perShape(shps_counts),
          m_area(std::vector(area_ySize + 2, std::vector<char>(area_xSize + 2, 0))),
          m_frontierTiles(
              std::vector(area_ySize + 3 - SQSZ, std::vector<frontierTilePossibs_t>(area_xSize + 3 - SQSZ))),
          m_firstTilePos(Pos{.y = static_cast<long long>(firstTile_yPos), .x = static_cast<long long>(firstTile_xPos)}),
          m_shapes_alterns(shps_alterns),
          m_shapesMaxEmpty(((SQSZ - 2) * (SQSZ - 2)) -
                           std::ranges::fold_left_first(
                               std::views::transform(m_shapes_alterns,
                                                     [](auto &vecOfAlterns) {
                                                         return vecOfAlterns.empty()
                                                                    ? 0
                                                                    : vecOfAlterns.front().count_filledBorderLess();
                                                     }),
                               [](size_t a, size_t b) { return std::min(a, b); })
                               .value_or(0uz)),
          m_pastComputed(pastReslts) {

        std::ranges::fill(m_area.front(), 1);
        for (auto &line : std::views::take(m_area, m_area.size() - 1) | std::views::drop(1)) {
            line.front() = 1;
            line.back()  = 1;
        }
        std::ranges::fill(m_area.back(), 1);

        // Make sure we are only using the shapes we actually have (match on size)
        m_useableCount_perShape.resize(m_shapes_alterns.size(), 0uz);

        auto ratiosHlprView = std::views::transform(
            m_useableCount_perShape,
            [sum = static_cast<double>(
                 std::ranges::fold_left_first(m_useableCount_perShape, std::plus{}).value_or(nextafter(0.0, 1.0)))](
                size_t oneCount) { return oneCount / sum; });

        m_shapesRatios_orig = decltype(m_shapesRatios_orig)(ratiosHlprView.begin(), ratiosHlprView.end());

        auto const ftPos = Pos{.y = static_cast<long long>(std::min(firstTile_yPos, area_ySize - SQSZ)),
                               .x = static_cast<long long>(std::min(firstTile_xPos, area_xSize - SQSZ))};


        auto &ft_possibs = getOrCompute_possibsFor(get_windowAtPos(ftPos).value());

        m_frontierTiles.at(ftPos.y).at(ftPos.x) = std::ref(ft_possibs);
        prime_fprng();
    }

    // #####################################################################
    // ### MEMBER VARS ###
    // #####################################################################
private:
    inline static constexpr size_t shapeOLCount_full   = (2 * SQSZ) - 1;
    inline static constexpr size_t shapeOLCount_border = (2 * SQSZ) - 3;
    inline static constexpr size_t shapeOLCount_inside = (2 * SQSZ) - 5;

    std::vector<std::vector<char>>  m_area;
    Pos                             m_firstTilePos;
    std::vector<std::vector<Shape>> m_shapes_alterns;
    size_t                          m_shapesMaxEmpty;

    std::vector<size_t>                       m_useableCount_perShape;
    std::vector<double>                       m_shapesRatios_orig;
    incom::standard::random::FastPseudoRandom m_fprng;

    // Memoization of what 'OverlayRes' we can use on a particular 'Shape'
    pastResMap_t                                    m_pastComputed;
    std::deque<Pos>                                 m_uncoverableFrontierPoss;
    std::vector<std::vector<frontierTilePossibs_t>> m_frontierTiles;


    // #####################################################################
    // ### Getting info on current state of BoxPacker ###
    // #####################################################################
public:
    std::string
    get_areaState() const {
        std::string                   toPrint{};
        constexpr std::array<char, 3> map{46, 35, 118};
        for (auto const &line : m_area) {
            toPrint.append(
                std::format("{:s}\n", std::views::transform(line, [&](char oneCh) -> char { return map[oneCh]; })));
        }
        return toPrint;
    }
    std::pair<size_t, size_t>
    get_areaSize() const {
        return {m_area.size(), m_area.size() > 0uz ? m_area.front().size() : 0uz};
    }
    std::pair<size_t, size_t>
    get_areaSize_borderless() const {
        return {m_area.size() > 0uz ? m_area.size() - 1uz : 0uz,
                m_area.size() > 0uz ? (m_area.front().size() > 0 ? m_area.front().size() - 1uz : 0uz) : 0uz};
    }


    std::pair<size_t, size_t>
    get_emptyFilled() const noexcept {
        std::pair<size_t, size_t> res{};

        for (size_t r = 1; r < m_area.size() - 1; ++r) {
            for (size_t c = 1; c < m_area.at(r).size() - 1; ++c) { m_area[r][c] == 0 ? res.first++ : res.second++; }
        }
        return res;
    }

    size_t
    get_useableShapeCountRemaining() const noexcept {
        return std::ranges::fold_left_first(m_useableCount_perShape, std::plus()).value_or(0uz);
    }
    size_t
    get_pastResSize() const noexcept {
        return m_pastComputed.size();
    }


    // #####################################################################
    // ### Cloning and reseting ###
    // #####################################################################
public:
    BoxPacker_2D
    clone_keepShapeData(std::vector<size_t> const &shps_counts) const {
        auto const [rDim, cDim] = get_areaSize_borderless();
        return BoxPacker_2D(rDim, cDim, m_shapes_alterns, shps_counts, m_firstTilePos.y, m_firstTilePos.x,
                            m_pastComputed);
    }
    BoxPacker_2D
    clone_keepShapeData(size_t const area_ySize, size_t const area_xSize,
                        std::vector<size_t> const &shps_counts) const {
        return BoxPacker_2D(area_ySize, area_xSize, m_shapes_alterns, shps_counts, m_firstTilePos.y, m_firstTilePos.x,
                            {});
    }

    void
    reset_allButNotPastComputed(std::vector<size_t> const &shps_counts) {
        reset_area();
        reset_frontier();
        reset_useableShapeCounts(shps_counts);
        prime_fprng();
    }

    void
    reset_allButNotPastComputed(size_t area_ySize, size_t area_xSize, std::vector<size_t> const &shps_counts) {
        reset_area(area_ySize, area_xSize);
        reset_frontier();
        reset_useableShapeCounts(shps_counts);
        prime_fprng();
    }

    void
    reset_allButNotPastComputed(size_t area_ySize, size_t area_xSize, std::vector<size_t> const &shps_counts,
                                Pos const &p) {
        reset_area(area_ySize, area_xSize);
        reset_frontier(p);
        reset_useableShapeCounts(shps_counts);
        prime_fprng();
    }

    void
    reset_area() noexcept {
        std::ranges::fill(m_area.front(), 1);
        for (auto &line : std::views::take(m_area, m_area.size() - 1) | std::views::drop(1)) {
            std::ranges::fill(line, 0);
            line.front() = 1;
            line.back()  = 1;
        }
        std::ranges::fill(m_area.back(), 1);
    }

    void
    reset_area(size_t const area_ySize, size_t const area_xSize) {
        m_area.resize(area_ySize + 2);
        for (auto &areaLine : m_area) { areaLine.resize(area_xSize + 2); }
        reset_area();
    }

    void
    reset_frontier() {
        auto firstTile = get_windowAtPos(m_firstTilePos).value();
        m_frontierTiles.resize(m_area.size() + 1 - SQSZ);

        size_t const newRowSz = m_area.empty() ? 0 : m_area.front().size() + 1 - SQSZ;
        for (auto &frontierLine : m_frontierTiles) {
            frontierLine.resize(newRowSz);
            for (auto &frontierPos : frontierLine) { frontierPos = std::nullopt; }
        }
        auto &ft_possibs                                          = getOrCompute_possibsFor(firstTile);
        m_frontierTiles.at(m_firstTilePos.y).at(m_firstTilePos.x) = std::ref(ft_possibs);
    }
    void
    reset_frontier(Pos const &firstTilePos) {
        auto const ftPos = Pos{.y = static_cast<long long>(std::min(firstTilePos.y, m_area.size() - SQSZ)),
                               .x = static_cast<long long>(
                                   std::min(firstTilePos.x, (m_area.size() > 0 ? m_area.front().size() : 0) - SQSZ))};
        m_firstTilePos   = ftPos;
        reset_frontier();
    }

    void
    reset_frontier(std::vector<Pos> const &firstTiles) noexcept {}

    void
    reset_useableShapeCounts(std::vector<size_t> const &shps_counts) {
        m_useableCount_perShape = shps_counts;
        m_useableCount_perShape.resize(m_shapes_alterns.size(), 0);
    }

    void
    reset_pastComputed() noexcept {
        m_pastComputed.clear();
    }

    // #####################################################################
    // ### Frontier manipulation ###
    // #####################################################################
public:
    size_t
    erase_fromFrontier(std::vector<Pos> const &shapePoss) {
        size_t res_removed = 0uz;
        for (Pos const &onePos : shapePoss) {
            if (m_frontierTiles.at(onePos.y).at(onePos.x) != std::nullopt) { res_removed++; }
            m_frontierTiles.at(onePos.y).at(onePos.x) = std::nullopt;
        }
        return res_removed;
    }

    size_t
    add_toFrontier(std::vector<Pos> const &shapePoss) {
        size_t resCount = 0uz;
        for (auto const &onePos : shapePoss) {
            auto window = get_windowAtPos(onePos);
            if (not window.has_value() || window.value().count_filledBorderLess() > m_shapesMaxEmpty) { continue; }

            auto &possibsForWindow = getOrCompute_possibsFor(window.value());
            if (possibsForWindow.size() > 0) { m_frontierTiles.at(onePos.y).at(onePos.x) = std::ref(possibsForWindow); }
            resCount++;
        }
        return resCount;
    }

    size_t
    add_toFrontier_allCorners() {
        size_t resCount = 0uz;
        if (m_area.size() - SQSZ < 0 || m_area.front().size() - SQSZ < 0) {}
        else {
            for (auto const [r, c] :
                 std::array<std::array<size_t, 2>, 4>{{{0, 0},
                                                       {0, m_area.front().size() - SQSZ},
                                                       {m_area.size() - SQSZ, 0},
                                                       {m_area.size() - SQSZ, m_area.front().size() - SQSZ}}}) {


                auto window = get_windowAtPos(Pos{static_cast<long long>(r), static_cast<long long>(c)});
                if (not window.has_value() || window.value().count_filledBorderLess() > m_shapesMaxEmpty) { continue; }

                auto &possibsForWindow = getOrCompute_possibsFor(window.value());
                if (possibsForWindow.size() > 0) { m_frontierTiles.at(r).at(c) = std::ref(possibsForWindow); }
                resCount++;
            }
        }
        return resCount;
    }


    // #####################################################################
    // ### Computations performed on solving ###
    // #####################################################################
private:
    [[nodiscard]] static ConsideredShapeOption
    make_consideredShapeOption(Pos const &p, PastRes const &pr, ConsideredShapeOption::Type const type) {
        return ConsideredShapeOption{.p = p, .pr_option = pr, .type = type};
    }

    [[nodiscard]] bool
    has_useableAlternatives(std::vector<PastRes> const &oneShpAltsVec) const {
        if (oneShpAltsVec.empty()) { return false; }
        return m_useableCount_perShape.at(oneShpAltsVec.front().ol_shpID.shpID) > 0uz;
    }

    template <typename Predicate>
    void
    collect_consideredOptionsAt(consideredOptionsByShape_t &toConsider, bool &anyFilled,
                                typename SolverPolicy::SelectionState &selectionState, Pos const &candidatePos,
                                possibilitiesByShape_t const &possibilitiesByShape,
                                std::vector<double> const &perShpScoringAdj, ConsideredShapeOption::Type const type,
                                Predicate const &predicate) const {
        for (auto const &v_pr2 : std::views::filter(possibilitiesByShape, [this](auto const &oneShpAltsVec) {
                 return has_useableAlternatives(oneShpAltsVec);
             })) {
            for (PastRes const &pr : std::views::filter(v_pr2, predicate)) {
                double const curAdjSOR = pr.ol_res.surfaceOpened_relative * perShpScoringAdj.at(pr.ol_shpID.shpID);

                if (selectionState.shouldStopOn(curAdjSOR)) { break; }
                if (selectionState.hasNewBest(curAdjSOR)) { selectionState.reset(toConsider, curAdjSOR); }

                toConsider.at(pr.ol_shpID.shpID).push_back(make_consideredShapeOption(candidatePos, pr, type));
                anyFilled = true;
            }
        }
    }

    std::vector<double>
    compute_perShapeScoringAdjustments() const {
        double const sum =
            static_cast<double>(std::ranges::fold_left_first(m_useableCount_perShape, std::plus{}).value_or(0));

        auto ratiosHlprView = std::views::zip(m_useableCount_perShape, m_shapesRatios_orig) |
                              std::views::transform([&](auto const &oneCount) {
                                  return (std::get<0>(oneCount) == 0uz ? std::numeric_limits<double>::max()
                                                                       : (sum / std::get<0>(oneCount))) *
                                         std::get<1>(oneCount);
                              });

        return std::vector<double>(ratiosHlprView.begin(), ratiosHlprView.end());
    }

    possibilitiesByShape_t &
    getOrCompute_possibsFor(Shape const &tile) {
        auto insRes = m_pastComputed.insert({tile, possibilitiesByShape_t(m_shapes_alterns.size())});
        if (insRes.second) {
            possibilitiesByShape_t &vpr = insRes.first->second;

            for (size_t shpID = 0uz; shpID < m_shapes_alterns.size(); ++shpID) {
                for (size_t alternID = 0uz; alternID < m_shapes_alterns.at(shpID).size(); ++alternID) {
                    auto rs = PastRes{.ol_shpID{shpID, alternID},
                                      .ol_res = tile.compute_overlayWith(m_shapes_alterns.at(shpID).at(alternID))};
                    if (SolverPolicy::allows(rs)) { vpr.at(shpID).push_back(rs); }
                }
            }

            // Sort so that the 'better' options are first in each vec
            for (auto &vprLine : vpr) { std::ranges::sort(vprLine, SolverPolicy::prefer_precomputed); }
        }
        return insRes.first->second;
    }

    // When we have some uncoverable points at the frontier
    std::optional<std::vector<std::vector<ConsideredShapeOption>>>
    findNextStep_covering() {

        if (m_uncoverableFrontierPoss.empty()) { return std::nullopt; }

        std::vector<char> tracker(m_frontierTiles.size() * m_frontierTiles.front().size(), 0);
        std::mdspan       mdsp(tracker.data(),
                               std::dextents<size_t, 2uz>{m_frontierTiles.size(), m_frontierTiles.front().size()});

        auto const perShpScoringAdj = compute_perShapeScoringAdjustments();

        while (not m_uncoverableFrontierPoss.empty()) {
            if (m_area.at(m_uncoverableFrontierPoss.front().y).at(m_uncoverableFrontierPoss.front().x) != 0) {
                m_uncoverableFrontierPoss.pop_front();
                continue;
            }

            auto explr = explorers::Chebyshev(
                [&](std::array<size_t, 2> const &item) { return m_area.at(item[0]).at(item[1]) == 0; },
                std::array{static_cast<size_t>(m_uncoverableFrontierPoss.front().y),
                           static_cast<size_t>(m_uncoverableFrontierPoss.front().x)},
                std::array{m_area.size(), m_area.empty() ? 0uz : m_area.front().size()});


            auto eva = [&](std::vector<Pos> const &poss) -> std::optional<consideredOptionsByShape_t> {
                consideredOptionsByShape_t            toConsider(m_shapes_alterns.size());
                bool                                  anyFilled = false;
                typename SolverPolicy::SelectionState selectionState{};

                for (auto const &onePos : poss) {
                    for (auto const &prPos : get_surrOverlappingPoss<false>(onePos)) {
                        if (mdsp[prPos.y, prPos.x] != 0) { continue; }
                        mdsp[prPos.y, prPos.x] = 1;
                        if (not m_frontierTiles.at(prPos.y).at(prPos.x).has_value()) { continue; }

                        collect_consideredOptionsAt(toConsider, anyFilled, selectionState, prPos,
                                                    m_frontierTiles.at(prPos.y).at(prPos.x).value().get(),
                                                    perShpScoringAdj, ConsideredShapeOption::Type::Gapcreating,
                                                    [](auto const &item) { return item.ol_res.gapsCount > 1; });
                    }
                }

                if (not anyFilled) { return std::nullopt; }
                return toConsider;
            };


            size_t           level = 0uz;
            std::vector<Pos> posToEval;

            while (not explr.is_atEnd()) {
                auto locPos = explr.get_next();
                posToEval.push_back({static_cast<long long>(locPos[0]), static_cast<long long>(locPos[1])});

                // If we got to another level we evaluate the found options
                if (level < explr.m_queueIDToUseNext) {
                    if (auto potRes = eva(posToEval); potRes.has_value()) { return potRes; }
                    posToEval.clear();
                }

                level = explr.m_queueIDToUseNext;
            }
            if (auto potRes = eva(posToEval); potRes.has_value()) { return potRes; }

            m_uncoverableFrontierPoss.pop_front(); // Pop front if the above while loop didn't return
        }

        return std::nullopt;
    }
    std::optional<std::vector<std::vector<ConsideredShapeOption>>>
    findNextStep_regular() const {

        consideredOptionsByShape_t            toConsider(m_shapes_alterns.size());
        bool                                  anyFilled = false;
        typename SolverPolicy::SelectionState selectionState{};
        auto const                            perShpScoringAdj = compute_perShapeScoringAdjustments();

        Pos curPos{.y = -1ll, .x = -1ll};

        for (auto const &frontierLine : m_frontierTiles) {
            curPos.y++;
            curPos.x = -1ll;
            for (auto const &frontierPos : frontierLine) {
                curPos.x++;
                if (frontierPos != std::nullopt) {
                    collect_consideredOptionsAt(toConsider, anyFilled, selectionState, curPos,
                                                frontierPos.value().get(), perShpScoringAdj,
                                                ConsideredShapeOption::Type::Gapless,
                                                [](auto const &item) { return item.ol_res.gapsCount < 2; });
                }
            }
        }

        if (not anyFilled) { return std::nullopt; }
        return toConsider;
    }
    std::optional<std::vector<std::vector<ConsideredShapeOption>>>
    findNextStep_withGap() const {
        consideredOptionsByShape_t            toConsider(m_shapes_alterns.size());
        bool                                  anyFilled = false;
        typename SolverPolicy::SelectionState selectionState{};
        auto const                            perShpScoringAdj = compute_perShapeScoringAdjustments();

        Pos curPos{.y = -1ll, .x = -1ll};

        for (auto const &frontierLine : m_frontierTiles) {
            curPos.y++;
            curPos.x = -1ll;
            for (auto const &frontierPos : frontierLine) {
                curPos.x++;
                if (frontierPos != std::nullopt) {
                    collect_consideredOptionsAt(toConsider, anyFilled, selectionState, curPos,
                                                frontierPos.value().get(), perShpScoringAdj,
                                                ConsideredShapeOption::Type::Dividing,
                                                [](auto const &item) { return item.ol_res.gapsCount > 1; });
                }
            }
        }

        if (not anyFilled) { return std::nullopt; }
        return toConsider;
    }

    std::optional<ConsideredShapeOption>
    select_oneCSO(std::vector<std::vector<ConsideredShapeOption>> const &VofV_csos) {

        size_t const optsCount = std::ranges::fold_left(
            VofV_csos, 0uz, [](size_t init, auto const &VofCSO) { return init + VofCSO.size(); });
        if (optsCount == 0uz) { return std::nullopt; }

        // Gets the 'n-th' id to consider (random from those possible)
        size_t numToConsider = m_fprng.pseudoRandom_0_to(optsCount - 1uz) + 1uz;

        for (auto const &VofCSO : VofV_csos) {
            if (VofCSO.size() < numToConsider) { numToConsider -= VofCSO.size(); }
            else { return VofCSO.at(numToConsider - 1uz); }
        }
        assert(false);
        std::unreachable();
    }
    // Returns Shape where 'true' means uncoverable empty place
    std::vector<Pos>
    verify_uncoverable(ConsideredShapeOption const &cso) const {

        std::vector<Pos>    res{};
        constexpr long long halfCount = shapeOLCount_border / 2;

        // For all Pos of the window of the CSO
        for (long long thisShpRow = cso.p.y; thisShpRow < (cso.p.y + SQSZ); ++thisShpRow) {
            for (long long thisShpCol = cso.p.x; thisShpCol < (cso.p.x + SQSZ); ++thisShpCol) {
                if (cso.pr_option.ol_res.res_shp.m_matrix[thisShpRow - cso.p.y][thisShpCol - cso.p.x] == true) {
                    continue;
                }
                bool onePointCovered      = false;
                bool atLeastOneWithoutGap = false;

                // For all Positions that may change the the Pos selected above
                for (long long influRow = thisShpRow - (SQSZ - 2); influRow < thisShpRow; ++influRow) {
                    for (long long influCol = thisShpCol - (SQSZ - 2); influCol < thisShpCol; ++influCol) {
                        if (not is_posValid(Pos{.y = influRow, .x = influCol})) { continue; }

                        // Get the PR options from frontierTiles
                        if (not m_frontierTiles.at(influRow).at(influCol).has_value()) { continue; }
                        for (auto const &prLine : m_frontierTiles.at(influRow).at(influCol).value().get()) {
                            for (PastRes const &onePR : prLine) {

                                // Bit OR to find out
                                onePointCovered      |= onePR.ol_res.res_shp.m_matrix
                                                            .at((SQSZ - 2) - (influRow - (thisShpRow - (SQSZ - 2))))
                                                            .at((SQSZ - 2) - (influCol - (thisShpCol - (SQSZ - 2))));
                                atLeastOneWithoutGap |= (onePR.ol_res.gapsCount < 2);
                            }
                        }
                    }
                }
                if (not onePointCovered || not atLeastOneWithoutGap) {
                    res.push_back(Pos{.y = thisShpRow, .x = thisShpCol});
                }
            }
        }

        return res;
    }


    // #####################################################################
    // ### Getting and setting Windows and Positions inside m_area ###
    // #####################################################################
private:
    template <bool INCLBorder = true>
    std::vector<Pos>
    get_surrOverlappingPoss(Pos const &shp_pos) const {
        size_t const rows = m_area.size();
        size_t const cols = m_area.size() > 0 ? m_area.front().size() : 0;

        constexpr size_t adj = (SQSZ - 2 + INCLBorder);

        std::vector<Pos> res;

        // shapePos needs to be the Pos of some valid window in our area (but that will be implicit )
        for (long long row = shp_pos.y - adj; row < (shp_pos.y + INCLBorder); ++row) {
            for (long long col = shp_pos.x - adj; col < (shp_pos.x + INCLBorder); ++col) {
                if (row < 0 || row > (rows - SQSZ) || col < 0 || col > (cols - SQSZ)) {}
                else { res.push_back(Pos{.y = row, .x = col}); }
            }
        }

        return res;
    }

    template <size_t OLCount>
    requires(OLCount % 2 == 1)
    std::vector<Pos>
    get_surrOverlappingPoss_forWindowsAt(Pos const &shp_pos) const {
        size_t const rows = m_area.size();
        size_t const cols = m_area.size() > 0 ? m_area.front().size() : 0;

        constexpr long long halfCount = OLCount / 2;

        std::vector<Pos> res;

        // shapePos needs to be the Pos of some valid window in our area (but that will be implicit )
        for (long long row = shp_pos.y - halfCount; row < (shp_pos.y + halfCount + 1); ++row) {
            for (long long col = shp_pos.x - halfCount; col < (shp_pos.x + halfCount + 1); ++col) {
                if (row < 0 || row > (rows - SQSZ) || col < 0 || col > (cols - SQSZ)) {}
                else { res.push_back(Pos{.y = row, .x = col}); }
            }
        }

        return res;
    }

    template <size_t OLCount>
    requires(OLCount % 2 == 1)
    std::array<std::array<std::optional<Shape>, OLCount>, OLCount>
    get_surrOverlappingWindows(Pos const &p) const {
        constexpr long long                                            halfCount = OLCount / 2;
        std::array<std::array<std::optional<Shape>, OLCount>, OLCount> res{};

        // shapePos needs to be the Pos of some valid window in our area (but that will be implicit )
        for (long long row = p.y - halfCount; row < (p.y + halfCount + 1); ++row) {
            for (long long col = p.x - halfCount; col < (p.x + halfCount + 1); ++col) {
                if (row < 0 || row > (m_area.size() - SQSZ) || col < 0 || col > (m_area.front().size() - SQSZ)) {}
                else {
                    res.at(row - (p.y - halfCount)).at(col - (p.x - halfCount)) =
                        get_windowAtPos(Pos{.y = row, .x = col});
                }
            }
        }

        return res;
    }


    std::optional<Shape>
    get_windowAtPos(Pos const &shapePos) const {
        size_t const rows = m_area.size();
        size_t const cols = m_area.size() > 0 ? m_area.front().size() : 0;

        if (shapePos.y >= 0 && shapePos.y <= (rows - SQSZ) && shapePos.x >= 0 && shapePos.x <= (cols - SQSZ)) {
            Shape res;
            for (int row = shapePos.y; row < shapePos.y + SQSZ; ++row) {
                for (int col = shapePos.x; col < (shapePos.x + SQSZ); ++col) {
                    res.m_matrix[row - shapePos.y][col - shapePos.x] = m_area[row][col];
                }
            }
            return res;
        }
        else { return std::nullopt; }
    }

    bool
    is_posValid(Pos const &p) const noexcept {
        long long const py = p.y;
        long long const px = p.x;
        if (py < 0ll || py > (static_cast<long long>(m_area.size()) - SQSZ) || px < 0ll ||
            px > (m_area.size() == 0 ? 0 : m_area.front().size() - SQSZ)) {
            return false;
        }
        return true;
    }

    bool
    set_windowAtPos(Pos const &shapePos, PastRes const &pr) {
        if (not is_posValid(shapePos)) { return false; }
        for (long long r = shapePos.y; r < (shapePos.y + SQSZ); ++r) {
            for (long long c = shapePos.x; c < (shapePos.x + SQSZ); ++c) {
                m_area[r][c] = pr.ol_res.res_shp.m_matrix[r - shapePos.y][c - shapePos.x];
            }
        }
        return true;
    }
    bool
    set_windowAtPos(Pos const &shapePos, Shape const &newWindow) {
        if (not is_posValid(shapePos)) { return false; }
        for (long long r = shapePos.y; r < (shapePos.y + SQSZ); ++r) {
            for (long long c = shapePos.x; c < (shapePos.x + SQSZ); ++c) {
                m_area[r][c] = newWindow.m_matrix[r - shapePos.y][c - shapePos.x];
            }
        }
        return true;
    }

    // #####################################################################
    // ### Other Helpers ###
    // #####################################################################
private:
    size_t
    prime_fprng() noexcept {
        size_t const res = hash_ofSelf();
        set_pseudoRandomSeed(res);
        return res;
    }

    void
    set_pseudoRandomSeed(uint64_t seed) noexcept {
        m_fprng.setSeed(seed);
    }

    std::size_t
    hash_ofSelf() const noexcept {
        XXH3_state_t *state = XXH3_createState();
        XXH3_64bits_reset_withSeed(state, 0);

        size_t const m_area_ySz = m_area.size();
        size_t const m_area_xSz = m_area.empty() ? 0uz : m_area.front().size();
        XXH3_64bits_update(state, &m_area_ySz, sizeof(size_t));
        XXH3_64bits_update(state, &m_area_xSz, sizeof(size_t));
        XXH3_64bits_update(state, &m_firstTilePos.y, sizeof(long long));
        XXH3_64bits_update(state, &m_firstTilePos.x, sizeof(long long));

        for (auto const &alternsLine : m_shapes_alterns) {
            for (auto const &shp : alternsLine) { XXH3Hash(shp, state); }
        }
        XXH3_64bits_update(state, m_useableCount_perShape.data(),
                           sizeof(typename std::remove_cvref_t<decltype(m_useableCount_perShape)>::value_type) *
                               m_useableCount_perShape.size());

        XXH64_hash_t result = XXH3_64bits_digest(state);
        XXH3_freeState(state);
        return result;
    }
};


// Deduction guides
template <size_t N>
requires(N > 0)
BoxPacker_2D(size_t const area_ySize, size_t const area_xSize,
             std::vector<std::array<std::array<bool, N>, N>> const &shps, std::vector<size_t> const &shps_counts)
    -> BoxPacker_2D<N + 2>;

template <size_t N>
requires(N > 0)
BoxPacker_2D(size_t const area_ySize, size_t const area_xSize,
             std::vector<std::array<std::array<bool, N>, N>> const &shps, std::vector<size_t> const &shps_counts,
             size_t) -> BoxPacker_2D<N + 2>;

template <size_t N>
requires(N > 0)
BoxPacker_2D(size_t const area_ySize, size_t const area_xSize,
             std::vector<std::array<std::array<bool, N>, N>> const &shps, std::vector<size_t> const &shps_counts,
             size_t, size_t) -> BoxPacker_2D<N + 2>;

} // namespace packing


} // namespace incom::standard::solvers