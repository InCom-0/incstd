#pragma once

#include <more_concepts/more_concepts.hpp>
#include <xxhash.h>

#include <incstd/core/concepts.hpp>


namespace incom::standard::hashing {
using namespace incom::standard;

namespace detail {
template <typename T>
concept has_to_ullong = requires(T a) {
    { a.to_ullong() } -> std::same_as<unsigned long long>;
};
} // namespace detail

struct XXH3Hasher {
    // DIRECTLY HASHABLE BECAUSE OF CONTIGUOUS DATA
    template <typename T>
    requires std::is_arithmetic_v<std::decay_t<T>>
    constexpr std::size_t operator()(T &&input) const {
        return XXH3_64bits(&input, sizeof(T));
    }
    template <typename T>
    requires more_concepts::random_access_container<std::remove_cvref_t<T>> &&
             std::is_arithmetic_v<typename T::value_type>
    constexpr std::size_t operator()(T &&input) const {
        return XXH3_64bits(input.data(),
                           sizeof(typename std::remove_cvref_t<decltype(input)>::value_type) * input.size());
    }
    // OTHER DIRECTLY HASHABLE
    template <typename T>
    requires detail::has_to_ullong<T>
    constexpr std::size_t operator()(T const &&input) const {
        return (*this)(input.to_ullong());
    }

    // REQUIRES GRADUAL BUILDUP OF XXH3_STATE OUT OF DIS-CONTIGUOUS DATA INSIDE THE INPUT TYPE
    template <typename T>
    constexpr std::size_t operator()(T &&input) const {
        XXH3_state_t *state = XXH3_createState();
        XXH3_64bits_reset(state);
        this->_hashTypeX(input, state);

        XXH64_hash_t result = XXH3_64bits_digest(state);
        XXH3_freeState(state);
        return result;
    }

    template <typename T>
    requires std::is_arithmetic_v<std::decay_t<T>>
    constexpr void _hashTypeX(T &input, XXH3_state_t *state) const {
        XXH3_64bits_update(state, &input, sizeof(T));
    }

    template <concepts::is_some_pair T>
    constexpr void _hashTypeX(T &input, XXH3_state_t *state) const {
        this->_hashTypeX(input.first, state);
        this->_hashTypeX(input.second, state);
    }

    template <typename T>
    requires std::is_same_v<std::remove_cvref_t<T>, std::string_view>
    constexpr void _hashTypeX(T &input, XXH3_state_t *state) const {
        XXH3_64bits_update(state, input.data(),
                           sizeof(typename std::remove_cvref_t<decltype(input)>::value_type) * input.size());
    }
    template <typename T>
    requires more_concepts::random_access_container<std::remove_cvref_t<T>> &&
             std::is_arithmetic_v<typename T::value_type>
    constexpr void _hashTypeX(T &input, XXH3_state_t *state) const {
        XXH3_64bits_update(state, input.data(),
                           sizeof(typename std::remove_cvref_t<decltype(input)>::value_type) * input.size());
    }
    template <typename T>
    requires more_concepts::random_access_container<std::remove_cvref_t<T>> &&
             concepts::is_some_pair<typename T::value_type>
    constexpr void _hashTypeX(T &input, XXH3_state_t *state) const {
        for (auto const &item : input) {
            this->_hashTypeX(item.first, state);
            this->_hashTypeX(item.second, state);
        }
    }

    template <typename T>
    requires more_concepts::random_access_container<std::remove_cvref_t<T>> &&
             more_concepts::random_access_container<std::remove_cvref_t<typename T::value_type>>
    constexpr void _hashTypeX(T &input, XXH3_state_t *state) const {
        for (auto &VinV : input) { this->_hashTypeX(VinV, state); }
    }
};

} // namespace incom::standard::hasing