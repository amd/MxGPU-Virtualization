/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

/*
 * Owns one dynamically loaded AMDSMI library for the lifetime of the CLI
 * host or guest API object. Call sites resolve symbols without repeating
 * LoadLibrary/dlopen or FreeLibrary/dlclose.
 */
class SmiDynamicLoader
{
public:
	SmiDynamicLoader(const char* win_dll_name, const char* linux_so_name,
			 bool global_scope = false);
	~SmiDynamicLoader();

	SmiDynamicLoader(const SmiDynamicLoader&)	     = delete;
	SmiDynamicLoader& operator=(const SmiDynamicLoader&) = delete;
	SmiDynamicLoader(SmiDynamicLoader&&)		     = delete;
	SmiDynamicLoader& operator=(SmiDynamicLoader&&)	     = delete;

	void* get_symbol(const char* name) const;
	bool is_loaded() const;
	const char* library_name() const;

private:
	void* handle_ {nullptr};
	const char* name_ {nullptr};
};
