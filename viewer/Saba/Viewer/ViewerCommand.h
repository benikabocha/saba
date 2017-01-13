//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_VIEWERCOMMAND_H_
#define SABA_VIEWER_VIEWERCOMMAND_H_

#include <string>
#include <vector>

namespace saba
{
	class ViewerCommand
	{
	public:
		bool Parse(const std::string& line);
		void Clear();

		void SetCommand(const std::string& cmd) { m_command = cmd; }
		const std::string& GetCommand() const { return m_command; }

		void AddArg(const std::string& arg) { m_args.push_back(arg); }
		void ClearArgs() { m_args.clear(); }
		const std::vector<std::string>& GetArgs() const { return m_args; }

	private:
		std::string					m_command;
		std::vector<std::string>	m_args;
	};
}

#endif // !SABA_VIEWER_VIEWERCOMMAND_H_
