#include <gtest/gtest.h>

#include <Saba/Base/Singleton.h>
namespace
{
	class SingletonTest
	{
	public:
		SingletonTest()
			: m_value(0)
		{
			m_construct++;
		}

		~SingletonTest()
		{
			m_construct--;
		}

		void Add()
		{
			m_value++;
		}

		int			m_value;
		static int	m_construct;
	};

	int SingletonTest::m_construct = 0;
}

TEST(BaseTest, Singleton)
{
	auto inst = saba::Singleton<SingletonTest>::Get();

	EXPECT_NE(nullptr, inst);

	// コンストラクタが呼ばれていることを確認
	EXPECT_EQ(0, saba::Singleton<SingletonTest>::Get()->m_value);

	// 同一のインスタンスでAddが呼ばれていることを確認
	saba::Singleton<SingletonTest>::Get()->Add();
	EXPECT_EQ(1, saba::Singleton<SingletonTest>::Get()->m_value);

	// コンストラクターが一度しか呼ばれていないことを確認
	EXPECT_EQ(1, SingletonTest::m_construct);
}
