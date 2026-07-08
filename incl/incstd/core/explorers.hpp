#pragma once

#include <algorithm>
#include <deque>
#include <vector>

#include <incstd/polyfills/mdspan.hpp>


namespace incom::standard::explorers {
using namespace incom::standard;

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
    static constexpr auto       c_IDs_sequence = std::make_index_sequence<Dims>{};
    static constexpr DirChngs_t m_dirChanges   = [] {
        DirChngs_t res{};
        for (size_t i = 0; i < Dims; ++i) {
            res[i * 2][i]     = -1;
            res[i * 2 + 1][i] = 1;
        }
        return res;
    }();

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

            for (auto const &oneDir : m_dirChanges) {
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
        }
        while (m_queueIDToUseNext < m_VofQueues.size() && m_VofQueues[m_queueIDToUseNext].empty()) {
            m_queueIDToUseNext++;
        }
        m_queuedCount--;
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

} // namespace incom::standard::explorers
