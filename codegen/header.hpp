auto constexpr fwdHeader = 1 + R"SRC(
#include "flags.hpp"
#include "handle.hpp"

#include <vulkan/vk_platform.h>

)SRC";


auto constexpr mainHeader = 1 + R"SRC(
#include "fwd.hpp"
#include "enums.hpp"
#include "structs.hpp"
#include "functions.hpp"

)SRC";

auto constexpr structsHeader = 1 + R"SRC(
#include "fwd.hpp"
#include "enums.hpp"

#include <array>
#include <vulkan/vulkan.h>

)SRC";

auto constexpr enumsHeader = 1 + R"SRC(
#include "fwd.hpp"

)SRC";

auto constexpr functionsHeader = 1 + R"SRC(
#include "fwd.hpp"
#include "enums.hpp"
#include "structs.hpp"
#include "result.hpp"
#include "range.hpp"

#include <vector>
#include <vulkan/vulkan.h>

)SRC";



//function templates
//normal
//if vk func signature is VkResult(...) without out param
//function return type is Result<void>
auto constexpr resultTemplate = 1 + R"SRC(
	auto res = %f(%a);
	return {res, VPP_FUNC_NAME};
)SRC";

//if vk func signature is void(...) without out param
//function return type is void
auto constexpr voidTemplate = 1 + R"SRC(
	%f(%a);
)SRC";

//if vk func signature is VkResult(...) with out param
//function return type is Result<*OutParamType*>
auto constexpr retResultTemplate = 1 + R"SRC(
	%t ret;
	auto res = %f(%a);
	return {ret, res, VPP_FUNC_NAME};
)SRC";

//if vk func signature is void(...) with out param
//function return type is *OutParamType*
auto constexpr retTemplate = 1 + R"SRC(
	%t ret;
	%f(%a);
	return ret;
)SRC";

//vec
//if vk func signature is VkResult(...) with out vec param
//function return type is Result<std::vector<*OutParamType*>>
auto constexpr vecFuncTemplate = 1 + R"SRC(
	std::vector<%t> ret;
	%ct count = 0u;
	auto res = %f(%a);
	if(!success(res)) return {res, VPP_FUNCTION_NAME}
	ret.resize(count);
	auto res = %f(%a);
	return {std::move(ret), res, function};
)SRC";

//if vk func signature is void(...) with out vec param
//function return type is std::vector<*OutParamType*>
auto constexpr vecFuncTemplateVoid = 1 + R"SRC(
	std::vector<%t> ret;
	%ct count = 0u;
	%f(%a);
	ret.resize(count);
	%f(%a);
	return ret;
)SRC";

//if vk func signature is VkResult(...) with out vec param which size is known.
//function return type is Result<std::vector<*OutParamType*>>
auto constexpr vecFuncTemplateRetGiven = 1 + R"SRC(
	std::vector<%t> ret;
	ret.resize(%c);
	auto res = %f(%a);
	if(!success(res)) return {res, VPP_FUNCTION_NAME};
	return {ret, res, VPP_FUNCTION_NAME};
)SRC";

//if vk func signature is void(...) with out vec param which size is known
//function return type is std::vector<*OutParamType*>
auto constexpr vecFuncTemplateRetGivenVoid = 1 + R"SRC(
	std::vector<%t> ret;
	ret.resize(%c);
	%f(%a);
	return ret;
)SRC";


//all c++ keywords that must be prefixes with an 'e' as enum
constexpr const char* keywords[]= {"alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand",
	"bitor", "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class", "compl",
	"const", "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do",
	"double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
	"for","friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
	"noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
	"protected public", "register", "reinterpret_cast","return","short","signed","sizeof",
	"static","static_assert","static_cast","struct", "switch", "template", "this","thread_local",
	"throw","true","try","typedef","typeid","typename","union","unsigned", "using","virtual",
	"void", "volatile","wchar_t","while","xor","xor_eq"};
