//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_BASE_LOG_H_
#define SABA_BASE_LOG_H_

#include "Singleton.h"

#include <memory>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <assert.h>

namespace saba
{
	class DefaultSink : public spdlog::sinks::sink
	{
	public:
		DefaultSink();
		void log(const spdlog::details::log_msg& msg) override;
		void flush() override;
	private:
		std::shared_ptr<spdlog::logger>	m_defaultLogger;
	};

	class Logger
	{
	public:
		Logger()
		{
			auto defaultSink = std::make_shared<DefaultSink>();
			m_logger = std::make_shared<spdlog::logger>("saba", defaultSink);
		}

		template <typename T, typename... Args>
		std::shared_ptr<T> AddSink(const Args&... args)
		{
			auto name = m_logger->name();
			auto sinks = m_logger->sinks();
			auto newSink = std::make_shared<T>(args...);
			sinks.push_back(newSink);
			m_logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
			return newSink;
		}

		void RemoveSink(spdlog::sinks::sink* removeSink)
		{
			auto name = m_logger->name();
			auto sinks = m_logger->sinks();
			auto removeIt = std::remove_if(
				sinks.begin(),
				sinks.end(),
				[removeSink](const spdlog::sink_ptr& ptr) { return ptr.get() == removeSink; }
			);
			sinks.erase(removeIt, sinks.end());
			m_logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
		}

		std::shared_ptr<spdlog::logger> GetLogger()
		{
			return m_logger;
		}

		template <typename... Args>
		void Info(const char* message, const Args&... args)
		{
			m_logger->info(message, args...);
		}

		template <typename... Args>
		void Warn(const char* message, const Args&... args)
		{
			m_logger->warn(message, args...);
		}

		template <typename... Args>
		void Error(const char* message, const Args&... args)
		{
			m_logger->error(message, args...);
		}

	private:
		std::shared_ptr<spdlog::logger> m_logger;

	};

	template <typename... Args>
	void Info(const char* message, const Args&... args)
	{
		Singleton<Logger>::Get()->Info(message, args...);
	}

	template <typename... Args>
	void Warn(const char* message, const Args&... args)
	{
		Singleton<Logger>::Get()->Warn(message, args...);
	}

	template <typename... Args>
	void Error(const char* message, const Args&... args)
	{
		Singleton<Logger>::Get()->Error(message, args...);
	}
}

#define SABA_INFO(message, ...)\
	saba::Info(message, ##__VA_ARGS__)

#define SABA_WARN(message, ...)\
	saba::Warn(message, ##__VA_ARGS__)

#define SABA_ERROR(message, ...)\
	saba::Error(message, ##__VA_ARGS__)

#define SABA_ASSERT(expr)\
	assert(expr)

#endif // !SABA_BASE_LOG_H_

