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

#include "chrf.h"

//---------------
int calculate_chrf_file_name(const char* ref,
                             const char* sys,
                             float& chrf,
                             Vector<float>& chrf_n,
                             int verbosity)
{
    FILE *refFile;
    FILE *sysFile;
    int ret;

    refFile=fopen(ref,"r");
    if(refFile==NULL)
    {
        cerr<<"Error while opening file with references: "<<ref<<endl;
        return ERROR;
    }

    sysFile=fopen(sys,"r");
    if(sysFile==NULL)
    {
        cerr<<"Error while opening file with translations: "<<sys<<endl;
        return ERROR;
    }

    ret=calculate_chrf_file(refFile,sysFile,chrf,chrf_n,verbosity);

    fclose(refFile);
    fclose(sysFile);

    return ret;
}

//---------------
int calculate_chrf_file(FILE *reff,
                        FILE *sysf,
                        float& chrf,
                        Vector<float>& chrf_n,
                        int verbosity)
{
    awkInputStream refStream;
    awkInputStream sysStream;
    unsigned int numSents=0;
    unsigned int refChars=0;
    unsigned int sysChars=0;
    chrf=0;

    chrf_n.clear();

    // Open files
    if(refStream.open_stream(reff)==ERROR)
    {
        cerr<<"Invalid file pointer to file with references."<<endl;
        return ERROR;
    }
    if(sysStream.open_stream(sysf)==ERROR)
    {
        cerr<<"Invalid file pointer to file with system translations."<<endl;
        return ERROR;
    }

    while(refStream.getln())
    {
        bool ok=sysStream.getln();
        if(!ok)
        {
            cerr<<"Unexpected end of system file."<<endl;
            return ERROR;
        }

        ++numSents;
        if(verbosity) cerr<<numSents<<endl;

        std:string refSentence=refStream.dollar(0);
        if(verbosity) cerr<<refSentence<<" ";

        std::string sysSentence=sysStream.dollar(0);
        if(verbosity) cerr<<sysSentence<<" ";

        refChars+=refSentence.size();
        sysChars+=sysSentence.size();

        if(verbosity) cerr<<endl;

        float chrf_i;
        calculate_chrf(refSentence, sysSentence, chrf_i);
        chrf_n.push_back(chrf_i);
        chrf+=chrf_i;

        if(verbosity) cerr<<"chrf: " << chrf_i;
        if(verbosity) cerr<<endl<<endl;
    }

    chrf /= numSents;

    if(verbosity)
    {
        cerr<<"#Sentences: "<<numSents<<endl;
        cerr<<"ref. chars: "<<refChars<<endl;
        cerr<<"sys. chars: "<<sysChars<<endl;
        cerr<<"chrf: "<<chrf<<endl;
    }

    return OK;
}

void calculate_chrf(const std::string& refSentence,
                    const std::string& sysSentence,
                    float& chrf)
{
    float totalPrecision = 0;
    float totalRecall = 0;

    std::string refSentenceTokenized;
    std::string sysSentenceTokenized;

    if (CONSIDER_WHITESPACE) {
        refSentenceTokenized = StrProcUtils::stringVectorToString(StrProcUtils::stringToStringVector(refSentence));
        sysSentenceTokenized = StrProcUtils::stringVectorToString(StrProcUtils::stringToStringVector(sysSentence));
    } else {
        refSentenceTokenized = StrProcUtils::stringVectorToStringWithoutSpaces(StrProcUtils::stringToStringVector(refSentence));
        sysSentenceTokenized = StrProcUtils::stringVectorToStringWithoutSpaces(StrProcUtils::stringToStringVector(sysSentence));
    }

    for(unsigned int i=1;i<=std::min((unsigned long)MAX_NGRAM_LENGTH,refSentenceTokenized.size());++i)
    {
        unsigned int count;
        float precision;
        float recall;

        count_ngrams(refSentenceTokenized,sysSentenceTokenized,i,precision,recall,count);
        totalPrecision += precision;
        totalRecall += recall;
    }

    totalPrecision /= std::min((unsigned long)MAX_NGRAM_LENGTH,sysSentenceTokenized.size());
    totalRecall /= std::min((unsigned long)MAX_NGRAM_LENGTH,refSentenceTokenized.size());

    if (totalPrecision==0 || totalRecall==0) chrf=0;
    else chrf = (1 + BETA*BETA)*(totalPrecision*totalRecall/(BETA*BETA*totalPrecision + totalRecall));
}

void count_ngrams(const std::string& refSentence,
                  const std::string& sysSentence,
                  unsigned int ngramLength,
                  float& precision,
                  float& recall,
                  unsigned int& count)
{
    unsigned int i;
    unsigned int j;
    unsigned int countsys=0;
    unsigned int countref=0;
    unsigned int systotal;
    unsigned int reftotal;
    Vector<bool> matched;


    if(ngramLength>sysSentence.size()) systotal=0;
    else systotal=sysSentence.size()-ngramLength+1;

    if(ngramLength>refSentence.size()) reftotal=0;
    else reftotal=refSentence.size()-ngramLength+1;

    count=0;

    for(i=0;i<reftotal;++i)
    {
        matched.push_back(false);
    }
    for(i=0;i<systotal;++i)
    {
        for(j=0;j<reftotal;++j)
        {
            bool match=true;
            for(unsigned int k=0;k<ngramLength;++k)
            {
                if(sysSentence[i+k]!=refSentence[j+k])
                {
                    match=false;
                    break;
                }
            }
            if(match && !matched[j])
            {
                ++countsys;
                matched[j]=true;
                break;
            }
        }
    }
    matched.clear();
    for(i=0;i<systotal;++i)
    {
        matched.push_back(false);
    }
    for(i=0;i<reftotal;++i)
    {
        for(j=0;j<systotal;++j)
        {
            bool match=true;
            for(unsigned int k=0;k<ngramLength;++k)
            {
                if(refSentence[i+k]!=sysSentence[j+k])
                {
                    match=false;
                    break;
                }
            }
            if(match && !matched[j])
            {
                ++countref;
                matched[j]=true;
                break;
            }
        }
    }

    count = std::min(countref, countsys);
    precision = systotal == 0 ? 1 : (float)count/systotal;
    recall = reftotal == 0 ? 1 : (float)count/reftotal;
}
