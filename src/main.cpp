#include <iostream>
#include <tgbot/tgbot.h>
#include <curl/curl.h>
#include "headers/Config.hpp"
#include "ReactorKun.hpp"
#include <thread>

int main()
{
    signal(SIGINT, [](int) {
        printf("SIGINT got\n");
        exit(0);
    });

/*    try
        {*/
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
        /*}
    catch (std::exception& e)
        {
            std:: cout << e.what() << std::endl;
        }*/

    return 0;
}
