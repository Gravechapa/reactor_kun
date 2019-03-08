#include "FileManager.hpp"
#include "ReactorParser.hpp"

FileManager::FileManager(std::string_view path)
{
    if (path.back() == '/')
    {
        path = path.substr(0, path.size() - 1);
    }
    _folderPath = fs::path(path);
    if (!_folderPath.empty())
    {
        if (fs::exists(_folderPath))
        {
            fs::remove_all(_folderPath);
        }
        fs::create_directories(_folderPath);
    }
}

FileStatus FileManager::getFile(std::string_view link, std::string_view fileName)
{
    std::unique_lock lockGuard(_lock);
    auto it = _index.find(fileName.data());
    shared_counter data;
    if (it == _index.end())
    {
        data = shared_counter(new safe_counter);
        data->first = FileState::NOTLOADED;
        _index.insert(std::pair(fileName, data));
    }
    else
    {
        data = it->second;
    }
    lockGuard.unlock();

    std::lock_guard counterLock(data->second);
    switch (data->first)
    {
        case FileState::DELETED:
            return FileStatus::NOTREADY;
        case FileState::NOTLOADED:
            if (!ReactorParser::getContent(link, _folderPath.string() + "/" + fileName.data()))
            {
                fs::remove(_folderPath.string() + "/" + it->first);
                return FileStatus::ERROR;
            }
            data->first = 1;
            break;
        default:
            data->first += 1;
            break;
    }
    return FileStatus::READY;
}

void FileManager::releaseFile(std::string_view fileName)
{
    std::unique_lock lockGuard(_lock);
    auto data = _index.at(fileName.data());
    lockGuard.unlock();

    std::lock_guard counterLock(data->second);
    data->first -= 1;
}

void FileManager::collectGarbage()
{
    std::lock_guard lockGuard(_lock);
    for(auto it = _index.begin(); it != _index.end();)
    {
        std::lock_guard counterLock(it->second->second);
        switch (it->second->first)
        {
            case 0:
                fs::remove(_folderPath.string() + "/" + it->first);
                [[fallthrough]];
            case FileState::NOTLOADED:
                it->second->first = FileState::DELETED;
                it = _index.erase(it);
                continue;
        }
        ++it;
    }
}

const fs::path& FileManager::getDir() const
{
    return _folderPath;
}

FileManager& FileManager::getInstance()
{
    static FileManager fileManager("data/");
    return fileManager;
}
