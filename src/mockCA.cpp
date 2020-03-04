#include "../include/ClientSocket.h"
#include "../include/SocketException.h"
#include <iostream>

std::string valid_commands = "valid commands are:\n"
                              "\"add {k} {v} \"\n"
                              "\"rem {k} \"\n"
                              "\"unr {k} \"\n"
                              "\"rev {k} \"\n"
                              "\"exi\"\n"; 

int main ()
{
    std::cout << valid_commands;
    while(true)
    {
        try
        {
            ClientSocket client_socket("localhost", 50000);
            std::string input;
            std::string response;
            std::cout << "Enter a command: ";
            std::getline(std::cin, input);
            std::cout << "\nSending command: " << input << "\n";

            client_socket << input;
            client_socket >> response;

            std::cout << response << "\n";

        }

        catch (SocketException &e)
        {
            std::cout << "Exception was caught:" << e.description() << "\n";
            exit(-1);
        }

    }
}