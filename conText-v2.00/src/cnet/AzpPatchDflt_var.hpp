/* * * * *
 *  AzpPatchDflt_var.hpp
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

#ifndef _AZP_PATCH_DFLT_VAR_HPP_
#define _AZP_PATCH_DFLT_VAR_HPP_

#include "AzpPatch_var_.hpp"
#include "AzPmat.hpp"
#include "AzPmatApp.hpp"
#include "AzpPatchDflt.hpp"

/*!!!!! for 1D data only !!!!!*/

class AzpPatchDflt_var : public virtual AzpPatch_var_ {
protected: 
  AzpPatchDflt_Param p; 
  
  int cc; 
  AzPintArr pia_inp_dcolind; 
  AzPmatApp app; 
  AzPintArr2 pia2_inp2out; 
    
  static const int version = 0; 
  static const int reserved_len = 64;  
public:  
  AzpPatchDflt_var() : cc(0) {}
  virtual void resetParam(AzParam &azp, const char *pfx_dflt, const char *pfx, bool is_warmstart) {
    p.resetParam(azp, pfx_dflt, is_warmstart); 
    p.resetParam(azp, pfx, is_warmstart); 
    p.checkParam(pfx); 
  }  
  virtual void printParam(const AzOut &out, const char *pfx) const { p.printParam(out, pfx); }
  virtual void printHelp(AzHelp &h) const {}  
  virtual void read(AzFile *file) { 
    p.read(file); 
    AzTools::read_header(file, reserved_len); 
  }
  virtual void write(AzFile *file) { 
    p.write(file); 
    AzTools::write_header(file, version, reserved_len); 
  }

  /*------------------------------------------------------------*/      
  virtual void reset(const AzOut &out, int _cc, bool is_spa) {
    const char *eyec = "AzpPatchDflt_var::reset"; 
    AzX::no_support((is_spa && p.pch_sz > 0), eyec, "patch generation with sparse input"); 
    if (is_spa) return; /* no support for sparse input */

    AzX::no_support((p.pch_sz <= 0), eyec, "all-in-one with variable-sized input"); 
  
    reset(); 
    cc = _cc; 
    AzXi::throw_if_nonpositive(cc, eyec, "#channels");     
  }

  /*------------------------------------------------------------*/     
  virtual AzpPatch_var_ *clone() const {
    AzpPatchDflt_var *o = new AzpPatchDflt_var(); 
    o->p = p; 
    o->cc = cc; 
    o->pia_inp_dcolind.reset(&pia_inp_dcolind); 
    o->pia2_inp2out.reset(&pia2_inp2out); 
    return o; 
  } 
  
  virtual int patch_length() const { return p.pch_sz*cc; }
 
  /*------------------------------------------------------------*/ 
  virtual void upward(bool is_test, 
                      const AzPmatVar *m_inp, /* col: pixel */
                      AzPmatVar *m_out) /* col: patch */
  {   
    if (do_asis()) {
      m_out->set(m_inp); 
      return;       
    }
    AzX::throw_if((m_inp->rowNum() != cc), "AzpPatchDflt_var::upward", "#channel mismatch"); 
    if (!is_test) pia_inp_dcolind.reset(m_inp->d_index()); 

    AzPintArr2 pia2_out2inp; /* column to column */ 
    AzIntArr ia_out_dataind; 
    if (is_test) map(m_inp, &ia_out_dataind, NULL, &pia2_out2inp);
    else         map(m_inp, &ia_out_dataind, &pia2_inp2out, &pia2_out2inp);     
    AzPmat m_out_pch(cc, pia2_out2inp.size()); 
    app.add_with_map(1, m_inp->data(), &m_out_pch, cc, &pia2_out2inp);
    m_out_pch.change_dim(p.pch_sz*cc, m_out_pch.size()/p.pch_sz/cc); 
    m_out->set(&m_out_pch, &ia_out_dataind); 
  }
  
  /*------------------------------------------------------------*/ 
  virtual void downward(const AzPmatVar *m_out, 
                        AzPmatVar *m_inp)
  {    
    if (do_asis()) {
      m_inp->set(m_out); 
      return;       
    }  
    m_inp->reform(cc, &pia_inp_dcolind); 
    AzPmat m_inp_pch(m_inp->rowNum(), m_inp->colNum()); 
    app.add_with_map(1, m_out->data(), &m_inp_pch, cc, &pia2_inp2out);
    m_inp->update(&m_inp_pch); 
  }  
  
protected:   
  bool do_asis() const {
    return (p.pch_sz == 1 && p.pch_step == 1 && p.padding == 0); 
  }
  /*------------------------------------------------------------*/ 
  void map(const AzPmatVar *m_inp, 
           AzIntArr *ia_out_dataind,  /* unit: patch */
           AzPintArr2 *pia2_inp2out, /* unit: column (pixel) */
           AzPintArr2 *pia2_out2inp) /* unit: column (pixel) */ 
  const 
  {  
    const char *eyec = "AzpPatchDflt_var::map"; 
    int dx, pch_num; 
    AzDataArr<AzIntArr> aia_inp2out, aia_out2inp; 
    if (pia2_inp2out != NULL) aia_inp2out.reset(m_inp->colNum()); 
    if (pia2_out2inp != NULL) {
      pch_num = 0; 
      for (dx = 0; dx < m_inp->dataNum(); ++dx) {
        int col0, col1; m_inp->get_begin_end(dx, col0, col1); 
        pch_num += DIVUP(p.padding*2+col1-col0-p.pch_sz, p.pch_step) + 1;      
      }
      aia_out2inp.reset(pch_num*p.pch_sz); 
    }
    ia_out_dataind->reset();    

    pch_num = 0; 
    int ox = 0; 
    for (dx = 0; dx < m_inp->dataNum(); ++dx) {
      ia_out_dataind->put(pch_num); 
      int col0, col1; 
      m_inp->get_begin_end(dx, col0, col1); 
      int sz = col1 - col0; 
      int p_num = DIVUP(p.padding*2+sz-p.pch_sz, p.pch_step) + 1; 
      int pch_no; 
      for (pch_no = 0; pch_no < p_num; ++pch_no) {
        int pos0 = p.pch_step*pch_no - p.padding; 
        int pos1 = pos0 + p.pch_sz; 
        int px; 
        for (px = pos0; px < pos1; ++px, ++ox) { /* position within data */
          if (px >= 0 && px < sz) {
            if (pia2_inp2out != NULL) aia_inp2out.point_u(px+col0)->put(ox); 
            if (pia2_out2inp != NULL) aia_out2inp.point_u(ox)->put(px+col0);             
          }
        }
      }
      
      pch_num += p_num; 
      ia_out_dataind->put(pch_num); /* unit is a patch, not a column/pixel */
    }       
    if (pia2_out2inp != NULL) {
      AzX::throw_if((ox != aia_out2inp.size()), eyec, "conflict in the number of output regions"); 
      AzX::throw_if((pch_num != aia_out2inp.size()/p.pch_sz), eyec, "conflict in the number of patches"); 
    }
    if (pia2_inp2out != NULL) pia2_inp2out->reset(&aia_inp2out); 
    if (pia2_out2inp != NULL) pia2_out2inp->reset(&aia_out2inp); 
  }

  /*------------------------------------------------------------*/     
  virtual void reset() {
    cc = 0; 
    pia_inp_dcolind.reset(); 
    pia2_inp2out.reset(); 
  }        
};      
#endif 