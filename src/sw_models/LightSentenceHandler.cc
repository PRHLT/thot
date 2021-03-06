/*
thot package for statistical machine translation
Copyright (C) 2013 Daniel Ortiz-Mart\'inez
 
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

/**
 * @file LightSentenceHandler.cc
 * 
 * @brief Definitions file for LightSentenceHandler.h
 */

//--------------- Include files --------------------------------------

#include "LightSentenceHandler.h"

//--------------- LightSentenceHandler class function definitions

//-------------------------
LightSentenceHandler::LightSentenceHandler(void)
{
  nsPairsInFiles=0;
  countFileExists=false;
  currFileSentIdx=0;
}

//-------------------------
bool LightSentenceHandler::readSentencePairs(const char *srcFileName,
                                             const char *trgFileName,
                                             const char *sentCountsFile,
                                             std::pair<unsigned int,unsigned int>& sentRange)
{
      // Clear sentence handler
 std::cerr<<"Initializing sentence handler..."<<std::endl;
 clear();
  
     // Fill first field of sentRange
 sentRange.first=0;

     // Open source file
 if(awkSrc.open(srcFileName)==THOT_ERROR)
 {
   std::cerr<<"Error in source language file: "<<srcFileName<<std::endl;
   return THOT_ERROR;
 }
 else
 {
       // Open target file
   if(awkTrg.open(trgFileName)==THOT_ERROR)
   {
     std::cerr<<"Error in target language file: "<<trgFileName<<std::endl;
     return THOT_ERROR;
   }
   else
   {
         // Open file with sentence counts
     if(strlen(sentCountsFile)==0)
     {
           // sentCountsFile is empty
       countFileExists=false;
     }
     else
     {
           // sentCountsFile is not empty
       if(awkSrcTrgC.open(sentCountsFile)==THOT_ERROR)
       {
         std::cerr<<"File with sentence counts "<<sentCountsFile<<" does not exist"<<std::endl;
         countFileExists=false;
       }
       else
         countFileExists=true;
     }
     
         // Read sentence pairs
     std::cerr<<"Reading sentence pairs from files: "<<srcFileName<<" and "<<trgFileName<<std::endl;
     if(countFileExists) std::cerr<<"Reading sentence pair counts from file "<<sentCountsFile<<std::endl;

     while(awkSrc.getln())
     {
       if(!awkTrg.getln())
       {
         std::cerr<<"Error: the number of source and target sentences differ!"<<std::endl;
         return THOT_ERROR;
       }

           // Display warnings if sentences are empty
       if(awkSrc.NF==0)
         std::cerr<<"Warning: source sentence "<<nsPairsInFiles<<" is empty"<<std::endl;
       if(awkTrg.NF==0)
         std::cerr<<"Warning: target sentence "<<nsPairsInFiles<<" is empty"<<std::endl;

       nsPairsInFiles+=1;
     }
         // Print statistics
     if(nsPairsInFiles>0)
       std::cerr<<"#Sentence pairs in files: "<<nsPairsInFiles<<std::endl;
   }
       // Fill second field of sentRange
   sentRange.second=nsPairsInFiles-1;

       // Rewind files
   rewindFiles();
     
   return THOT_OK;	
 }
}

//-------------------------
void LightSentenceHandler::rewindFiles(void)
{
      // Rewind files
  awkSrc.rwd();
  awkTrg.rwd();
  awkSrcTrgC.rwd();

      // Read first entry
  getNextLineFromFiles();

      // Reset currFileSentIdx
  currFileSentIdx=0;
}

//-------------------------
void LightSentenceHandler::addSentPair(std::vector<std::string> srcSentStr,
                                       std::vector<std::string> trgSentStr,
                                       Count c,
                                       std::pair<unsigned int,unsigned int>& sentRange)
{
      // Fill sentRange information
  sentRange.first=nsPairsInFiles+sentPairCont.size();
  sentRange.second=sentRange.first;
      // add to sentPairCont
  sentPairCont.push_back(std::make_pair(srcSentStr,trgSentStr));
      // add to sentPairCount
  sentPairCount.push_back(c);

      // Display warnings if sentences are empty
  if(srcSentStr.empty())
    std::cerr<<"Warning: source sentence "<<sentRange.first<<" is empty"<<std::endl;
  if(trgSentStr.empty())
    std::cerr<<"Warning: target sentence "<<sentRange.first<<" is empty"<<std::endl;
}

//-------------------------
unsigned int LightSentenceHandler::numSentPairs(void)
{
  return nsPairsInFiles+sentPairCont.size();
}

//-------------------------
int LightSentenceHandler::nthSentPair(unsigned int n,
                                      std::vector<std::string>& srcSentStr,
                                      std::vector<std::string>& trgSentStr,
                                      Count& c)
{
  if(n>=numSentPairs())
    return THOT_ERROR;
  else
  {
    if(n<nsPairsInFiles)
    {
      return nthSentPairFromFiles(n,srcSentStr,trgSentStr,c);
    }
    else
    {
      size_t vecIdx=n-nsPairsInFiles;
      
      srcSentStr=sentPairCont[vecIdx].first;

      trgSentStr=sentPairCont[vecIdx].second;
      
      c=sentPairCount[vecIdx];
      
      return THOT_OK;
    }
  }
}

//-------------------------
int LightSentenceHandler::nthSentPairFromFiles(unsigned int n,
                                               std::vector<std::string>& srcSentStr,
                                               std::vector<std::string>& trgSentStr,
                                               Count& c)

{
      // Check if entry is contained in files
  if(n>=nsPairsInFiles)
    return THOT_ERROR;
  
      // Find corresponding entries
  if(currFileSentIdx>n)
      rewindFiles();

  if(currFileSentIdx!=n)
  {
    while(getNextLineFromFiles())
    {
      if(currFileSentIdx==n) break;
    }
  }

      // Reset variables
  srcSentStr.clear();
  trgSentStr.clear();

      // Extract information
  for(unsigned int i=1;i<=awkSrc.NF;++i) 
  {
    srcSentStr.push_back(awkSrc.dollar(i));
  }
  for(unsigned int i=1;i<=awkTrg.NF;++i)
  {
    trgSentStr.push_back(awkTrg.dollar(i));
  }

  if(countFileExists)
  {
    c=atof(awkSrcTrgC.dollar(1).c_str());
  }
  else
  {
    c=1;
  }
    
  return THOT_OK;
}

//-------------------------
bool LightSentenceHandler::getNextLineFromFiles(void)
{
  bool ret;
  
  ret=awkSrc.getln();
  if(ret==false) return false;

  ret=awkTrg.getln();
  if(ret==false) return false;

  if(countFileExists)
  {
    ret=awkSrcTrgC.getln();
    if(ret==false) return false;
  }

  ++currFileSentIdx;

  return true;
}

//-------------------------
int LightSentenceHandler::getSrcSent(unsigned int n,
                                     std::vector<std::string>& srcSentStr)
{
  std::vector<std::string> trgSentStr;
  Count c;

  int ret=nthSentPair(n,srcSentStr,trgSentStr,c);

  return ret;  
}

//-------------------------
int LightSentenceHandler::getTrgSent(unsigned int n,
                                     std::vector<std::string>& trgSentStr)
{
  std::vector<std::string> srcSentStr;
  Count c;

  int ret=nthSentPair(n,srcSentStr,trgSentStr,c);

  return ret;  
}

//-------------------------
int LightSentenceHandler::getCount(unsigned int n,
                                   Count& c)
{
  std::vector<std::string> srcSentStr;
  std::vector<std::string> trgSentStr;

  int ret=nthSentPair(n,srcSentStr,trgSentStr,c);

  return ret;  
}

//-------------------------
bool LightSentenceHandler::printSentPairs(const char *srcSentFile,
                                          const char *trgSentFile,
                                          const char *sentCountsFile)
{
  std::ofstream srcOutF;
  std::ofstream trgOutF;
  std::ofstream countsOutF;

      // Open file with source sentences
  srcOutF.open(srcSentFile,std::ios::out);
  if(!srcOutF)
  {
    std::cerr<<"Error while printing file with source sentences."<<std::endl;
    return THOT_ERROR;
  }

      // Open file with target sentences
  trgOutF.open(trgSentFile,std::ios::out);
  if(!trgOutF)
  {
    std::cerr<<"Error while printing file with target sentences."<<std::endl;
    return THOT_ERROR;
  }

      // Open file with sentence counts
  countsOutF.open(sentCountsFile,std::ios::out);
  if(!countsOutF)
  {
    std::cerr<<"Error while printing file with sentence counts."<<std::endl;
    return THOT_ERROR;
  }

  for(unsigned int n=0;n<numSentPairs();++n)
  {
    std::vector<std::string> srcSentStr;
    std::vector<std::string> trgSentStr;
    Count c;

    nthSentPair(n,srcSentStr,trgSentStr,c);
    
        // print source sentence
    for(unsigned int j=0;j<srcSentStr.size();++j)
    {
      srcOutF<<srcSentStr[j];
      if(j<srcSentStr.size()-1) srcOutF<<" ";
    }
    srcOutF<<std::endl;
      
        // print target sentence
    for(unsigned int j=0;j<trgSentStr.size();++j)
    {
      trgOutF<<trgSentStr[j];
      if(j<trgSentStr.size()-1) trgOutF<<" ";
    }
    trgOutF<<std::endl;
      
        // print count
    countsOutF<<c<<std::endl;
  }

    
      // Close output streams
  srcOutF.close();
  trgOutF.close();
  countsOutF.close();
  
  return THOT_OK;
}

//-------------------------
void LightSentenceHandler::clear(void)
{
  sentPairCont.clear();
  sentPairCount.clear();
  nsPairsInFiles=0;
  awkSrc.close();
  awkTrg.close();
  awkSrcTrgC.close();
  countFileExists=false;
  currFileSentIdx=0;
}
