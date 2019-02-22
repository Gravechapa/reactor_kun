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


int main()
{
    ::signal(SIGINT, [](int) {
        printf("SIGINT got\n");
        exit(0);
    });

   ::signal(SIGSEGV, &failStackTrace);
   ::signal(SIGABRT, &failStackTrace);

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

    return 0;
}
