#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <vector>
#include <algorithm>

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
                ret_value = sizeof(unsigned_byte_t);
                break;

            case Datatype::signed_byte:
                ret_value = sizeof(signed_byte_t);
                break;

            case Datatype::unsigned_halfword:
                ret_value = sizeof(unsigned_halfword_t);
                break;

            case Datatype::signed_halfword:
                ret_value = sizeof(signed_halfword_t);
                break;

            case Datatype::unsigned_word:
                ret_value = sizeof(unsigned_word_t);
                break;

            case Datatype::signed_word:
                ret_value = sizeof(signed_word_t);
                break;

            case Datatype::unsigned_longword:
                ret_value = sizeof(unsigned_longword_t);
                break;

            case Datatype::signed_longword:
                ret_value = sizeof(signed_longword_t);
                break;

            case Datatype::single_precision_float:
                ret_value = sizeof(single_precision_float_t);
                break;

            case Datatype::double_precision_float:
                ret_value = sizeof(double_precision_float_t);
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
        _volume_dim = {0, 0, 0};
        _voxel_count = 0;
        _voxel_type = Datatype::none;
        _voxel_dim = {0, 0, 0};
        _voxel_sizeof = 0;
        _valid = false;
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

            std::sort(_raw_files.begin(), _raw_files.end());

            _valid = true;
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
        if (this->_raw_files.size() < 1) return std::string("");
        else if (n > (this->_raw_files.size() - 1))
            n = this->_raw_files.size() - 1;

        return this->_raw_files[n];
    }
    /**
     * \brief loads the scalar-valued volume data of a given timestep
     *
     * \param vConf configuration object of the volume dataset
     * \param n number of the requested timestep (starting from 0)
     * \param swap flag if the byte order of the raw data shall be swapped
     *
     * \return pointer to the loaded data
     *
     * Note: - the calling function has to delete the returned volume data to
     *       free the used memory
     *
    */
    void *loadScalarVolumeDataTimestep(
        VolumeConfig vConf, unsigned int n, bool swap)
    {
        void *volumeData = nullptr;

        switch(vConf.getVoxelType())
        {
            case Datatype::unsigned_byte:
                volumeData = reinterpret_cast<void *>(
                    new unsigned_byte_t[vConf.getVoxelCount()]);
                loadRaw<unsigned_byte_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<unsigned_byte_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::signed_byte:
                volumeData = reinterpret_cast<void *>(
                    new signed_byte_t[vConf.getVoxelCount()]);
                loadRaw<signed_byte_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<signed_byte_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::unsigned_halfword:
                volumeData = reinterpret_cast<void *>(
                    new unsigned_halfword_t[vConf.getVoxelCount()]);
                loadRaw<unsigned_halfword_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<unsigned_halfword_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::signed_halfword:
                volumeData = reinterpret_cast<void *>(
                    new signed_halfword_t[vConf.getVoxelCount()]);
                loadRaw<signed_halfword_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<signed_halfword_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::unsigned_word:
                volumeData = reinterpret_cast<void *>(
                    new unsigned_word_t[vConf.getVoxelCount()]);
                loadRaw<unsigned_word_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<unsigned_word_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::signed_word:
                volumeData = reinterpret_cast<void *>(
                    new signed_word_t[vConf.getVoxelCount()]);
                loadRaw<signed_word_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<signed_word_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::unsigned_longword:
                volumeData = reinterpret_cast<void *>(
                    new unsigned_longword_t[vConf.getVoxelCount()]);
                loadRaw<unsigned_longword_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<unsigned_longword_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::signed_longword:
                volumeData = reinterpret_cast<void *>(
                    new signed_longword_t[vConf.getVoxelCount()]);
                loadRaw<signed_longword_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<signed_longword_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::single_precision_float:
                volumeData = reinterpret_cast<void *>(
                    new single_precision_float_t[vConf.getVoxelCount()]);
                loadRaw<single_precision_float_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<single_precision_float_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            case Datatype::double_precision_float:
                volumeData = reinterpret_cast<void *>(
                    new double_precision_float_t[vConf.getVoxelCount()]);
                loadRaw<double_precision_float_t>(
                    vConf.getTimestepFile(n),
                    reinterpret_cast<double_precision_float_t*>(volumeData),
                    vConf.getVoxelCount(),
                    swap);
                break;

            default:
                break;
        }

        return volumeData;
    }

    void deleteVolumeData(VolumeConfig vConf, void *volumeData)
    {
        switch(vConf.getVoxelType())
        {
            case Datatype::unsigned_byte:
                delete[] reinterpret_cast<unsigned_byte_t*>(volumeData);
                break;

            case Datatype::signed_byte:
                delete[] reinterpret_cast<signed_byte_t*>(volumeData);
                break;

            case Datatype::unsigned_halfword:
                delete[] reinterpret_cast<unsigned_halfword_t*>(volumeData);
                break;

            case Datatype::signed_halfword:
                delete[] reinterpret_cast<signed_halfword_t*>(volumeData);
                break;

            case Datatype::unsigned_word:
                delete[] reinterpret_cast<unsigned_word_t*>(volumeData);
                break;

            case Datatype::signed_word:
                delete[] reinterpret_cast<signed_word_t*>(volumeData);
                break;

            case Datatype::unsigned_longword:
                delete[] reinterpret_cast<unsigned_longword_t*>(volumeData);
                break;

            case Datatype::signed_longword:
                delete[] reinterpret_cast<signed_longword_t*>(volumeData);
                break;

            case Datatype::single_precision_float:
                delete[] reinterpret_cast<single_precision_float_t*>(
                    volumeData);
                break;

            case Datatype::double_precision_float:
                delete[] reinterpret_cast<double_precision_float_t*>(
                    volumeData);
                break;

            default:
                break;
        }
    }
}

