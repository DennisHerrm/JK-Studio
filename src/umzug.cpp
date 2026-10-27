// umzug.cpp - siehe umzug.h
#include "bhed/umzug.h"

#include <cctype>
#include <filesystem>
#include <string>
#include <system_error>

namespace bhed::umzug {
namespace {

namespace fs = std::filesystem;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool endetAuf(const std::string& s, const std::string& e) { return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0; }

std::string utf8(const fs::path& p) {
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}

}  // namespace

bool auslassen(const std::string& relativ) {
    const std::string r = klein(relativ);
    const std::string erster = r.substr(0, r.find('/'));
    // ganze Ordner, die neu entstehen (Zwischenspeicher, Arbeitsordner, Daten einer Fassung)
    for (const char* o : {"shader", "data", "karte_bau", "karte_bsp", "karte_zusammen"}) {
        if (erster == o) { return true; }
    }
    const std::string name = r.substr(r.rfind('/') == std::string::npos ? 0 : r.rfind('/') + 1);
    if (name.rfind("karte_modellindex_", 0) == 0) { return true; }
    return endetAuf(name, ".log") || endetAuf(name, ".dmp");
}

Ergebnis uebernehmen(const fs::path& alt, const fs::path& neu) {
    Ergebnis e;
    std::error_code ec;
    if (fs::exists(neu, ec) || !fs::is_directory(alt, ec)) { return e; }
    fs::path zwischen = neu;
    zwischen += ".umzug";
    fs::remove_all(zwischen, ec);
    fs::create_directories(zwischen, ec);
    if (ec) {
        e.fehler = "cannot create " + utf8(zwischen) + ": " + ec.message();
        return e;
    }
    for (fs::recursive_directory_iterator it(alt, fs::directory_options::skip_permission_denied, ec), ende; it != ende; it.increment(ec)) {
        if (ec) {
            e.fehler = ec.message();
            break;
        }
        const fs::path rel = fs::relative(it->path(), alt, ec);
        const std::string r = utf8(rel);
        if (auslassen(r)) {
            ++e.ausgelassen;
            if (it->is_directory(ec)) { it.disable_recursion_pending(); }
            continue;
        }
        const fs::path ziel = zwischen / rel;
        if (it->is_directory(ec)) {
            fs::create_directories(ziel, ec);
        } else if (it->is_regular_file(ec)) {
            fs::create_directories(ziel.parent_path(), ec);
            fs::copy_file(it->path(), ziel, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                e.fehler = "cannot copy " + r + ": " + ec.message();
                break;
            }
            ++e.dateien;
        }
    }
    if (!e.fehler.empty()) {
        fs::remove_all(zwischen, ec);
        return e;
    }
    fs::rename(zwischen, neu, ec);
    if (ec) {
        e.fehler = "cannot rename " + utf8(zwischen) + ": " + ec.message();
        fs::remove_all(zwischen, ec);
        return e;
    }
    e.uebernommen = true;
    return e;
}

}  // namespace bhed::umzug
