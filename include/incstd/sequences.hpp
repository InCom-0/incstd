#pragma once

#include <cassert>

#include <ankerl/unordered_dense.h>
#include <more_concepts/more_concepts.hpp>

#include <incstd/hashing.hpp>


namespace incom::standard::seq {
using namespace incom::standard;

template <typename T, typename F>
requires more_concepts::container<T> && std::predicate<F, std::vector<typename T::value_type>>
auto build_map_uniqueSubSeq2startPos(T const &inputSequence, F filter_subSeq, int const min_occurenceOfUniqueSubseq = 1,
                                     int const max_subSeqSize = INT_MAX, int const min_subSeqSize = 1) {
    ankerl::unordered_dense::map<std::vector<typename T::value_type>,
                                 ankerl::unordered_dense::set<size_t, hashing::XXH3Hasher>, hashing::XXH3Hasher>
        mapToBuild;
    assert(
        (void("Unique subsequences cannot occur 0 times, that wouldn't make sense"), min_occurenceOfUniqueSubseq > 0));
    assert((void("Maximum subsequence size cannot be less than 1, that wouldn't make sense"), max_subSeqSize > 0));
    assert((void("Maximum subsequence size cannot be less than minimum subsequence size, that wouldn't make sense"),
            max_subSeqSize >= min_subSeqSize));

    if (min_occurenceOfUniqueSubseq == 1) {
        std::vector dp(inputSequence.size() + 1, 0);
        for (int i = inputSequence.size() - 1; i >= 0; --i) {
            dp[i] = 1 + std::min(dp[i + 1], max_subSeqSize - 1);
            for (int add = dp[i]; add >= min_subSeqSize; --add) {
                std::vector<typename T::value_type> tmpSubSeq(inputSequence.begin() + i,
                                                              inputSequence.begin() + i + add);
                if (filter_subSeq(tmpSubSeq)) { mapToBuild[tmpSubSeq].emplace(i); }
            }
        }
    }
    else {
        std::vector dp(inputSequence.size() + 1, std::vector(inputSequence.size() + 1, 0));
        for (int i = inputSequence.size() - 1; i >= 0; --i) {
            for (int j = inputSequence.size() - 1; j > i; --j) {
                if (inputSequence[i] == inputSequence[j]) {
                    dp[i][j] = 1 + std::min(dp[i + 1][j + 1], std::min(max_subSeqSize - 1, j - i - 1));
                    for (int add = dp[i][j]; add >= min_subSeqSize; --add) {
                        std::vector<typename T::value_type> tmpSubSeq(inputSequence.begin() + i,
                                                                      inputSequence.begin() + i + add);
                        if (filter_subSeq(tmpSubSeq)) {
                            mapToBuild[tmpSubSeq].emplace(i);
                            mapToBuild[tmpSubSeq].emplace(j);
                        }
                    }
                }
            }
        }
        if (min_occurenceOfUniqueSubseq > 2) {
            std::erase_if(mapToBuild,
                          [&](auto const &item) { return item.second.size() < min_occurenceOfUniqueSubseq; });
        }
    }
    return mapToBuild;
}

template <typename T>
requires more_concepts::container<T>
// Overload: No filter of subsequences
auto build_map_uniqueSubSeq2startPos(T const &inputSequence, int const min_occurenceOfUniqueSubseq = 1,
                                     int const max_subSeqSize = INT_MAX, int const min_subSeqSize = 1) {
    return build_map_uniqueSubSeq2startPos(
        inputSequence, [](std::vector<typename T::value_type> const &a) { return true; }, min_occurenceOfUniqueSubseq,
        min_subSeqSize, max_subSeqSize);
}

namespace solvers {
template <typename T, typename F, size_t max_numOfRes = 1, size_t min_repCountOfUnique = 1,
          size_t max_repCountOfUnique = SIZE_MAX>
requires more_concepts::container<T> && std::predicate<F, std::vector<typename T::value_type>>
auto solve_seqFromRepUniqueSubseq(T const &inputSequence, F const filter_subSeq,
                                  int const max_ofUniqueSubseqInRes = INT_MAX, int const min_ofUniqueSubseqInRes = 1,
                                  int min_occurenceOfUniqueSubseq = 1, int const max_subSeqSize = INT_MAX,
                                  int const min_subSeqSize = 1)
    -> std::optional<std::vector<std::vector<std::vector<typename T::value_type>>>> {

    assert((void("Maximum number of unique subsequence in result cannot be less than 1"), max_ofUniqueSubseqInRes > 0));
    assert((void("Maximum number of unique subsequence in result cannot be less than minimum number of unique "
                 "subsequences in result"),
            max_ofUniqueSubseqInRes >= min_ofUniqueSubseqInRes));

    ankerl::unordered_dense::map<std::vector<typename T::value_type>,
                                 ankerl::unordered_dense::set<size_t, hashing::XXH3Hasher>, hashing::XXH3Hasher> const
        mp_subseq_2_ids = build_map_uniqueSubSeq2startPos(inputSequence, filter_subSeq, min_occurenceOfUniqueSubseq,
                                                          max_subSeqSize, min_subSeqSize);

    ankerl::unordered_dense::map<size_t, std::vector<std::vector<typename T::value_type>>, hashing::XXH3Hasher>
        mp_pos_2_subseq;

    for (auto const &mpItem : mp_subseq_2_ids) {
        for (auto const &posItem : mpItem.second) {
            mp_pos_2_subseq.insert({posItem, std::vector<std::vector<typename T::value_type>>()});
            mp_pos_2_subseq[posItem].push_back(mpItem.first);
        }
    }

    constexpr size_t const min_repCountOfUnique_adj = (min_repCountOfUnique == 0 ? 1 : min_repCountOfUnique);
    size_t                 curHead                  = 0;

    ankerl::unordered_dense::map<typename std::vector<typename T::value_type>, size_t, hashing::XXH3Hasher> selTracker;
    if constexpr (max_repCountOfUnique == 0) {
        static_assert(false,
                      "Trying to solve for 'maximum repeat count of unique subsequence = 0' does not make sense");
    }

    if constexpr (max_numOfRes == 0) {
        static_assert(false, "Trying to solve for 'maximum number of results = 0' does not make sense");
    }
    else {
        size_t                                                        inSelTrack_smaller = 0;
        std::vector<std::vector<typename T::value_type>>              res_inProgress;
        std::vector<std::vector<std::vector<typename T::value_type>>> res_storage;

        // Recursive solver.
        // Explores in a DFS manner all the possible arrangements of subsequences from the beginning
        // Respects how many different subsequences can be used (therefore short circuits on most of the unsuitable
        // parts of the tree)
        auto rec_inside_solver = [&](this auto &self) -> void {
            for (auto const &selOption : mp_pos_2_subseq[curHead]) {

                if (selTracker.contains(selOption)) {
                    if (selTracker.at(selOption) == max_repCountOfUnique) { continue; }
                    selTracker.at(selOption)++;
                }
                else if (selTracker.size() < max_ofUniqueSubseqInRes) {
                    selTracker.emplace(selOption, 1);
                    inSelTrack_smaller++;
                }
                else { continue; }

                res_inProgress.push_back(selOption);
                if (selTracker.at(selOption) == min_repCountOfUnique_adj) { inSelTrack_smaller--; }
                curHead += selOption.size();

                if (curHead == inputSequence.size() && selTracker.size() >= min_ofUniqueSubseqInRes &&
                    inSelTrack_smaller == 0) {
                    res_storage.push_back(res_inProgress); // Success! push_back one result, return one level up
                }
                // Recursive call
                else { self(); }

                if (res_storage.size() == max_numOfRes) { return; }

                curHead -= selOption.size();
                if (selTracker.at(selOption) == min_repCountOfUnique_adj) { inSelTrack_smaller++; }
                res_inProgress.pop_back();

                selTracker.at(selOption)--;
                if (selTracker.at(selOption) == 0) {
                    selTracker.erase(selOption);
                    inSelTrack_smaller--;
                }
            }
            return;
        };

        rec_inside_solver();
        if (res_storage.empty()) { return std::nullopt; }
        else { return res_storage; }
    }
}
template <typename T, size_t max_numOfRes = 1, size_t min_repCountOfUnique = 1, size_t max_repCountOfUnique = SIZE_MAX>
requires more_concepts::container<T>
// Overload: No filter of subsequences
auto solve_seqFromRepUniqueSubseq(T const &inputSequence, int const max_ofUniqueSubseqInRes = INT_MAX,
                                  int const min_ofUniqueSubseqInRes = 1, int min_occurenceOfUniqueSubseq = 1,
                                  int const max_subSeqSize = INT_MAX, int const min_subSeqSize = 1) {
    auto funcToPass = [](std::vector<typename T::value_type> const &a) { return true; };
    return solve_seqFromRepUniqueSubseq<T, decltype(funcToPass), max_numOfRes, min_repCountOfUnique,
                                        max_repCountOfUnique>(inputSequence, funcToPass, max_ofUniqueSubseqInRes,
                                                              min_ofUniqueSubseqInRes, min_occurenceOfUniqueSubseq,
                                                              max_subSeqSize, min_subSeqSize);
}

} // namespace solvers
} // namespace incom::standard::seq