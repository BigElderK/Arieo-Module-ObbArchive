#pragma once
#include "interface/archive/archive.h"
#include <fstream>
#include <zlib.h>
#include <vector>
#include <cstring>
namespace Arieo
{
#pragma pack(push, 1)
    // ZIP file structures for OBB reading
    struct ZipLocalFileHeader {
        uint32_t signature;
        uint16_t version;
        uint16_t flags;
        uint16_t compression;
        uint16_t mod_time;
        uint16_t mod_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t filename_length;
        uint16_t extra_length;
    };

    struct ZipCentralDirectoryHeader {
        uint32_t signature;
        uint16_t version_made_by;
        uint16_t version_needed;
        uint16_t flags;
        uint16_t compression;
        uint16_t mod_time;
        uint16_t mod_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t filename_length;
        uint16_t extra_length;
        uint16_t comment_length;
        uint16_t disk_number;
        uint16_t internal_attributes;
        uint32_t external_attributes;
        uint32_t local_header_offset;
    };
#pragma pack(pop)

    struct ZipFileEntry {
        std::string filename;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint32_t crc32;
        uint16_t compression_method;
        uint32_t file_data_offset;
    };

    class OBBArchive
        : public Interface::Archive::IArchive
    {
    protected:
        std::filesystem::path m_obb_file_path;
        std::unordered_map<std::filesystem::path, std::tuple<void*, size_t>> m_file_buffer_cache_map;
        std::unordered_map<std::string, ZipFileEntry> m_zip_entries;
        mutable std::ifstream m_obb_file;
        bool m_is_valid;
    public:
        OBBArchive(std::filesystem::path obb_file_path)
            : m_obb_file_path(obb_file_path), m_is_valid(false)
        {
            parseOBBFile();
        }

        ~OBBArchive()
        {
            clearCache();
            if (m_obb_file.is_open()) {
                m_obb_file.close();
            }
        }

        std::tuple<void*, size_t> getFileBuffer(const std::filesystem::path& relative_path) override;

        void clearCache();
        bool isValid() const { return m_is_valid; }

    private:
        void parseOBBFile();
        bool findCentralDirectory();
        bool parseCentralDirectory(uint32_t cd_offset, uint32_t cd_size, uint16_t total_entries);
    };

    class OBBArchiveManager
        : public Interface::Archive::IArchiveManager
    {
    public:
        void initialize()
        {

        }

        void finalize()
        {

        }
    public:
        Interface::Archive::IArchive* createArchive(const std::filesystem::path& obb_file_path) override
        {
            OBBArchive* created_archive = Base::newT<OBBArchive>(obb_file_path);
            if(created_archive->isValid() == false)
            {
                Base::deleteT(created_archive);
                return nullptr;
            }
            return created_archive;
        }

        void destroyArchive(Interface::Archive::IArchive* archive) override
        {
            OBBArchive* obb_archive = Base::castInterfaceToInstance<OBBArchive>(archive);
            return Base::deleteT(obb_archive);
        }
    };
}