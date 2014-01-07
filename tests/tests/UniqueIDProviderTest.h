#ifndef UNIQUEIDPROVIDERTEST_H_INCLUDED
#define UNIQUEIDPROVIDERTEST_H_INCLUDED

#include "UniqueIDProvider.h"

namespace Sigma {
	TEST(UniqueIDProviderTest, UniqueIDProviderBasic) {
		UniqueIDProvider p;
		ASSERT_EQ(0, UniqueIDProvider::GetID()) << "First ID should be 0";
		ASSERT_TRUE(UniqueIDProvider::IsInUse(0)) << "ID has not been marked as used";
		ASSERT_EQ(1, UniqueIDProvider::GetID()) << "ID should be 1";
		ASSERT_EQ(2, UniqueIDProvider::GetID()) << "ID should be 2";
		ASSERT_EQ(3, UniqueIDProvider::GetID()) << "ID should be 3";
		ASSERT_EQ(4, UniqueIDProvider::GetID()) << "ID should be 4";
		ASSERT_EQ(5, UniqueIDProvider::GetID()) << "ID should be 5";
		ASSERT_EQ(6, UniqueIDProvider::GetID()) << "ID should be 6";
		UniqueIDProvider::Release(3);
		ASSERT_EQ(3, UniqueIDProvider::GetID()) << "ID should be 3";
		ASSERT_FALSE(UniqueIDProvider::Reserve(3)) << "Possible duplicate ID";
		UniqueIDProvider::Release(0);
		UniqueIDProvider::Release(1);
		UniqueIDProvider::Release(2);
		UniqueIDProvider::Release(3);
		UniqueIDProvider::Release(4);
		UniqueIDProvider::Release(5);
		UniqueIDProvider::Release(6);
	}

	TEST(UniqueIDProviderTest, UniqueIDProviderAdvandced) {
		UniqueIDProvider p;
		for ( auto i = 0; i < 32; i++) {
			UniqueIDProvider::GetID();
		}
		ASSERT_EQ(32, UniqueIDProvider::GetID()) << "ID should be 32";
		UniqueIDProvider::Release(10);
		UniqueIDProvider::Release(20);
		ASSERT_EQ(10, UniqueIDProvider::GetID()) << "ID should be 10";
	}
}

#endif // UNIQUEIDPROVIDERTEST_H_INCLUDED
