#include "editor_file_system.h"
#include "core/config/project_settings.h"

void EditorFileSystem::register_lazy_import_setting() {
	// Add a new project setting for lazy import mode
	GLOBAL_DEF("editor/import/use_lazy_import", false);

	ResourceImporter::lazy_import_mode = GLOBAL_GET("editor/import/use_lazy_import");
}