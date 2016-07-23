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

	constexpr bool valid() const { return function && call::checkResult(result); }
	constexpr T unwrap() const
	{
		if(!valid()) throw VulkanError(function, result);
		function = nullptr;
		return std::move(object);
	}

	constexpr operator bool() const { return valid(); }
	constexpr operator T() const { return unwrap(); }
};

template<>
class VulkanResult<void>
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

	constexpr vk::Result handle() const { return result; function = nullptr; }
	constexpr bool valid() const { return function && call::checkResult(result); }
	constexpr T unwrap() const
	{
		if(!valid()) throw VulkanError(function, result);
		function = nullptr;
		return std::move(object);
	}

	constexpr operator bool() const { return valid(); }
	constexpr operator T() const { return unwrap(); }
};



//The result from a vulkan function call must be handled by the caller.
auto instanceResult = vk::createInstance({});
if(!instanceResult)
{
	... //handle the error here
}

//otherwise the returned object can be retrieved by unwrapping the result.
auto instance = instanceResult().unwrap();

//Alternatively this can be done in one call but throw if there was an error.
instance = vk::createInstance({}).unwrap(); //will throw if error

//If the vulkan call result is not handled in any way (i.e. it is not unwrapped or handled)
//and VPP_CALL_THROW is defined, the result will throw on destruction.
{

//Here, The returned result is immediatly destructed.
//Therefore it will throw here on error and if VPP_CALL_THROW is defined.
vk::resetCommandPool(dev, pool, {});


//In this case the result is retrieved.
auto result = vk::resetCommandPool(dev, pool, {});
... //If the error is handled here or the result is unwrapped the desctruction will not throw

//otherwise the implicit destruction when going out of scope will throw a vulkan error if
//the call failed and VPP_CALL_THROW is defined.
}
