/* * * * *
 *  AzpWeight_adad.hpp
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

#ifndef _AZP_WEIGHT_ADAD_HPP_
#define _AZP_WEIGHT_ADAD_HPP_

#include "AzpWeightDflt.hpp"
#include "AzpLmAdaD.hpp"

/*---  AdaDelta  ---*/

/**********************************************************/  
class AzpWeight_adad : public virtual AzpWeightDflt {
protected: 
  AzDataArr<AzpLmAdaD> lmods_adad; 
  AzpLmAdaD_Param pa; 
  
  virtual void reset_lms() {
    lms_const.realloc(lmods_adad.size(), "AzpWeight_adad::reset_lms", "lms_const"); 
    lms.realloc(lmods_adad.size(), "AzpWeight_adad::reset_lms", "lms");    
    for (int lx = 0; lx < lmods_adad.size(); ++lx) {
      lms_const(lx, lmods_adad[lx]); 
      lms(lx, lmods_adad(lx)); 
    }
  }

public: 
  AzpWeight_adad() {}
  virtual void reset(int loc_num, /* used only when is_localw is true */
             int w_num, int inp_dim, bool is_spa, bool is_var) {
    AzX::throw_if((is_localw && (is_spa || is_var)), AzInputError, 
                  "AzpWeight_adad::reset", "No support for local weights with sparse or variable-sized input"); 
    int sz = (!is_localw) ? 1 : loc_num; 
    lmods_sgd.reset(); 
    lmods_adad.reset(sz); 
    for (int lx = 0; lx < lmods_adad.size(); ++lx) ((AzpLm *)lmods_adad(lx))->reset(w_num, p, inp_dim);
    reset_lms();    
  }    
  virtual AzpWeight_ *clone() const {
    AzpWeight_adad *o = new AzpWeight_adad();    
    o->lmods_adad.reset(&lmods_adad); 
    o->reset_lms(); 
    o->p = p; 
    o->pa = pa; 
    o->is_localw = is_localw;    
    o->lw.reset(&lw); 
    return o; 
  }  
  virtual void read(AzFile *file) {  
    read_common(file); 
    lmods_adad.read(file); /* AzpLm::read */ reset_lms(); 
  }  
  virtual void write(AzFile *file) {
    write_common(file); 
    lmods_adad.write(file);  /* AzpLm::write */
  }   
  
  /*------------------------------------------------------------*/ 
  virtual void _resetParam(AzParam &azp, const char *pfx, bool is_warmstart=false) {
    if (!is_warmstart) { /* no support for do_thru */
      azp.reset_prefix(pfx); 
      azp.swOn(&is_localw, kw_is_localw); 
      azp.reset_prefix(""); 
    }    
    p.resetParam(azp, pfx, is_warmstart); 
    pa.resetParam(azp, pfx, is_warmstart); 
  }   
  virtual void printParam(const AzOut &out, const char *pfx) const {
    AzPrint o(out, pfx);
    o.printSw(kw_is_localw, is_localw);   
    p.printParam(out, pfx); 
    pa.printParam(out, pfx); 
  }
  virtual void resetParam(AzParam &azp, const char *pfx, bool is_warmstart=false) {
    _resetParam(azp, pfx, is_warmstart); 
    p.checkParam(pfx); 
    pa.checkParam(pfx); 
  }  
  virtual void resetParam(AzParam &azp, const char *pfx_dflt, const char *pfx, bool is_warmstart=false) {
    _resetParam(azp, pfx_dflt, is_warmstart); 
    _resetParam(azp, pfx, is_warmstart); 
    p.checkParam(pfx); 
    pa.checkParam(pfx); 
  }  
  
  /*---  do something about parameters  ---*/
  virtual void multiply_to_stepsize(double factor, const AzOut *out=NULL) {
    pa.coeff = factor; 
  }  
  virtual void set_momentum(double newmom, const AzOut *out=NULL) {}

  virtual void flushDelta() {
    for (int lx = 0; lx < lmods_adad.size(); ++lx) lmods_adad(lx)->flushDelta(p, pa); 
  }
}; 

#endif 