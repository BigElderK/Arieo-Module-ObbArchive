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
            OBBArchiveManager obb_archive_manager_instance;
            Base::Interop::SharedRef<Interface::Archive::IArchiveManager> obb_archive_manager = Base::Interop::makePersistentShared<Interface::Archive::IArchiveManager>(obb_archive_manager_instance);

            DllLoader()
            {
                obb_archive_manager_instance.initialize();
                Core::ModuleManager::registerInterface<Interface::Archive::IArchiveManager>(
                    "obb_archive",
                    obb_archive_manager
                );
            }

            ~DllLoader()
            {
                Core::ModuleManager::unregisterInterface<Interface::Archive::IArchiveManager>(
                    obb_archive_manager
                );
                obb_archive_manager_instance.finalize();
            }
        } dll_loader;
    }
}




