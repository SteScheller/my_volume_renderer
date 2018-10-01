#ifndef __CONFIGRAW_HPP
#define __CONFIGRAW_HPP

#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <vector>
#include <cstdint>

#include <GL/gl3w.h>

#include "util/util.hpp"

typedef uint8_t unsigned_byte_t;
typedef int8_t signed_byte_t;
typedef uint16_t unsigned_halfword_t;
typedef int16_t signed_halfword_t;
typedef uint32_t unsigned_word_t;
typedef int32_t signed_word_t;
typedef uint64_t unsigned_longword_t;
typedef int64_t signed_longword_t;
typedef float single_precision_float_t;
typedef double double_precision_float_t;

namespace cr
{
    // ------------------------------------------------------------------------
    // type definitions
    // ------------------------------------------------------------------------
    /**
     * \brief enumeration for encoding type information
    */
    enum class Datatype : int
    {
        none = 0,
        unsigned_byte,
        signed_byte,
        unsigned_halfword,
        signed_halfword,
        unsigned_word,
        signed_word,
        unsigned_longword,
        signed_longword,
        single_precision_float,
        double_precision_float
    };

    // ------------------------------------------------------------------------
    // forward declarations
    // ------------------------------------------------------------------------
    unsigned int datatypeSize(cr::Datatype type);
    Datatype dotconfigValToDatatype(std::string value);
    class VolumeConfig;
    void *loadScalarVolumeDataTimestep(
        VolumeConfig vConf, unsigned int n, bool swap);
    void deleteVolumeData(VolumeConfig vConf, void *volumeData);
    GLuint loadScalarVolumeTex(VolumeConfig vConf, void* volumeData);
    std::vector<util::bin_t > *bucketVolumeData(
        VolumeConfig vConf,
        void* values,
        size_t numBins,
        float min,
        float max);

    // ------------------------------------------------------------------------
    // classes
    // ------------------------------------------------------------------------
    class VolumeConfig
    {
        private:
        unsigned int _num_timesteps;        //!< length of the time series
        std::array<size_t, 3> _volume_dim;  //!< number of cells/ nodes in
                                            //!< the spatial dimensions of
                                            //!< the volume
        size_t _voxel_count;                //!< total number of voxels
        Datatype _voxel_type;               //!< voxel type information
        std::array<size_t, 3> _voxel_dim;   //!< dimensionality of a voxel
        size_t _voxel_sizeof;               //!< size of a voxel in byte
        std::string _raw_file_dir;          //!< path to raw files
        std::string _raw_file_exp;          //!< filter regex for raw files
        std::vector<std::string> _raw_files;//!< vector of file paths to
                                            //!< the raw data
        bool _valid;                        //!< health flag

        public:
        VolumeConfig();                         //!< default constructor
        VolumeConfig(std::string const &path);  //!< construction from file
        ~VolumeConfig();                        //!< destructor

        /*
         * \brief returns the path of datafile containing the n-th timestep
         *
         * \param n temporal index of the timestep {0, 1, 2 ..}
         * \return path to the according datafile
        */
        std::string getTimestepFile(unsigned int n);

        /*
         * \brief indicator if the object represents a valid configuration
        */
        bool isValid(){ return _valid; }

        // getter and setter
        unsigned int getNumTimesteps(){ return _num_timesteps; }
        std::array<size_t, 3> getVolumeDim(){ return _volume_dim; }
        size_t getVoxelCount(){ return _voxel_count; }
        Datatype getVoxelType(){ return _voxel_type; }
        std::array<size_t, 3> getVoxelDim(){ return _voxel_dim; }
        size_t getVoxelSizeOf(){ return _voxel_sizeof; }
        std::string getRawFileDir(){ return _raw_file_dir; }
        std::string getRawFileExp(){ return _raw_file_exp; }
    };

    // ------------------------------------------------------------------------
    // function templates
    // ------------------------------------------------------------------------
    /**
     * \brief swaps the byteorder of the given value
     */
    template<typename T>
    T swapByteOrder(T value)
    {
        unsigned char *v = reinterpret_cast<unsigned char*>(&value);

        switch(sizeof(T))
        {
            case 8:
                value =
                    (static_cast<uint64_t>(v[0]) << 56) |
                    (static_cast<uint64_t>(v[1]) << 48) |
                    (static_cast<uint64_t>(v[2]) << 40) |
                    (static_cast<uint64_t>(v[3]) << 32) |
                    (static_cast<uint64_t>(v[4]) << 24) |
                    (static_cast<uint64_t>(v[5]) << 16) |
                    (static_cast<uint64_t>(v[6]) << 8) |
                    static_cast<uint64_t>(v[7]);
                break;

            case 4:
                value =
                    (static_cast<uint32_t>(v[0]) << 24) |
                    (static_cast<uint32_t>(v[1]) << 16) |
                    (static_cast<uint32_t>(v[2]) << 8) |
                    static_cast<uint32_t>(v[3]);
                break;

            case 2:
                value =
                    (static_cast<uint16_t>(v[0]) << 8) |
                    static_cast<uint16_t>(v[1]);
                break;

            case 1:
            default:
                break;
        }

        return value;
    }

    /**
     * \brief loads a series of T values into a given buffer
     * \param path Destination of the file to be read
     * \param buffer Pointer to an array where the read values are stored
     * \param size Number of values to be read
     * \param swap True if the byte order of the read values shall be swapped
    */
    template<typename T>
    void loadRaw(
        std::string path, T *buffer, std::size_t size, bool swap = false)
    {
        std::ifstream fs (path.c_str(), std::ios::in | std::ios::binary);

        if (!fs.is_open())
        {
            std::cerr << "Error while loading data: cannot open file!\n";
            return;
        }

        fs.read(reinterpret_cast<char*>(buffer), size * sizeof(T));
        fs.close();

        if (swap)
        {
            for (std::size_t i = 0; i < size; ++i)
            {
                buffer[i] = swapByteOrder(buffer[i]);
            }
        }
    }

    /**
     * \brief loads a subset of 3d volume data from a linear array
     * \param path Destination of the file to be read
     * \param buffer Pointer to an array where the read values are stored
     * \param swap True if the byte order of the read values shall be swapped
     *
     * Loads subset of a cuboid volume dataset that is layed out in a flat
     * array. The subset itself is a cuboid volume dataset that is embedded
     * in the complete volume dataset.
    */
    template<typename T>
    void loadSubset3dCuboid(
        std::string path, T *buffer,
        std::array<size_t, 3> volumeDim,
        std::array<size_t, 3> subsetMin,
        std::array<size_t, 3> subsetMax,
        bool swap = false)
    {
        std::ifsteam fs (path.c_str(), std::ios::in | std::ios::binary);

        if (!fs.is_open())
        {
            std::cerr <<
                "Error while loading data subset: cannot open file!\n";
            return;
        }


        if (swap)
        {
            for (std::size_t i = 0; i < ...; ++i)
            {
                buffer[i] = swapByteOrder(buffer[i]);
            }
        }
    }
}
#endif

