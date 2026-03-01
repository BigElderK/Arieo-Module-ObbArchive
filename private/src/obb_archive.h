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
    class FileBuffer final
        : public Base::IBuffer
    {
    protected:
        void* m_buffer = nullptr;
        size_t m_size = 0;
    public:
        FileBuffer(void* buffer, size_t size)
            : m_buffer(buffer), m_size(size)
        {
        }

        ~FileBuffer()
        {
            if(m_buffer)
            {
                Base::Memory::free(m_buffer);
                m_buffer = nullptr;
            }
        }

        void* getBuffer() override { return m_buffer; }
        size_t getBufferSize() override { return m_size; }
    };

    
    struct ZipFileEntry {
        std::string filename;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint32_t crc32;
        uint16_t compression_method;
        uint32_t file_data_offset;
    };

    class OBBArchive final
        : public Interface::Archive::IArchive
    {
    protected:
        std::filesystem::path m_obb_file_path;
        std::unordered_set<Base::Interop::SharedRef<Base::IBuffer>> m_file_buffers;
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

        Base::Interop::SharedRef<Base::IBuffer> aquireFileBuffer(const Base::Interop::StringView& related_path) override;

        void clearCache();
        bool isValid() const { return m_is_valid; }

    private:
        void parseOBBFile();
        bool findCentralDirectory();
        bool parseCentralDirectory(uint32_t cd_offset, uint32_t cd_size, uint16_t total_entries);
    };

    class OBBArchiveManager final
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
        Base::Interop::SharedRef<Interface::Archive::IArchive> createArchive(const Base::Interop::StringView& root_path) override
        {
            std::filesystem::path obb_path(root_path.getString());
            // Check if obb_file_path exists and is a regular file
            if(std::filesystem::exists(obb_path) == false || std::filesystem::is_regular_file(obb_path) == false)
            {
                Core::Logger::error("Invalid OBB file path: {}", obb_path.string());
                return nullptr;
            }
            
            Base::Interop::SharedRef<Interface::Archive::IArchive> created_archive 
                = Base::Interop::createInstance<Interface::Archive::IArchive, OBBArchive>(obb_path);
            
            if(created_archive.castToInstance<OBBArchive>()->isValid() == false)
            {
                return nullptr;
            }
            return created_archive;
        }
    };
}




