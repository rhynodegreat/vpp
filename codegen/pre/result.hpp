// Copyright © 2016 nyorain
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the “Software”), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "fwd.hpp"

//check call macro
#define VPP_CHECK_CALL(x) { auto res = x; if(!res) return {res, VPP_FUNC_NAME}; }

//macro for querying the current function name on all platforms
#if defined(__GNUC__) || defined(__clang__)
 #define VPP_FUNC_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
 #define VPP_FUNC_NAME __FUNCTION__
#else
 #define VPP_FUNC_NAME __func__
#endif

namespace vk
{

inline bool success(Result result) { return (static_cast<std::int64_t>(result) >= 0); }
inline std::string to_string(Result result)
{
    switch(result)
    {
	    case Result::success: return "success";
	    case Result::notReady: return "notReady";
	    case Result::timeout: return "timeout";
	    case Result::eventSet: return "eventSet";
	    case Result::eventReset: return "eventReset";
	    case Result::incomplete: return "incomplete";
	    case Result::errorOutOfHostMemory: return "errorOutOfHostMemory";
	    case Result::errorOutOfDeviceMemory: return "errorOutOfDeviceMemory";
	    case Result::errorInitializationFailed: return "errorInitializationFailed";
	    case Result::errorDeviceLost: return "errorDeviceLost";
	    case Result::errorMemoryMapFailed: return "errorMemoryMapFailed";
	    case Result::errorLayerNotPresent: return "errorLayerNotPresent";
	    case Result::errorExtensionNotPresent: return "errorExtensionNotPresent";
	    case Result::errorFeatureNotPresent: return "errorFeatureNotPresent";
	    case Result::errorIncompatibleDriver: return "errorIncompatibleDriver";
	    case Result::errorTooManyObjects: return "errorTooManyObjects";
	    case Result::errorFormatNotSupported: return "errorFormatNotSupported";
	    case Result::errorSurfaceLostKHR: return "errorSurfaceLostKHR";
	    case Result::errorNativeWindowInUseKHR: return "errorNativeWindowInUseKHR";
	    case Result::suboptimalKHR: return "suboptimalKHR";
	    case Result::errorOutOfDateKHR: return "errorOutOfDateKHR";
	    case Result::errorIncompatibleDisplayKHR: return "errorIncompatibleDisplayKHR";
	    case Result::errorValidationFailedEXT: return "errorValidationFailedEXT";
	    default: return "unknown";
    }
}

template<typename T>
class Result
{
public:
	T object;
	vk::Result result;
	const char* function;

public:
	VulkanResult(vk::Result xresult, const char* xfunction)
		: object(), result(xresult), function(xfunction) {}

	VulkanResult(T&& xobject, vk::Result xresult, const char* xfunction)
		: object(std::move(xobject)), result(xresult), function(xfunction) {}

	~VulkanResult()
	{
		#ifdef VPP_CALL_THROW
		 if(!valid() && function) throw VulkanError();
		#endif
	}

	///Returns the stored vulkan result.
	///This function also signals the result that the result is handled and it therefore not has
	///to throw on destruction if the result is an error.
	vk::Result handle() const { function = nullptr; return result; }

	///Returns whether the Result comes from a successful vulkan call.
	bool valid() const { return success(result); }

	///Unwraps the result, i.e. returns the stored object.
	///\exception VulkanError If there is no such object because the vulkan call failed
	///\exception MulitpleUnwrap If the result was already unwrapped
	T unwrap() const
	{
		if(!valid()) throwVulkanError();
		if(!function) throw MultipleUnwrap{};
		function = nullptr;
		return std::move(object);
	}

	///Calls valid()
	operator bool() const { return valid(); }

	///Calls unwrap()
	operator T() const { return unwrap(); }

protected:
	void throwVulkanError()
	{
		std::string msg = "Vulkan function call '";
		msg += function;
		msg += "' failed: " + to_string(result);
		throw VulkanError(msg, result);
	}
};

template<>
class VulkanResult<void>
{
public:
	vk::Result result;
	const char* function;

public:
	VulkanResult(vk::Result xresult, const char* xfunction)
		: object(), result(xresult), function(xfunction) {}

	~VulkanResult()
	{
		#ifdef VPP_CALL_THROW
		 if(!valid() && function) throwVulkanError();
		#endif
	}

	///Returns the stored vulkan result.
	///This function also signals the result that the result is handled and it therefore not has
	///to throw on destruction if the result is an error.
	vk::Result handle() const { function = nullptr; return result; }

	///Returns whether the Result comes from a successful vulkan call.
	bool valid() const { return function && success(result); }

	///Unwraps the result, i.e. returns the stored object.
	///\exception VulkanError If there is no such object because the vulkan call failed
	///\exception MulitpleUnwrap If the result was already unwrapped
	void unwrap() const
	{
		if(!valid()) throwVulkanError();
		if(!function) throw MultipleUnwrap{};
		function = nullptr;
	}

	///Calls valid()
	operator bool() const { return valid(); }

protected:
	void throwVulkanError()
	{
		std::string msg = "Vulkan function call '";
		msg += function;
		msg += "' failed: " + to_string(result);
		throw VulkanError(msg, result);
	}
};

}
