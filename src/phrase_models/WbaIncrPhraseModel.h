/*
thot package for statistical machine translation
Copyright (C) 2013-2017 Daniel Ortiz-Mart\'inez, Adam Harasimowicz
 
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
 
/********************************************************************/
/*                                                                  */
/* Module: WbaIncrPhraseModel                                       */
/*                                                                  */
/* Prototype file: WbaIncrPhraseModel.h                             */
/*                                                                  */
/* Description: Defines the WbaIncrPhraseModel class.               */
/*              WbaIncrPhraseModel implements a phrase model which  */
/*              use word-based alignments (as those obtained with   */
/*              the GIZA++ tool).                                   */
/*                                                                  */
/********************************************************************/

#ifndef _WbaIncrPhraseModel_h
#define _WbaIncrPhraseModel_h

//--------------- Include files --------------------------------------

#if HAVE_CONFIG_H
#  include <thot_config.h>
#endif /* HAVE_CONFIG_H */

#ifndef THOT_HAVE_CXX11
#  include "StlPhraseTable.h"
#else
#  include "HatTriePhraseTable.h"
#endif

#include "_wbaIncrPhraseModel.h"
#include "PhraseExtractionTable.h"

//--------------- Constants ------------------------------------------

#define VERBOSE_AACHEN  -1
	 
//--------------- function declarations ------------------------------


//--------------- Classes --------------------------------------------


//--------------- WbaIncrPhraseModel class

class WbaIncrPhraseModel: public _wbaIncrPhraseModel
{
 public:

    typedef _wbaIncrPhraseModel::SrcTableNode SrcTableNode;
    typedef _wbaIncrPhraseModel::TrgTableNode TrgTableNode;

        // Constructor
    WbaIncrPhraseModel(void):_wbaIncrPhraseModel()
      {

#ifndef THOT_HAVE_CXX11
        basePhraseTablePtr = new StlPhraseTable;
#else
        basePhraseTablePtr = new HatTriePhraseTable;
#endif
      }

        // Destructor
	~WbaIncrPhraseModel();
	
 protected:

        // Functions to print models using standard C library
    void printTTable(FILE* file);
};

#endif
