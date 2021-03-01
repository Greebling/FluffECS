#ifndef FLUFF_ECS_SERIALIZATION_H
#define FLUFF_ECS_SERIALIZATION_H

#include <fstream>
#include "ComponentContainer.h"

namespace flf
{
	template<typename T>
	inline constexpr bool IsSerializable = false;
	
	/// Marks the given type as specially serializable. Types marked as such need to implement the
	/// methods Serialize(std::ofstream &) and Deserialize(std::ifstream &)
#define Fluff_MarkSpecialSerializable(Type)namespace fluff { template<> inline constexpr bool IsSerializable< Type > = true;}
	
	/// Helper function for easier binary serialisation
	/// \param stream to write to
	/// \param value to write into stream
	template<typename T>
	inline void Write(std::ofstream &stream, const T &value)
	{
		stream.write(reinterpret_cast<const char *>(&value), sizeof(T));
	}
	
	template<typename T>
	inline void Write(std::ofstream &stream, const std::vector<T> &value)
	{
		auto size = value.size();
		Write(stream, size);
		stream.write(reinterpret_cast<const char *>(value.data()), value.size() * sizeof(T));
	}
	
	template<typename T>
	inline void Write(std::ofstream &stream, const std::pmr::vector<T> &value)
	{
		auto size = value.size();
		Write(stream, size);
		stream.write(reinterpret_cast<const char *>(value.data()), value.size() * sizeof(T));
	}
	
	/// Helper function for easier binary deserialisation
	/// \param stream to read from
	/// \param place to write the read value to
	template<typename T>
	inline void Read(std::ifstream &stream, T &place)
	{
		stream.read(reinterpret_cast<char *>(&place), sizeof(T));
	}
	
	template<typename T>
	inline void Read(std::ifstream &stream, std::vector<T> &place)
	{
		std::size_t size;
		Read(stream, size);
		
		place.resize(size);
		stream.read(reinterpret_cast<char *>(place.data()), size * sizeof(T));
	}
	
	template<typename T>
	inline void Read(std::ifstream &stream, std::pmr::vector<T> &place)
	{
		std::size_t size;
		Read(stream, size);
		
		place.resize(size);
		stream.read(reinterpret_cast<char *>(place.data()), size * sizeof(T));
	}
}

#endif //FLUFF_ECS_SERIALIZATION_H
