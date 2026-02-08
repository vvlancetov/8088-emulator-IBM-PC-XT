#pragma once
#include <vector>
#include <string>

class Breakpointer
{
private:

	struct comment
	{
		int address;
		std::string text;
	};

	void load_breakpoints();
	void load_comments();
	std::vector<comment> breakpoints;               // точки останова
	std::vector<comment> comments;					//комментарии к программе
	bool breakpoinst_ON = 0;		//активировать точки
	bool comments_ON = 0;			//выводить комментарии


public:
	Breakpointer();
	void check_points();			//проверить попадание в точку останова или комментария
	void set_bp_EN(bool turn_on)	//включить точки
	{
		breakpoinst_ON = turn_on;
	}
	void set_comments_EN(bool turn_on)	//включить комменты
	{
		comments_ON = turn_on;
	}
};


