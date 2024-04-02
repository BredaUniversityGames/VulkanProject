
#include "Application.h"

#include <iostream>
#include <vector>



int main() 
{
    //sets default 600-900, window
    VulkanProject::AppConfig config;
    config.name = "VulkanProject";

    try
    {
        VulkanProject::Application app{ config };
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }



    

    return 0;
}