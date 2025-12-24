#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "App/Models/UniqueCircularBuffer.h"

/*
 * Unit tests for UniqueCircularBuffer<T, ID, KeyOfFn>
 * 
 * Template parameters:
 *   T: The type of items stored in the buffer
 *   ID: The type of the unique identifier/key
 *   KeyOfFn: A callable type that extracts the ID from a T (function, lambda, etc.)
 * 
 * Constructor: UniqueCircularBuffer(int capacity, KeyOfFn f)
 * 
 * Note: Some tests may fail due to implementation bugs:
 *   - push() doesn't add keys to m_keys after inserting items (uniqueness check fails)
 *   - pop() doesn't update m_tail or m_size
 *   - at() doesn't check bounds against m_size, only uses m_data.at()
 */

class UniqueCircularBufferTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

// Test basic construction
TEST_F(UniqueCircularBufferTest, Construction)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);
	EXPECT_EQ(buffer.Size(), 0);
}

// Test construction with different capacities
TEST_F(UniqueCircularBufferTest, ConstructionWithDifferentCapacities)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 1, decltype(getKey)> buffer1(getKey);
	EXPECT_EQ(buffer1.Size(), 0);

	UniqueCircularBuffer<int, int, 100, decltype(getKey)> buffer2(getKey);
	EXPECT_EQ(buffer2.Size(), 0);

	UniqueCircularBuffer<int, int, 2000, decltype(getKey)> buffer3(getKey);
	EXPECT_EQ(buffer3.Size(), 0);
}

// Test size() on empty buffer
TEST_F(UniqueCircularBufferTest, SizeOnEmptyBuffer)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);
	EXPECT_EQ(buffer.Size(), 0);
}

// Test pop from empty buffer throws exception
TEST_F(UniqueCircularBufferTest, PopFromEmptyThrows)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);
	EXPECT_THROW(buffer.Pop(), std::out_of_range);
}

// Test at() on empty buffer (should throw)
TEST_F(UniqueCircularBufferTest, AtOnEmptyBuffer)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);
	// The current implementation uses m_data.at() which will throw for out of bounds
	EXPECT_THROW(buffer.At(0), std::out_of_range);
}

// Test push and size tracking
TEST_F(UniqueCircularBufferTest, PushAndSize)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	EXPECT_EQ(buffer.Size(), 1);

	buffer.Push(2);
	EXPECT_EQ(buffer.Size(), 2);

	buffer.Push(3);
	EXPECT_EQ(buffer.Size(), 3);
}

// Test uniqueness enforcement
TEST_F(UniqueCircularBufferTest, UniquenessEnforcement)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	buffer.Push(2);
	buffer.Push(1); // Duplicate - should be ignored
	EXPECT_EQ(buffer.Size(), 2);

	buffer.Push(3);
	buffer.Push(1); // Duplicate again - should be ignored
	EXPECT_EQ(buffer.Size(), 3);
}

// Test circular behavior when capacity is exceeded
TEST_F(UniqueCircularBufferTest, CircularBehavior)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 3, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	buffer.Push(2);
	buffer.Push(3);
	EXPECT_EQ(buffer.Size(), 3);

	// Adding a 4th item should remove the oldest (1)
	buffer.Push(4);
	EXPECT_EQ(buffer.Size(), 3);
	// Note: The actual order depends on implementation details
	// The buffer should contain 2, 3, 4 (1 was removed)
}

// Test pop operation
TEST_F(UniqueCircularBufferTest, PopOperation)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	buffer.Push(2);
	buffer.Push(3);

	// Pop should return the oldest item
	int popped = buffer.Pop();
	EXPECT_EQ(popped, 1);
	EXPECT_EQ(buffer.Size(), 2);
}

// Test push range
TEST_F(UniqueCircularBufferTest, PushRange)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 10, decltype(getKey)> buffer(getKey);

	std::vector<int> items = { 1, 2, 3, 4, 5 };
	buffer.Push(items);

	EXPECT_EQ(buffer.Size(), 5);
}

// Test push range with duplicates
TEST_F(UniqueCircularBufferTest, PushRangeWithDuplicates)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 10, decltype(getKey)> buffer(getKey);

	std::vector<int> items = { 1, 2, 1, 3, 2, 4 };
	buffer.Push(items);

	EXPECT_EQ(buffer.Size(), 4); // Only unique items: 1, 2, 3, 4
}

// Test capacity limit
TEST_F(UniqueCircularBufferTest, CapacityLimit)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 3, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	buffer.Push(2);
	buffer.Push(3);
	buffer.Push(4);
	buffer.Push(5);

	EXPECT_EQ(buffer.Size(), 3);
}

// Test with different types
TEST_F(UniqueCircularBufferTest, DifferentTypes)
{
	auto getKeyStr = [](const std::string & s) { return s; };
	UniqueCircularBuffer<std::string, std::string, 10, decltype(getKeyStr)> bufferStr(getKeyStr);
	EXPECT_EQ(bufferStr.Size(), 0);

	auto getKeyDouble = [](double d) { return static_cast<int>(d); };
	UniqueCircularBuffer<double, int, 5, decltype(getKeyDouble)> bufferDouble(getKeyDouble);
	EXPECT_EQ(bufferDouble.Size(), 0);
}

// Test with custom struct type
TEST_F(UniqueCircularBufferTest, CustomStructType)
{
	struct Person
	{
		int id;
		std::string name;
	};

	auto getKey = [](const Person & p) { return p.id; };
	UniqueCircularBuffer<Person, int, 5, decltype(getKey)> buffer(getKey);
	EXPECT_EQ(buffer.Size(), 0);
}

// Test custom type with key extractor
TEST_F(UniqueCircularBufferTest, CustomTypeWithKeyExtractor)
{
	struct Person
	{
		int id;
		std::string name;
	};

	auto getKey = [](const Person & p) { return p.id; };
	UniqueCircularBuffer<Person, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(Person { 1, "Alice" });
	buffer.Push(Person { 2, "Bob" });
	buffer.Push(Person { 1, "Alice2" }); // Same ID, should be ignored

	EXPECT_EQ(buffer.Size(), 2);
	EXPECT_EQ(buffer.At(0).id, 1);
	EXPECT_EQ(buffer.At(0).name, "Alice"); // Original name preserved
	EXPECT_EQ(buffer.At(1).id, 2);
}

// Test that the class can be instantiated with various template parameters
TEST_F(UniqueCircularBufferTest, TemplateInstantiation)
{
	// Test with primitive types
	auto getKeyInt = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKeyInt)> bufferInt(getKeyInt);

	auto getKeyFloat = [](float value) { return static_cast<int>(value); };
	UniqueCircularBuffer<float, int, 5, decltype(getKeyFloat)> bufferFloat(getKeyFloat);

	auto getKeyChar = [](char value) { return static_cast<int>(value); };
	UniqueCircularBuffer<char, int, 5, decltype(getKeyChar)> bufferChar(getKeyChar);

	// Test with string types
	auto getKeyString = [](const std::string & s) { return s; };
	UniqueCircularBuffer<std::string, std::string, 5, decltype(getKeyString)> bufferString(getKeyString);

	auto getKeyStringInt = [](const std::string & s) { return static_cast<int>(s.length()); };
	UniqueCircularBuffer<std::string, int, 5, decltype(getKeyStringInt)> bufferStringInt(getKeyStringInt);

	// All should construct successfully
	EXPECT_EQ(bufferInt.Size(), 0);
	EXPECT_EQ(bufferFloat.Size(), 0);
	EXPECT_EQ(bufferChar.Size(), 0);
	EXPECT_EQ(bufferString.Size(), 0);
	EXPECT_EQ(bufferStringInt.Size(), 0);
}

// Test multiple instances
TEST_F(UniqueCircularBufferTest, MultipleInstances)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer1(getKey);
	UniqueCircularBuffer<int, int, 10, decltype(getKey)> buffer2(getKey);
	UniqueCircularBuffer<int, int, 100, decltype(getKey)> buffer3(getKey);

	EXPECT_EQ(buffer1.Size(), 0);
	EXPECT_EQ(buffer2.Size(), 0);
	EXPECT_EQ(buffer3.Size(), 0);
}

// Test sequential push operations
TEST_F(UniqueCircularBufferTest, SequentialPush)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	for (int i = 1; i <= 5; ++i)
	{
		buffer.Push(i);
		EXPECT_EQ(buffer.Size(), i);
	}
}

// Test sequential push pop
TEST_F(UniqueCircularBufferTest, SequentialPushPop)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	for (int i = 1; i <= 10; ++i)
		buffer.Push(i);

	EXPECT_EQ(buffer.Size(), 5);

	// Pop all
	for (int i = 0; i < 5; ++i)
		buffer.Pop();

	EXPECT_EQ(buffer.Size(), 0);
	EXPECT_THROW(buffer.Pop(), std::out_of_range);
}

// Test at() modification
TEST_F(UniqueCircularBufferTest, AtModification)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	buffer.Push(2);

	buffer.At(0) = 10;
	EXPECT_EQ(buffer.At(0), 10);
}

// Test at() out of range
TEST_F(UniqueCircularBufferTest, AtOutOfRange)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(1);
	buffer.Push(2);

	EXPECT_THROW(buffer.At(2), std::out_of_range);
	EXPECT_THROW(buffer.At(100), std::out_of_range);
}

// Test with function pointer as key extractor
namespace {
int getKeyFunc(int value)
{
	return value;
}
}

TEST_F(UniqueCircularBufferTest, FunctionPointerKeyExtractor)
{
	UniqueCircularBuffer<int, int, 5, decltype(&getKeyFunc)> buffer(getKeyFunc);

	buffer.Push(1);
	buffer.Push(2);
	EXPECT_EQ(buffer.Size(), 2);
}

// Test with member function pointer (if applicable)
TEST_F(UniqueCircularBufferTest, MemberFunctionKeyExtractor)
{
	struct TestStruct
	{
		int id;

		int getId() const
		{
			return id;
		}
	};

	auto getKey = [](const TestStruct & s) { return s.getId(); };
	UniqueCircularBuffer<TestStruct, int, 5, decltype(getKey)> buffer(getKey);

	buffer.Push(TestStruct { 1 });
	buffer.Push(TestStruct { 2 });
	EXPECT_EQ(buffer.Size(), 2);
}

// Test large capacity
TEST_F(UniqueCircularBufferTest, LargeCapacity)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 2000, decltype(getKey)> buffer(getKey);

	for (int i = 0; i < 2000; ++i)
		buffer.Push(i);

	EXPECT_EQ(buffer.Size(), 2000);

	const auto oldFront = buffer.At(0);
	// Add one more, should remove oldest
	buffer.Push(2000);
	EXPECT_EQ(buffer.Size(), 2000);
	EXPECT_NE(oldFront, buffer.At(0));
}

// Test empty state after operations
TEST_F(UniqueCircularBufferTest, EmptyStateAfterOperations)
{
	auto getKey = [](int value) { return value; };
	UniqueCircularBuffer<int, int, 5, decltype(getKey)> buffer(getKey);

	EXPECT_EQ(buffer.Size(), 0);

	buffer.Push(1);
	buffer.Push(2);
	EXPECT_EQ(buffer.Size(), 2);

	buffer.Pop();
	buffer.Pop();
	EXPECT_EQ(buffer.Size(), 0);

	// Should throw when popping from empty buffer
	EXPECT_THROW(buffer.Pop(), std::out_of_range);
}
