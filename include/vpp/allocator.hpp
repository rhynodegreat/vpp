#pragma once

#include <vpp/fwd.hpp>
#include <vpp/resource.hpp>
#include <vpp/memory.hpp>
#include <vpp/utility/allocation.hpp>

#include <memory>
#include <map>
#include <vector>

namespace vpp
{

//TODO: make DMA::Entry to class MemoryEntry (only typedef now)
///Makes it possible to allocate a few vk::DeviceMemory objects for many buffers/images.
///Basically a memory pool
class DeviceMemoryAllocator : public Resource
{
public:
	class Entry : public ResourceReference<Entry>
	{
	public:
		Entry() = default;
		Entry(DeviceMemory* memory, const Allocation& alloc); //XXX: needed? allowed? reasonable?
		~Entry();

		Entry(Entry&& other) noexcept;
		Entry& operator=(Entry&& other) noexcept;

		///Will try to map the Memory and return a view to the location where this entry is placed.
		///Throws a std::logic_error if the DeviceMemory is not mappable.
		MemoryMapView map() const;

		///Returns whether memory on the device was allocated for this entry.
		bool allocated() const { return (allocation_.size > 0); }

		///Assures that there is memory allocated and associated with this entry.
		void allocate() const { if(!allocated()) allocator_->allocate(*this); }

		DeviceMemory* memory() const { return allocated() ? memory_ : nullptr; };
		DeviceMemoryAllocator* allocator() const { return allocated() ? nullptr : allocator_; };
		std::size_t offset() const { return allocation_.offset; };
		std::size_t size() const { return allocation_.size; }
		const Allocation& allocation() const { return allocation_; }

		const Resource& resourceRef() const
			{ if(allocated()) return *memory_; else return *allocator_; }
		friend void swap(Entry& a, Entry& b) noexcept;

	protected:
		void free();

	protected:
		friend class DeviceMemoryAllocator;

		//if there is an allocation associated with this entry (the allocation size is > 0)
		//the memory member wil be active, otherwise the allocator.
		union
		{
			DeviceMemoryAllocator* allocator_ {};
			DeviceMemory* memory_;
		};

		Allocation allocation_ {};
	};

public:
	DeviceMemoryAllocator() = default;
	DeviceMemoryAllocator(const Device& dev);
	~DeviceMemoryAllocator();

	DeviceMemoryAllocator(DeviceMemoryAllocator&& other) noexcept;
	DeviceMemoryAllocator& operator=(DeviceMemoryAllocator&& other) noexcept;

	///Requests memory for the given vulkan buffer and stores a (pending) reference to it into
	///the given entry.
	void request(vk::Buffer requestor, const vk::MemoryRequirements& reqs, Entry& entry);

	///Requests memory for the given vulkan image and stores a (pending) reference to it into
	///the given entry. It additionally requires the tiling of the image to fulfill vulkans
	///granularity requiremens.
	void request(vk::Image requestor,  const vk::MemoryRequirements& reqs, vk::ImageTiling tiling,
		Entry& entry);

	///Removes the (pending) request from this allocator. Returns false if the given entry could
	///not be found.
	bool removeRequest(const Entry& entry);

	///This function will be called when a stored entry is moved.
	///Will return false if the given entry is not found.
	bool moveEntry(const Entry& oldOne, Entry& newOne);

	///Allocates and associated device memory for all pending requests.
	void allocate();

	///Makes sure that the given entry has associated memory.
	///Will return false if the given entry cannot be found.
	bool allocate(const Entry& entry);

	///Returns all memories that this allocator manages.
	std::vector<DeviceMemory*> memories() const;

	friend void swap(DeviceMemoryAllocator& a, DeviceMemoryAllocator& b) noexcept;

protected:
	enum class RequirementType
	{
		linearImage,
		optimalImage,
		buffer
	};

	struct Requirement
	{
		RequirementType type {};
		vk::DeviceSize size {};
		vk::DeviceSize alignment {};
		std::uint32_t memoryTypes {};
		Entry* entry {};
		union { vk::Buffer buffer; vk::Image image; }; //type determines which is active
	};

	using Requirements = std::vector<Requirement>;

protected:
	static AllocationType toAllocType(RequirementType reqType);
	static bool suppportsType(const Requirement& req, unsigned int type);
	static bool suppportsType(std::uint32_t bits, unsigned int type);

	//utility allocation functions
	void allocate(unsigned int type);
	void allocate(unsigned int type, const std::vector<Requirement*>& requirements);
	DeviceMemory* findMem(Requirement& req);
	Requirements::iterator findReq(const Entry& entry);
	std::map<unsigned int, std::vector<Requirement*>> queryTypes();
	unsigned int findBestType(std::uint32_t typeBits) const;

protected:
	Requirements requirements_;
	std::vector<std::unique_ptr<DeviceMemory>> memories_;
};

///Convinience typedef for a DeviceMemoryAllocator::Entry
using MemoryEntry = DeviceMemoryAllocator::Entry;

///Memory Resource initializer.
///Useful template class for easy and safe 2-step-resource initialization.
template<typename T> class MemoryResourceInitializer
{
public:
	///Constructs the underlaying resource with the given arguments.
	template<typename... Args>
	MemoryResourceInitializer(Args&&... args)
	{
		resource_.initMemoryLess(std::forward<Args>(args)...);
	};

	///Initialized the memory resources of the resource with the given arguments and returns
	///the completely intialized object. After this the Initializer object is invalid and if this
	///function will be called a second time, it will throw a std::logic_error, since it has
	///no more a resource to return.
	template<typename... Args>
	T init(Args&&... args)
	{
		if(!valid_) throw std::logic_error("Called MemoryResourceInitializer::init 2 times");

		valid_ = 0;
		resource_.initMemoryResources(std::forward<Args>(args)...);
		return std::move(resource_);
	}

protected:
	bool valid_ {1};
	T resource_;
};

}
