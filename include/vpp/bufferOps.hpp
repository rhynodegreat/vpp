#pragma once

#include <vpp/fwd.hpp>
#include <vpp/buffer.hpp>
#include <vpp/work.hpp>
#include <vpp/utility/range.hpp>
#include <vpp/utility/allocation.hpp>
#include <vpp/bits/apply.inl>

///\file Defines several utility operations for buffers such as updating or reading them.

namespace vpp
{

///Vulkan shader data types.
///Defines all possible types that can be passed as buffer update paramter.
enum class ShaderType
{
	none, //used for things like e.g. containers
	buffer, //used for buffer that shall be plain copied
	scalar,
	scalar_64,
	vec2,
	vec3,
	vec4,
	vec2_64,
	vec3_64,
	vec4_64,
	matrix, //Vulkan type has additional "major" and "minor" and "transpose" members
	structure //Vulkan type has additional "member" tuples with member pointers
};

///Returns the alignment the given ShaderType requires.
///Returns 0 for special values such as none, buffer, structure or matrix.
constexpr unsigned int align(ShaderType type);

///Specifies the different buffer alignment methods.
///For the differences, read https://www.opengl.org/wiki/Interface_Block_(GLSL)#Memory_layout.
///Uniform buffer are by default std140 while storage buffers are by default std430.
///Both defaults can be explicitly changed in the shader files using the buffers.
enum class BufferLayout
{
	std140,
	std430
};

///Base for classes that operator on a buffer such as BufferUpdate, BufferReader or BufferSizer.
///Uses the CRTP.
template<typename B>
class BufferOperator
{
public:
	using Size = std::size_t;

public:
	BufferOperator(BufferLayout align) : align_(align) {}
	~BufferOperator() = default;

	///Will operator on the given object. The type of the given object must have
	///a specialization for the VulkanType template struct that carriers information about
	///the corresponding shader variable type of the object to align it correctly.
	///The given object can also be a container/array of such types.
	///If one wants the operator to just use raw data it can use vpp::raw for
	///an object which will wrap it into a temporary RawBuffer object that can also be operated
	///on without any alignment or offsets.
	template<typename T> void addSingle(T&& obj);

	///Utility function that calls addSingle for all ob the given objects.
	template<typename... T> void add(T&&... obj);

	///Returns the current offset on the buffer.
	Size offset() const { return offset_; }

	BufferLayout alignType() const { return align_; }
	bool std140() const { return align_ == BufferLayout::std140; }
	bool std430() const { return align_ == BufferLayout::std430; }

protected:
	BufferLayout align_;
	Size offset_ {};
};

///This class can be used to update the contents of a buffer.
///It will automatically align the data as specified but can also be used to just upload/write
///raw data to the buffer. Can also be used to specify custom offsets or alignment for the data.
class BufferUpdate : public BufferOperator<BufferUpdate>, public ResourceReference<BufferUpdate>
{
public:
	///The given align type will influence the applied alignments.
	///\param direct Specifies if direct updates should be preferred. Will be ignored
	///if the buffer is filled by mapping and in some cases, direct filling is not possible.
	///\exception std::runtime_error if the device has no queue that supports graphics/compute or
	///transfer operations and the buffer is not mappable.
	///\sa BufferAlign
	BufferUpdate(const Buffer& buffer, BufferLayout align, bool direct = false);
	~BufferUpdate();

	///Writes size bytes from ptr to the buffer.
	void operate(const void* ptr, std::size_t size);

	///Offsets the current position on the buffer by size bytes. If update is true, it will
	///override the bytes with zero, otherwise they will not be changed.
	void offset(std::size_t size, bool update = true);

	///Assure that the current position on the buffer meets the given alignment requirements.
	void align(std::size_t align);

	void alignUniform();
	void alignStorage();
	void alignTexel();

	///Writes the stores data to the buffer.
	WorkPtr apply();

	///Returns the internal offset, i.e. the position on the internal stored data.
	std::size_t internalOffset() const { return internalOffset_; }

	const Buffer& buffer() const { return *buffer_; }

	using BufferOperator::offset;
	using BufferOperator::alignType;
	using BufferOperator::std140;
	using BufferOperator::std430;

	const Buffer& resourceRef() const { return *buffer_; }

protected:
	void checkCopies();
	std::uint8_t& data();

protected:
	const Buffer* buffer_ {};
	WorkPtr work_ {};

	MemoryMapView map_ {}; //for mapping (buffer/transfer)
	std::vector<std::uint8_t> data_; //for direct copying
	std::vector<vk::BufferCopy> copies_; //for transfer (direct/transfer)
	std::size_t internalOffset_ {};

	bool direct_ = false;
};

///Can be used to calculate the size that would be needed to fit certain objects with certain
///alignments on a buffer.
///Alternative name: BufferSizeCalculator
class BufferSizer : public BufferOperator<BufferSizer>, public Resource
{
public:
	BufferSizer(const Device& dev, BufferLayout align) : BufferOperator(align), Resource(dev) {}
	~BufferSizer() = default;

	void operate(const void* ptr, Size size) { offset_ += size; }

	void offset(Size size) { offset_ += size; }
	void align(Size align) { offset_ = vpp::align(offset_, align); }

	using BufferOperator::offset;
	using BufferOperator::alignType;
	using BufferOperator::std140;
	using BufferOperator::std430;

	void alignUniform();
	void alignStorage();
	void alignTexel();
};

///Class that can be used to read raw data into objects using the coorect alignment.
class BufferReader : public BufferOperator<BufferReader>, public Resource
{
public:
	//XXX: better use owned data (a vector, data copy) here?
	///Constructs the BufferReader with the given data range.
	///Note that the range must stay valid until destruction.
	BufferReader(const Device& dev, BufferLayout align, const Range<std::uint8_t>& data);
	~BufferReader() = default;

	void operate(void* ptr, Size size);

	void offset(Size size) { offset_ += size; }
	void align(Size align) { offset_ = vpp::align(offset_, align); }

	using BufferOperator::offset;
	using BufferOperator::alignType;
	using BufferOperator::std140;
	using BufferOperator::std430;

	void alignUniform();
	void alignStorage();
	void alignTexel();

protected:
	Range<std::uint8_t> data_;
};

///Fills the buffer with the given data.
///Does this either by memory mapping the buffer or by copying it via command buffer.
///Expects that buffer was created fillable, so either the buffer is memory mappable or
///it is allowed to copy data into it and the device was created with a matching queue.
///Note that this operation may be asnyc, therefore a work unique ptr is returned.
///If multiple (device local) buffers are to fill it brings usually huge performace gains
///to first call fill() on them all and then make sure that the needed work finishes.
///\param buf The Buffer to fill.
///\param align Specifies the align of the data to update (either std140 or std430).
///\param args The arguments to fill the buffer with. Notice that this function is just a
///utility wrapper around the BufferUpdate class (which may be used for more detailed updates)
///and therefore expects all given arguments to have a specialization for the VulkanType
///template which is used to specify their matching shader type.
///If raw (un- or custom-aligned) data should be written into the buffer, the vpp::raw() function
///can be used.
///The given arguments must only be valid until this function finishes, i.e. they can go out
///of scope before the returned work is finished.
///\sa BufferUpdate
///\sa ShaderType
///\sa fill140
///\sa fill430
template<typename... T>
WorkPtr fill(const Buffer& buf, BufferLayout align, const T&... args)
{
	BufferUpdate update(buf, align);
	update.add(args...);
	return update.apply();
}

///Utilty shortcut for filling the buffer with data using the std140 layout.
///\sa fill
///\sa BufferUpdate
template<typename... T> WorkPtr fill140(const Buffer& buf, const T&... args)
	{ return fill(buf, BufferLayout::std140, args...); }

///Utilty shortcut for filling the buffer with data using the std430 layout.
///\sa fill
///\sa BufferUpdate
template<typename... T> WorkPtr fill430(const Buffer& buf, const T&... args)
	{ return fill(buf, BufferLayout::std430, args...); }

///Retrives the data stored in the buffer.
///\param size The size of the range to retrive. If size is vk::wholeSize (default) the range
///from offset until the end of the buffer will be retrieved.
///\return A Work ptr that is able to retrive an array of std::uint8_t values storing the data.
DataWorkPtr retrieve(const Buffer& buf, vk::DeviceSize offset = 0,
	vk::DeviceSize size = vk::wholeSize);

///Reads the data stored in the given buffer aligned into the given objects.
///Note that the given objects MUST remain valid until the work finishes.
///You can basically pass all argument types that you can pass to the fill command.
///Internally just uses a combination of a retrieve work operation and the reads the
///retrieved data into the given arguments using the BufferReader class.
///\sa BufferReader
template<typename... T>
WorkPtr read(const Buffer& buf, BufferLayout align, T&... args);

///Reads the given buffer into the given objects using the std140 layout.
///\sa read
///\sa BufferReader
template<typename... T> WorkPtr read140(const Buffer& buf, T&... args)
	{ return read(buf, BufferLayout::std140, args...); }

///Reads the given buffer into the given objects using the std430 layout.
///\sa read
///\sa BufferReader
template<typename... T> WorkPtr read430(const Buffer& buf, T&... args)
	{ return read(buf, BufferLayout::std430, args...); }


//TODO: constexpr where possible. One does not have to really pass the objects in most cases
//(except raw buffers and containers.)

///Calculates the size a vulkan buffer must have to be able to store all the given objects.
///\sa BufferSizer
template<typename... T>
std::size_t needeBufferSize(const Device& dev, BufferLayout align, const T&... args)
{
	BufferSizer sizer(dev, align);
	sizer.add(args...);
	return sizer.offset();
}

///Calcualtes the size a vulkan buffer must have to be able to store all the given objects using
///the std140 layout.
///\sa neededBufferSize
///\sa BufferSizer
template<typename... T> std::size_t neededBufferSize140(const Device& dev, const T&... args)
	{ return neededBufferSize(dev, BufferLayout::std140, args...); }

///Calcualtes the size a vulkan buffer must have to be able to store all the given objects using
///the std430 layout.
///\sa neededBufferSize
///\sa BufferSizer
template<typename... T> std::size_t neededBufferSize430(const Device& dev, const T&... args)
	{ return neededBufferSize(dev, BufferLayout::std430, args...); }


///Implementation of buffer operations and vukan types
#include <vpp/bits/vulkanTypes.inl>
#include <vpp/bits/bufferOps.inl>

/*

//simple example for the size/fill/read api:
//does not use the features for which the more comlex OO-api is needed such as
//custom alignments and offsets.

//query the size needed for a buffer in layout std140 with the given objects.
//note that for all of these calls you have to specialize the VulkanType template correectly
//for the given types (except the rawBuffer if gotten from vpp::raw and the container).
auto size = vpp::neededBufferSize140(dev, myvec2, myvec3, mymat3, myvector, myrawdata);
auto buffer = createBuffer(dev, size);

//fill the data with the buffer
//Usually one would one finish this work in place but batch it together with other work
//can be done using a vpp::WorkManager.
vpp::fill140(dev, myvec2, myvec3, mymat3, myvector, myrawdata)->finish();

//some operations/shader access here that changes the data of the buffer

//read the data back in
//the first call retrieves the data the second one parses it/reads it into the given variables.
//Usually one would not finish the work in place to retrieve the data.
auto data = vpp::retrieve(buffer)->data();
vpp::read140(dev, data, myvec2, myvec3, mymat3, myvector, myrawdata);

*/

}
