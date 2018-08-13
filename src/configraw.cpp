#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <json.hpp>

#include "configraw.hpp"

namespace bfs = boost::filesystem;

using json = nlohmann::json;

namespace cr
{
    /**
     * \brief configuration object for a time-dependet volume data set
     *
     * \param type datatype as defined in enum cr::Datatype
     * \return size of a value with the given datatype in byte
    */
    unsigned int datatypeSize(Datatype type)
    {
        unsigned int ret_value = 0;

        switch(type)
        {
            case Datatype::unsigned_byte:
            case Datatype::signed_byte:
                ret_value = 1;
                break;

            case Datatype::unsigned_halfword:
            case Datatype::signed_halfword:
                ret_value = 2;
                break;

            case Datatype::unsigned_word:
            case Datatype::signed_word:
            case Datatype::single_precision_float:
                ret_value = 4;
                break;

            case Datatype::unsigned_longword:
            case Datatype::signed_longword:
            case Datatype::double_precision_float:
                ret_value = 8;
                break;

            default:
                break;
        }

        return ret_value;
    }


    /**
     * \brief Converts VOLUME_DATA_TYPE into a cr::Datatype
     *
     * \param value string value of VOLUME_DATA_TYPE in the .config file
     * \return according datatype as cr::Datatype
     *
     * Converts the VOLUME_DATA_TYPE string value used in freysn .config files
     * into a cr::Datatype.
    */
    Datatype dotconfigValToDatatype(std::string value)
    {
        Datatype ret_value = Datatype::none;

        if (0 == value.compare("UCHAR"))
            ret_value = Datatype::unsigned_byte;

        return ret_value;
    }

    // ------------------------------------------------------------------------
    // Definition of VolumeConfig member functions
    // ------------------------------------------------------------------------
    VolumeConfig::VolumeConfig()
    {
        _num_timesteps = 0;
        _offset_timesteps = 0;
        _volume_dim = {0, 0, 0};
        _voxel_count = 0;
        _voxel_type = Datatype::none;
        _voxel_dim = {0, 0, 0};
        _voxel_sizeof = 0;
    }

    VolumeConfig::~VolumeConfig()
    {
        return;
    }

    VolumeConfig::VolumeConfig(std::string const &path) :
        VolumeConfig::VolumeConfig()
    {
        std::ifstream fs;

        fs.open(path.c_str(), std::ofstream::in);

        // try loading as json config file
        try
        {
            json json_config;

            fs >> json_config;

            _num_timesteps = json_config["VOLUME_NUM_TIMESTEPS"];
            _offset_timesteps = json_config["VOLUME_TIMESTEPS_OFFSET"];
            _volume_dim = json_config["VOLUME_DIM"];
            _voxel_count = _volume_dim[0] * _volume_dim[1] * _volume_dim[2];
            _voxel_type = dotconfigValToDatatype(
                json_config["VOLUME_DATA_TYPE"]);
            _voxel_dim = json_config["VOXEL_SIZE"];
            _voxel_sizeof = datatypeSize(_voxel_type);

            _raw_file_dir = json_config["VOLUME_FILE_DIR"];
            _raw_file_exp = json_config["VOLUME_FILE_REGEX"];

            bfs::path p;
            if (bfs::path(_raw_file_dir).is_absolute())
                p = bfs::path(_raw_file_dir);
            else
                p = bfs::path(path).parent_path() / bfs::path(_raw_file_dir);

            for (bfs::directory_entry& x : bfs::directory_iterator(p))
                if(boost::regex_match(x.path().filename().string(),
                    boost::regex(_raw_file_exp)))
                    _raw_files.push_back(x.path().string());

            // TODO add sorting

            return;
        }
        catch(json::exception &e)
        {
            std::cout << "Error loading volume configuration file: " << path <<
                std::endl;
            std::cout << "JSON exception: " << e.what() << std::endl;
        }
        catch(std::exception &e)
        {
            std::cout << "Error loading volume configuration file: " << path <<
                std::endl;
            std::cout << "General exception: " << e.what() << std::endl;
        }
        catch(...)
        {
            std::cout << "Error loading volume configuration file: " << path <<
                std::endl;
        }
    }

    std::string VolumeConfig::getTimestepFile(unsigned int n)
    {
        return std::string("");
    }
}
