#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace incom::standard::web {
using namespace std::literals;

class URISyntaxException : public std::invalid_argument {
public:
    explicit URISyntaxException(std::string_view message);
    URISyntaxException(std::string_view message, std::string_view uri);
};

// TODO: Move this class into incstd after some experience with it
class URI {
public:
    using QueryParameters = std::vector<std::pair<std::string, std::string>>;

    URI();
    explicit URI(std::string_view uri, bool heuristics = false);
    explicit URI(const char *uri, bool heuristics = false);
    URI(std::string_view scheme, std::string_view pathEtc);
    URI(std::string_view scheme, std::string_view authority, std::string_view pathEtc);
    URI(std::string_view scheme, std::string_view authority, std::string_view path, std::string_view query);
    URI(std::string_view scheme, std::string_view authority, std::string_view path, std::string_view query,
        std::string_view fragment);
    URI(const URI &uri);
    URI(URI &&uri) noexcept;
    URI(const URI &baseURI, std::string_view relativeURI);
    explicit URI(const std::filesystem::path &path);
    ~URI();

    URI &operator=(const URI &uri);
    URI &operator=(URI &&uri) noexcept;
    URI &operator=(std::string_view uri);
    URI &operator=(const char *uri);

    void swap(URI &uri) noexcept;
    void clear();

    std::string toString() const;

    const std::string &getScheme() const;
    void               setScheme(std::string_view scheme);

    const std::string &getUserInfo() const;
    void               setUserInfo(std::string_view userInfo);

    const std::string &getHost() const;
    void               setHost(std::string_view host);

    unsigned short getPort() const;
    void           setPort(unsigned short port);
    unsigned short getSpecifiedPort() const;

    std::string getAuthority() const;
    void        setAuthority(std::string_view authority);

    const std::string &getPath() const;
    void               setPath(std::string_view path);

    std::string        getQuery() const;
    void               setQuery(std::string_view query);
    void               addQueryParameter(std::string_view param, std::string_view val = "");
    const std::string &getRawQuery() const;
    void               setRawQuery(std::string_view query);
    QueryParameters    getQueryParameters(bool plusIsSpace = true) const;
    void               setQueryParameters(const QueryParameters &params);

    std::string getFragment() const;
    void        setFragment(std::string_view fragment);
    std::string getRawFragment() const;
    void        setRawFragment(std::string_view fragment);

    void        setPathEtc(std::string_view pathEtc);
    std::string getPathEtc() const;
    std::string getPathAndQuery() const;

    void resolve(std::string_view relativeURI);
    void resolve(const URI &relativeURI);

    bool isRelative() const;
    bool empty() const;

    bool operator==(const URI &uri) const;
    bool operator==(std::string_view uri) const;
    bool operator!=(const URI &uri) const;
    bool operator!=(std::string_view uri) const;

    void normalize();
    void getPathSegments(std::vector<std::string> &segments) const;

    static void encode(std::string_view str, std::string_view reserved, std::string &encodedStr);
    static void decode(std::string_view str, std::string &decodedStr, bool plusAsSpace = false);

protected:
    bool           equals(const URI &uri) const;
    bool           isWellKnownPort() const;
    unsigned short getWellKnownPort() const;

    void        parse(std::string_view uri);
    void        parse(std::string_view uri, bool heuristics);
    void        parseAuthority(std::string_view authority);
    void        parseHostAndPort(std::string_view hostAndPort);
    void        parsePath(std::string_view path);
    void        parsePathEtc(std::string_view pathEtc);
    void        parseQuery(std::string_view query);
    void        parseFragment(std::string_view fragment);
    void        mergePath(std::string_view path);
    void        removeDotSegments(bool removeLeading = true);
    static void getPathSegments(std::string_view path, std::vector<std::string> &segments);
    void        buildPath(const std::vector<std::string> &segments, bool leadingSlash, bool trailingSlash);

    static const std::string RESERVED_PATH;
    static const std::string RESERVED_QUERY;
    static const std::string RESERVED_QUERY_PARAM;
    static const std::string RESERVED_FRAGMENT;
    static const std::string ILLEGAL;

private:
    std::string    _scheme;
    std::string    _userInfo;
    std::string    _host;
    unsigned short _port;
    std::string    _path;
    std::string    _query;
    std::string    _fragment;
};

inline const std::string &URI::getScheme() const {
    return _scheme;
}

inline const std::string &URI::getUserInfo() const {
    return _userInfo;
}

inline const std::string &URI::getHost() const {
    return _host;
}

inline const std::string &URI::getPath() const {
    return _path;
}

inline const std::string &URI::getRawQuery() const {
    return _query;
}

inline std::string URI::getRawFragment() const {
    return _fragment;
}

inline unsigned short URI::getSpecifiedPort() const {
    return _port;
}

inline void swap(URI &u1, URI &u2) noexcept {
    u1.swap(u2);
}

std::pair<bool, std::string> parse_uri_string(std::string_view uri);

namespace detail {
inline void toLowerInPlace(std::string &value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

inline void appendHex2(std::string &output, unsigned char value) {
    static constexpr char HEX[] = "0123456789ABCDEF";
    output.push_back(HEX[(value >> 4) & 0x0F]);
    output.push_back(HEX[value & 0x0F]);
}

inline bool tryParseInt(std::string_view sv, int &value) {
    if (sv.empty()) { return false; }

    bool        negative = false;
    std::size_t pos      = 0;
    if (sv[pos] == '+' || sv[pos] == '-') {
        negative = (sv[pos] == '-');
        ++pos;
    }
    if (pos >= sv.size()) { return false; }

    int  parsed    = 0;
    auto first     = sv.data() + static_cast<std::ptrdiff_t>(pos);
    auto last      = sv.data() + static_cast<std::ptrdiff_t>(sv.size());
    auto [ptr, ec] = std::from_chars(first, last, parsed);
    if (ec != std::errc() || ptr != last) { return false; }

    value = negative ? -parsed : parsed;
    return true;
}

inline std::string makeSyntaxMessage(std::string_view message, std::string_view uri) {
    if (uri.empty()) { return std::string(message); }
    std::ostringstream oss;
    oss << message << ": " << uri;
    return oss.str();
}

inline bool isLikelyBrowserAuthority(std::string_view authority) {
    if (authority.empty()) { return false; }
    if (authority.find('@') != std::string_view::npos) { return true; }
    if (authority == "localhost") { return true; }
    if (authority.front() == '[') { return authority.find(']') != std::string_view::npos; }
    if (authority.find('.') != std::string_view::npos) { return true; }

    const std::size_t colonPos = authority.rfind(':');
    if (colonPos != std::string_view::npos && colonPos + 1 < authority.size()) {
        for (std::size_t i = colonPos + 1; i < authority.size(); ++i) {
            if (authority[i] < '0' || authority[i] > '9') { return false; }
        }
        return true;
    }

    return false;
}

inline bool hasWindowsDrivePrefix(std::string_view value) {
    return value.size() >= 2 && std::isalpha(static_cast<unsigned char>(value[0])) && value[1] == ':';
}

inline bool startsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

inline bool isWindowsPathLike(std::string_view uri) {
    if (uri.empty()) { return false; }

    if (hasWindowsDrivePrefix(uri)) { return true; }

    if (uri[0] == '\\') { return true; }
    if (startsWith(uri, ".\\") || startsWith(uri, "..\\")) { return true; }

    return uri.find('\\') != std::string_view::npos;
}

inline void normalizePathSeparatorsInPlace(std::string &value) {
    std::replace(value.begin(), value.end(), '\\', '/');
}

} // namespace detail


inline const std::string URI::RESERVED_PATH        = "?#";
inline const std::string URI::RESERVED_QUERY       = "?#/:;+@";
inline const std::string URI::RESERVED_QUERY_PARAM = "?#/:;+@&=";
inline const std::string URI::RESERVED_FRAGMENT    = "";
inline const std::string URI::ILLEGAL              = "%<>{}|\\\"^`!*'()$,[]";


inline URISyntaxException::URISyntaxException(std::string_view message) : std::invalid_argument(std::string(message)) {}


inline URISyntaxException::URISyntaxException(std::string_view message, std::string_view uri)
    : std::invalid_argument(detail::makeSyntaxMessage(message, uri)) {}


inline URI::URI() : _port(0) {}


inline URI::URI(std::string_view uri, bool heuristics) : _port(0) {
    parse(uri, heuristics);
}


inline URI::URI(const char *uri, bool heuristics) : _port(0) {
    parse(uri, heuristics);
}


inline URI::URI(std::string_view scheme, std::string_view pathEtc) : _scheme(scheme), _port(0) {
    detail::toLowerInPlace(_scheme);
    parsePathEtc(pathEtc);
}


inline URI::URI(std::string_view scheme, std::string_view authority, std::string_view pathEtc) : _scheme(scheme) {
    detail::toLowerInPlace(_scheme);
    parseAuthority(authority);
    parsePathEtc(pathEtc);
}


inline URI::URI(std::string_view scheme, std::string_view authority, std::string_view path, std::string_view query)
    : _scheme(scheme), _path(path), _query(query) {
    detail::toLowerInPlace(_scheme);
    parseAuthority(authority);
}


inline URI::URI(std::string_view scheme, std::string_view authority, std::string_view path, std::string_view query,
                std::string_view fragment)
    : _scheme(scheme), _path(path), _query(query), _fragment(fragment) {
    detail::toLowerInPlace(_scheme);
    parseAuthority(authority);
}


inline URI::URI(const URI &uri)
    : _scheme(uri._scheme), _userInfo(uri._userInfo), _host(uri._host), _port(uri._port), _path(uri._path),
      _query(uri._query), _fragment(uri._fragment) {}


inline URI::URI(URI &&uri) noexcept
    : _scheme(std::move(uri._scheme)), _userInfo(std::move(uri._userInfo)), _host(std::move(uri._host)),
      _port(uri._port), _path(std::move(uri._path)), _query(std::move(uri._query)),
      _fragment(std::move(uri._fragment)) {}


inline URI::URI(const URI &baseURI, std::string_view relativeURI)
    : _scheme(baseURI._scheme), _userInfo(baseURI._userInfo), _host(baseURI._host), _port(baseURI._port),
      _path(baseURI._path), _query(baseURI._query), _fragment(baseURI._fragment) {
    resolve(relativeURI);
}


inline URI::URI(const std::filesystem::path &path) : _scheme("file"), _port(0) {
    _path = std::filesystem::absolute(path).generic_string();
}


inline URI::~URI() = default;


inline URI &URI::operator=(const URI &uri) {
    if (&uri != this) {
        _scheme   = uri._scheme;
        _userInfo = uri._userInfo;
        _host     = uri._host;
        _port     = uri._port;
        _path     = uri._path;
        _query    = uri._query;
        _fragment = uri._fragment;
    }
    return *this;
}


inline URI &URI::operator=(URI &&uri) noexcept {
    _scheme   = std::move(uri._scheme);
    _userInfo = std::move(uri._userInfo);
    _host     = std::move(uri._host);
    _port     = uri._port;
    _path     = std::move(uri._path);
    _query    = std::move(uri._query);
    _fragment = std::move(uri._fragment);
    return *this;
}


inline URI &URI::operator=(std::string_view uri) {
    clear();
    parse(uri);
    return *this;
}


inline URI &URI::operator=(const char *uri) {
    clear();
    parse(uri);
    return *this;
}


inline void URI::swap(URI &uri) noexcept {
    std::swap(_scheme, uri._scheme);
    std::swap(_userInfo, uri._userInfo);
    std::swap(_host, uri._host);
    std::swap(_port, uri._port);
    std::swap(_path, uri._path);
    std::swap(_query, uri._query);
    std::swap(_fragment, uri._fragment);
}


inline void URI::clear() {
    _scheme.clear();
    _userInfo.clear();
    _host.clear();
    _port = 0;
    _path.clear();
    _query.clear();
    _fragment.clear();
}


inline std::string URI::toString() const {
    std::string uri;
    if (isRelative()) { encode(_path, RESERVED_PATH, uri); }
    else {
        uri               = _scheme;
        uri              += ':';
        std::string auth  = getAuthority();
        if (! auth.empty() || _scheme == "file") {
            uri.append("//");
            uri.append(auth);
        }
        if (! _path.empty()) {
            if (! auth.empty() && _path[0] != '/') { uri += '/'; }
            encode(_path, RESERVED_PATH, uri);
        }
        else if (! _query.empty() || ! _fragment.empty()) { uri += '/'; }
    }
    if (! _query.empty()) {
        uri += '?';
        uri.append(_query);
    }
    if (! _fragment.empty()) {
        uri += '#';
        uri.append(_fragment);
    }
    return uri;
}


inline void URI::setScheme(std::string_view scheme) {
    _scheme = scheme;
    detail::toLowerInPlace(_scheme);
}


inline void URI::setUserInfo(std::string_view userInfo) {
    _userInfo.clear();
    decode(userInfo, _userInfo);
}


inline void URI::setHost(std::string_view host) {
    _host = host;
}


inline unsigned short URI::getPort() const {
    if (_port == 0) { return getWellKnownPort(); }
    return _port;
}


inline void URI::setPort(unsigned short port) {
    _port = port;
}


inline std::string URI::getAuthority() const {
    std::string auth;
    if (! _userInfo.empty()) {
        auth.append(_userInfo);
        auth += '@';
    }
    if (_host.find(':') != std::string::npos) {
        auth += '[';
        auth += _host;
        auth += ']';
    }
    else { auth.append(_host); }
    if (_port && ! isWellKnownPort()) {
        auth += ':';
        auth += std::to_string(_port);
    }
    return auth;
}


inline void URI::setAuthority(std::string_view authority) {
    _userInfo.clear();
    _host.clear();
    _port = 0;
    parseAuthority(authority);
}


inline void URI::setPath(std::string_view path) {
    _path.clear();
    decode(path, _path);
    detail::normalizePathSeparatorsInPlace(_path);
}


inline void URI::setRawQuery(std::string_view query) {
    _query = query;
}


inline void URI::setQuery(std::string_view query) {
    _query.clear();
    encode(query, RESERVED_QUERY, _query);
}


inline void URI::addQueryParameter(std::string_view param, std::string_view val) {
    if (! _query.empty()) { _query += '&'; }
    encode(param, RESERVED_QUERY_PARAM, _query);
    _query += '=';
    encode(val, RESERVED_QUERY_PARAM, _query);
}


inline std::string URI::getQuery() const {
    std::string query;
    decode(_query, query);
    return query;
}


inline URI::QueryParameters URI::getQueryParameters(bool plusIsSpace) const {
    QueryParameters             result;
    std::string::const_iterator it(_query.begin());
    std::string::const_iterator end(_query.end());
    while (it != end) {
        std::string name;
        std::string value;
        while (it != end && *it != '=' && *it != '&') {
            if (plusIsSpace && (*it == '+')) { name += ' '; }
            else { name += *it; }
            ++it;
        }
        if (it != end && *it == '=') {
            ++it;
            while (it != end && *it != '&') {
                if (plusIsSpace && (*it == '+')) { value += ' '; }
                else { value += *it; }
                ++it;
            }
        }
        std::string decodedName;
        std::string decodedValue;
        URI::decode(name, decodedName);
        URI::decode(value, decodedValue);
        result.push_back(std::make_pair(decodedName, decodedValue));
        if (it != end && *it == '&') { ++it; }
    }
    return result;
}


inline void URI::setQueryParameters(const QueryParameters &params) {
    _query.clear();
    for (const auto &p : params) { addQueryParameter(p.first, p.second); }
}


inline std::string URI::getFragment() const {
    std::string fragment;
    decode(_fragment, fragment);
    return fragment;
}


inline void URI::setFragment(std::string_view fragment) {
    _fragment.clear();
    encode(fragment, RESERVED_FRAGMENT, _fragment);
}


inline void URI::setRawFragment(std::string_view fragment) {
    _fragment = fragment;
}


inline void URI::setPathEtc(std::string_view pathEtc) {
    _path.clear();
    _query.clear();
    _fragment.clear();
    parsePathEtc(pathEtc);
}


inline std::string URI::getPathEtc() const {
    std::string pathEtc;
    encode(_path, RESERVED_PATH, pathEtc);
    if (! _query.empty()) {
        pathEtc += '?';
        pathEtc += _query;
    }
    if (! _fragment.empty()) {
        pathEtc += '#';
        pathEtc += _fragment;
    }
    return pathEtc;
}


inline std::string URI::getPathAndQuery() const {
    std::string pathAndQuery;
    encode(_path, RESERVED_PATH, pathAndQuery);
    if (! _query.empty()) {
        pathAndQuery += '?';
        pathAndQuery += _query;
    }
    return pathAndQuery;
}


inline void URI::resolve(std::string_view relativeURI) {
    URI parsedURI(relativeURI);
    resolve(parsedURI);
}


inline void URI::resolve(const URI &relativeURI) {
    if (! relativeURI._scheme.empty()) {
        _scheme   = relativeURI._scheme;
        _userInfo = relativeURI._userInfo;
        _host     = relativeURI._host;
        _port     = relativeURI._port;
        _path     = relativeURI._path;
        _query    = relativeURI._query;
        removeDotSegments();
    }
    else {
        if (! relativeURI._host.empty()) {
            _userInfo = relativeURI._userInfo;
            _host     = relativeURI._host;
            _port     = relativeURI._port;
            _path     = relativeURI._path;
            _query    = relativeURI._query;
            removeDotSegments();
        }
        else {
            if (relativeURI._path.empty()) {
                if (! relativeURI._query.empty()) { _query = relativeURI._query; }
            }
            else {
                if (relativeURI._path[0] == '/') {
                    _path = relativeURI._path;
                    removeDotSegments();
                }
                else { mergePath(relativeURI._path); }
                _query = relativeURI._query;
            }
        }
    }
    _fragment = relativeURI._fragment;
}


inline bool URI::isRelative() const {
    return _scheme.empty();
}


inline bool URI::empty() const {
    return _scheme.empty() && _host.empty() && _path.empty() && _query.empty() && _fragment.empty();
}


inline bool URI::operator==(const URI &uri) const {
    return equals(uri);
}


inline bool URI::operator==(std::string_view uri) const {
    URI parsedURI(uri);
    return equals(parsedURI);
}


inline bool URI::operator!=(const URI &uri) const {
    return ! equals(uri);
}


inline bool URI::operator!=(std::string_view uri) const {
    URI parsedURI(uri);
    return ! equals(parsedURI);
}


inline bool URI::equals(const URI &uri) const {
    return _scheme == uri._scheme && _userInfo == uri._userInfo && _host == uri._host && getPort() == uri.getPort() &&
           _path == uri._path && _query == uri._query && _fragment == uri._fragment;
}


inline void URI::normalize() {
    removeDotSegments(! isRelative());
}


inline void URI::removeDotSegments(bool removeLeading) {
    if (_path.empty()) { return; }

    bool                     leadingSlash  = *(_path.begin()) == '/';
    bool                     trailingSlash = *(_path.rbegin()) == '/';
    std::vector<std::string> segments;
    std::vector<std::string> normalizedSegments;
    getPathSegments(segments);
    for (const auto &s : segments) {
        if (s == "..") {
            if (! normalizedSegments.empty()) {
                if (normalizedSegments.back() == "..") { normalizedSegments.push_back(s); }
                else { normalizedSegments.pop_back(); }
            }
            else if (! removeLeading) { normalizedSegments.push_back(s); }
        }
        else if (s != ".") { normalizedSegments.push_back(s); }
    }
    buildPath(normalizedSegments, leadingSlash, trailingSlash);
}


inline void URI::getPathSegments(std::vector<std::string> &segments) const {
    getPathSegments(_path, segments);
}


inline void URI::getPathSegments(std::string_view path, std::vector<std::string> &segments) {
    std::size_t start = 0;
    while (start < path.size()) {
        const std::size_t slashPos = path.find('/', start);
        const std::size_t endPos   = (slashPos == std::string_view::npos) ? path.size() : slashPos;

        if (endPos > start) { segments.emplace_back(path.substr(start, endPos - start)); }

        if (slashPos == std::string_view::npos) { break; }
        start = slashPos + 1;
    }
}


inline void URI::encode(std::string_view str, std::string_view reserved, std::string &encodedStr) {
    for (unsigned char c : str) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            encodedStr += static_cast<char>(c);
        }
        else if (c <= 0x20 || c >= 0x7F || ILLEGAL.find(static_cast<char>(c)) != std::string::npos ||
                 reserved.find(static_cast<char>(c)) != std::string::npos) {
            encodedStr += '%';
            detail::appendHex2(encodedStr, c);
        }
        else { encodedStr += static_cast<char>(c); }
    }
}


inline void URI::decode(std::string_view str, std::string &decodedStr, bool plusAsSpace) {
    bool                             inQuery = false;
    std::string_view::const_iterator it      = str.begin();
    std::string_view::const_iterator end     = str.end();
    while (it != end) {
        char c = *it++;
        if (c == '?') { inQuery = true; }
        if (inQuery && plusAsSpace && c == '+') { c = ' '; }
        else if (c == '%') {
            if (it == end) { throw URISyntaxException("URI encoding: no hex digit following percent sign", str); }
            char hi = *it++;
            if (it == end) { throw URISyntaxException("URI encoding: two hex digits must follow percent sign", str); }
            char lo = *it++;
            if (hi >= '0' && hi <= '9') { c = static_cast<char>(hi - '0'); }
            else if (hi >= 'A' && hi <= 'F') { c = static_cast<char>(hi - 'A' + 10); }
            else if (hi >= 'a' && hi <= 'f') { c = static_cast<char>(hi - 'a' + 10); }
            else { throw URISyntaxException("URI encoding: not a hex digit"); }
            c = static_cast<char>(c * 16);
            if (lo >= '0' && lo <= '9') { c = static_cast<char>(c + lo - '0'); }
            else if (lo >= 'A' && lo <= 'F') { c = static_cast<char>(c + lo - 'A' + 10); }
            else if (lo >= 'a' && lo <= 'f') { c = static_cast<char>(c + lo - 'a' + 10); }
            else { throw URISyntaxException("URI encoding: not a hex digit"); }
        }
        decodedStr += c;
    }
}


inline bool URI::isWellKnownPort() const {
    return _port == getWellKnownPort();
}


inline unsigned short URI::getWellKnownPort() const {
    if (_scheme == "ftp") { return 21; }
    else if (_scheme == "ssh") { return 22; }
    else if (_scheme == "telnet") { return 23; }
    else if (_scheme == "smtp") { return 25; }
    else if (_scheme == "dns") { return 53; }
    else if (_scheme == "http" || _scheme == "ws") { return 80; }
    else if (_scheme == "nntp") { return 119; }
    else if (_scheme == "imap") { return 143; }
    else if (_scheme == "ldap") { return 389; }
    else if (_scheme == "https" || _scheme == "wss") { return 443; }
    else if (_scheme == "smtps") { return 465; }
    else if (_scheme == "rtsp") { return 554; }
    else if (_scheme == "ldaps") { return 636; }
    else if (_scheme == "dnss") { return 853; }
    else if (_scheme == "imaps") { return 993; }
    else if (_scheme == "sip") { return 5060; }
    else if (_scheme == "sips") { return 5061; }
    else if (_scheme == "xmpp") { return 5222; }
    return 0;
}


inline void URI::parse(std::string_view uri) {
    parse(uri, false);
}


inline void URI::parse(std::string_view uri, bool heuristics) {
    if (uri.empty()) { return; }

    if (detail::isWindowsPathLike(uri)) {
        parsePathEtc(uri);
        return;
    }

    if (heuristics) {
        if (uri.size() >= 2 && uri[0] == '/' && uri[1] == '/') {
            setScheme("https");
            std::size_t       pos       = 2;
            const std::size_t authStart = pos;
            while (pos < uri.size() && uri[pos] != '/' && uri[pos] != '?' && uri[pos] != '#') { ++pos; }
            parseAuthority(uri.substr(authStart, pos - authStart));
            if (pos < uri.size()) { parsePathEtc(uri.substr(pos)); }
            else { _path = "/"; }
            return;
        }

        if (uri[0] != '/' && uri[0] != '\\' && uri[0] != '.' && uri[0] != '?' && uri[0] != '#') {
            const std::size_t tokenEnd  = uri.find_first_of("/?#");
            const std::size_t schemeSep = uri.find(':');
            if (schemeSep == std::string_view::npos || (tokenEnd != std::string_view::npos && schemeSep > tokenEnd)) {
                const std::string_view authority = (tokenEnd == std::string_view::npos) ? uri : uri.substr(0, tokenEnd);
                if (detail::isLikelyBrowserAuthority(authority)) {
                    setScheme("https");
                    parseAuthority(authority);
                    if (tokenEnd == std::string_view::npos) { _path = "/"; }
                    else { parsePathEtc(uri.substr(tokenEnd)); }
                    return;
                }
            }
        }
    }

    if (uri[0] != '/' && uri[0] != '\\' && uri[0] != '.' && uri[0] != '?' && uri[0] != '#') {
        std::size_t pos = 0;
        while (pos < uri.size() && uri[pos] != ':' && uri[pos] != '?' && uri[pos] != '#' && uri[pos] != '/') { ++pos; }

        if (pos < uri.size() && uri[pos] == ':') {
            setScheme(uri.substr(0, pos));
            ++pos;
            if (pos == uri.size()) {
                throw URISyntaxException("URI scheme must be followed by authority or path", uri);
            }

            if (uri[pos] == '/') {
                ++pos;
                if (pos < uri.size() && uri[pos] == '/') {
                    ++pos;
                    const std::size_t authStart = pos;
                    while (pos < uri.size() && uri[pos] != '/' && uri[pos] != '?' && uri[pos] != '#') { ++pos; }
                    parseAuthority(uri.substr(authStart, pos - authStart));
                }
                else { --pos; }
            }

            parsePathEtc(uri.substr(pos));
            return;
        }
    }

    parsePathEtc(uri);
}


inline void URI::parseAuthority(std::string_view authority) {
    const std::size_t atPos = authority.rfind('@');
    if (atPos == std::string_view::npos) {
        _userInfo.clear();
        parseHostAndPort(authority);
        return;
    }

    _userInfo.assign(authority.substr(0, atPos));
    parseHostAndPort(authority.substr(atPos + 1));
}


inline void URI::parseHostAndPort(std::string_view hostAndPort) {
    if (hostAndPort.empty()) { return; }

    std::string_view hostView;
    std::size_t      pos = 0;
    if (hostAndPort[pos] == '[') {
        const std::size_t endBracketPos = hostAndPort.find(']', 1);
        if (endBracketPos == std::string_view::npos) { throw URISyntaxException("unterminated IPv6 address"); }
        hostView = hostAndPort.substr(1, endBracketPos - 1);
        pos      = endBracketPos + 1;
    }
    else {
        const std::size_t colonPos = hostAndPort.find(':');
        if (colonPos == std::string_view::npos) {
            hostView = hostAndPort;
            pos      = hostAndPort.size();
        }
        else {
            hostView = hostAndPort.substr(0, colonPos);
            pos      = colonPos;
        }
    }

    if (pos < hostAndPort.size() && hostAndPort[pos] == ':') {
        ++pos;
        const std::string_view port = hostAndPort.substr(pos);
        if (! port.empty()) {
            int nport = 0;
            if (detail::tryParseInt(port, nport) && nport > 0 && nport < 65536) {
                _port = static_cast<unsigned short>(nport);
            }
            else { throw URISyntaxException("bad or invalid port number", port); }
        }
        else { _port = 0; }
    }
    else { _port = 0; }
    _host.assign(hostView);
    if (_host.size() && _host[0] != '%') { detail::toLowerInPlace(_host); }
}


inline void URI::parsePath(std::string_view path) {
    _path.clear();
    decode(path, _path);
    detail::normalizePathSeparatorsInPlace(_path);
}


inline void URI::parsePathEtc(std::string_view pathEtc) {
    if (pathEtc.empty()) { return; }

    std::size_t pos = 0;
    if (pathEtc[pos] != '?' && pathEtc[pos] != '#') {
        const std::size_t pathEnd = pathEtc.find_first_of("?#", pos);
        if (pathEnd == std::string_view::npos) {
            parsePath(pathEtc);
            return;
        }
        parsePath(pathEtc.substr(0, pathEnd));
        pos = pathEnd;
    }

    if (pos < pathEtc.size() && pathEtc[pos] == '?') {
        ++pos;
        const std::size_t queryEnd = pathEtc.find('#', pos);
        if (queryEnd == std::string_view::npos) {
            parseQuery(pathEtc.substr(pos));
            return;
        }
        parseQuery(pathEtc.substr(pos, queryEnd - pos));
        pos = queryEnd;
    }

    if (pos < pathEtc.size() && pathEtc[pos] == '#') {
        ++pos;
        parseFragment(pathEtc.substr(pos));
    }
}


inline void URI::parseQuery(std::string_view query) {
    _query.assign(query);
}


inline void URI::parseFragment(std::string_view fragment) {
    _fragment.assign(fragment);
}


inline void URI::mergePath(std::string_view path) {
    std::vector<std::string> segments;
    std::vector<std::string> normalizedSegments;
    bool                     addLeadingSlash = false;
    if (! _path.empty()) {
        getPathSegments(segments);
        bool endsWithSlash = *(_path.rbegin()) == '/';
        if (! endsWithSlash && ! segments.empty()) { segments.pop_back(); }
        addLeadingSlash = _path[0] == '/';
    }
    getPathSegments(path, segments);
    addLeadingSlash       = addLeadingSlash || (! path.empty() && path[0] == '/');
    bool hasTrailingSlash = (! path.empty() && *(path.rbegin()) == '/');
    bool addTrailingSlash = false;
    for (const auto &s : segments) {
        if (s == "..") {
            addTrailingSlash = true;
            if (! normalizedSegments.empty()) { normalizedSegments.pop_back(); }
        }
        else if (s != ".") {
            addTrailingSlash = false;
            normalizedSegments.push_back(s);
        }
        else { addTrailingSlash = true; }
    }
    buildPath(normalizedSegments, addLeadingSlash, hasTrailingSlash || addTrailingSlash);
}


inline void URI::buildPath(const std::vector<std::string> &segments, bool leadingSlash, bool trailingSlash) {
    _path.clear();
    bool first = true;
    for (const auto &s : segments) {
        if (first) {
            first = false;
            if (leadingSlash) { _path += '/'; }
            else if (_scheme.empty() && s.find(':') != std::string::npos) { _path.append("./"); }
        }
        else { _path += '/'; }
        _path.append(s);
    }
    if (trailingSlash) { _path += '/'; }
}


inline std::pair<bool, std::string> parse_uri_string(std::string_view uri) {
    try {
        URI parsed(uri);
        return {true, parsed.getScheme()};
    }
    catch (const URISyntaxException &) {
        return {false, {}};
    }
}
} // namespace incom::standard::web
