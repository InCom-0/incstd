#pragma once

#include <algorithm>
#include <deque>
#include <stack>
#include <vector>

#include <incstd/polyfills/mdspan.hpp>


namespace incom::standard::explorers {
using namespace incom::standard;

namespace _detail {
template <size_t N>
struct NoOpAlwaysTrue {
    constexpr bool
    operator()(std::array<size_t, N> const &) const noexcept {
        return true;
    }
};

template <size_t N>
struct NoOpOnFill {
    constexpr void
    operator()(std::array<size_t, N> const &) const noexcept {}
};

struct BitRef {
    std::uint64_t *m_word{};
    std::uint64_t  m_mask{};

    constexpr
    operator bool() const noexcept {
        return (*m_word & m_mask) != 0;
    }

    constexpr BitRef &
    operator=(bool v) noexcept {
        if (v) { *m_word |= m_mask; }
        else { *m_word &= ~m_mask; }
        return *this;
    }

    constexpr BitRef &
    operator=(BitRef const &other) noexcept {
        return *this = static_cast<bool>(other);
    }
};

struct BitAccessor {
    using offset_policy    = BitAccessor;
    using element_type     = bool;
    using reference        = BitRef;
    using data_handle_type = std::uint64_t *;

    constexpr reference
    access(data_handle_type p, size_t i) const noexcept {
        return reference{.m_word = p + (i >> 6), .m_mask = (std::uint64_t{1} << (i & 63))};
    }

    constexpr data_handle_type
    offset(data_handle_type p, size_t i) const noexcept {
        return p + (i >> 6);
    }
};
} // namespace _detail

// Helper functions to
namespace directions {
// template <size_t Dims, typename INT = long long, INT StepSz = 1LL, size_t Simult = 1uz>
// requires(std::is_signed<INT>::value) && (Dims > 1uz)
// inline consteval auto
// get_dirChanges_simult() {
//     constexpr size_t posCount = []<size_t... Is>(std::index_sequence<Is...>) {
//         return (0uz + ... + (static_cast<void>(Is), 2uz));
//     }(std::make_index_sequence<Dims>{});


//     std::array<std::array<long long, 0LL>, Dims> changes;


//     std::array<std::array<INT, Dims>, posCount> res{};
//     for (size_t oneDir = 0uz; oneDir < Dims; ++oneDir) {
//         res[oneDir * 2][oneDir]       = StepSz;
//         res[(oneDir * 2) + 1][oneDir] = (-1 * StepSz);
//     }
//     return res;
// }

template <size_t Dims, typename INT = long long, INT StepSz = 1LL>
requires(std::is_signed<INT>::value) && (Dims > 0uz)
inline consteval auto
get_dirChanges() {
    constexpr size_t posCount = []<size_t... Is>(std::index_sequence<Is...>) {
        return (0uz + ... + (static_cast<void>(Is), 2uz));
    }(std::make_index_sequence<Dims>{});

    std::array<std::array<INT, Dims>, posCount> res{};
    for (size_t oneDir = 0uz; oneDir < Dims; ++oneDir) {
        res[oneDir * 2][oneDir]       = StepSz;
        res[(oneDir * 2) + 1][oneDir] = (-1 * StepSz);
    }
    return res;
}
template <typename INT = long long, INT StepSz = 1LL>
requires(std::is_signed<INT>::value)
inline consteval auto
get_dirChanges_2D() {
    return get_dirChanges<2uz, INT, StepSz>();
}
template <typename INT = long long, INT StepSZ = 1LL>
requires(std::is_signed<INT>::value)
inline consteval auto
get_dirChanges_3D() {
    return get_dirChanges<3uz, INT, StepSZ>();
}
} // namespace directions


// ####################################
// ### CHEBYSHEV EXPLORER
// ####################################

// Explores 'Dims-dimensional' space in Chebyshev-layered fashion (as if by chessboard distance)
template <typename F_Allowed, size_t Dims>
requires(Dims > 0) && requires(F_Allowed f, std::array<size_t, Dims> const &item) {
    { f(item) } -> std::same_as<bool>; // The F_Allowed need to be able to take 'Pos_t const&'
}
class Chebyshev {

#if defined(INCSTD_MDSPAN_UNDER_KOKKOS)
    template <class IndexType, size_t Rank>
    using pf_dextents = Kokkos::dextents<IndexType, Rank>;

    template <class ElementType, class Extents>
    using pf_mdspan = Kokkos::mdspan<ElementType, Extents>;
#else
    template <class IndexType, size_t Rank>
    using pf_dextents = std::dextents<IndexType, Rank>;

    template <class ElementType, class Extents>
    using pf_mdspan = std::mdspan<ElementType, Extents>;
#endif


public:
    using Pos_t   = std::array<size_t, Dims>;
    using Extents = pf_dextents<size_t, Dims>;
    using View_t  = pf_mdspan<char, Extents>;

    using DirChngs_t = std::array<Pos_t, Dims * 2>;


public:
    static constexpr auto c_IDs_sequence = std::make_index_sequence<Dims>{};

    Pos_t m_areaSzs_perDim;
    Pos_t m_areaMins_perDim;
    Pos_t m_startPos;

    std::vector<char> m_visited_storage;
    View_t            m_visited;

    F_Allowed m_f_allowed;

    std::vector<std::deque<Pos_t>> m_VofQueues;
    size_t                         m_queuedCount;
    size_t                         m_queueIDToUseNext = 0uz;


public:
    Chebyshev(Pos_t startPos, Pos_t areaSzs)
        : m_areaSzs_perDim(std::move(areaSzs)), m_areaMins_perDim{}, m_startPos(startPos),
          m_visited_storage(_ctor_total_sz(m_areaSzs_perDim), '.'),
          m_visited(m_visited_storage.data(), _ctor_make_extents(m_areaSzs_perDim)), m_f_allowed{},
          m_VofQueues(1, std::deque<Pos_t>{std::move(startPos)}), m_queuedCount(1uz) {}

    // F_allowed is a unary functor(lambda) taking std::array<size_t, Dims> const &
    Chebyshev(F_Allowed &&f, Pos_t startPos, Pos_t areaSzs)
        : m_areaSzs_perDim(std::move(areaSzs)), m_areaMins_perDim{}, m_startPos(startPos),
          m_visited_storage(_ctor_total_sz(m_areaSzs_perDim), '.'),
          m_visited(m_visited_storage.data(), _ctor_make_extents(m_areaSzs_perDim)),
          m_f_allowed(std::forward<F_Allowed>(f)), m_VofQueues(1, std::deque<Pos_t>{std::move(startPos)}),
          m_queuedCount(1uz) {}

    // F_allowed is a unary functor(lambda) taking std::array<size_t, Dims> const &
    Chebyshev(F_Allowed &&f, Pos_t startPos, Pos_t areaSzs, Pos_t areaMins)
        : m_areaSzs_perDim(std::move(areaSzs)), m_areaMins_perDim{std::move(areaMins)}, m_startPos(startPos),
          m_visited_storage(_ctor_total_sz(m_areaSzs_perDim), '.'),
          m_visited(m_visited_storage.data(), _ctor_make_extents(m_areaSzs_perDim)),
          m_f_allowed(std::forward<F_Allowed>(f)), m_VofQueues(1, std::deque<Pos_t>{std::move(startPos)}),
          m_queuedCount(1uz) {}


    bool
    is_inArea(Pos_t const &p) {
        return [&]<size_t... Is>(Pos_t const &p, std::index_sequence<Is...>) {
            return ((p[Is] >= m_areaMins_perDim[Is]) && ...) && ((p[Is] < m_areaSzs_perDim[Is]) && ...);
        }(p, c_IDs_sequence);
    }

    void
    visit_at(Pos_t const &p) {
        [&]<size_t... Is>(Pos_t const &p, std::index_sequence<Is...>) -> void {
            m_visited[p[Is]...] = 2;
        }(p, c_IDs_sequence);
    }

    bool
    is_alreadyVisited(Pos_t const &p) {
        return [&]<size_t... Is>(Pos_t const &p, std::index_sequence<Is...>) -> bool {
            return m_visited[p[Is]...] != '.';
        }(p, c_IDs_sequence);
    }

    bool
    is_atEnd() {
        return m_queuedCount == 0uz;
    }

    Pos_t
    get_next() {
        Pos_t res = make_filledPos_size_t();
        if (m_queuedCount != 0uz) {
            res = m_VofQueues[m_queueIDToUseNext].front();
            m_VofQueues[m_queueIDToUseNext].pop_front();

            // Make sure we create a new queue if we are at the 'end'
            if (m_queueIDToUseNext == (m_VofQueues.size() - 1)) { m_VofQueues.emplace_back(); }

            for (auto const &oneDir : directions::get_dirChanges<Dims>()) {
                Pos_t toInsert = res;
                [&]<size_t... Is>(std::index_sequence<Is...>) { ((toInsert[Is] += oneDir[Is]), ...); }(c_IDs_sequence);

                if (is_inArea(toInsert) && not is_alreadyVisited(toInsert) && m_f_allowed(toInsert)) {
                    visit_at(toInsert);
                    size_t queInsertID = 0uz;
                    for (int i = 0; i < Dims; ++i) {
                        queInsertID =
                            std::max(queInsertID, (toInsert[i] > m_startPos[i] ? toInsert[i] - m_startPos[i]
                                                                               : m_startPos[i] - toInsert[i]));
                    }

                    m_VofQueues.at(queInsertID).push_back(std::move(toInsert));
                    m_queueIDToUseNext = std::min(m_queueIDToUseNext, queInsertID);
                    m_queuedCount++;
                }
            }
            while (m_queueIDToUseNext < m_VofQueues.size() && m_VofQueues[m_queueIDToUseNext].empty()) {
                m_queueIDToUseNext++;
            }
            m_queuedCount--;
        }
        return res;
    }


private:
    static size_t
    _ctor_total_sz(Pos_t const &sizes) {
        return std::ranges::fold_left(sizes, 1uz, std::multiplies{});
    }

    static Extents
    _ctor_make_extents(Pos_t const &sizes) {
        return [&]<size_t... Is>(std::index_sequence<Is...>) { return Extents(sizes[Is]...); }(c_IDs_sequence);
    }

    static constexpr Pos_t
    make_filledPos_size_t() {
        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return Pos_t{((void)Is, std::numeric_limits<size_t>::max())...};
        }(std::make_index_sequence<Dims>{});
    }
};

// Deduction guides
template <size_t N>
Chebyshev(std::array<size_t, N> const &, std::array<size_t, N> const &)
    -> Chebyshev<std::remove_cvref_t<decltype([](auto const &item) { return true; })>, N>;

template <typename F, size_t N>
Chebyshev(F &&, std::array<size_t, N> const &, std::array<size_t, N> const &) -> Chebyshev<std::remove_cvref_t<F>, N>;

template <typename F, size_t N>
Chebyshev(F &&, std::array<size_t, N> const &, std::array<size_t, N> const &, std::array<size_t, N> const &)
    -> Chebyshev<std::remove_cvref_t<F>, N>;


// ####################################
// ### FLOODFILL
// ####################################

template <size_t Dims, typename F_Allowed, typename F_OnFill>
requires(Dims > 1) && requires(std::array<size_t, Dims> const &item, F_Allowed f_a, F_OnFill f_of) {
    { f_a(item) } -> std::same_as<bool>;  // The F_Allowed need to be able to take 'Pos_t const&'
    { f_of(item) } -> std::same_as<void>; // The F_OnFill need to be able to take 'Pos_t const&'
}
class FloodFill {
public:
    using Pos_t = std::array<size_t, Dims>;

    FloodFill(Pos_t areaSzs) : FloodFill(areaSzs, _detail::NoOpAlwaysTrue<Dims>{}) {}
    // F_allowed is a unary functor(lambda) taking std::array<size_t, Dims> const &

    FloodFill(Pos_t areaSzs, F_Allowed &&f_a)
        : FloodFill(areaSzs, std::forward<F_Allowed>(f_a), _detail::NoOpOnFill<Dims>{}) {}

    // F_allowed is a unary functor(lambda) taking std::array<size_t, Dims> const &
    FloodFill(Pos_t areaSzs, F_Allowed &&f_a, F_OnFill &&f_of)
        : FloodFill(areaSzs, {}, std::forward<F_Allowed>(f_a), std::forward<F_OnFill>(f_of)) {}

    // F_allowed is a unary functor(lambda) taking std::array<size_t, Dims> const &
    FloodFill(Pos_t areaSzs, Pos_t areaMins, F_Allowed &&f_a = _detail::NoOpAlwaysTrue<Dims>{},
              F_OnFill &&f_of = _detail::NoOpOnFill<Dims>{})
        : m_areaSzs_perDim(std::move(areaSzs)), m_areaMins_perDim{std::move(areaMins)},
          m_visited_storage(_ctor_total_words(m_areaSzs_perDim), 0),
          m_visited(m_visited_storage.data(), _ctor_make_extents(m_areaSzs_perDim)),
          m_f_allowed(std::forward<F_Allowed>(f_a)), m_f_onfill(std::forward<F_OnFill>(f_of)) {}


    constexpr bool
    is_inArea(Pos_t const &p) const {
        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return ((p[Is] >= m_areaMins_perDim[Is]) && ...) && ((p[Is] < m_areaSzs_perDim[Is]) && ...);
        }(c_IDs_sequence);
    }

    constexpr bool
    is_alreadyVisited(Pos_t const &p) const {
        return [&]<size_t... Is>(std::index_sequence<Is...>) -> bool { return m_visited[p[Is]...]; }(c_IDs_sequence);
    }

    constexpr void
    visit_at(Pos_t const &p) {
        [&]<size_t... Is>(std::index_sequence<Is...>) -> void { m_visited[p[Is]...] = true; }(c_IDs_sequence);
    }

    constexpr std::size_t
    execute_fill(Pos_t seed) {
        using DimRange_t = std::array<Pos_t, 2uz>;
        using Frame_t    = std::array<DimRange_t, 2 * (Dims - 1)>;

        std::size_t         res{};
        std::stack<Frame_t> seedScanRngs{};

        std::optional<Pos_t> leftSeed = seed;
        Pos_t                rightSeed;

        auto fillFromSeed = [&]() {
            Pos_t &ls = leftSeed.value();
            rightSeed = ls;
            rightSeed.back()++;

            while (is_inArea(ls) && m_f_allowed(leftSeed.value())) {
                res++;
                visit_at(ls);
                m_f_onfill(ls);
                ls.back()--;
            }
            while (is_inArea(rightSeed) && m_f_allowed(rightSeed)) {
                res++;
                visit_at(rightSeed);
                m_f_onfill(rightSeed);
                rightSeed.back()++;
            }
            ls.back()++;
            rightSeed.back()--;

            // Adding new ranges to scan
            seedScanRngs.push([&]<size_t... Is>(std::index_sequence<Is...>) {
                return Frame_t{{((void)Is, DimRange_t{ls, rightSeed})...}};
            }(c_IDs_sequenceMinusDouble));


            [&]<size_t... Is>(std::index_sequence<Is...>) {
                for (size_t id{}; auto const &oneDir : directions::get_dirChanges<Dims - 1>()) {
                    ((seedScanRngs.top().at(id).front()[Is] += oneDir[Is]), ...);
                    id++;
                }

                // seedScanRngs.top();
            }(c_IDs_sequenceMinus);
        };

        auto searchForSeed = [&]() -> std::optional<Pos_t> {
            std::optional<Pos_t> res{};

            while (not seedScanRngs.empty()) {
                Frame_t &oneFrame = seedScanRngs.top();
                for (auto &[from, to] : oneFrame) {
                    while ((is_alreadyVisited(from) || not m_f_allowed(from)) && from.back() <= to.back()) {
                        from.back()++;
                    }
                    if (from.back() <= to.back()) {
                        res = from;
                        from.back()++;
                        goto RET;
                    }
                }
                seedScanRngs.pop();
            }

        RET:
            return res;
        };

        if (not is_inArea(leftSeed.value()) || not m_f_allowed(leftSeed.value()) ||
            is_alreadyVisited(leftSeed.value())) {
            goto RET;
        }

        fillFromSeed();
        while (leftSeed = searchForSeed(), leftSeed) { fillFromSeed(); }

    RET:
        return res;
    }

private:
#if defined(INCSTD_MDSPAN_UNDER_KOKKOS)
    template <class IndexType, size_t Rank>
    using pf_dextents = Kokkos::dextents<IndexType, Rank>;

    template <class ElementType, class Extents, class... Args>
    using pf_mdspan = Kokkos::mdspan<ElementType, Extents, Args...>;

    using pf_layout_right = Kokkos::layout_right;
#else
    template <class IndexType, size_t Rank>
    using pf_dextents = std::dextents<IndexType, Rank>;

    template <class ElementType, class Extents, class... Args>
    using pf_mdspan = std::mdspan<ElementType, Extents, Args...>;

    using pf_layout_right = std::layout_right;
#endif

    using Extents_t = pf_dextents<size_t, Dims>;
    using View_t    = pf_mdspan<bool, Extents_t, pf_layout_right, _detail::BitAccessor>;

    static constexpr auto c_IDs_sequence            = std::make_index_sequence<Dims>{};
    static constexpr auto c_IDs_sequenceMinus       = std::make_index_sequence<Dims - 1>{};
    static constexpr auto c_IDs_sequenceMinusDouble = std::make_index_sequence<2 * (Dims - 1)>{};

    Pos_t m_areaSzs_perDim;
    Pos_t m_areaMins_perDim;

    std::vector<std::uint64_t> m_visited_storage;
    View_t                     m_visited;

    F_Allowed m_f_allowed;
    F_OnFill  m_f_onfill;


private:
    static constexpr size_t
    _ctor_total_sz(Pos_t const &sizes) {
        return std::ranges::fold_left(sizes, 1uz, std::multiplies{});
    }

    static constexpr size_t
    _ctor_total_words(Pos_t const &sizes) {
        auto const bits = _ctor_total_sz(sizes);
        return (bits + 63uz) / 64uz;
    }

    static constexpr Extents_t
    _ctor_make_extents(Pos_t const &sizes) {
        return [&]<size_t... Is>(std::index_sequence<Is...>) { return Extents_t(sizes[Is]...); }(c_IDs_sequence);
    }
};

// Deduction guides
template <size_t N>
FloodFill(std::array<size_t, N> const &) -> FloodFill<N, _detail::NoOpAlwaysTrue<N>, _detail::NoOpOnFill<N>>;

template <size_t N, typename F_A>
FloodFill(std::array<size_t, N> const &, F_A &&) -> FloodFill<N, std::remove_cvref_t<F_A>, _detail::NoOpOnFill<N>>;

template <size_t N, typename F_A, typename F_OF>
FloodFill(std::array<size_t, N> const &, F_A &&, F_OF &&)
    -> FloodFill<N, std::remove_cvref_t<F_A>, std::remove_cvref_t<F_OF>>;

template <size_t N, typename F_A, typename F_OF>
FloodFill(std::array<size_t, N> const &, std::array<size_t, N> const &, F_A &&, F_OF &&)
    -> FloodFill<N, std::remove_cvref_t<F_A>, std::remove_cvref_t<F_OF>>;
} // namespace incom::standard::explorers
