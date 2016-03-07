#pragma once

#include <vpp/vk.hpp>
#include <vpp/fwd.hpp>
#include <vpp/resource.hpp>

namespace vpp
{

vk::ShaderModule loadShader(vk::Device dev, const std::string& file);

///Wrapper around the one single shader stage module.
class ShaderStage : public Resource
{
public:
	struct CreateInfo
	{
		std::string filename;
		vk::ShaderStageFlagBits stage;
		const vk::SpecializationInfo* specializationInfo {nullptr};
		std::string entry { u8"main"};
	};

protected:
	vk::PipelineShaderStageCreateInfo stageInfo_;

public:
	ShaderStage() = default;
	ShaderStage(const Device& device, const CreateInfo& info);
	~ShaderStage();

	ShaderStage(ShaderStage&& other) noexcept;
	ShaderStage& operator=(ShaderStage&& other) noexcept;

	void init(const Device& device, const CreateInfo& info);
	void destroy();

	const vk::PipelineShaderStageCreateInfo& vkStageInfo() const { return stageInfo_; }
};

///ShaderProgram with multiple stages for graphic pipelines.
class ShaderProgram : public Resource
{
protected:
	std::vector<ShaderStage> stages_;

public:
	ShaderProgram() = default;
	ShaderProgram(const Device& device, const std::vector<ShaderStage::CreateInfo>& infos = {});
	~ShaderProgram() = default;

	ShaderProgram(ShaderProgram&& other) noexcept = default;
	ShaderProgram& operator=(ShaderProgram&& other) noexcept = default;

	void init(const Device& device, const std::vector<ShaderStage::CreateInfo>& infos = {});
	void destroy();

	void addStage(const ShaderStage::CreateInfo& createInfo);
	void addStages(const std::vector<ShaderStage::CreateInfo>& crrateInfo);

	const std::vector<ShaderStage>& stages() const { return stages_; }
	std::vector<vk::PipelineShaderStageCreateInfo> vkStageInfos() const;
};

}
