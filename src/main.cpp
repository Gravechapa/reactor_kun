#define BOOST_STACKTRACE_LINK
#include <tgbot/tgbot.h>
#include <curl/curl.h>
#include "headers/Config.hpp"
#include "ReactorKun.hpp"
#include <thread>
#include <boost/stacktrace.hpp>
#include <csignal>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

static std::atomic_bool run{true};

void failStackTrace(int signum)
{
    std::signal(signum, SIG_DFL);
    PLOGF << boost::stacktrace::stacktrace();
    std::raise(SIGABRT);
}

int main()
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto logName = std::string(std::ctime(&time)) + ".txt";
    plog::init(plog::verbose, logName.c_str(), 5 * 1024 * 1024, 2);
    plog::ColorConsoleAppender<plog::TxtFormatter> colorConsole;
    plog::init<1>(plog::info, &colorConsole);
    plog::get()->addAppender(plog::get<1>());
    PLOGV << "Log initiated";

    std::signal(SIGINT, [](int)
        {
            PLOGI << "SIGINT got";
            run.store(false);
        });

    std::signal(SIGSEGV, &failStackTrace);
    std::signal(SIGABRT, &failStackTrace);

    curl_global_init(CURL_GLOBAL_ALL);
    try
    {
        TgBot::CurlHttpClient curlClient;
        ReactorKun reactorKun(Config ("configs/config.json"), curlClient);
        TgBot::TgLongPoll longPoll(reactorKun);
        while (run.load())
        {
            PLOGI << "Long poll started";
            try
            {
                longPoll.start();
            }
            catch (std::exception& e)
            {
                PLOGE << e.what();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    catch (std::exception& e)
    {
        PLOGF << e.what();
    }

    curl_global_cleanup();
}
