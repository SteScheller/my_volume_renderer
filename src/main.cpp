#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "mvr.hpp"

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
int applyProgramOptions(
    int argc,
    char *argv[],
    mvr::Renderer &renderer,
    std::string &output);

//-----------------------------------------------------------------------------
// main program
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    mvr::Renderer renderer;
    std::string output = "";

    ret = renderer.initialize();
    if (EXIT_SUCCESS != ret)
    {
        std::cout << "Error: failed to initialize renderer." << std::endl;
        return ret;
    }

    ret = applyProgramOptions(argc, argv, renderer, output);
    if (EXIT_SUCCESS != ret)
    {
        std::cout <<
            "Error: failed to apply command line arguments." << std::endl;
        return ret;
    }

    if ("" == output)
        ret = renderer.run();
    else
    {
        ret = renderer.renderToFile(output);
        if (EXIT_SUCCESS == ret)
            std::cout << "Successfully rendered to " << output << std::endl;
        else
            std::cout << "Error: failed rendering to " << output << std::endl;
    }


    if (EXIT_SUCCESS != ret)
    {
        printf("Error: renderer terminated with error code (%i).\n", ret);
        return ret;
    }

    return ret;
}

//-----------------------------------------------------------------------------
// subroutines
//-----------------------------------------------------------------------------
int applyProgramOptions(
        int argc,
        char *argv[],
        mvr::Renderer& renderer,
        std::string& output)
{
    // Declare the supported options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("volume,v", po::value<std::string>(), "volume description file")
        ("config,c", po::value<std::string>(), "renderer configuration file")
        ("output-file,o", po::value<std::string>(), "batch mode output file")
    ;

    int ret = EXIT_SUCCESS;

    try
    {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (vm.count("config"))
        {
            ret = renderer.loadConfigFromFile(vm["config"].as<std::string>());
            if (EXIT_SUCCESS != ret)
            {
                std::cout <<
                    "Error: failed to apply config file." << std::endl;
                return ret;
            }
        }

        if (vm.count("volume"))
        {
            ret = renderer.loadVolumeFromFile(
                    vm["volume"].as<std::string>(), 0);
            if (EXIT_SUCCESS != ret)
            {
                std::cout <<
                    "Error: failed to load volume data set." << std::endl;
                return ret;
            }
        }

        if (vm.count("output-file"))
            output = vm["output-file"].as<std::string>();

    }
    catch(std::exception &e)
    {
        std::cout << "Invalid program options!" << std::endl;
        return EXIT_FAILURE;
    }

    return ret;
}

