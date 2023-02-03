#include "FileManager.hpp"
#include "Parser.hpp"
#include "SpinGuard.hpp"

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
    SpinGuard lockGuard(_lock);
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
        if (!Parser::getContent(link, _folderPath.string() + "/" + fileName.data()))
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
    SpinGuard lockGuard(_lock);
    auto data = _index.at(fileName.data());
    lockGuard.unlock();

    std::lock_guard counterLock(data->second);
    data->first -= 1;
}

void FileManager::collectGarbage()
{
    SpinGuard lockGuard(_lock);
    for (auto it = _index.begin(); it != _index.end();)
    {
        std::unique_lock counterLock(it->second->second, std::try_to_lock);
        if (counterLock.owns_lock())
        {
            switch (it->second->first)
            {
            case 0:
                fs::remove(_folderPath.string() + "/" + it->first);
                [[fallthrough]];
            case FileState::NOTLOADED:
                it->second->first = FileState::DELETED;
                counterLock.unlock();
                counterLock.release();
                it = _index.erase(it);
                continue;
            }
        }
        ++it;
    }
}

const fs::path &FileManager::getDir() const
{
    return _folderPath;
}

FileManager &FileManager::getInstance()
{
    static FileManager fileManager("data/");
    return fileManager;
}
