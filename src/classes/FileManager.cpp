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
        data->first = 0;
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
        case -1:
            return FileStatus::NOTREADY;
        case 0:
            if (!ReactorParser::getContent(link, _folderPath.string() + "/" + fileName.data()))
            {
                return FileStatus::ERROR;
            }
            [[fallthrough]];
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
        if(it->second->first == 0)
        {
            fs::remove(_folderPath.string() + "/" + it->first);
            it->second->first = -1;
            it = _index.erase(it);
        }
        else
        {
            ++it;
        }
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
