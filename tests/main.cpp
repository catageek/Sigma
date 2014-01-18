#include "gtest/gtest.h"
#include "tests/BitArrayTest.h"
#include "tests/VectorMapTest.h"
#include "tests/UniqueIDProviderTest.h"
#include "tests/PropertyTest.h"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	system("Pause");
	return ret;
}
