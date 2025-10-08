#include "resource_importer.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"

ResourceImporter::ImportState ResourceFormatImporter::_validate_import_state(const String &p_path, PathAndType &r_pat) const {
    // Quick check first
    if (!FileAccess::exists(p_path + ".import")) {
        return ResourceImporter::IMPORT_STATE_NEEDS_REIMPORT;
    }

    // Get file modification times
    r_pat.source_modified_time = FileAccess::get_modified_time(p_path);
    r_pat.import_modified_time = FileAccess::get_modified_time(p_path + ".import");

    // If source is newer than import, needs reimport
    if (r_pat.source_modified_time > r_pat.import_modified_time) {
        return ResourceImporter::IMPORT_STATE_NEEDS_REIMPORT;
    }

    // Load and validate import metadata
    Ref<ConfigFile> config;
    config.instantiate();
    Error err = config->load(p_path + ".import");
    if (err != OK) {
        return ResourceImporter::IMPORT_STATE_INVALID;
    }

    // Get importer and verify it exists
    String importer_name = config->get_value("remap", "importer", "");
    if (importer_name.is_empty()) {
        return ResourceImporter::IMPORT_STATE_INVALID;
    }

    Ref<ResourceImporter> importer = get_importer_by_name(importer_name);
    if (importer.is_null()) {
        return ResourceImporter::IMPORT_STATE_INVALID;
    }

    // Check import settings are still valid
    if (!importer->are_import_settings_valid(p_path, r_pat.metadata)) {
        return ResourceImporter::IMPORT_STATE_NEEDS_REIMPORT;
    }

    // Check all imported files exist
    bool valid = config->get_value("remap", "valid", false);
    if (!valid) {
        return ResourceImporter::IMPORT_STATE_INVALID;
    }

    for (const String &path : r_pat.imported_files) {
        if (!FileAccess::exists(path)) {
            return ResourceImporter::IMPORT_STATE_NEEDS_REIMPORT;
        }
    }

    // Get and validate dependencies
    if (config->has_section("deps")) {
        Array deps = config->get_value("deps", "files", Array());
        for (int i = 0; i < deps.size(); i++) {
            r_pat.dependencies.push_back(deps[i]);
            // Note: we don't recursively validate dependencies here
            // That's handled during actual import
        }
    }

    return ResourceImporter::IMPORT_STATE_VALID;
}

bool ResourceFormatImporter::_test_quick_reimport(const String &p_path, const PathAndType &p_pat) const {
    // Quick check without loading full metadata
    if (!FileAccess::exists(p_path + ".import")) {
        return true;
    }

    uint64_t source_modified = FileAccess::get_modified_time(p_path);
    uint64_t import_modified = FileAccess::get_modified_time(p_path + ".import");

    if (source_modified > import_modified) {
        return true;
    }

    // Check imported files exist
    for (const String &path : p_pat.imported_files) {
        if (!FileAccess::exists(path)) {
            return true;
        }
    }

    return false;
}