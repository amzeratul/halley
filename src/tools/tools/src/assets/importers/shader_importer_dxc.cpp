#include "shader_importer_dxc.h"

#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/shader.h"

#ifdef _MSC_VER
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#endif

#include "shader_importer_dxc.inl"

using namespace Halley;

static DxcCreateInstanceProc getDxcCreateInstanceFunction(const char* dllName)
{
	// NOTE: This leaks the DLL module, FreeLibrary() is never called.

	HMODULE dll = LoadLibraryA(dllName);
	DxcCreateInstanceProc fn = nullptr;

	if (dll != nullptr) {
	    void* addr = (void*) GetProcAddress(dll, "DxcCreateInstance");
		fn = (DxcCreateInstanceProc) addr;

		if (fn == nullptr) {
			Logger::logWarning("DxcCreateInstance() not found in " + String(dllName), true);
		}
	} else {
		Logger::logWarning("Failed to load DLL: " + String(dllName), true);
	}

	return fn;
}

Bytes ShaderImporterDXC::compileDXIL(const String& name, ShaderType type, const Bytes& bytes, const String& language, const MaterialDefinition& material) {
#ifdef _MSC_VER
    if (language == "dxil") {
    	static DxcCreateInstanceProc createInstanceFn = getDxcCreateInstanceFunction("dxcompiler.dll");
    	if (createInstanceFn != nullptr) {
    		return compileDXILCommon(createInstanceFn, name, type, bytes, language, material);
    	}
    }

#ifdef HAS_DXC_X
	if (language == "dxbc_x") {
		static DxcCreateInstanceProc createInstanceFn = getDxcCreateInstanceFunction("dxcompiler_x.dll");
    	if (createInstanceFn != nullptr) {
    		return compileDXILCommon(createInstanceFn, name, type, bytes, language, material);
    	}
    }
#endif

#ifdef HAS_DXC_XS
	if (language == "dxbc_xs") {
		static DxcCreateInstanceProc createInstanceFn = getDxcCreateInstanceFunction("dxcompiler_xs.dll");
    	if (createInstanceFn != nullptr) {
    		return compileDXILCommon(createInstanceFn, name, type, bytes, language, material);
    	}
    }
#endif

	return {};

#else
    Logger::logWarning("Compiling DXIL shaders is not supported on non-Windows platforms.");
	return Bytes();
#endif
}
