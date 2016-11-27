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

		const std::string& GetCommand() const { return m_command; }
		const std::vector<std::string>& GetArgs() const { return m_args; }

	private:
		std::string					m_command;
		std::vector<std::string>	m_args;
	};
}

#endif // !SABA_VIEWER_VIEWERCOMMAND_H_
