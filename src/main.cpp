#define BOOST_STACKTRACE_LINK
#include <iostream>
#include <tgbot/tgbot.h>
#include <curl/curl.h>
#include "headers/Config.hpp"
#include "ReactorKun.hpp"
#include <thread>
#include <boost/stacktrace.hpp>
#include <csignal>

static std::atomic_bool run{true};

void failStackTrace(int signum)
{
    std::signal(signum, SIG_DFL);
    std::cerr << boost::stacktrace::stacktrace();
    std::raise(SIGABRT);
}

int main()
{
    std::signal(SIGINT, [](int)
        {
            printf("SIGINT got\n");
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
            std::cout << "Long poll started" << std::endl;
            try
            {
                longPoll.start();
            }
            catch (std::exception& e)
            {
                std:: cout << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    catch (std::exception& e)
    {
        std:: cout << e.what() << std::endl;
    }

    curl_global_cleanup();
}
