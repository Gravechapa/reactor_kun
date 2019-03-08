#pragma once
#include <string>
#include "filesystem"
#include <map>
#include <mutex>

namespace fs = std::filesystem;

enum class FileStatus: short
{
    READY,
    NOTREADY,
    ERROR
};

class FileManager
{
    using safe_counter = std::pair<int64_t, std::mutex>;
    using shared_counter = std::shared_ptr<safe_counter>;

    enum FileState: short
    {
        NOTLOADED = -1,
        DELETED = -2
    };

public:
    FileManager(std::string_view path);
    static FileManager& getInstance();

    FileStatus getFile(std::string_view link, std::string_view fileName);
    void releaseFile(std::string_view fileName);
    void collectGarbage();

    const fs::path& getDir() const;

private:
    fs::path _folderPath;
    std::map<std::string, shared_counter> _index;
    std::mutex _lock;
};
