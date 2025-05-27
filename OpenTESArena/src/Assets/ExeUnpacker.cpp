#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "ExeUnpacker.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/vfs/manager.hpp"

namespace
{
	// Performance optimization for bit reading (replaces the unnecessary heap 
	// allocation of std::vector<bool>). Use BitVector::bitsUsed instead of 
	// BitVector::bits.size().
	struct BitVector
	{
		std::array<bool, 9> bits;
		int count;

		BitVector()
		{
			this->bits.fill(false);
			this->count = 0;
		}
	};

	// A simple binary tree for retrieving a decoded value, given a vector of bits.
	class BitTree
	{
	private:
		struct Node
		{
			// Only leaves will have non-null values.
			std::unique_ptr<int> value;
			std::unique_ptr<Node> left;
			std::unique_ptr<Node> right;

			bool isLeaf() const
			{
				return (this->left.get() == nullptr) && (this->right.get() == nullptr);
			}
		};

		BitTree::Node root;
	public:
		// Inserts a node into the tree, overwriting any existing entry.
		void insert(BufferView<const bool> bits, int value)
		{
			BitTree::Node *node = &this->root;

			// Walk the tree, creating new nodes as necessary. Internal nodes have null values.
			for (int i = 0; i < bits.getCount(); i++)
			{
				const bool bit = bits[i];

				// Decide which branch to use.
				if (bit)
				{
					// Right.
					if (node->right.get() == nullptr)
					{
						// Make a new node.
						node->right = std::make_unique<BitTree::Node>();
					}

					node = node->right.get();
				}
				else
				{
					// Left.
					if (node->left.get() == nullptr)
					{
						// Make a new node.
						node->left = std::make_unique<BitTree::Node>();
					}

					node = node->left.get();
				}
				
				// Set the node's value if it's the desired leaf.
				if (i == (bits.getCount() - 1))
				{
					node->value = std::make_unique<int>(value);
				}
			}
		}

		// Returns a pointer to a decoded value in the tree, or null if no entry exists.
		const int *get(const BitVector &bitVector)
		{
			const int *value = nullptr;
			const BitTree::Node *left = this->root.left.get();
			const BitTree::Node *right = this->root.right.get();

			// Walk the tree.
			for (int i = 0; i < bitVector.count; i++)
			{
				const bool bit = bitVector.bits.at(i);

				// Decide which branch to use.
				if (bit)
				{
					// Right.
					DebugAssert(right != nullptr);

					// Check if it's a leaf.
					if (right->isLeaf())
					{
						value = right->value.get();
					}

					left = right->left.get();
					right = right->right.get();
				}
				else
				{
					// Left.
					DebugAssert(left != nullptr);

					// Check if it's a leaf.
					if (left->isLeaf())
					{
						value = left->value.get();
					}

					right = left->right.get();
					left = left->left.get();
				}
			}

			return value;
		}
	};

	// Bit table from pklite_specification.md, section 4.3.1 "Number of bytes".
	// The decoded value for a given vector is (index + 2) before index 11, and
	// (index + 1) after index 11.
	constexpr bool Duplication1_2[] = { true, false };
	constexpr bool Duplication1_3[] = { true, true };
	constexpr bool Duplication1_4[] = { false, false, false };
	constexpr bool Duplication1_5[] = { false, false, true, false };
	constexpr bool Duplication1_6[] = { false, false, true, true };
	constexpr bool Duplication1_7[] = { false, true, false, false };
	constexpr bool Duplication1_8[] = { false, true, false, true, false };
	constexpr bool Duplication1_9[] = { false, true, false, true, true };
	constexpr bool Duplication1_10[] = { false, true, true, false, false };
	constexpr bool Duplication1_11[] = { false, true, true, false, true, false };
	constexpr bool Duplication1_12[] = { false, true, true, false, true, true };
	constexpr bool Duplication1_SpecialCase[] = { false, true, true, true, false, false };
	constexpr bool Duplication1_13[] = { false, true, true, true, false, true, false };
	constexpr bool Duplication1_14[] = { false, true, true, true, false, true, true };
	constexpr bool Duplication1_15[] = { false, true, true, true, true, false, false };
	constexpr bool Duplication1_16[] = { false, true, true, true, true, false, true, false };
	constexpr bool Duplication1_17[] = { false, true, true, true, true, false, true, true };
	constexpr bool Duplication1_18[] = { false, true, true, true, true, true, false, false };
	constexpr bool Duplication1_19[] = { false, true, true, true, true, true, false, true, false };
	constexpr bool Duplication1_20[] = { false, true, true, true, true, true, false, true, true };
	constexpr bool Duplication1_21[] = { false, true, true, true, true, true, true, false, false };
	constexpr bool Duplication1_22[] = { false, true, true, true, true, true, true, false, true };
	constexpr bool Duplication1_23[] = { false, true, true, true, true, true, true, true, false };
	constexpr bool Duplication1_24[] = { false, true, true, true, true, true, true, true, true };

	const BufferView<const bool> Duplication1[] =
	{
		Duplication1_2,
		Duplication1_3,
		Duplication1_4,
		Duplication1_5,
		Duplication1_6,
		Duplication1_7,
		Duplication1_8,
		Duplication1_9,
		Duplication1_10,
		Duplication1_11,
		Duplication1_12,
		Duplication1_SpecialCase,
		Duplication1_13,
		Duplication1_14,
		Duplication1_15,
		Duplication1_16,
		Duplication1_17,
		Duplication1_18,
		Duplication1_19,
		Duplication1_20,
		Duplication1_21,
		Duplication1_22,
		Duplication1_23,
		Duplication1_24
	};

	// Bit table from pklite_specification.md, section 4.3.2 "Offset".
	// The decoded value for a given vector is simply its index.
	constexpr bool Duplication2_0[] = { true };
	constexpr bool Duplication2_1[] = { false, false, false, false };
	constexpr bool Duplication2_2[] = { false, false, false, true };
	constexpr bool Duplication2_3[] = { false, false, true, false, false };
	constexpr bool Duplication2_4[] = { false, false, true, false, true };
	constexpr bool Duplication2_5[] = { false, false, true, true, false };
	constexpr bool Duplication2_6[] = { false, false, true, true, true };
	constexpr bool Duplication2_7[] = { false, true, false, false, false, false };
	constexpr bool Duplication2_8[] = { false, true, false, false, false, true };
	constexpr bool Duplication2_9[] = { false, true, false, false, true, false };
	constexpr bool Duplication2_10[] = { false, true, false, false, true, true };
	constexpr bool Duplication2_11[] = { false, true, false, true, false, false };
	constexpr bool Duplication2_12[] = { false, true, false, true, false, true };
	constexpr bool Duplication2_13[] = { false, true, false, true, true, false };
	constexpr bool Duplication2_14[] = { false, true, false, true, true, true, false };
	constexpr bool Duplication2_15[] = { false, true, false, true, true, true, true };
	constexpr bool Duplication2_16[] = { false, true, true, false, false, false, false };
	constexpr bool Duplication2_17[] = { false, true, true, false, false, false, true };
	constexpr bool Duplication2_18[] = { false, true, true, false, false, true, false };
	constexpr bool Duplication2_19[] = { false, true, true, false, false, true, true };
	constexpr bool Duplication2_20[] = { false, true, true, false, true, false, false };
	constexpr bool Duplication2_21[] = { false, true, true, false, true, false, true };
	constexpr bool Duplication2_22[] = { false, true, true, false, true, true, false };
	constexpr bool Duplication2_23[] = { false, true, true, false, true, true, true };
	constexpr bool Duplication2_24[] = { false, true, true, true, false, false, false };
	constexpr bool Duplication2_25[] = { false, true, true, true, false, false, true };
	constexpr bool Duplication2_26[] = { false, true, true, true, false, true, false };
	constexpr bool Duplication2_27[] = { false, true, true, true, false, true, true };
	constexpr bool Duplication2_28[] = { false, true, true, true, true, false, false };
	constexpr bool Duplication2_29[] = { false, true, true, true, true, false, true };
	constexpr bool Duplication2_30[] = { false, true, true, true, true, true, false };
	constexpr bool Duplication2_31[] = { false, true, true, true, true, true, true };

	const BufferView<const bool> Duplication2[] =
	{
		Duplication2_0,
		Duplication2_1,
		Duplication2_2,
		Duplication2_3,
		Duplication2_4,
		Duplication2_5,
		Duplication2_6,
		Duplication2_7,
		Duplication2_8,
		Duplication2_9,
		Duplication2_10,
		Duplication2_11,
		Duplication2_12,
		Duplication2_13,
		Duplication2_14,
		Duplication2_15,
		Duplication2_16,
		Duplication2_17,
		Duplication2_18,
		Duplication2_19,
		Duplication2_20,
		Duplication2_21,
		Duplication2_22,
		Duplication2_23,
		Duplication2_24,
		Duplication2_25,
		Duplication2_26,
		Duplication2_27,
		Duplication2_28,
		Duplication2_29,
		Duplication2_30,
		Duplication2_31
	};
}

bool ExeUnpacker::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	// Generate the bit trees for "duplication mode". Since the Duplication1 table has 
	// a special case at index 11, split the insertions up for the first bit tree.
	BitTree bitTree1, bitTree2;

	for (int i = 0; i < 11; i++)
	{
		bitTree1.insert(Duplication1[i], i + 2);
	}

	bitTree1.insert(Duplication1[11], 13);

	for (int i = 12; i < static_cast<int>(std::size(Duplication1)); i++)
	{
		bitTree1.insert(Duplication1[i], i + 1);
	}

	for (int i = 0; i < static_cast<int>(std::size(Duplication2)); i++)
	{
		bitTree2.insert(Duplication2[i], i);
	}

	// Beginning and end of compressed data in the executable.
	const uint8_t *compressedStart = srcPtr + 752;
	const uint8_t *compressedEnd = srcPtr + (src.getCount() - 8);

	// Last word of compressed data must be 0xFFFF.
	const uint16_t lastCompWord = Bytes::getLE16(compressedEnd - 2);
	if (lastCompWord != 0xFFFF)
	{
		DebugLogError("Invalid last compressed word \"0x" +
			String::toHexString(lastCompWord) + "\".");
		return false;
	}

	// Calculate length of decompressed data -- more precise method (for A.EXE).
	const int decompLen = [compressedEnd]()
	{
		const uint16_t segment = Bytes::getLE16(compressedEnd);
		const uint16_t offset = Bytes::getLE16(compressedEnd + 2);
		return (segment * 16) + offset;
	}();

	// Buffer for the decompressed data (also little endian).
	this->exeData.init(decompLen);
	this->exeData.fill(0);

	// Current position for inserting decompressed data.
	int decompIndex = 0;

	// A 16-bit array of compressed data.
	uint16_t bitArray = Bytes::getLE16(compressedStart);

	// Offset from start of compressed data (start at 2 because of the bit array).
	int byteIndex = 2;

	// Number of bits consumed in the current 16-bit array.
	int bitsRead = 0;

	// Continually read bit arrays from the compressed data and interpret each bit. 
	// Break once a compressed byte equals 0xFF in duplication mode.
	while (true)
	{
		// Lambda for getting the next byte from compressed data.
		auto getNextByte = [compressedStart, &byteIndex]()
		{
			const uint8_t byte = compressedStart[byteIndex];
			byteIndex++;

			return byte;
		};

		// Lambda for getting the next bit in the theoretical bit stream.
		auto getNextBit = [&bitArray, &bitsRead, &getNextByte]()
		{
			const bool bit = (bitArray & (1 << bitsRead)) != 0;
			bitsRead++;

			// Advance the bit array if done with the current one.
			if (bitsRead == 16)
			{
				bitsRead = 0;

				// Get two bytes in little endian format.
				const uint8_t byte1 = getNextByte();
				const uint8_t byte2 = getNextByte();
				bitArray = byte1 | (byte2 << 8);
			}

			return bit;
		};

		// Decide which mode to use for the current bit.
		if (getNextBit())
		{
			// "Duplication" mode.
			// Calculate which bytes in the decompressed data to duplicate and append.
			BitVector copyBits;
			const int *copyPtr = nullptr;

			// Read bits until they match a bit tree leaf.
			while (copyPtr == nullptr)
			{
				copyBits.bits.at(copyBits.count) = getNextBit();
				copyBits.count++;
				copyPtr = bitTree1.get(copyBits);
			}

			// Calculate the number of bytes in the decompressed data to copy.
			uint16_t copyCount = 0;

			// Lambda for comparing a bit vector's equality with the special case.
			auto matchesSpecialCase = [](const BitVector &bitVector)
			{
				const BufferView<const bool> specialCase = Duplication1[11];
				const bool equalSize = bitVector.count == specialCase.getCount();

				if (!equalSize)
				{
					return false;
				}
				else
				{
					for (int i = 0; i < bitVector.count; i++)
					{
						if (bitVector.bits.at(i) != specialCase[i])
						{
							return false;
						}
					}

					return true;
				}
			};

			// Check for the special bit vector case "011100".
			if (matchesSpecialCase(copyBits))
			{
				// Read a compressed byte.
				const uint8_t encryptedByte = getNextByte();

				if (encryptedByte == 0xFE)
				{
					// Skip the current bit.
					continue;
				}
				else if (encryptedByte == 0xFF)
				{
					// All done with decompression.
					break;
				}
				else
				{
					// Combine the compressed byte with 25 for the byte count.
					copyCount = encryptedByte + 25;
				}
			}
			else
			{
				// Use the decoded value from the first bit table.
				copyCount = *copyPtr;
			}

			// Calculate the offset in decompressed data. It is a two byte value.
			// The most significant byte is 0 by default.
			uint8_t mostSigByte = 0;

			// If the copy count is not 2, decode the most significant byte.
			if (copyCount != 2)
			{
				BitVector offsetBits;
				const int* offsetPtr = nullptr;

				// Read bits until they match a bit tree leaf.
				while (offsetPtr == nullptr)
				{
					offsetBits.bits.at(offsetBits.count) = getNextBit();
					offsetBits.count++;
					offsetPtr = bitTree2.get(offsetBits);
				}

				// Use the decoded value from the second bit table.
				mostSigByte = *offsetPtr;
			}

			// Get the least significant byte of the two bytes.
			const uint8_t leastSigByte = getNextByte();

			// Combine the two bytes.
			const uint16_t offset = leastSigByte | (mostSigByte << 8);

			// Finally, duplicate the decompressed data using the calculated offset and size.
			const int duplicateBegin = decompIndex - offset;
			const int duplicateEnd = duplicateBegin + copyCount;
			for (int i = duplicateBegin; i < duplicateEnd; i++, decompIndex++)
			{
				this->exeData[decompIndex] = this->exeData[i];
			}
		}
		else
		{
			// "Decryption" mode.
			// Read the next byte from the compressed data.
			const uint8_t encryptedByte = getNextByte();

			// Lambda for decrypting an encrypted byte with an XOR operation based on 
			// the current bit index. "bitsRead" is between 0 and 15. It is 0 if the
			// 16th bit of the previous array was used to get here.
			auto decrypt = [](uint8_t encryptedByte, int bitsRead)
			{
				const uint8_t key = 16 - bitsRead;
				const uint8_t decryptedByte = encryptedByte ^ key;
				return decryptedByte;
			};

			// Decrypt the byte.
			const uint8_t decryptedByte = decrypt(encryptedByte, bitsRead);

			// Append the decrypted byte onto the decompressed data.
			this->exeData[decompIndex] = decryptedByte;
			decompIndex++;
		}
	}

	return true;
}

BufferView<const uint8_t> ExeUnpacker::getData() const
{
	return this->exeData;
}
