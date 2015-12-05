/* * * * *
 *  AzpDataSetDflt.hpp
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

#include "AzpData_.hpp"
#include "AzpData_img.hpp"
#include "AzpData_sparse.hpp"
#include "AzpData_sparse_multi.hpp"

/***
 *  sparse_large or sparse_large_multi should be used only when the number 
 *  of non-zero components exceed 2GB.  
 *  Otherwise, sparse or sparse_multi is better as it consumes less CPU memory.
 ***/
class AzpDataSetDflt : public virtual AzpDataSet_ {
protected: 
  AzpData_img img; 
  AzpData_sparse<AzSmatc> sparse; 
  AzpData_sparse_multi<AzSmatc> sparse_multi; 
  AzpData_sparse<AzSmat> sparse_large; 
  AzpData_sparse_multi<AzSmat> sparse_large_multi;   
public:   
  virtual void printHelp(AzHelp &h, bool do_train, bool do_test, bool is_there_y) const {
    AzpDataSet_::printHelp(h, do_train, do_test, is_there_y); 
    h.item_required(kw_datatype, "Dataset type.  \"image\" | \"sparse\" | \"sparse_multi\" | \"sparse_large\" | \"sparse_large_multi\""); 
    /* img.printHelp(h, do_train, is_there_y); */
    sparse.printHelp(h, do_train, is_there_y); 
    /* sparse_multi.printHelp(h, do_train, is_there_y); */
  }  
protected:   
  virtual const AzpData_ *ptr(const AzBytArr &s_typ) const { 
    if      (s_typ.compare("image") == 0)  return &img; 
    else if (s_typ.compare("sparse") == 0) return &sparse; 
    else if (s_typ.compare("sparse_multi") == 0) return &sparse_multi;      
    else if (s_typ.compare("sparse_large") == 0) return &sparse_large; 
    else if (s_typ.compare("sparse_large_multi") == 0) return &sparse_large_multi;     
    else {
      AzX::throw_if(true, AzInputError, "AzpDataSetDflt::ptr", "Unknown data type: ", s_typ.c_str()); return NULL; 
    }
  }
}; 