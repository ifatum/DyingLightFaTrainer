#pragma once
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include "logic.h"
#include "version.h"

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#ifndef FATRAINER_OFFLINE
#include "net_win.h"
#endif
#else
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace fs = std::filesystem;

namespace platform {

const std::string FOLDER_PICKER_MISSING = "\x01";

#ifdef _WIN32
const bool LINUX = false;
inline HWND window = nullptr;
#ifndef FATRAINER_OFFLINE
namespace net = ::net;
#endif

inline std::string utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], n, nullptr, nullptr);
    return out;
}

inline std::string registry_string(HKEY root, const wchar_t* key, const wchar_t* name) {
    wchar_t value[MAX_PATH] = {};
    DWORD size = sizeof value;
    if (RegGetValueW(root, key, name, RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS) return {};
    return utf8(value);
}

inline std::vector<fs::path> steam_roots() {
    std::vector<fs::path> roots;
    for (const std::string& r : {registry_string(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"),
                                 registry_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath"),
                                 registry_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath")})
        if (!r.empty()) roots.push_back(fs::u8path(r));
    roots.push_back("C:\\Program Files (x86)\\Steam");
    return roots;
}

inline void pick_folder(const std::function<void(std::string)>& done) {
    std::string picked;
    IFileOpenDialog* dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        FILEOPENDIALOGOPTIONS options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        dialog->SetTitle(L"Choose the Dying Light folder (the one with DyingLightGame.exe)");
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->Show(window)) && SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) picked = utf8(path), CoTaskMemFree(path);
            item->Release();
        }
        dialog->Release();
    }
    done(picked);
}

#else
const bool LINUX = true;

const int NOT_STARTED = -1, NOT_FOUND = 127;

inline int run(const std::vector<std::string>& args, const std::function<bool(const char*, size_t)>& out) {
    int pipe_fd[2];
    if (pipe(pipe_fd)) return NOT_STARTED;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipe_fd[1], 1);
    posix_spawn_file_actions_addclose(&actions, pipe_fd[0]);
    posix_spawn_file_actions_addclose(&actions, pipe_fd[1]);
    posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
    std::vector<char*> argv;
    for (const std::string& a : args) argv.push_back((char*)a.c_str());
    argv.push_back(nullptr);
    pid_t pid = 0;
    int started = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipe_fd[1]);
    if (started) {
        close(pipe_fd[0]);
        return NOT_STARTED;
    }
    char buffer[16384];
    bool wanted = true;
    for (ssize_t n; (n = read(pipe_fd[0], buffer, sizeof buffer)) > 0;)
        if (wanted && !out(buffer, (size_t)n)) wanted = false, kill(pid, SIGTERM);
    close(pipe_fd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : NOT_STARTED;
}

inline std::string home() {
    const char* h = getenv("HOME");
    return h ? h : "";
}

inline std::vector<fs::path> steam_roots() {
    std::vector<fs::path> roots;
    if (const char* data = getenv("XDG_DATA_HOME")) roots.push_back(fs::path(data) / "Steam");
    for (const char* relative : {".steam/steam", ".steam/root", ".local/share/Steam", ".var/app/com.valvesoftware.Steam/.local/share/Steam",
                                 "snap/steam/common/.local/share/Steam"})
        roots.push_back(fs::path(home()) / relative);
    return roots;
}

inline void pick_folder(const std::function<void(std::string)>& done) {
    std::thread([done] {
        const char* title = "Choose the Dying Light folder (the one with DyingLightGame.exe)";
        const std::vector<std::vector<std::string>> pickers = {
            {"zenity", "--file-selection", "--directory", std::string("--title=") + title},
            {"kdialog", "--getexistingdirectory", home(), "--title", title},
            {"qarma", "--file-selection", "--directory", std::string("--title=") + title}};
        for (const auto& picker : pickers) {
            std::string out;
            int status = run(picker, [&](const char* d, size_t n) { out.append(d, n); return true; });
            if (status == NOT_STARTED || status == NOT_FOUND) continue;
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
            return done(status == 0 ? out : std::string());
        }
        done(FOLDER_PICKER_MISSING);
    }).detach();
}

#ifndef FATRAINER_OFFLINE
namespace net {
using Sink = std::function<bool(const char* data, size_t n, unsigned long long total)>;

inline bool get(const std::string& url, const Sink& sink, std::string& error) {
    const std::vector<std::vector<std::string>> tools = {
        {"curl", "--fail", "--location", "--silent", "--proto", "=https", "--max-time", "300", url},
        {"wget", "--quiet", "--https-only", "--timeout=30", "--output-document=-", url}};
    for (const auto& tool : tools) {
        int status = run(tool, [&](const char* d, size_t n) { return sink(d, n, 0); });
        if (status == 0) return true;
        if (status != NOT_STARTED && status != NOT_FOUND) return error = "could not download from GitHub (" + tool[0] + " error " + std::to_string(status) + ")", false;
    }
    return error = "downloading needs curl or wget, install one of them", false;
}

inline bool get_text(const std::string& url, std::string& out, std::string& error) {
    const size_t LIMIT = 1 << 20;
    out.clear();
    return get(url, [&](const char* d, size_t n, unsigned long long) { out.append(d, n); return out.size() < LIMIT; }, error);
}
}
#endif
#endif

const char* GAME_EXE = "DyingLightGame.exe";
const char* DLL_NAME = "xinput1_3.dll";
const char* BACKUP_NAME = "xinput1_3.dll.before-fatrainer";
const char* DOWNLOAD_NAME = "xinput1_3.dll.fatrainer-download";
const char* APP_ID = "239140";

inline std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), {});
}

inline bool is_game_dir(const fs::path& dir) {
    std::error_code ec;
    return !dir.empty() && fs::is_regular_file(dir / GAME_EXE, ec);
}

inline fs::path find_game() {
    for (const fs::path& root : steam_roots()) {
        std::vector<fs::path> libraries = {root};
        for (const std::string& lib : logic::vdf_values(read_file(root / "steamapps" / "libraryfolders.vdf"), "path"))
            libraries.push_back(fs::u8path(lib));
        for (const fs::path& lib : libraries) {
            auto dirs = logic::vdf_values(read_file(lib / "steamapps" / ("appmanifest_" + std::string(APP_ID) + ".acf")), "installdir");
            fs::path game = lib / "steamapps" / "common" / fs::u8path(dirs.empty() ? "Dying Light" : dirs[0]);
            if (is_game_dir(game)) return game;
        }
    }
    return {};
}

}
