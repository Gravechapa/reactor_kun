#define BOOST_STACKTRACE_LINK
#include "Config.hpp"
#include "ReactorKun.hpp"
#include <boost/stacktrace.hpp>
#include <csignal>
#include <curl/curl.h>
#include <filesystem>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <thread>

void failStackTrace(int signum)
{
    std::signal(signum, SIG_DFL);
    PLOGF << boost::stacktrace::stacktrace();
    std::raise(SIGABRT);
}

int main()
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto logName = "logs/" + std::string(std::ctime(&time)) + ".txt";
    std::filesystem::create_directory("logs");
    plog::init(plog::verbose, logName.c_str(), 5 * 1024 * 1024, 2);
    plog::ColorConsoleAppender<plog::TxtFormatter> colorConsole;
    plog::init<1>(plog::info, &colorConsole);
    plog::get()->addAppender(plog::get<1>());
    PLOGV << "Log initiated";

    std::signal(SIGSEGV, &failStackTrace);
    std::signal(SIGABRT, &failStackTrace);

    curl_global_init(CURL_GLOBAL_ALL);
    try
    {
        auto config = Config("configs/config.json");
        ReactorKun reactorKun(config);
        PLOGI << "Bot started.";
        reactorKun.run();
    }
    catch (std::exception &e)
    {
        PLOGF << e.what();
    }

    curl_global_cleanup();
}
