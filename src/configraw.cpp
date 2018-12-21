#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <vector>
#include <algorithm>

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

#include <boost/regex.hpp>

#include <json.hpp>
using json = nlohmann::json;

#include "configraw.hpp"
#include "util/util.hpp"



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
}

// ------------------------------------------------------------------------
// Definition of VolumeConfig member functions
// ------------------------------------------------------------------------
cr::VolumeConfig::VolumeConfig()
{
    _num_timesteps = 0;
    _volume_dim = {0, 0, 0};
    _voxel_count = 0;
    _voxel_type = Datatype::none;
    _voxel_dim = {0, 0, 0};
    _voxel_sizeof = 0;
    _valid = false;
}

cr::VolumeConfig::~VolumeConfig()
{
}

cr::VolumeConfig::VolumeConfig(std::string const &path) :
    VolumeConfig()
{
    std::ifstream fs;

    fs.open(path.c_str(), std::ofstream::in);

    // try loading as json config file
    try
    {
        json json_config;

        fs >> json_config;

        _num_timesteps =
            json_config["VOLUME_NUM_TIMESTEPS"].get<unsigned int>();
        if (
                json_config["SUBSET_MIN"].is_array() &&
                json_config["SUBSET_MAX"].is_array())
        {
            _subset = true;
            _orig_volume_dim =
                json_config["VOLUME_DIM"].get<std::array<size_t, 3>>();
            _subset_min =
                json_config["SUBSET_MIN"].get<std::array<size_t, 3>>();
            _subset_max =
                json_config["SUBSET_MAX"].get<std::array<size_t, 3>>();
            _volume_dim[0] = (_subset_max[0] - _subset_min[0] + 1);
            _volume_dim[1] = (_subset_max[1] - _subset_min[1] + 1);
            _volume_dim[2] = (_subset_max[2] - _subset_min[2] + 1);
        }
        else
        {
            _subset = false;
            _volume_dim =
                json_config["VOLUME_DIM"].get<std::array<size_t, 3>>();
            _orig_volume_dim = _volume_dim;
            _subset_min[0] = 0; _subset_min[1] = 0; _subset_min[2] = 0;
            _subset_max[0] = _volume_dim[0] - 1;
            _subset_max[1] = _volume_dim[1] - 1;
            _subset_max[2] = _volume_dim[2] - 1;
        }
        _voxel_count = _volume_dim[0] * _volume_dim[1] * _volume_dim[2];
        _voxel_type = json_config["VOLUME_DATA_TYPE"].get<Datatype>();
        _voxel_dim = json_config["VOXEL_SIZE"].get<std::array<size_t, 3>>();
        _voxel_sizeof = datatypeSize(_voxel_type);

        _raw_file_dir = json_config["VOLUME_FILE_DIR"].get<std::string>();
        _raw_file_exp = json_config["VOLUME_FILE_REGEX"].get<std::string>();

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
}

std::string cr::VolumeConfig::getTimestepFile(unsigned int n)
{
    if (this->_raw_files.size() < 1) return std::string("");
    else if (n > (this->_raw_files.size() - 1))
        n = this->_raw_files.size() - 1;

    return this->_raw_files[n];
}

//-------------------------------------------------------------------------
// convenience functions
//-------------------------------------------------------------------------
/**
 * \brief loads the scalar-valued volume data of a given timestep
 *
 * \param volumeConfig configuration object of the volume dataset
 * \param n number of the requested timestep (starting from 0)
 * \param swap flag if the byte order of the raw data shall be swapped
 *
 * \return pointer to the loaded data
 *
 * Note: Calling function has to delete the returned volume data.
 *
*/
std::unique_ptr<cr::VolumeDataBase> cr::loadScalarVolumeTimestep(
    VolumeConfig volumeConfig, unsigned int n, bool swap)
{
    void *rawData = nullptr;
    std::unique_ptr<VolumeDataBase> pVolumeData = nullptr;

    if (!volumeConfig.getSubset())
    {
        // the volume shall be loaded completely
        switch(volumeConfig.getVoxelType())
        {
            case Datatype::unsigned_byte:
                rawData = reinterpret_cast<void *>(
                        new unsigned_byte_t[volumeConfig.getVoxelCount()]);
                loadRaw<unsigned_byte_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_byte_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_byte_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_byte_t*>(rawData));
                break;

            case Datatype::signed_byte:
                rawData = reinterpret_cast<void *>(
                    new signed_byte_t[volumeConfig.getVoxelCount()]);
                loadRaw<signed_byte_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_byte_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_byte_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_byte_t*>(rawData));
                break;

            case Datatype::unsigned_halfword:
                rawData = reinterpret_cast<void *>(
                    new unsigned_halfword_t[volumeConfig.getVoxelCount()]);
                loadRaw<unsigned_halfword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_halfword_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_halfword_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_halfword_t*>(rawData));
                break;

            case Datatype::signed_halfword:
                rawData = reinterpret_cast<void *>(
                    new signed_halfword_t[volumeConfig.getVoxelCount()]);
                loadRaw<signed_halfword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_halfword_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_halfword_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_halfword_t*>(rawData));
                break;

            case Datatype::unsigned_word:
                rawData = reinterpret_cast<void *>(
                    new unsigned_word_t[volumeConfig.getVoxelCount()]);
                loadRaw<unsigned_word_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_word_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_word_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_word_t*>(rawData));
                break;

            case Datatype::signed_word:
                rawData = reinterpret_cast<void *>(
                    new signed_word_t[volumeConfig.getVoxelCount()]);
                loadRaw<signed_word_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_word_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_word_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_word_t*>(rawData));
                break;

            case Datatype::unsigned_longword:
                rawData = reinterpret_cast<void *>(
                    new unsigned_longword_t[volumeConfig.getVoxelCount()]);
                loadRaw<unsigned_longword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_longword_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_longword_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_longword_t*>(rawData));
                break;

            case Datatype::signed_longword:
                rawData = reinterpret_cast<void *>(
                    new signed_longword_t[volumeConfig.getVoxelCount()]);
                loadRaw<signed_longword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_longword_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_longword_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_longword_t*>(rawData));
                break;

            case Datatype::single_precision_float:
                rawData = reinterpret_cast<void *>(
                    new single_precision_float_t[
                        volumeConfig.getVoxelCount()]);
                loadRaw<single_precision_float_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<single_precision_float_t*>(
                        rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<single_precision_float_t>>(
                        volumeConfig,
                        reinterpret_cast<single_precision_float_t*>(rawData));
                break;

            case Datatype::double_precision_float:
                rawData = reinterpret_cast<void *>(
                    new double_precision_float_t[
                        volumeConfig.getVoxelCount()]);
                loadRaw<double_precision_float_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<double_precision_float_t*>(rawData),
                    volumeConfig.getVoxelCount(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<double_precision_float_t>>(
                        volumeConfig,
                        reinterpret_cast<double_precision_float_t*>(rawData));
                break;

            default:
                break;
        }
    }
    else
    {
        // only a subset of the volume shall be loaded
        switch(volumeConfig.getVoxelType())
        {
            case Datatype::unsigned_byte:
                rawData = reinterpret_cast<void *>(
                    new unsigned_byte_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<unsigned_byte_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_byte_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_byte_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_byte_t*>(rawData));
                break;

            case Datatype::signed_byte:
                rawData = reinterpret_cast<void *>(
                    new signed_byte_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<signed_byte_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_byte_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_byte_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_byte_t*>(rawData));
                break;

            case Datatype::unsigned_halfword:
                rawData = reinterpret_cast<void *>(
                    new unsigned_halfword_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<unsigned_halfword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_halfword_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_halfword_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_halfword_t*>(rawData));
                break;

            case Datatype::signed_halfword:
                rawData = reinterpret_cast<void *>(
                    new signed_halfword_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<signed_halfword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_halfword_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_halfword_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_halfword_t*>(rawData));
                break;

            case Datatype::unsigned_word:
                rawData = reinterpret_cast<void *>(
                    new unsigned_word_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<unsigned_word_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_word_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_word_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_word_t*>(rawData));
                break;

            case Datatype::signed_word:
                rawData = reinterpret_cast<void *>(
                    new signed_word_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<signed_word_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_word_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_word_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_word_t*>(rawData));
                break;

            case Datatype::unsigned_longword:
                rawData = reinterpret_cast<void *>(
                    new unsigned_longword_t[
                        volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<unsigned_longword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<unsigned_longword_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<unsigned_longword_t>>(
                        volumeConfig,
                        reinterpret_cast<unsigned_longword_t*>(rawData));
                break;

            case Datatype::signed_longword:
                rawData = reinterpret_cast<void *>(
                    new signed_longword_t[volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<signed_longword_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<signed_longword_t*>(rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<signed_longword_t>>(
                        volumeConfig,
                        reinterpret_cast<signed_longword_t*>(rawData));
                break;

            case Datatype::single_precision_float:
                rawData = reinterpret_cast<void *>(
                    new single_precision_float_t[
                        volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<single_precision_float_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<single_precision_float_t*>(
                        rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<single_precision_float_t>>(
                        volumeConfig,
                        reinterpret_cast<single_precision_float_t*>(rawData));
                break;

            case Datatype::double_precision_float:
                rawData = reinterpret_cast<void *>(
                    new double_precision_float_t[
                        volumeConfig.getVoxelCount()]);
                loadSubset3dCuboid<double_precision_float_t>(
                    volumeConfig.getTimestepFile(n),
                    reinterpret_cast<double_precision_float_t*>(
                        rawData),
                    volumeConfig.getOrigVolumeDim(),
                    volumeConfig.getSubsetMin(),
                    volumeConfig.getSubsetMax(),
                    swap);
                pVolumeData =
                    std::make_unique<VolumeData<double_precision_float_t>>(
                        volumeConfig,
                        reinterpret_cast<double_precision_float_t*>(rawData));
                break;

            default:
                break;
        }
    }

    return pVolumeData;
}

/**
 * \brief creates a 3d texture from the given volume data
 *
 * \param volumeConfig configuration object of the volume dataset
 * \param volumeData typeless pointer to the volume data
 *
 * \return 3D texture object
 *
 * Note: Texture has to be deleted by the calling function
*/
util::texture::Texture3D cr::loadScalarVolumeTex(
    const VolumeDataBase &volumeData)
{
    bool supported = true;
    GLenum type = GL_UNSIGNED_BYTE;
    VolumeConfig volumeConfig = volumeData.getVolumeConfig();

    switch(volumeConfig.getVoxelType())
    {
        case Datatype::unsigned_byte:
            type = GL_UNSIGNED_BYTE;
            break;

        case Datatype::signed_byte:
            type = GL_BYTE;
            break;

        case Datatype::unsigned_halfword:
            type = GL_UNSIGNED_SHORT;
            break;

        case Datatype::signed_halfword:
            type = GL_SHORT;
            break;

        case Datatype::unsigned_word:
            type = GL_UNSIGNED_INT;
            break;

        case Datatype::signed_word:
            type = GL_INT;
            break;

        case Datatype::single_precision_float:
            type = GL_FLOAT;
            break;

        case Datatype::double_precision_float:
        case Datatype::unsigned_longword:
        case Datatype::signed_longword:
        default:
            std::cerr << "Error: unsupported volume datatype." <<
                std::endl;
            supported = false;
            break;
    }

    if (true == supported)
        return util::texture::Texture3D(
            GL_RED,
            GL_RED,
            0,
            type,
            GL_LINEAR,
            GL_CLAMP_TO_BORDER,
            volumeConfig.getVolumeDim()[0],
            volumeConfig.getVolumeDim()[1],
            volumeConfig.getVolumeDim()[2],
            volumeData.getRawData());

    else
        return util::texture::Texture3D();
}

/**
 * \brief groups the volume data values into bins for use in a histogram
 *
 * \param volumeData volume dataset representative class object
 * \param numBins number of histogram bins
 * \param min lower histogram x axis limit
 * \param max upper histogram x axis limit
 *
 * \returns an vector of bin objects
 *
 * Note: vector of bins has to be deleted by the calling function
*/
std::vector<util::bin_t> cr::bucketVolumeData(
    const VolumeDataBase &volumeData,
    size_t numBins,
    float min,
    float max)
{
    std::vector<util::bin_t> bins(0);
    VolumeConfig volumeConfig = volumeData.getVolumeConfig();
    void *values = reinterpret_cast<void*>(volumeData.getRawData());

    switch(volumeConfig.getVoxelType())
    {
        case Datatype::unsigned_byte:
            bins = util::binData<unsigned_byte_t>(
                numBins,
                static_cast<unsigned_byte_t>(min),
                static_cast<unsigned_byte_t>(max),
                reinterpret_cast<unsigned_byte_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::signed_byte:
            bins = util::binData<signed_byte_t>(
                numBins,
                static_cast<signed_byte_t>(min),
                static_cast<signed_byte_t>(max),
                reinterpret_cast<signed_byte_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::unsigned_halfword:
            bins = util::binData<unsigned_halfword_t>(
                numBins,
                static_cast<unsigned_halfword_t>(min),
                static_cast<unsigned_halfword_t>(max),
                reinterpret_cast<unsigned_halfword_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::signed_halfword:
            bins = util::binData<signed_halfword_t>(
                numBins,
                static_cast<signed_halfword_t>(min),
                static_cast<signed_halfword_t>(max),
                reinterpret_cast<signed_halfword_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::unsigned_word:
            bins = util::binData<unsigned_word_t>(
                numBins,
                static_cast<unsigned_word_t>(min),
                static_cast<unsigned_word_t>(max),
                reinterpret_cast<unsigned_word_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::signed_word:
            bins = util::binData<signed_word_t>(
                numBins,
                static_cast<signed_word_t>(min),
                static_cast<signed_word_t>(max),
                reinterpret_cast<signed_word_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::unsigned_longword:
            bins = util::binData<unsigned_longword_t>(
                numBins,
                static_cast<unsigned_longword_t>(min),
                static_cast<unsigned_longword_t>(max),
                reinterpret_cast<unsigned_longword_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::signed_longword:
            bins = util::binData<signed_longword_t>(
                numBins,
                static_cast<signed_longword_t>(min),
                static_cast<signed_longword_t>(max),
                reinterpret_cast<signed_longword_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::single_precision_float:
            bins = util::binData<single_precision_float_t>(
                numBins,
                static_cast<single_precision_float_t>(min),
                static_cast<single_precision_float_t>(max),
                reinterpret_cast<single_precision_float_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        case Datatype::double_precision_float:
            bins = util::binData<double_precision_float_t>(
                numBins,
                static_cast<double_precision_float_t>(min),
                static_cast<double_precision_float_t>(max),
                reinterpret_cast<double_precision_float_t*>(values),
                volumeConfig.getVoxelCount());
            break;

        default:
            break;
    }

    return bins;
}

