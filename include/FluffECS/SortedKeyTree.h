#ifndef FLUFFTEST_SORTEDKEYTREE_H
#define FLUFFTEST_SORTEDKEYTREE_H

#include <vector>
#include <array>
#include <memory_resource>
#include <utility>
#include "Keywords.h"

namespace flf::internal
{
	/// A tree structure that associates an ascending sequence of values with a given value
	/// Note that for performance/memory saving reasons an default constructed TValue is interpreted as empty and will be ignored.
	/// Thus TValue should mainly be used with pointers where this would be a nullptr
	/// \tparam TKey that is used for lookup, see description for notes
	/// \tparam TValue that will be associated with a sequence of keys
	template<typename TKey, typename TValue>
	class SortedKeyTree
	{
	public:
		static_assert(std::is_default_constructible_v<TValue>);
		
		template<typename T>
		using VectorOf = std::pmr::vector<T>;
		
		explicit SortedKeyTree(std::pmr::memory_resource &resource)
				: _resource(resource), _head(resource)
		{
		}
		
		struct Node
		{
			Node() noexcept = default;
			
			Node(const Node &) noexcept = default;
			
			Node(Node &&) noexcept = default;
			
			Node &operator=(const Node &) noexcept = default;
			
			Node &operator=(Node &&) noexcept = default;
			
			explicit Node(std::pmr::memory_resource &resource) noexcept
					: _next(&resource)
			{
			}
			
			Node(TKey key, std::pmr::memory_resource &resource) noexcept
					: _key(key), _next(&resource)
			{
			}
			
			Node(TKey key, TValue val) noexcept
					: _value(val), _key(key)
			{
			}
			
			[[nodiscard]] inline TKey Key() const noexcept
			{
				return _key;
			}
			
			[[nodiscard]] inline TValue Value() const noexcept
			{
				return _value;
			}
			
			/// Searches for all elements that have lower or equal keys in comparison to the given key
			/// \param key to search for
			/// \return std::numeric_limits<std::size_t>::max() if none is found, else the key of the highest key below or equal to key
			[[nodiscard]] std::size_t GetAllUntil(TKey key) const noexcept
			{
				if (_next.empty() || _next.front().Key() > key)
				{
					return std::numeric_limits<std::size_t>::max();
				}
				
				if (_next.back().Key() < key)
				{
					return _next.size() - 1;
				}
				
				std::size_t startIndex = 0;
				std::size_t endIndex = _next.size();
				
				while (startIndex != endIndex)
				{
					std::size_t midIndex = (startIndex + endIndex) / 2u;
					
					if (_next[midIndex].Key() == key)
					{
						return midIndex;
					}
					
					if (_next[midIndex].Key() < key)
					{
						startIndex = midIndex;
					} else
					{
						endIndex = midIndex;
					}
				}
				
				if (_next[endIndex].Key() <= key) FLUFF_LIKELY
				{
					return endIndex;
				} else
				{
					return std::numeric_limits<std::size_t>::max();
				}
			}
			
			[[nodiscard]] inline Node *GetChild(TKey key)
			{
				std::size_t index = GetAllUntil(key);
				if (index == std::numeric_limits<std::size_t>::max())
				{
					return nullptr;
				}
				
				if (_next[index].Key() == key)
				{
					return &_next[index];
				} else
				{
					return nullptr;
				}
			}
			
			[[nodiscard]] inline const Node *GetChild(TKey key) const
			{
				return const_cast<Node *>(this)->GetNext(key);
			}
			
			/// Adds a node as a following node
			/// \param node to add
			inline Node &AddFollowing(Node &&node) FLUFF_NOEXCEPT
			{
				std::size_t insertionPos = 0;
				for (; insertionPos < _next.size(); ++insertionPos)
				{
					if (_next[insertionPos].Key() > node.Key())
					{
						break;
					}
				}
				_next.insert(_next.cbegin() + insertionPos, std::forward<Node>(node));
				return _next[insertionPos];
			}
			
			[[nodiscard]] inline bool HasChildren() const noexcept
			{
				return _next.empty();
			}
			
			[[nodiscard]] inline const VectorOf<Node> &Next() const noexcept
			{
				return _next;
			}
			
			[[nodiscard]] inline VectorOf<Node> &Next() noexcept
			{
				return _next;
			}
		
		public:
			TValue _value{};
		private:
			TKey _key{};
			VectorOf<Node> _next{};
		};
	
	public:
		/// Adds a value that has a given sequence of keys associated with it
		/// \param keySequence of the value
		/// \param value to save
		template<typename TAlloc = std::allocator<TKey>>
		void Insert(const std::vector<TKey, TAlloc> keySequence, TValue value)
		{
			Node *currNode = &_head;
			std::size_t currentIndex = 0;
			for (; currentIndex < keySequence.size(); ++currentIndex)
			{
				if (Node *next = currNode->GetChild(keySequence[currentIndex]); next != nullptr)
				{
					currNode = next;
				} else
				{
					currNode = &currNode->AddFollowing(Node(keySequence[currentIndex], _resource));
				}
			}
			
			currNode->_value = value;
		}
		
		/// Searches the tree for all values, that have at least keySequence associated with them in
		/// on average O(keySequence.size() * log(TreeSize)) time
		/// \param keySequence to look for
		/// \return all value that have at least keySequence associated with them
		template<typename TAlloc = std::allocator<TKey>>
		[[nodiscard]] std::vector<TValue> GetAllFromSequence(const std::vector<TKey, TAlloc> &keySequence) const
		{
			std::vector<TValue> values{};
			CollectFromSeq(keySequence, 0, _head, values);
			return values;
		}
		
		/// Searches the tree for all values, that have at least keySequence associated with them in
		/// on average O(keySequence.size() * log(TreeSize)) time
		/// \param keySequence to look for
		/// \return all value that have at least keySequence associated with them
		template<std::size_t ARRAY_SIZE>
		[[nodiscard]] std::vector<TValue> GetAllFromSequence(const std::array<TKey, ARRAY_SIZE> &keySequence) const
		{
			std::vector<TValue> values{};
			CollectFromSeq(keySequence, 0, _head, values);
			return values;
		}
		
		/// Searches the tree for all values, that have at least keySequence associated with them in
		/// on average O(keySequence.size() * log(TreeSize)) time
		/// \param keySequence to look for
		/// \return all value that have at least keySequence associated with them
		template<typename TAlloc = std::allocator<TKey>>
		[[nodiscard]] std::vector<TValue> operator[](const std::vector<TKey, TAlloc> &keySequence) const
		{
			std::vector<TValue> values{};
			CollectFromSeq(keySequence, 0, _head, values);
			return values;
		}
	
	private:
		
		/// Searches the children of a given node for all values, that have at least keySequence associated with them in
		/// \param keySequence to search for
		/// \param seqIndex index into the keySequence we currently are at
		/// \param curr current node
		/// \param results vector where fitting values will be saved to
		template<typename TAlloc>
		static void CollectFromSeq(const std::vector<TKey, TAlloc> &keySequence, std::size_t seqIndex, const Node &curr,
		                           std::vector<TValue> &results) noexcept
		{
			if (seqIndex >= keySequence.size())
			{
				CollectChildValues(curr, results);
			} else
			{
				std::size_t nextIndex = curr.GetAllUntil(keySequence[seqIndex]);
				
				if (nextIndex != std::numeric_limits<std::size_t>::max())
				{
					// check if we found the exact value; only then we can advance to the next key in the keySequence
					const Node &foundNode = curr.Next()[nextIndex];
					if (foundNode.Key() == keySequence[seqIndex])
					{
						CollectFromSeq(keySequence, seqIndex + 1, foundNode, results);
					} else
					{
						CollectFromSeq(keySequence, seqIndex, foundNode, results);
					}
					
					// the rest are stuck at the current seqIndex
					for (std::size_t i = 0; i < nextIndex; ++i)
					{
						CollectFromSeq(keySequence, seqIndex, curr.Next()[i], results);
					}
				}
			}
		}
		
		/// Searches the children of a given node for all values, that have at least keySequence associated with them in
		/// \param keySequence to search for
		/// \param seqIndex index into the keySequence we currently are at
		/// \param curr current node
		/// \param results vector where fitting values will be saved to
		template<std::size_t ARRAY_SIZE>
		static void CollectFromSeq(const std::array<TKey, ARRAY_SIZE> &keySequence, std::size_t seqIndex, const Node &curr,
		                           std::vector<TValue> &results) noexcept
		{
			if (seqIndex >= keySequence.size())
			{
				CollectChildValues(curr, results);
			} else
			{
				std::size_t nextIndex = curr.GetAllUntil(keySequence[seqIndex]);
				
				if (nextIndex != std::numeric_limits<std::size_t>::max())
				{
					// check if we found the exact value; only then we can advance to the next key in the keySequence
					const Node &foundNode = curr.Next()[nextIndex];
					if (foundNode.Key() == keySequence[seqIndex])
					{
						CollectFromSeq(keySequence, seqIndex + 1, foundNode, results);
					} else
					{
						CollectFromSeq(keySequence, seqIndex, foundNode, results);
					}
					
					// the rest are stuck at the current seqIndex
					for (std::size_t i = 0; i < nextIndex; ++i)
					{
						CollectFromSeq(keySequence, seqIndex, curr.Next()[i], results);
					}
				}
			}
		}
		
		/// Collects all values of a given node and the values of all its children into the results vector
		/// \param node
		/// \param results
		static void CollectChildValues(const Node &node, std::vector<TValue> &results) noexcept
		{
			if (node.Value() != TValue())
			{
				results.push_back(node.Value());
			}
			
			for (auto &next : node.Next())
			{
				CollectChildValues(next, results);
			}
		}
	
	private:
		std::pmr::memory_resource &_resource;
		Node _head{&_resource};
	};
}

#endif //FLUFFTEST_SORTEDKEYTREE_H
