/* * * * *
 *  AzpCompo_.hpp
 *  Copyright (C) 2014-2015 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#ifndef _AZP_COMPO__HPP_
#define _AZP_COMPO__HPP_

#include "AzParam.hpp"
#include "AzHelp.hpp"

/*------------------------------------------------------------*/
class AzpCompo_ { 
public:
  virtual ~AzpCompo_() {}
  virtual void resetParam(const AzOut &out, AzParam &azp, 
                          const char *pfx_dflt, const char *pfx, bool is_warmstart=false) {
    resetParam(azp, pfx_dflt, pfx, is_warmstart); 
    printParam(out, pfx); 
  }   
  virtual void resetParam(AzParam &azp, const char *pfx0, const char *pfx1, bool is_warmstart=false) = 0;  
  virtual void printParam(const AzOut &out, const char *pfx) const = 0; 
  virtual void printHelp(AzHelp &h) const = 0; 
  virtual void write(AzFile *file) = 0; 
  virtual void read(AzFile *file) = 0;
}; 
#endif 

