/* * * * *
 *  AzpPoolingDflt.hpp
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

#ifndef _AZP_POOLING_DFLT_HPP_
#define _AZP_POOLING_DFLT_HPP_

#include "AzPrint.hpp"
#include "AzPmat.hpp"
#include "AzPmatApp.hpp"
#include "AzpPatchDflt.hpp"
#include "AzpPooling_.hpp"

#define AzpPoolingDflt_Avg 'A'
#define AzpPoolingDflt_Max 'M'
#define AzpPoolingDflt_L2 'L'
#define AzpPoolingDflt_None 'N'

#define kw_pl_sz "pooling_size="
#define kw_pl_step "pooling_stride="
#define kw_pl_type "pooling_type="
#define kw_do_pl_simple_grid "SimpleGridPool"  
#define kw_no_pl_simple_grid "NoSimpleGridPool"  
#define kw_pl_num "num_pooling="

/*-----   Pooling with fixed-sized input   -----*/
/*---  parameters  ---*/
class AzpPoolingDflt_Param {
protected: 
  static const int version = 0; 
/*  static const int reserved_len = 64;  */
  static const int reserved_len = 60; /* for pl_num: 7/2/2015 */
public: 
  int pl_num;  /* added 7/2/2015 */
  int pl_sz, pl_step; 
  AzByte ptyp;
  AzBytArr s_pl_type; 
  bool do_pl_simple_grid; 
  AzpPoolingDflt_Param() : pl_num(-1), s_pl_type("None"), ptyp(AzpPoolingDflt_None), pl_sz(-1), pl_step(-1), do_pl_simple_grid(true) {}
    
  void resetParam(AzParam &azp, const char *pfx, bool is_warmstart) {
    azp.reset_prefix(pfx); 
    if (!is_warmstart) {
      azp.vStr(kw_pl_type, &s_pl_type); 
      if (s_pl_type.length() > 0) ptyp = *s_pl_type.point(); 
      else                        ptyp = AzpPoolingDflt_None; 
      azp.vInt(kw_pl_num, &pl_num); 
      azp.vInt(kw_pl_sz, &pl_sz); 
      azp.vInt(kw_pl_step, &pl_step); 
      azp.swOff(&do_pl_simple_grid, kw_no_pl_simple_grid); 
    }
    azp.reset_prefix(); 
  }
  void checkParam(const char *pfx) {
    const char *eyec = "AzpPoolingDflt::resetParam"; 
    AzXi::invalid_input((ptyp != AzpPoolingDflt_Avg && ptyp != AzpPoolingDflt_Max && 
        ptyp != AzpPoolingDflt_L2 && ptyp != AzpPoolingDflt_None), eyec, kw_pl_type, pfx);    
    if (ptyp == AzpPoolingDflt_None) {   
      pl_sz = pl_step = 1; 
      pl_num = -1; 
      do_pl_simple_grid = false; 
    } 
    AzX::throw_if((pl_num <= 0 && pl_sz <= 0), AzInputError, eyec, "Either num_pooling or pooling_size must be specified.  layer#", pfx); 
    AzX::throw_if((pl_num > 0 && pl_sz > 0), AzInputError, eyec, "Either num_pooling or pooling_size must be specified.  Not both.  layer#", pfx); 
    AzX::throw_if((pl_sz > 0 && pl_step <= 0), AzInputError, eyec, "pooling_size must be accompanied with pooling_stride.  layer#", pfx); 
  }
  
  virtual void printParam(const AzOut &out, const char *pfx) const {
    if (ptyp == AzpPoolingDflt_None) return; 
    AzPrint o(out, pfx); 
    o.printV(kw_pl_type, s_pl_type); 
    o.printV_posiOnly(kw_pl_num, pl_num); 
    o.printV(kw_pl_sz, pl_sz); 
    o.printV(kw_pl_step, pl_step); 
    if (!do_pl_simple_grid) o.printSw(kw_no_pl_simple_grid, true); /* show only when it's off */
    o.printEnd(); 
  }    
  virtual void printHelp(AzHelp &h) const {
    h.item(kw_pl_type, "Pooling type.  \"Max\" | \"Avg\" | \"L2\" | \"None\"", "None"); 
    h.item(kw_pl_sz, "Pooling region size."); 
    h.item(kw_pl_step, "Pooling region stride."); 
    h.item(kw_pl_num, "Number of pooling units.  Specify either pooling_size or num_pooling.  Not both.");     
    /* kw_do_pl_simple_grid */
  }
    
  void write(AzFile *file) {
    AzTools::write_header(file, version, reserved_len); 
    file->writeInt(pl_num); /* 7/2/2015 */
    file->writeInt(pl_sz); 
    file->writeInt(pl_step); 
    file->writeByte(ptyp); 
    s_pl_type.write(file); 
    file->writeBool(do_pl_simple_grid); 
  }
  void read(AzFile *file) {
    AzTools::read_header(file, reserved_len); 
    pl_num = file->readInt(); /* 7/2/2015 */
    pl_sz = file->readInt(); 
    pl_step = file->readInt(); 
    ptyp = file->readByte(); 
    s_pl_type.read(file);    
    do_pl_simple_grid = file->readBool(); 
  }
};

/*------------------------------------------------------------*/
class AzpPoolingDflt : public virtual AzpPooling_ {
protected:
  AzpPoolingDflt_Param p; 
  AzpPatchDflt map; 
  AzPintArr2 pia2_out2inp, pia2_inp2out; 
  AzPintArr pia_out2num; 

  int innum; 
  
  AzPintArr pia_chosen; /* used only by max pooling */
  AzPmat m_inp_save, m_out_save; /* used only by l2 pooling */
  
  AzPmatApp app;   
  static const int version = 0; 
  static const int reserved_len = 64;    
public:
  virtual void get_chosen(AzIntArr *ia_chosen) const { pia_chosen.get(ia_chosen); }
  AzpPoolingDflt() : innum(0) {} 
  virtual void resetParam(AzParam &azp, const char *pfx_dflt, const char *pfx, bool is_warmstart) {
    p.resetParam(azp, pfx_dflt, is_warmstart); 
    p.resetParam(azp, pfx, is_warmstart); 
    p.checkParam(pfx); 
  }  
  virtual void printParam(const AzOut &out, const char *pfx) const { p.printParam(out, pfx); }
  virtual void printHelp(AzHelp &h) const { p.printHelp(h); }
  
  void reset(const AzOut &out, const AzxD *input, AzxD *output) {
    if (p.pl_num > 0) {
      AzX::throw_if((input->get_dim() != 1), "AzpPoolingDflt::reset", 
                    "num_pooling is allowed only with 1D data"); 
      p.pl_sz = p.pl_step =  DIVUP(input->sz(0), p.pl_num);                    
      AzBytArr s("Given num_pooling="); s << p.pl_num; 
      s << ", setting pooling size and stride to " << p.pl_sz; 
      AzPrint::writeln(out, s);       
    }
    int minsz = input->get_min_size();   
    if (p.pl_sz > minsz) {
      p.pl_sz = minsz; 
      AzBytArr s("pooling unit size is too large: shrinking to "); s.cn(p.pl_sz); 
      AzPrint::writeln(out, s); 
    }  
    int padding = 0; 
    map.reset_for_pooling(input, p.pl_sz, p.pl_step, padding, p.do_pl_simple_grid, 
                          &pia2_out2inp, &pia2_inp2out, &pia_out2num);     
    innum = input->size(); 
    if (p.ptyp == AzpPoolingDflt_None) {
      AzX::throw_if((!map.no_change_in_shape()), "AzpPoolingDflt::reset", "input and output must be the same for no pooling"); 
    }
    map.output_region(output); 
  }
  virtual void output(AzxD *output) {
    map.output_region(output); 
  }
  AzpPooling_ *clone() const {
    AzpPoolingDflt *o = new AzpPoolingDflt(); 
    o->p = p; 
    o->map.reset(&map); 
    o->innum = innum; 
    o->pia2_out2inp.reset(&pia2_out2inp); 
    o->pia2_inp2out.reset(&pia2_inp2out); 
    o->pia_out2num.reset(&pia_out2num); 
    o->m_inp_save.set(&m_inp_save); 
    o->m_out_save.set(&m_out_save); 
    o->pia_chosen.reset(&pia_chosen); 
    return o; 
  }
  virtual int input_size() const { return innum; }
  virtual int output_size() const { return pia2_out2inp.size(); }
  virtual void upward(bool is_test, const AzPmat *m_inp, AzPmat *m_out) {
    if (is_test) _apply(m_inp, m_out); 
    else         _upward(m_inp, m_out); 
  }
  virtual void downward(const AzPmat *m_lossd_after, AzPmat *m_lossd_before) const {
    int r_num = m_lossd_after->rowNum(); 
    int c_num = m_lossd_after->colNum()/output_size()*input_size(); 
    m_lossd_before->reform(r_num, c_num); 
    if      (p.ptyp == AzpPoolingDflt_Avg) downward_avg(m_lossd_after, m_lossd_before); 
    else if (p.ptyp == AzpPoolingDflt_Max) downward_max(m_lossd_after, m_lossd_before); 
    else if (p.ptyp == AzpPoolingDflt_L2)  downward_l2(m_lossd_after, m_lossd_before); 
    else if (p.ptyp == AzpPoolingDflt_None) downward_asis(m_lossd_after, m_lossd_before); 
  }
  
  void write(AzFile *file) {
    p.write(file); 
    AzTools::write_header(file, version, reserved_len);   
    map.write(file); 
    file->writeInt(innum); 
    pia2_out2inp.write(file); 
    pia2_inp2out.write(file); 
    pia_out2num.write(file); 
  }
  void read(AzFile *file) {
    p.read(file); 
    AzTools::read_header(file, reserved_len); 
    map.read(file); 
    innum = file->readInt(); 
    pia2_out2inp.read(file); 
    pia2_inp2out.read(file);
    pia_out2num.read(file);
  }
    
  /*------------------------------------------------------------*/
  inline void show(const AzOut &out) const {
    AzPrint::writeln(out, "AzpPoolingDflt::show"); 
    int sz = pia2_out2inp.size(); 
    int ox;
    for (ox = 0; ox < sz; ++ox) {
      AzBytArr s("["); s.cn(ox); s.c("] "); 
      AzIntArr ia; 
      pia2_out2inp.get(ox, &ia); 
      ia.print(out, s.c_str()); 
    }
  }
  
protected:
  virtual void _upward(const AzPmat *m_inp, AzPmat *m_out) {  
    int r_num = m_inp->rowNum(); 
    int c_num = m_inp->colNum()/input_size()*output_size(); 
    m_out->reform(r_num, c_num); 
    if      (p.ptyp == AzpPoolingDflt_Avg) upward_avg(m_inp, m_out); 
    else if (p.ptyp == AzpPoolingDflt_Max) upward_max(m_inp, m_out); 
    else if (p.ptyp == AzpPoolingDflt_L2)  upward_l2(m_inp, m_out); 
    else if (p.ptyp == AzpPoolingDflt_None) upward_asis(m_inp, m_out); 
  }
  virtual void _apply(const AzPmat *m_inp, AzPmat *m_out) const {
    m_out->reform(m_inp->rowNum(), m_inp->colNum()/input_size()*output_size()); 
    if      (p.ptyp == AzpPoolingDflt_Avg) upward_avg(m_inp, m_out); 
    else if (p.ptyp == AzpPoolingDflt_Max) apply_max(m_inp, m_out); 
    else if (p.ptyp == AzpPoolingDflt_L2)  apply_l2(m_inp, m_out); 
    else if (p.ptyp == AzpPoolingDflt_None) upward_asis(m_inp, m_out); 
  }

  /*---  no pooling  ---*/
  inline void upward_asis(const AzPmat *m_inp, AzPmat *m_out) const {
    m_out->set_chk(m_inp); 
  }
  inline void downward_asis(const AzPmat *m_lossd_after, AzPmat *m_lossd_before) const {
    m_lossd_before->set_chk(m_lossd_after); 
  }

  /*---  average pooling  ---*/
  inline void upward_avg(const AzPmat *m_inp, AzPmat *m_out) const {
    app.pooling_avg(m_inp, input_size(), m_out, &pia2_out2inp); 
  }
  inline void downward_avg(const AzPmat *m_out, AzPmat *m_inp) const {
    app.unpooling_avg(m_inp, m_out, output_size(), &pia2_inp2out, &pia_out2num); 
  }
  
  /*---  l2 pooling  ---*/
  inline void upward_l2(const AzPmat *m_inp, AzPmat *m_out) {
    app.pooling_l2(m_inp, input_size(), m_out, &pia2_out2inp); 
    m_inp_save.set(m_inp); 
    m_out_save.set(m_out); 
  }
  inline void apply_l2(const AzPmat *m_inp, AzPmat *m_out) const {
    app.pooling_l2(m_inp, input_size(), m_out, &pia2_out2inp); 
  }
  inline void downward_l2(const AzPmat *m_out, AzPmat *m_inp) const {
    app.unpooling_l2(m_inp, m_out, output_size(), &pia2_inp2out, &m_inp_save, &m_out_save); 
  }
  
  /*---  max pooling  ---*/
  inline void upward_max(const AzPmat *m_inp, AzPmat *m_out) {
    pia_chosen.alloc(m_out->size()); 
    pia_chosen.set(-1); 
    app.pooling_max(m_inp, input_size(), m_out, &pia2_out2inp, &pia_chosen); 
  }
  inline void apply_max(const AzPmat *m_inp, AzPmat *m_out) const {
    app.pooling_max(m_inp, input_size(), m_out, &pia2_out2inp, NULL); 
  }
  inline void downward_max(const AzPmat *m_out, AzPmat *m_inp) const {
    app.unpooling_max(m_inp, m_out, &pia_chosen, &pia2_inp2out); 
  }
}; 
#endif 

