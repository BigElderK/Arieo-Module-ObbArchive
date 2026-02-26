#include "base/prerequisites.h"
#include "core/core.h"
#include "obb_archive.h"
namespace Arieo
{
    GENERATOR_MODULE_ENTRY_FUN()
    ARIEO_DLLEXPORT void ModuleMain()
    {
        Core::Logger::setDefaultLogger("obb_archive");

        static struct DllLoader
        {
            Base::Instance<OBBArchiveManager> obb_archive_manager;

            DllLoader()
            {
                obb_archive_manager->initialize();
                Core::ModuleManager::registerInstance<Interface::Archive::IArchiveManager, OBBArchiveManager>(
                    "obb_archive", 
                    obb_archive_manager
                );
            }

            ~DllLoader()
            {
                Core::ModuleManager::unregisterInstance<Interface::Archive::IArchiveManager, OBBArchiveManager>(
                    obb_archive_manager
                );
                obb_archive_manager->finalize();
            }
        } dll_loader;
    }
}