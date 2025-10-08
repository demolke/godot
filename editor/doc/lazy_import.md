Lazy Import Feature
================

The lazy import feature in Godot allows for improved startup performance by deferring the actual import of resources until they are first used, rather than importing everything during project startup.

How it Works
-----------

1. When enabled via `editor/import/use_lazy_import` in project settings, resource importing is deferred until first use.

2. During filesystem scanning:
   - Import metadata is still validated
   - Files are marked as needing reimport if modified
   - Actual import is skipped until first use

3. During resource loading:
   - First use triggers import for modified files
   - Import results are cached as usual after first import
   - Subsequent uses use the cached imported resource 

Benefits
--------

- Faster project startup time
- Reduced initial memory usage
- Imports only what is actually needed
- Preserves all import validation and dependency tracking

Usage
-----

Enable in Project Settings:
- Set `editor/import/use_lazy_import` to true
- Project must be restarted for changes to take effect

Note that first use of a resource will trigger import, which may cause a brief pause. Consider using resource pre-loading for frequently accessed resources that need to load quickly.