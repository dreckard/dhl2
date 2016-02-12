#include <fstream>
#include <string>

using namespace std;

#define SETTINGS_FILENAME	"settings.ini"

int find(string str, string str1);

int		OpenSettingsFile(char *filename);
int		DoesSettingExist(char *filename, string str);

void	WriteSettingToFileNum(char *filename, string str, string num);
void	WriteSettingToFileStr(char *filename, string str, string str2);

int		ReadSettingNum(char *filename, char *str);
//char	*ReadSettingStr(char *filename, char *str);
string	ReadSettingStr(string filename, string str);

// TODO:
void	ChangeSettingNum(char *filename, string str, string num);
void	ChangeSettingStr(char *filename, string str, string str2);
// END TODO

//char	*StripQuotes(char *str);
string	StripQuotes(string str);

// for practical purposes
void	BackupFile(string filename, string newfilename);
