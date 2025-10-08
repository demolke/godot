#include "resource_importer.h"
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"

bool ResourceFormatImporter::_test_reimport_metadata(const String &p_path, const PathAndType &p_pat) const {
	// Check if import file needs reimport by validating:
	// 1. Source file modification time 
	// 2. Import file modification time
	// 3. Import destination files exist
	// 4. Import settings hash matches

	if (!FileAccess::exists(p_path + ".import")) {
		return true; // No import file, needs reimport
	}

	uint64_t source_modified = FileAccess::get_modified_time(p_path);
	uint64_t import_modified = FileAccess::get_modified_time(p_path + ".import");

	if (source_modified > import_modified) {
		return true; // Source newer than import
	}

	// Check all imported files exist
	for (int i = 0; i < p_pat.imported_files.size(); i++) {
		if (!FileAccess::exists(p_pat.imported_files[i])) {
			return true; // Missing imported file
		}
	}

	// Validate import settings
	if (!p_pat.importer.is_empty()) {
		Ref<ResourceImporter> importer = get_importer_by_name(p_pat.importer); 
		if (importer.is_valid() && !importer->are_import_settings_valid(p_path, p_pat.metadata)) {
			return true; // Import settings changed
		}
	}

	return false; // No reimport needed
}