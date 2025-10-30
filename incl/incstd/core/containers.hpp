#pragma once

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <iterator>
#include <ranges>
#include <vector>


namespace incom::standard::containers {
namespace detail {

using namespace incom::standard;

template <typename T>
struct _RingVector_sentinel {};

template <typename T>
class _RingVector_iter {
    using base_iterator  = std::ranges::iterator_t<std::vector<T>>;
    using base_sentinel  = std::ranges::sentinel_t<std::vector<T>>;
    using base_reference = std::ranges::range_reference_t<std::vector<T>>;

    base_iterator _iter;
    base_iterator _sentinel;
    base_iterator _iter_underlying_beg;
    base_sentinel _sentinel_underlying;
    bool          circled = false;

public:
    using difference_type = std::ptrdiff_t;
    using value_type      = T;
    using reference       = base_reference;

    using iterator_category = std::forward_iterator_tag;

    _RingVector_iter() = default;

    _RingVector_iter(base_iterator begin, base_iterator begin_underlying, base_sentinel end_underlying)
        : _iter(begin), _sentinel(begin), _iter_underlying_beg(begin_underlying), _sentinel_underlying(end_underlying) {
    }

    T &operator*() const { return *_iter; };
    T &operator*() { return *_iter; };


    _RingVector_iter &operator++() {
        if (++_iter == _sentinel_underlying) {
            _iter   = _iter_underlying_beg;
            circled = true;
        }
        return *this;
    }
    _RingVector_iter operator++(int) // post-incrementable, returns prev value
    {
        _RingVector_iter temp = *this;
        ++(*this);
        return temp;
    }

    auto operator==(const _RingVector_iter &other) const {
        return other._iter == _iter && other._sentinel == _sentinel &&
               other._iter_underlying_beg == _iter_underlying_beg && other._sentinel_underlying == _sentinel_underlying;
    }

    auto operator<=>(const _RingVector_iter<T> &other) const = default;

    auto operator==(const _RingVector_sentinel<T> &) const -> bool { return (circled == true && _iter == _sentinel); }
};

} // namespace detail

// Ring vector for future usage in 'scrolling plot' scenarios
template <typename T>
class RingVector {
private:
    std::vector<T> _m_buf;
    size_t         _m_head = 0;
    size_t         _m_tail = 0;

public:
    void changeTailTo(T const &item) { _m_buf[_m_tail] = item; }
    void changeTailTo(T &&item) { _m_buf[_m_tail] = std::move(item); }

    void update_preRotate(T const &item) {
        changeTailTo(item);
        rotate_byOne();
    }
    void update_preRotate(T &&item) {
        changeTailTo(std::forward<decltype(item)>(item));
        rotate_byOne();
    }
    void update_postRotate(T const &item) {
        rotate_byOne();
        changeTailTo(item);
    }
    void update_postRotate(T &&item) {
        rotate_byOne();
        changeTailTo(std::forward<decltype(item)>(item));
    }

    void rotate_byOne() {
        _m_tail = _m_head;
        _m_head = ((_m_head + 1 == _m_buf.size()) ? 0 : (_m_head + 1));
    }
    RingVector(std::vector<T> &&t) : _m_buf(std::move(t)), _m_tail(std::max(0uz, _m_buf.size() - 1)) {};

    RingVector(std::vector<T> const &t) : _m_buf(t), _m_tail(std::max(0uz, _m_buf.size() - 1)) {};


    [[nodiscard]] constexpr detail::_RingVector_iter<T> begin() {
        return detail::_RingVector_iter<T>(_m_buf.begin() + _m_head, _m_buf.begin(), _m_buf.end());
    };
    [[nodiscard]] constexpr detail::_RingVector_sentinel<T> end() { return detail::_RingVector_sentinel<T>{}; };
    [[nodiscard]] constexpr auto                            cbegin() {
        return detail::_RingVector_iter<T>(_m_buf.begin() + _m_head, _m_buf.begin(), _m_buf.end());
    };
    [[nodiscard]] constexpr auto cend() { return detail::_RingVector_sentinel<T>{}; };

    [[nodiscard]] constexpr const auto begin() const {
        return detail::_RingVector_iter<T>(_m_buf.begin() + _m_head, _m_buf.begin(), _m_buf.end());
    };
    [[nodiscard]] constexpr const auto end() const { return detail::_RingVector_sentinel<T>{}; };

    size_t size() { return _m_buf.size(); }

    // TODO: Might not need to create a copy here or below once std::views::concatenate from C++26 exists
    constexpr std::vector<T> create_copy() const {
        std::vector<T> res(_m_buf.begin() + _m_head, _m_buf.end());
        for (int i = 0; i < _m_head; ++i) { res.push_back(_m_buf[i]); }
        return res;
    }

    constexpr std::vector<T> create_copy_reversed() const {
        std::vector<T> res(_m_buf.rbegin() + (_m_buf.size() - _m_head), _m_buf.rend());
        for (int i = (_m_buf.size() - 1); i >= _m_head; --i) { res.push_back(_m_buf[i]); }
        return res;
    }
};

template <typename T>
RingVector(std::vector<T> &&t) -> RingVector<T>;
template <typename T>
RingVector(std::vector<T> const &t) -> RingVector<T>;


} // namespace incom::standard::containers