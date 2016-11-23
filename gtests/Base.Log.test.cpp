#include "Saba/Base/Log.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

class TestSink : public spdlog::sinks::sink
{
public:
	void log(const spdlog::details::log_msg& msg) override
	{
		m_buffer.push_back(msg.formatted.str());
	}

	void flush() override
	{
	}

	std::vector<std::string> m_buffer;
};

TEST(BaseTest, LogTest)
{
	auto logger = saba::Singleton<saba::Logger>::Get();
	auto testSink = logger->AddSink<TestSink>();

	EXPECT_NE(nullptr, testSink.get());

	SABA_INFO("test1");
	SABA_WARN("test2");
	SABA_ERROR("test3");

	EXPECT_EQ(3, testSink->m_buffer.size());

	EXPECT_EQ(2, logger->GetLogger()->sinks().size());
	logger->RemoveSink(testSink.get());
	EXPECT_EQ(1, logger->GetLogger()->sinks().size());

	SABA_INFO("test4");
	EXPECT_EQ(3, testSink->m_buffer.size());
}
