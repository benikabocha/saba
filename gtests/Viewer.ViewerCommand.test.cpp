#include <Saba/Viewer/ViewerCommand.h>

#include <gtest/gtest.h>


TEST(ViewerTest, ViewerCommandTest)
{
	{
		const char* commandLine = "abc";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = "abc arg1 arg2";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("arg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("arg2"), cmd.GetArgs()[1]);
	}

	{
		const char* commandLine = " abc arg1 arg2 ";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("arg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("arg2"), cmd.GetArgs()[1]);
	}

	{
		const char* commandLine = "abc \"arg1\"\t'arg2'";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("arg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("arg2"), cmd.GetArgs()[1]);
	}

	{
		const char* commandLine = "abc \"a r\tg1\" 'a r\tg2'";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("a r\tg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("a r\tg2"), cmd.GetArgs()[1]);
	}

	{
		const char* commandLine = "abc \"'arg1'\"\t'\"arg2\"'";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("'arg1'"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("\"arg2\""), cmd.GetArgs()[1]);
	}

	{
		const char* commandLine = "";
		saba::ViewerCommand cmd;
		EXPECT_FALSE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = " ";
		saba::ViewerCommand cmd;
		EXPECT_FALSE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = "abc \"arg1 ";
		saba::ViewerCommand cmd;
		EXPECT_FALSE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = "abc \"arg1";
		saba::ViewerCommand cmd;
		EXPECT_FALSE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = "abc 'arg1 ";
		saba::ViewerCommand cmd;
		EXPECT_FALSE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = "abc 'arg1";
		saba::ViewerCommand cmd;
		EXPECT_FALSE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());
	}

	{
		const char* commandLine = "abc arg1 arg2";
		saba::ViewerCommand cmd;
		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("arg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("arg2"), cmd.GetArgs()[1]);

		cmd.Clear();
		EXPECT_EQ(std::string(""), cmd.GetCommand());
		EXPECT_EQ(0, cmd.GetArgs().size());

		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("arg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("arg2"), cmd.GetArgs()[1]);

		EXPECT_TRUE(cmd.Parse(commandLine));
		EXPECT_EQ(std::string("abc"), cmd.GetCommand());
		EXPECT_EQ(2, cmd.GetArgs().size());
		EXPECT_EQ(std::string("arg1"), cmd.GetArgs()[0]);
		EXPECT_EQ(std::string("arg2"), cmd.GetArgs()[1]);
	}
}
