#pragma once

#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <vector>
#include <cstdint>
#include <memory>

#include <GL/gl3w.h>

#include <json.hpp>
using json = nlohmann::json;

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

    NLOHMANN_JSON_SERIALIZE_ENUM(
        Datatype, {
            {Datatype::none, "NONE"},
            {Datatype::unsigned_byte, "UCHAR"},
            {Datatype::single_precision_float, "FLOAT"}});

    // ------------------------------------------------------------------------
    // forward declarations
    // ------------------------------------------------------------------------
    class VolumeConfig;
    class VolumeDataBase;
    unsigned int datatypeSize(cr::Datatype type);
    std::unique_ptr<VolumeDataBase> loadScalarVolumeTimestep(
        VolumeConfig volumeConfig, unsigned int n, bool swap);
    util::texture::Texture3D loadScalarVolumeTex(
        const VolumeDataBase &volumeData);
    std::vector<util::bin_t> bucketVolumeData(
        const VolumeDataBase &volumeData,
        size_t numBins,
        float min,
        float max);

    // ------------------------------------------------------------------------
    // class declarations
    // ------------------------------------------------------------------------

    // volume dataset configuration class
    class VolumeConfig
    {
        private:
        unsigned int _num_timesteps;        //!< length of the time series
        std::array<size_t, 3> _volume_dim;  //!< number of cells/ nodes in
                                            //!< the spatial dimensions of
                                            //!< the loaded volume
        std::array<size_t, 3> _orig_volume_dim;
                                            //!< number of cells/ nodes in
                                            //!< the spatial dimensions of
                                            //!< the whole volume
        bool _subset;                       //!< indicator if only a subset
                                            //!< of the available volume data
                                            //!< shall be loaded
        std::array<size_t, 3> _subset_min;  //!< index of lower left voxel
                                            //!< of loaded volume (sub-)cuboid
        std::array<size_t, 3> _subset_max;  //!< index of upper right voxel
                                            //!< of loaded volume (sub-)cuboid
        size_t _voxel_count;                //!< total number of voxels in the
                                            //!< loaded volume
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
        unsigned int getNumTimesteps() const { return _num_timesteps; }
        std::array<size_t, 3> getVolumeDim() const { return _volume_dim; }
        std::array<size_t, 3> getOrigVolumeDim() const
        {
            return _orig_volume_dim;
        }
        bool getSubset() const { return _subset; }
        std::array<size_t, 3> getSubsetMin() const { return _subset_min; }
        std::array<size_t, 3> getSubsetMax() const { return _subset_max; }
        size_t getVoxelCount() const { return _voxel_count; }
        Datatype getVoxelType() const { return _voxel_type; }
        std::array<size_t, 3> getVoxelDim() const { return _voxel_dim; }
        size_t getVoxelSizeOf() const { return _voxel_sizeof; }
        std::string getRawFileDir() const { return _raw_file_dir; }
        std::string getRawFileExp() const { return _raw_file_exp; }
    };

    // volume dataset representative
    class VolumeDataBase
    {
        public:
        VolumeDataBase() : m_config() {}
        VolumeDataBase(VolumeConfig volumeConfig) :
            m_config(volumeConfig)
        {
        }
        virtual ~VolumeDataBase() {};

        virtual void* getRawData() const = 0;
        VolumeConfig getVolumeConfig() const { return m_config; }

        private:
        VolumeConfig m_config;
    };

    template<typename T>
    class VolumeData : public VolumeDataBase
    {
        public:
        VolumeData() : m_rawData(nullptr) {}
        VolumeData(VolumeConfig volumeConfig, T* rawData) :
            VolumeDataBase(volumeConfig),
            m_rawData(rawData)
        {
        }

        VolumeData(const VolumeData& other) = delete;
        VolumeData& operator=(VolumeData& other) = delete;
        VolumeData(VolumeData&& other) :
            VolumeDataBase(std::move(other.m_config)),
            m_rawData(std::move(other.m_rawData))
        {
            other.m_config = VolumeConfig();
            other.m_rawData = nullptr;
        }
        VolumeData& operator=(VolumeData&& other)
        {
            if (nullptr != this->m_rawData)
                delete[] this->m_rawData;

            this->m_config = std::move(other.m_config);
            this->m_rawData = std::move(other.m_rawData);
            other.m_rawData = nullptr;

            return this;
        }
        ~VolumeData() { if (nullptr != m_rawData) delete[] m_rawData; }

        void* getRawData() const override
        {
            return reinterpret_cast<void*>(m_rawData);
        }

        private:
        T* m_rawData;
    };


    // ------------------------------------------------------------------------
    // templated utility functions
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
     * \param origVolumeDim dimensions of the whole volume
     * \param subsetMin index of the lower left voxel of the loaded cuboid
     * \param subsetMax index of the upper right voxel of the loaded cuboid
     * \param swap True if the byte order of the read values shall be swapped
     *
     * Loads subset of a cuboid volume dataset that is layed out in a flat
     * array. The subset itself is a cuboid volume dataset that is embedded
     * in the complete volume dataset.
    */
    template<typename T>
    void loadSubset3dCuboid(
        std::string path,
        T *buffer,
        std::array<size_t, 3> origVolumeDim,
        std::array<size_t, 3> subsetMin,
        std::array<size_t, 3> subsetMax,
        bool swap = false)
    {
        std::ifstream fs (path.c_str(), std::ios::in | std::ios::binary);

        if (!fs.is_open())
        {
            std::cerr <<
                "Error while loading data subset: cannot open file!\n";
            return;
        }

        // load the data in chunks along the x axis
        size_t volumeIdx = 0, bufferIdx = 0;
        size_t chunkSize = subsetMax[0] - subsetMin[0] + 1;

        for (size_t z = subsetMin[2]; z <= subsetMax[2]; ++z)
        for (size_t y = subsetMin[1]; y <= subsetMax[1]; ++y)
        {
            volumeIdx = sizeof(T) * (
                subsetMin[0] +
                y * origVolumeDim[0] +
                z * origVolumeDim[0] * origVolumeDim[1]);

            fs.seekg(volumeIdx);
            fs.read(
                reinterpret_cast<char*>(&buffer[bufferIdx]),
                sizeof(T) * chunkSize);

            bufferIdx += chunkSize;
        }

        if (swap)
        {
            size_t count = (
                (subsetMax[0] - subsetMin[0] + 1) *
                (subsetMax[1] - subsetMin[1] + 1) *
                (subsetMax[2] - subsetMin[2] + 1));

            for (std::size_t i = 0; i < count; ++i)
            {
                buffer[i] = swapByteOrder(buffer[i]);
            }
        }
    }
}

