#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "mvr.hpp"

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void applyProgramOptions(int argc, char *argv[], mvr::Renderer&);

//-----------------------------------------------------------------------------
// main program
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int return_code = 0;
    mvr::Renderer renderer;

    renderer.initialize();
    applyProgramOptions(argc, argv, renderer);
    return_code = renderer.run();

    return return_code;
}

//-----------------------------------------------------------------------------
// subroutines
//-----------------------------------------------------------------------------
void applyProgramOptions(int argc, char *argv[], mvr::Renderer& renderer)
{
    // Declare the supported options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("volume", po::value<std::string>(), "volume description file")
    ;

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

        if (vm.count("volume"))
        {
            if (EXIT_SUCCESS !=
                renderer.loadVolumeFromFile(vm["volume"].as<std::string>()))
            {
                std::cout << "Invalid volume description file!" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

    }
    catch(std::exception &e)
    {
        std::cout << "Invalid program options!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

