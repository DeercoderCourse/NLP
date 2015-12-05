/* * * * *
 *  AzpActiv_.hpp
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

#ifndef _AZP_ACTIV__HPP_
#define _AZP_ACTIV__HPP_

#include "AzpCompo_.hpp"
#include "AzPmat.hpp"

/*------------------------------------------------------------*/
class AzpActiv_ : public virtual AzpCompo_ {
public:
  virtual ~AzpActiv_() {}
  virtual AzpActiv_ *clone() const = 0; 
  virtual void reset(const AzOut &out) = 0;  
  virtual void setup_no_activation() = 0; 
  virtual void upward(bool is_test, AzPmat *m) = 0; 
  virtual void downward(AzPmat *m) = 0; 
  virtual void updateDelta(int d_num) {}
  virtual void flushDelta() {}
  virtual void end_of_epoch() {}

  virtual int output_channels(int node_num) const { return node_num; }
  virtual void show_stat(AzBytArr &s) const {}
  virtual bool is_param_trained() const { return false; }
  virtual void copy_trained_params_from(const AzpActiv_ *) {
    AzX::no_support(true, "AzpActiv_::copy_trained_params", "copying trained parameters"); 
  }
}; 

class AzpActiv_sel_ {
public: 
  virtual AzpActiv_ *activ(const AzOut &out, AzParam &azp, const char *pfx, bool is_warmstart=false) = 0; 
  virtual AzpActiv_ *activ(const AzOut &out, AzParam &azp, const char *dflt_pfx, const char *pfx, bool is_warmstart=false) = 0;   
};
#endif 