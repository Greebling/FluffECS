#pragma once

#include <fstream>
#include "Archetype.h"

namespace flf
{
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
