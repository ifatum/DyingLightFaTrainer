#pragma once
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>

#define FATRAINER_VERSION "2.0"
#ifdef FATRAINER_OFFLINE
#define FATRAINER_EDITION "Nexus Version"
#define FATRAINER_EDITION_CAPS "NEXUS VERSION"
#else
#define FATRAINER_EDITION "Fatum Version"
#define FATRAINER_EDITION_CAPS "FATUM VERSION"
#endif
inline const char VERSION_TAG[] = "FaTrainer-version:" FATRAINER_VERSION;
inline const char* VERSION = VERSION_TAG + sizeof "FaTrainer-version:" - 1;
inline const char* RELEASE_DOWNLOADS = "https://github.com/ifatum/DyingLightFaTrainer/releases/latest/download/";

inline std::vector<long> version_parts(const std::string& v) {
    std::vector<long> parts;
    for (const char* p = v.c_str(); *p;) {
        char* end;
        parts.push_back(strtol(p, &end, 10));
        if (end == p || (*end && *end != '.')) return {};
        p = *end ? end + 1 : end;
    }
    return parts;
}

inline bool newer_version(const std::string& candidate, const std::string& current) {
    std::vector<long> a = version_parts(candidate), b = version_parts(current);
    if (a.empty() || b.empty()) return false;
    a.resize(std::max(a.size(), b.size())), b.resize(a.size());
    return a > b;
}

struct ReleaseInfo { std::string version, sha256; unsigned long long size = 0; };

inline ReleaseInfo parse_release_info(const std::string& text) {
    ReleaseInfo r;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? text.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.rfind("version ", 0) == 0) r.version = line.substr(8);
        else if (line.rfind("sha256 ", 0) == 0) r.sha256 = line.substr(7);
        else if (line.rfind("size ", 0) == 0) r.size = strtoull(line.c_str() + 5, nullptr, 10);
    }
    return r;
}
