/*

	settings.cpp

	Some mixed stuff (that is mixing stl strings and c strings), due to compiler, will try to clean this up a bit.

*/

#include "cbase.h"

//#include <windows.h>
//#include <windef.h>
//#include "hlservertool.h"
#include "settings.h"
#include <cstdlib>

using namespace std;

// We need to define this here, the compiler (VC .net) doesn't like returning local variables it seems :)
char dir[1024]; // I dont think it can read more than MAX_PATH = 260 though
//char ret[1024];
string ret;

int OpenSettingsFile(char *filename)
{
	ifstream in(filename);

	if(in.is_open())
		return TRUE;

	in.close();

	ofstream out(filename);
	out.close();

	return FALSE;
}

int find(string str, string str1)
{
	int len = (int)str.size();

	for(int i=0; i<len; i++)
	{
		if(str[i] != str1[i])
			return FALSE;
	}

	return TRUE;
}

int DoesSettingExist(char *filename, string str)
{
	ifstream in(filename);

	char oneline[1024];

	while(!in.eof())
	{
		in.getline(oneline, 1024);
		if(find(str, oneline))
		{
			in.close();
			return TRUE;
		}
	}

	in.close();

	return FALSE;
}

void WriteSettingToFileNum(char *filename, string str, string num)
{
	if(DoesSettingExist(filename, str))
	{
		ChangeSettingNum(filename, str, num);
		return;
	}

	ofstream out(filename, ios::app);

	out << str << " " << num << "\n";

	out.close();
}

void WriteSettingToFileStr(char *filename, string str, string str2)
{
	if(DoesSettingExist(filename, str))
	{
		ChangeSettingStr(filename, str, str2);
		return;
	}

	ofstream out(filename, ios::app);

	out << str << " " << "\"" << str2 << "\"" << "\n";

	out.close();
}

int ReadSettingNum(char *filename, char *str)
{
	char strnew[1024], oneline[1024];
	char test[8] = "%d\n";
	int ret;

	sprintf(strnew, "%s %s", str, test);

	ifstream in(filename);

	while(!in.eof())
	{
		in.getline(oneline, 1024);
		if(sscanf(oneline, strnew, &ret))
		{
			in.close();
			return ret;
		}
	}

	in.close();

	return -1; // i.e. return -1;
}

// TODO: make this use c++ strings, and not c style strings (ie char arrays)
/*char *ReadSettingStr(char *filename, char *str)
{
	char strnew[1024], oneline[1024];
	char test[8] = "%s\n";

	sprintf(strnew, "%s %s", str, test);

	ifstream in(filename);

	while(!in.eof())
	{
		in.getline(oneline, 1024);
		if(sscanf(oneline, strnew, &ret))
		{
			in.close();
			return ret;
		}
	}

	in.close();

	return "ERROR#1";
}*/

string ReadSettingStr(string filename, string str)
{
	string strnew, oneline;
	//char test[8] = "%s\n";

	strnew = str + "%s\n";

	//sprintf(strnew, "%s %s", str, test);

	ifstream in(filename.c_str());

	/*while(!in.eof())
	{
		in.getline(oneline, 1024);
		if(sscanf(oneline, strnew, &ret))
		{
			in.close();
			return ret;
		}
	}*/

	while(getline(in, oneline))
	{
		if(sscanf(oneline.c_str(), strnew.c_str(), &ret))
		{
			in.close();
			return ret;
		}
	}

	in.close();

	return "ERROR#1";
}

// TODO:
// void	ChangeSettingNum(char *filename, char *str, int num);
// void	ChangeSettingStr(char *filename, char *str, char *str2);
// END TODO

void ChangeSettingNum(char *filename, string str, string num)
{
	// this searches for a setting defined in str, use this ONLY for changing numerals.
	ifstream in(filename);

	string wholefile, line;

	while(getline(in, line))
	{
		string::size_type pos = line.find(str, 0);
		if(pos == 0) // found the setting
		{
			int writelen = (int)line.size() - ((int)str.size()+1);
			line.replace(str.size()+1,writelen,num);
			//cout << line << endl;
		}

		line.append("\n");
		wholefile.append(line);
	}

	in.close();

	// write out
	ofstream out(filename);
	out << wholefile;
	out.close();
}

void ChangeSettingStr(char *filename, string str, string str2)
{
	str2.replace(0,0, "\"");
	str2.append("\"");

	// this searches for a setting defined in str, use this ONLY for changing strings.
	ifstream in(filename);

	string wholefile, line;

	while(getline(in, line))
	{
		string::size_type pos = line.find(str, 0);
		if(pos == 0) // found the setting
		{
			int writelen = (int)line.size() - ((int)str.size()+1);
			line.replace(str.size()+1,writelen,str2);
			//cout << line << endl;
		}

		line.append("\n");
		wholefile.append(line);
	}

	in.close();

	// write out
	ofstream out(filename);
	out << wholefile;
	out.close();
}

string StripQuotes(string str)
{
	int len = (int)str.size(); //(int)strlen(str);

	str[0] = str[1];

	for(int i=1; i<len; i++)
	{
		if(str[i+1] == '\"')
		{
			str[i+1] = '\0';
		}

		str[i] = str[i+1];
	}

	return str;
}

void BackupFile(string filename, string newfilename)
{
	string wholefile, line;

	// load the file into ifstream in
	ifstream in(filename.c_str());

	while(getline(in, line))
		wholefile.append(line);

	in.close();

	// write out
	ofstream out(newfilename.c_str());
	out << wholefile;
	out.close();
}