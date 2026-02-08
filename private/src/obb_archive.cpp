#include "base/prerequisites.h"
#include "core/core.h"

#include "obb_archive.h"

namespace Arieo
{
    std::tuple<void*, size_t> OBBArchive::getFileBuffer(const std::filesystem::path& relative_path)
    {
        auto found_cache_iter = m_file_buffer_cache_map.find(relative_path);
        if(found_cache_iter != m_file_buffer_cache_map.end())
        {
            return found_cache_iter->second;
        }
        else
        {
            if (!m_is_valid) {
                Core::Logger::error("OBB file is not valid: {}", m_obb_file_path.string());
                return std::make_tuple(nullptr, 0);
            }

            std::string path_str = relative_path.string();
            auto entry_iter = m_zip_entries.find(path_str);
            if (entry_iter == m_zip_entries.end()) {
                Core::Logger::error("File not found in OBB: {}", path_str);
                return std::make_tuple(nullptr, 0);
            }

            const ZipFileEntry& entry = entry_iter->second;
            
            // Read compressed data from OBB file
            m_obb_file.seekg(entry.file_data_offset, std::ios::beg);
            std::vector<uint8_t> compressed_data(entry.compressed_size);
            m_obb_file.read(reinterpret_cast<char*>(compressed_data.data()), entry.compressed_size);

            void* buffer = nullptr;
            size_t buffer_size = 0;

            if (entry.compression_method == 0) {
                // No compression - store method
                buffer_size = entry.uncompressed_size;
                buffer = Base::Memory::malloc(buffer_size);
                std::memcpy(buffer, compressed_data.data(), buffer_size);
            } else if (entry.compression_method == 8) {
                // Deflate compression
                buffer_size = entry.uncompressed_size;
                buffer = Base::Memory::malloc(buffer_size);
                
                z_stream strm = {};
                strm.next_in = compressed_data.data();
                strm.avail_in = entry.compressed_size;
                strm.next_out = static_cast<uint8_t*>(buffer);
                strm.avail_out = buffer_size;

                if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
                    Core::Logger::error("Failed to initialize zlib for: {}", path_str);
                    Base::Memory::free(buffer);
                    return std::make_tuple(nullptr, 0);
                }

                int ret = inflate(&strm, Z_FINISH);
                inflateEnd(&strm);

                if (ret != Z_STREAM_END) {
                    Core::Logger::error("Failed to decompress file: {}", path_str);
                    Base::Memory::free(buffer);
                    return std::make_tuple(nullptr, 0);
                }
            } else {
                Core::Logger::error("Unsupported compression method {} for file: {}", entry.compression_method, path_str);
                return std::make_tuple(nullptr, 0);
            }

            // Cache the buffer
            m_file_buffer_cache_map.emplace(relative_path, std::make_tuple(buffer, buffer_size));
            return std::make_tuple(buffer, buffer_size);
        }
    }

    void OBBArchive::clearCache()
    {
        for(auto iter : m_file_buffer_cache_map)
        {
            void* buffer = std::get<0>(iter.second);
            Base::Memory::free(buffer);
        }
        m_file_buffer_cache_map.clear();
    }

    void OBBArchive::parseOBBFile()
    {
        m_obb_file.open(m_obb_file_path, std::ios::binary);
        if (!m_obb_file.is_open()) {
            Core::Logger::error("Cannot open OBB file: {}", m_obb_file_path.string());
            return;
        }

        // Find End of Central Directory record
        if (!findCentralDirectory()) {
            Core::Logger::error("Invalid OBB file format: {}", m_obb_file_path.string());
            return;
        }

        m_is_valid = true;
        Core::Logger::info("Successfully loaded OBB file with {} entries", m_zip_entries.size());
    }

    bool OBBArchive::findCentralDirectory()
    {
        // Look for End of Central Directory signature (0x06054b50) from the end of file
        m_obb_file.seekg(0, std::ios::end);
        std::streamsize file_size = m_obb_file.tellg();
        
        const uint32_t EOCD_SIGNATURE = 0x06054b50;
        const size_t EOCD_MIN_SIZE = 22;
        const size_t SEARCH_BUFFER_SIZE = std::min(static_cast<size_t>(file_size), size_t(65557)); // Max comment size + EOCD size
        
        m_obb_file.seekg(-static_cast<std::streamoff>(SEARCH_BUFFER_SIZE), std::ios::end);
        std::vector<uint8_t> buffer(SEARCH_BUFFER_SIZE);
        m_obb_file.read(reinterpret_cast<char*>(buffer.data()), SEARCH_BUFFER_SIZE);

        // Search for EOCD signature backwards
        for (int i = SEARCH_BUFFER_SIZE - EOCD_MIN_SIZE; i >= 0; i--) {
            uint32_t signature = *reinterpret_cast<const uint32_t*>(&buffer[i]);
            if (signature == EOCD_SIGNATURE) {
                // Found EOCD, extract central directory info
                uint16_t total_entries = *reinterpret_cast<const uint16_t*>(&buffer[i + 10]);
                uint32_t cd_size = *reinterpret_cast<const uint32_t*>(&buffer[i + 12]);
                uint32_t cd_offset = *reinterpret_cast<const uint32_t*>(&buffer[i + 16]);
                
                return parseCentralDirectory(cd_offset, cd_size, total_entries);
            }
        }
        return false;
    }

    bool OBBArchive::parseCentralDirectory(uint32_t cd_offset, uint32_t cd_size, uint16_t total_entries)
    {
        m_obb_file.seekg(cd_offset, std::ios::beg);
        
        for (uint16_t i = 0; i < total_entries; i++) {
            ZipCentralDirectoryHeader header;
            m_obb_file.read(reinterpret_cast<char*>(&header), sizeof(header));
            
            if (header.signature != 0x02014b50) { // Central directory signature
                Core::Logger::error("Invalid central directory entry signature");
                return false;
            }

            // Read filename
            std::string filename(header.filename_length, '\0');
            m_obb_file.read(&filename[0], header.filename_length);

            // Skip extra field and comment
            m_obb_file.seekg(header.extra_length + header.comment_length, std::ios::cur);

            // Create entry
            ZipFileEntry entry;
            entry.filename = filename;
            entry.compressed_size = header.compressed_size;
            entry.uncompressed_size = header.uncompressed_size;
            entry.crc32 = header.crc32;
            entry.compression_method = header.compression;
            
            // Calculate actual file data offset (skip local header)
            std::streampos current_pos = m_obb_file.tellg();
            m_obb_file.seekg(header.local_header_offset, std::ios::beg);
            
            ZipLocalFileHeader local_header;
            m_obb_file.read(reinterpret_cast<char*>(&local_header), sizeof(local_header));
            entry.file_data_offset = header.local_header_offset + sizeof(ZipLocalFileHeader) + 
                                   local_header.filename_length + local_header.extra_length;

            m_obb_file.seekg(current_pos, std::ios::beg);

            // Store entry
            m_zip_entries[filename] = entry;
        }
        
        return true;
    }
}