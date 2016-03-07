#include <vpp/image.hpp>
#include <utility>

namespace vpp
{

Image::Image(const Device& dev, const vk::ImageCreateInfo& info, vk::MemoryPropertyFlags mflags)
	 : Resource(dev)
{
	vk::MemoryRequirements reqs;
	vk::createImage(vkDevice(), &info, nullptr, &image_);
	vk::getImageMemoryRequirements(vkDevice(), image_, &reqs);

	auto type = device().memoryType(reqs.memoryTypeBits(), mflags);
	if(type == -1)
	{
		throw std::runtime_error("vpp::Image: no matching deviceMemoryType");
	}

	auto memory = std::make_shared<DeviceMemory>(dev, reqs.size(), type);

	auto alloc = memory->alloc(reqs.size(), reqs.alignment());
	memoryEntry_ = DeviceMemory::Entry(memory, alloc);

	vk::bindImageMemory(vkDevice(), image_, memory->vkDeviceMemory(), alloc.offset);
}

Image::Image(DeviceMemoryAllocator& allctr, const vk::ImageCreateInfo& info, vk::MemoryPropertyFlags mflags)
	: Resource(allctr.device())
{
	vk::MemoryRequirements reqs;
	vk::createImage(vkDevice(), &info, nullptr, &image_);
	vk::getImageMemoryRequirements(vkDevice(), image_, &reqs);

	reqs.memoryTypeBits(device().memoryTypeBits(reqs.memoryTypeBits(), mflags));
	allctr.request(image_, reqs, info.tiling(), memoryEntry_);
}

Image::Image(Image&& other) noexcept
{
	this->swap(other);
}

Image& Image::operator=(Image&& other) noexcept
{
	Image swapper(std::move(other));
	this->swap(swapper);

	return *this;
}

Image::~Image()
{
	destroy();
}

void Image::swap(Image& other) noexcept
{
	using std::swap;

	swap(memoryEntry_, other.memoryEntry_);
	swap(image_, other.image_);
	swap(device_, other.device_);
}

void Image::destroy()
{
	if(vkImage()) vk::destroyImage(vkDevice(), vkImage(), nullptr);

	memoryEntry_ = {};
	image_ = {};
}

MemoryMap Image::memoryMap() const
{
	return MemoryMap(memoryEntry_);
}

}
