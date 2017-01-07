//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "ViewerCommand.h"

#include <algorithm>
#include <cctype>

namespace saba
{
	bool ViewerCommand::Parse(const std::string & line)
	{
		Clear();

		auto beginIt = line.begin();
		auto endIt = line.end();
		while (beginIt != endIt)
		{
			beginIt = std::find_if_not(
				beginIt,
				endIt,
				isspace
			);

			if (beginIt == endIt)
			{
				break;
			}

			std::string part;
			if ((*beginIt) == '\"')
			{
				auto lastIt = std::find(beginIt + 1, endIt, '\"');
				if (lastIt == endIt)
				{
					Clear();
					return false;
				}
				part.assign(beginIt + 1, lastIt);
				beginIt = lastIt + 1;
			}
			else if ((*beginIt) == '\'')
			{
				auto lastIt = std::find(beginIt + 1, endIt, '\'');
				if (lastIt == endIt)
				{
					Clear();
					return false;
				}
				part.assign(beginIt + 1, lastIt);
				beginIt = lastIt + 1;
			}
			else
			{
				auto lastIt = std::find_if(
					beginIt + 1,
					line.end(),
					isspace
				);
				part.assign(beginIt, lastIt);
				beginIt = lastIt;
			}

			if (m_command.empty())
			{
				m_command = std::move(part);
			}
			else
			{
				m_args.emplace_back(std::move(part));
			}
		}

		if (m_command.empty())
		{
			return false;
		}

		return true;
	}

	void ViewerCommand::Clear()
	{
		m_command.clear();
		m_args.clear();
	}
}
