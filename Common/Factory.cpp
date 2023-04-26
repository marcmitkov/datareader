#include "Strategy.h"
Strategy*  CreateStrategy(Marketable * c, string s, string name, string group, int freq)
{
    //MR NewCmake
    (void) group;
    (void)name;
    (void)freq;
    (void) c;
	if (s == "A")
		return 0; /*new Strategy(c, name, group, freq); */
	/*
	else if (s == "B")
		return new StrategyB(c, name, group);
	*/
	else
		cout << " not a valid strategy name" << endl;
	return NULL;
}