/*
thot package for statistical machine translation
Copyright (C) 2017 Adam Harasimowicz
 
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with this program; If not, see <http://www.gnu.org/licenses/>.
*/
 
/*********************************************************************/
/*                                                                   */
/* Module: thot_lextable_to_leveldb.cc                               */
/*                                                                   */
/* Definitions file: thot_lextable_to_leveldb.cc                     */
/*                                                                   */
/* Description: Convert lex table file in binary format to           */
/*              LevelDB format.                                      */
/*                                                                   */   
/*********************************************************************/


//--------------- Include files ---------------------------------------

#include "IncrLexLevelDbTable.h"

#include <fstream>
#include <stdlib.h>
#include "options.h"

//--------------- Constants -------------------------------------------

//--------------- Function Declarations -------------------------------

int convert(void);
int takeParameters(int argc, char *argv[]);
void printUsage(void);
void printVersion(void);

//--------------- Type definitions ------------------------------------


//--------------- Global variables ------------------------------------

std::string inputFile;
std::string outputPath;

//--------------- Function Definitions --------------------------------


//--------------- main function
int main(int argc, char *argv[])
{
    if(takeParameters(argc, argv) == THOT_OK)
    {
        return convert();
    }
    else return THOT_ERROR;
}

//--------------- convert function
int convert()
{
    IncrLexLevelDbTable lexTable;
    lexTable.init(outputPath);

    return lexTable.load(inputFile.c_str());
}

//--------------- takeParameters function
int takeParameters(int argc, char *argv[])
{
    int err;

        /* Verify --help option */
    err = readOption(argc, argv, "--help");
    if(err != -1)
    {
        printUsage();
        return THOT_ERROR;
    }

        /* Verify --version option */
    err = readOption(argc, argv, "--version");
    if (err != -1)
    {
        printVersion();
        return THOT_ERROR;
    }

        /* Takes the input file path */
    err = readSTLstring(argc, argv, "-i", &inputFile);
    if(err == -1)
    {
        printUsage();
        return THOT_ERROR;
    }

        /* Takes the output path */
    err = readSTLstring(argc, argv, "-o", &outputPath);
    if(err == -1)
    {
        printUsage();
        return THOT_ERROR;
    }

    return THOT_OK;  
}

//--------------- printUsage function
void printUsage(void)
{
    std::cerr << "Usage: thot_lextable_to_leveldb -i <string> -o <string>" << std::endl;
    std::cerr << "                   [-v] [--help] [--version]" << std::endl << std::endl;
    std::cerr << "-i <string>        Input file with lex table in binary format" << std::endl;
    std::cerr << "-o <string>        Output path for LevelDB with lex table" << std::endl;
    std::cerr << "--help             Display this help and exit." << std::endl;
    std::cerr << "--version          Output version information and exit." << std::endl;
}

//--------------- printVersion function
void printVersion(void)
{
    std::cerr << "thot_lextable_to_leveldb is part of the thot package " << std::endl;
    std::cerr << "thot version " << THOT_VERSION << std::endl;
    std::cerr << "thot is GNU software written by Daniel Ortiz" << std::endl;
}
