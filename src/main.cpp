#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "mvr.hpp"

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void applyProgramOptions(int argc, char *argv[]);

//-----------------------------------------------------------------------------
// main program
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int return_code = 0;

    applyProgramOptions(argc, argv);

    mvr::Renderer renderer;

    return_code = renderer.run();

    return return_code;
}

//-----------------------------------------------------------------------------
// subroutines
//-----------------------------------------------------------------------------
void applyProgramOptions(int argc, char *argv[])
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

        std::string desc_file = "";
        if (vm.count("volume"))
            desc_file = vm["volume"].as<std::string>();
        else
            desc_file = DEFAULT_VOLUME_JSON_FILE;

        cr::VolumeConfig tempConf = cr::VolumeConfig(desc_file);
        if(tempConf.isValid())
            strncpy(
                gui_volume_desc_file, desc_file.c_str(), MAX_FILEPATH_LENGTH);
        else
        {
            std::cout << "Invalid volume description!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    catch(std::exception &e)
    {
        std::cout << "Invalid program options!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

