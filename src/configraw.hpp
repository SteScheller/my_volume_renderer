#ifndef __CONFIGRAW_HPP
#define __CONFIGRAW_HPP

#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <vector>

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
    // function declarations
    // ------------------------------------------------------------------------
    unsigned int datatypeSize(cr::Datatype type);
    Datatype dotconfigValToDatatype(std::string value);

    // ------------------------------------------------------------------------
    // classes
    // ------------------------------------------------------------------------
    class VolumeConfig
    {
        private:
        unsigned int _num_timesteps;    //!< length of the time series
        unsigned int _offset_timesteps; //!< starting index of time series
        std::array<unsigned int, 3> _volume_dim; //!< number of cells/ nodes in
                                                 //!< the spatial dimensions of
                                                 //!< the volume
        unsigned int _voxel_count;      //!< total number of voxels
        Datatype _voxel_type;           //!< type information of voxel values
        std::array<unsigned int, 3> _voxel_dim; //!< dimensionality of a voxel
        unsigned int _voxel_sizeof;     //!< size of a voxel in byte
        std::string _raw_file_dir;      //!< path to raw files
        std::string _raw_file_exp;      //!< filter regex for raw files
        std::vector<std::string> _raw_files;    //!< vector of file paths to
                                                //!< the raw data

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

        // getter and setter
        unsigned int getNumTimesteps(){ return _num_timesteps; }
        unsigned int getOffsetTimesteps(){ return _offset_timesteps; }
        std::array<unsigned int, 3> getVolumeDim(){ return _volume_dim; }
        unsigned int getVoxelCount(){ return _voxel_count; }
        Datatype getVoxelType(){ return _voxel_type; }
        std::array<unsigned int, 3> getVoxelDim(){ return _voxel_dim; }
        unsigned int getVoxelSizeOf(){ return _voxel_sizeof; }
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
        std::ifstream fs;

        fs.open(path.c_str(), std::ofstream::in | std::ofstream::binary);

        if (!fs.is_open())
        {
            std::cerr << "Error while loading data: cannot open file!\n";
            return;
        }

        T value;
        for (std::size_t i = 0; i < size; i++)
        {
            if (fs.good())
            {
                fs.read(reinterpret_cast<char*>(&value), sizeof(T));
                if (swap) value = swapByteOrder(value);
                buffer[i] = value;
            }
            else
                std::cerr << "Error while loading data: read failed!\n";
        }
        fs.close();
    }
}
#endif

