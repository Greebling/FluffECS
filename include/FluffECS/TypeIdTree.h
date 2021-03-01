#ifndef FLUFF_ECS_TYPEIDTREE_H
#define FLUFF_ECS_TYPEIDTREE_H

#include <cassert>
#include <optional>
#include <vector>
#include <array>
#include <memory_resource>
#include "TypeId.h"
#include "Keywords.h"

namespace flf::internal
{
	// TODO: This is still bugged, rewrite it entirely?
	template<typename ValueType>
	class TypeIdTree
	{
	public:
		/// Inserts a value into the tree
		/// \param value to be inserted
		/// \param sequence that acts as the key to that value. NEEDS TO BE SORTED IN ASCENDING ORDER
		template<typename TAlloc = std::allocator<TypeIdTree>>
		void Insert(ValueType value, const std::vector<IdType, TAlloc> &sequence) FLUFF_NOEXCEPT
		{
			for(std::size_t i = 1; i < sequence.size(); ++i)
			{
				if(sequence[i - 1] >= sequence[i])
				{
					assert("Given Sequence was not ordered or contained doubles!" && false);
				}
			}
			
			std::size_t currSequenceIndex = 0;
			
			TreeNode *currNode = &_head;
			// find the highest node that currently exist in tree
			while(currSequenceIndex != sequence.size())
			{
				std::size_t nextNodeIndex = currNode->IndexOf(sequence[currSequenceIndex]);
				
				// check if a next node was found
				if(nextNodeIndex == std::numeric_limits<std::size_t>::max())
				{
					// no next node was found
					break;
				}
				
				currNode = currNode->nextNodes[nextNodeIndex];
				currSequenceIndex++;
			}
			
			// insert needed nodes that do not exist to that found node
			while(currSequenceIndex != sequence.size())
			{
				TreeNode *newNode = new(_memory.allocate(sizeof(TreeNode))) TreeNode(&_memory);
				newNode->id = sequence[currSequenceIndex];
				
				
				// binary find the index to insert to
				std::size_t insertionPosition = currNode->FindPositionOf(sequence[currSequenceIndex]);
				if(currNode->nextNodes.size() > 0 && currNode->nextNodes[insertionPosition]->id < sequence[currSequenceIndex])
				{
					currNode->nextNodes.insert(currNode->nextNodes.begin() + static_cast<long>(insertionPosition + 1), newNode);
				} else
				{
					currNode->nextNodes.insert(currNode->nextNodes.begin() + static_cast<long>(insertionPosition), newNode);
				}
				
				currNode = newNode;
				currSequenceIndex++;
			}
			
			currNode->value = value;
		}
		
		/// Removes the key at exactly the given sequence
		/// \param sequence that belongs to the key that will be removed
		template<typename TAlloc = std::allocator<TypeIdTree>>
		void Remove(const std::vector<IdType, TAlloc> &sequence) FLUFF_NOEXCEPT
		{
			for(std::size_t i = 1; i < sequence.size(); ++i)
			{
				if(sequence[i - 1] >= sequence[i])
				{
					assert("Given Sequence was not ordered or contained doubles!" && false);
				}
			}
			
			std::size_t currSequenceIndex = 0;
			
			TreeNode *currNode = &_head;
			// find the highest node that currently exist in tree
			while(currSequenceIndex != sequence.size())
			{
				std::size_t nextNodeIndex = currNode->IndexOf(sequence[currSequenceIndex]);
				
				// check if a next node was found
				if(nextNodeIndex == std::numeric_limits<std::size_t>::max())
				{
					// no next node was found
					currNode = nullptr;
					break;
				}
				
				currNode = currNode->nextNodes[nextNodeIndex];
				currSequenceIndex++;
			}
			
			if(currNode != nullptr)
			{
				currNode->value.reset();
			}
		}
		
		/// Gets all values that have at least the given sequence
		/// \param sequence that values must have associated with them
		/// \return a list of values
		template<typename TAlloc = std::allocator<TypeIdTree>>
		std::vector<ValueType> GetVec(const std::vector<IdType, TAlloc> &sequence) FLUFF_NOEXCEPT
		{
			std::vector<ValueType> result{};
			
			CollectAllLowerThan(_head, sequence, 0, result);
			return result;
		}
		
		/// Gets all values that have at least the given sequence
		/// \param sequence that values must have associated with them
		/// \return a list of values
		template<typename TAlloc = std::allocator<TypeIdTree>>
		std::vector<const ValueType> GetVec(const std::vector<IdType, TAlloc> &sequence) const FLUFF_NOEXCEPT
		{
			std::vector<ValueType> result{};
			
			CollectAllLowerThan(_head, sequence, 0, result);
			return result;
		}
		
		/// Gets all values that have at least the given sequence
		/// \param sequence that values must have associated with them
		/// \return a list of values
		template<std::size_t ARRAY_SIZE>
		std::vector<ValueType> Get(const std::array<IdType, ARRAY_SIZE> &sequence) FLUFF_NOEXCEPT
		{
			std::vector<ValueType> result{};
			
			CollectAllLowerThan(_head, sequence, 0, result);
			return result;
		}
		
		/// Gets all values that have at least the given sequence
		/// \param sequence that values must have associated with them
		/// \return a list of values
		template<std::size_t ARRAY_SIZE>
		std::vector<const ValueType> Get(const std::array<IdType, ARRAY_SIZE> &sequence) const FLUFF_NOEXCEPT
		{
			std::vector<ValueType> result{};
			
			CollectAllLowerThan(_head, sequence, 0, result);
			return result;
		}
	
	private:
		struct TreeNode
		{
			explicit TreeNode(std::pmr::memory_resource *resource) FLUFF_NOEXCEPT
					: nextNodes(resource)
			{
			}
			
			IdType id{};
			std::optional<ValueType> value{};
			std::pmr::vector<TreeNode *> nextNodes;
			
			[[nodiscard]] inline std::size_t IndexOfAllNotHigherThan(IdType idToFind) const FLUFF_NOEXCEPT
			{
				if(nextNodes.size() == 0)
				{
					return 0;
				}
				std::size_t index = FindPositionOf(idToFind);
				
				if((nextNodes[index])->id <= idToFind)
				{
					return index;
				} else
				{
					// no element is lower than the given id
					return std::numeric_limits<std::size_t>::max();
				}
			}
			
			
			[[nodiscard]] std::size_t IndexOf(IdType idToFind) const FLUFF_NOEXCEPT
			{
				if(nextNodes.size() == 0)
				{
					return std::numeric_limits<std::size_t>::max();
				}
				
				std::size_t index = FindPositionOf(idToFind);
				
				if((nextNodes[index])->id == idToFind)
				{
					return index;
				}
				
				// element not found in list
				return std::numeric_limits<std::size_t>::max();
			}
			
			/// Finds the last position of the value that was lower or equal to a given value
			/// \param idToFind value to find
			/// \return index to the highest value that was not higher than idToFind
			[[nodiscard]] inline std::size_t FindPositionOf(IdType idToFind) const FLUFF_NOEXCEPT
			{
				for(std::size_t i = 0; i < nextNodes.size(); ++i)
				{
					if(nextNodes[i]->id > idToFind)
					{
						if(i != 0)
						{
							return i - 1;
						} else
						{
							return 0;
						}
					}
				}
				
				if(nextNodes.size() != 0)
					return nextNodes.size() - 1;
				else
					return 0;
			}
			
			bool operator<(const TreeNode &other) const FLUFF_NOEXCEPT
			{
				return id < other.id;
			}
		};
	
	private:
		/// Collects all values, that have lower values than the numbers in the given sequence
		/// \param node use as root
		/// \param sequence to have as a comparison
		/// \param sequenceIndex index into the sequence we currently are (usually set  to 0)
		/// \param resultVec where the values will be saved to
		template<typename TContainer, typename TAlloc = std::allocator<ValueType>>
		static void CollectAllLowerThan(const TreeNode &node, const TContainer &sequence, std::size_t sequenceIndex,
		                                std::vector<ValueType, TAlloc> &resultVec) FLUFF_NOEXCEPT
		{
			std::size_t index = node.IndexOfAllNotHigherThan(sequence[sequenceIndex]);
			
			if(index == std::numeric_limits<std::size_t>::max() || node.nextNodes.size() == 0)
			{
				// no nodes left to go to
				return;
			} else
			{
				// look if in other nodes have the exact id in their children
				for(std::size_t i = 0; i < index; ++i)
				{
					CollectAllLowerThan(*node.nextNodes[i], sequence, sequenceIndex, resultVec);
				}
				
				// we found a match and can go further in the sequence
				if(node.nextNodes[index]->id == sequence[sequenceIndex])
				{
					if(sequenceIndex == sequence.size() - 1)
					{
						// we have matched the entire sequence
						CollectAllChildren(*node.nextNodes[index], resultVec);
					} else
					{
						CollectAllLowerThan(*node.nextNodes[index], sequence, sequenceIndex + 1, resultVec);
					}
				} else
				{
					CollectAllLowerThan(*node.nextNodes[index], sequence, sequenceIndex, resultVec);
				}
			}
		}
		
		/// Collects all values of the child nodes of a given node
		/// \param node whose child values we collect
		/// \param resultVec to store the values
		template<typename TAlloc = std::allocator<ValueType>>
		static void CollectAllChildren(const TreeNode &node, std::vector<ValueType, TAlloc> &resultVec) FLUFF_NOEXCEPT
		{
			if(node.value.has_value())
			{
				resultVec.emplace_back(node.value.value());
			}
			
			for(TreeNode *child : node.nextNodes)
			{
				CollectAllChildren(*child, resultVec);
			}
		}
	
	private:
		std::pmr::unsynchronized_pool_resource _memory{{32, 512}};
		TreeNode _head{&_memory};
	};
}

#endif //FLUFF_ECS_TYPEIDTREE_H
