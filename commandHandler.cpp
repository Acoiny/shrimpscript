#include "commandHandler.hpp"

#include <iostream>

using namespace std;

/*
*          foreground background
* black        30         40
* red          31         41
* green        32         42
* yellow       33         43
* blue         34         44
* magenta      35         45
* cyan         36         46
* white        37         47
*/

#define NONE "\033[0m"
#define RED "\033[31m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define YELLOW "\033[33m"
#define GREEN "\033[32m"

void commandHandler::printHelp()
{
	cout <<
		"usage:\n"
		"\t'shrimpp [path]' to interpret file\n"
		"\t'shrimpp' to enter repl\n"
		"\t'shrimpp [option]'\n\n"
		"options:\n"
		"\t-help\t\tto print this help text\n"
		"\t-h\t\tsame as -help\n"
		"\t-functions\tprints native functions\n"
		"\t-f\t\tsame as -functions\n" << endl;
}

void commandHandler::printNativeFunctions()
{
	cout << BLUE << "Functions:\n" << NONE <<
		"clock()\t\t\treturns the time since the program started\n\n"
		"print(args)\t\ttakes one or more arguments and prints them to the standard output\n\n"
		"println(args)\t\tsame as print() but adds a newline at the end\n\n"
		"type(arg)\t\treturns the type of the argument as a string\n\n"
		"input(arg)\t\tawaits user input in standard input after printing optional argument\n\n"
		"open(arg, mode)\t\ttakes a path and returns the file object if succeeding in read(\"r\") or write(\"w\") mode, defaults to read\n\n"
		"to_chr(arg)\t\treturns the character corresponding to the given ascii number\n" << endl;
	
	string tmp;
	cout << "quit(q) or continue(any)";
	getline(cin, tmp);

	if (tmp == "q") {
		return;
	}

	printNativeStringFunctions();

	cout << "quit(q) or continue(any)";
	getline(cin, tmp);

	if (tmp == "q") {
		return;
	}

	printNativeMathFunctions();

	cout << "quit(q) or continue(any)";
	getline(cin, tmp);

	if (tmp == "q") {
		return;
	}

	printNativeListFunctions();
}

void commandHandler::printNativeStringFunctions()
{
	cout << endl << CYAN <<
		"String Functions:\n" << NONE <<
		"these functions can only be used on any string, e.g. \"hello world\".len()\n\n"
		"len()\t\t\treturns the length of the string\n\n"
		"slice(index, index)\ttakes two indices and returns a substring from these positions\n\n"
		"chr(index)\t\treturns the ascii number of the character at given index, defaults to first character\n\n"
		"at(index)\t\treturns the character at the given index\n" << endl;
}

void commandHandler::printNativeListFunctions()
{
	cout << endl << YELLOW <<
		"List Functions:\n" << NONE <<
		"these functions can only be used on list objects, e.g. [1,2,3].len()\n\n"
		"len()\t\t\treturns the length of the list\n\n"
		"append(arg)\t\tappends the given value to the array\n\n"
		"insert(index, arg)\tinserts the given value at the given index\n" << endl;
}

void commandHandler::printNativeFileFunctions()
{
	cout << endl << GREEN <<
		"File Functions:\n" << NONE <<
		"these functions can only be used on file objects, returned by the open() function\n\n"
		"read()\t\treturns the contents of the file as a string\n\n"
		"write(msg)\t\twrites a string to the file, must be opened in write-mode\n\n"
		"close()\t\tcloses a file, note that the garbage collector eventually automatically closes files, but at an undetermined time\n" << endl;
}

void commandHandler::printNativeMathFunctions()
{
	cout << endl << RED << "Math functions:\n" << NONE <<
		"all these functions are used on the global 'Math' object, e.g. Math.rand()\n\n"
		"rand()\t\t\treturns a pseudo-random number between 0 and 1\n\n"
		"sqrt(num)\t\treturns the square root of the given number\n\n"
		"floor(num)\t\treturns the given number rounded down\n\n"
		"ceil(num)\t\treturns the given number rounded up\n" << endl;
}

commandHandler::commandHandler(std::string command)
{
	command = command.substr(1, command.length() - 1);


	if (command == "help" || command == "h") {
		printHelp();
	} else
		if (command == "functions" || command == "f") {
			printNativeFunctions();
		}
}
