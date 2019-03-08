#define BOOST_STACKTRACE_LINK
#include <iostream>
#include <tgbot/tgbot.h>
#include <curl/curl.h>
#include "headers/Config.hpp"
#include "ReactorKun.hpp"
#include <thread>
#include <boost/stacktrace.hpp>
#include <csignal>

void failStackTrace(int signum)
{
    ::signal(signum, SIG_DFL);
    std::cerr << boost::stacktrace::stacktrace();
    ::raise(SIGABRT);
}

[[noreturn]]void cleanup()
{
    curl_global_cleanup();
    exit(0);
}

int main()
{
    ::signal(SIGINT, [](int) {
        printf("SIGINT got\n");
        cleanup();
    });

   ::signal(SIGSEGV, &failStackTrace);
   ::signal(SIGABRT, &failStackTrace);

    curl_global_init(CURL_GLOBAL_ALL);
    try
    {
        TgBot::CurlHttpClient curlClient;
        ReactorKun reactorKun(Config ("configs/config.json"), curlClient);
        TgBot::TgLongPoll longPoll(reactorKun);
        while (true)
        {
            std:: cout << "Long poll started" << std::endl;
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

    cleanup();
}
