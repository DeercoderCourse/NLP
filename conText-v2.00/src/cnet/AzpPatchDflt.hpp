/* * * * *
 *  AzpPatchDflt.hpp
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

#ifndef _AZP_PATCH_DFLT_HPP_
#define _AZP_PATCH_DFLT_HPP_

#include "AzpPatch_.hpp"
#include "AzPmatApp.hpp"
#include "AzxD.hpp"
#include "Az2D.hpp"

/* generate patches/filters for convolutional NNs */

class AzpPatchDflt_Param {  
protected: 
  static const int version = 0; 
  static const int reserved_len = 64;  
public: 
  int pch_sz, pch_step, padding; 
  bool do_simple_grid; 
   
  AzpPatchDflt_Param() : pch_sz(-1), pch_step(1), padding(0), do_simple_grid(true) {}    

  #define kw_pch_sz "patch_size="
  #define kw_pch_step "patch_stride="
  #define kw_padding "padding="
  #define kw_no_simple_grid "NoSimpleGridPatch"    
  virtual void setup_allinone() {
    pch_sz=-1; pch_step=1; padding=0; 
  }
  virtual void setup_asis() {
    pch_sz=1; pch_step=1; padding=0; 
  }
  virtual void setup(int _pch_sz, int _pch_step, int _padding, bool _do_simple_grid=true) {
    pch_sz=_pch_sz; pch_step=_pch_step; padding=_padding; do_simple_grid=_do_simple_grid; 
  }
  virtual void resetParam(AzParam &azp, const char *pfx, bool is_warmstart) {
    azp.reset_prefix(pfx); 
    if (!is_warmstart) {
      azp.vInt(kw_pch_sz, &pch_sz); 
      azp.vInt(kw_pch_step, &pch_step); 
      azp.vInt(kw_padding, &padding); 
      azp.swOff(&do_simple_grid, kw_no_simple_grid); 
    }  
    azp.reset_prefix(); 
  }
  virtual void checkParam(const char *pfx) {
    const char *eyec = "AzpPatchDflt::checkParam"; 
    if (pch_sz > 0) {
      AzXi::throw_if_nonpositive(pch_step, eyec, kw_pch_step, pfx); 
      AzXi::throw_if_negative(padding, eyec, kw_padding, pfx); 
    }
    else {
      pch_step = -1; padding = 0; do_simple_grid = true; 
    }
  }
  virtual void printParam(const AzOut &out, const char *pfx) const {
    AzPrint o(out); 
    o.reset_prefix(pfx); 
    if (pch_sz > 0) {
      o.printV(kw_pch_sz, pch_sz); 
      o.printV(kw_pch_step, pch_step); 
      o.printV(kw_padding, padding); 
      if (!do_simple_grid) o.printSw(kw_no_simple_grid, true); 
    }
  }
  virtual void printHelp(AzHelp &h) const {
    h.item(kw_pch_sz, "Region size in the convolution layer with dense input.  If not specified, the layer becomes a fully-connected layer."); 
    h.item(kw_pch_step, "Region stride.  For dense input only."); 
    h.item(kw_padding, "Padding size at the edge.  For dense input only."); 
    /* do_simple_grid */
  }
  virtual void write(AzFile *file) {
    AzTools::write_header(file, version, reserved_len);  
    file->writeInt(pch_sz); 
    file->writeInt(pch_step); 
    file->writeInt(padding); 
    file->writeBool(do_simple_grid); 
  }
  virtual void read(AzFile *file) {
    AzTools::read_header(file, reserved_len);   
    pch_sz = file->readInt(); 
    pch_step = file->readInt(); 
    padding = file->readInt(); 
    do_simple_grid = file->readBool(); 
  }    
}; 

class AzpPatchDflt : public virtual AzpPatch_ {
protected: 
  AzpPatchDflt_Param p; 

  AzxD i_region, o_region; 
  int cc;  /* # of channels */
  bool is_asis, is_allinone; 
  AzPintArr2 pia2_inp2out, pia2_out2inp; 
  Az2D tmpl;  /* filter template */
  AzIntArr ia_x0, ia_y0; /* filter location: upper left corner */ 
 
  AzPmatApp app; 
  
  static const int version = 0; 
  static const int reserved_len = 64;   
public:  
  AzpPatchDflt() : cc(0), is_asis(false), is_allinone(false) {}
  virtual void resetParam(AzParam &azp, const char *pfx_dflt, const char *pfx, bool is_warmstart) {
    p.resetParam(azp, pfx_dflt, is_warmstart); 
    p.resetParam(azp, pfx, is_warmstart); 
    p.checkParam(pfx); 
  }  
  virtual void printParam(const AzOut &out, const char *pfx) const { p.printParam(out, pfx); }
  virtual void printHelp(AzHelp &h) const { p.printHelp(h); }
  
  virtual void reset(const AzOut &out, const AzxD *input, int _cc, bool is_spa, bool is_var) {
    const char *eyec = "AzpPatchDflt::reset"; 
    reset(); 
    int dim = input->get_dim(); 
    AzX::no_support((dim != 1 && dim != 2), eyec, "anything other than 1D or 2D data"); 
    i_region.reset(input); 
    cc = _cc; 
    AzXi::throw_if_nonpositive(cc, eyec, "#channels"); 
    if (p.pch_sz <= 0) def_allinone(); 
    else {
      AzX::no_support(is_spa, eyec, "convolution layer with sparse input."); 
      def_filters(); 
    }     
  }  
  virtual void setup_allinone(const AzOut &out, const AzxD *input, int _cc) {
    p.setup_allinone(); 
    bool is_spa = false, is_var = false; 
    reset(out, input, _cc, is_spa, is_var); 
  }
  virtual void setup_asis(const AzOut &out, const AzxD *_input, int _cc) {
    p.setup_asis(); 
    AzxD input(_input); 
    if (!input.is_valid()) {
      int sz=2; 
      input.reset(&sz, 1); /* dummy as reset expects 1D or 2D fixed-sized data */
    }
    bool is_spa = false, is_var = false; 
    reset(out, &input, _cc, is_spa, is_var); 
  }
  virtual const AzxD *input_region(AzxD *o=NULL) const { 
    if (o != NULL) o->reset(&i_region); 
    return &i_region; 
  }
  virtual const AzxD *output_region(AzxD *o=NULL) const { 
    if (o != NULL) o->reset(&o_region); 
    return &o_region; 
  }
  
  /* size: # of pixels, length: size times cc */
  virtual int patch_size() const { return tmpl.size(); }  
  virtual int patch_length() const { return cc*patch_size(); }
  
  inline virtual bool no_change_in_shape() const {
    if (is_asis) return true; 
    return i_region.isSame(&o_region); 
  }  

  virtual void show_input(const char *header, const AzOut &out) const { i_region.show(header, out); }  
  virtual void show_output(const char *header, const AzOut &out) const { o_region.show(header, out); }  

  virtual int get_channels() const { return cc; }
  
  virtual bool is_convolution() const { return (p.pch_sz > 0); } 
  
  /*------------------------------------------------------------*/ 
  virtual void upward(bool is_test, /* not used */
                      const AzPmat *m_inp, /* each column represents a pixel; more than one data point */
                      AzPmat *m_out) const { /* each column represents a patch */
    const char *eyec = "AzpPatchDflt::upward"; 
    if (is_asis) {
      m_out->set(m_inp); 
      return; 
    }

    AzX::throw_if((m_inp->rowNum() != cc), eyec, "Conflict in # of channels"); 
    int isize = input_region()->size(); 
    int osize = output_region()->size(); 
    AzX::throw_if((m_inp->colNum() % isize != 0), eyec, "Conflict in input length"); 
    int data_num = m_inp->colNum() / isize; 
  
    int occ = patch_size()*cc;
    int rnum = occ; 
    int cnum = osize*data_num; 
    m_out->reform(rnum, cnum); 
    if (is_allinone) {
      m_out->fill(m_inp); 
      return; 
    }
   
    app.add_with_map(data_num, m_inp, m_out, cc, &pia2_out2inp);    
  }

  /*------------------------------------------------------------*/ 
  virtual void downward(const AzPmat *m_out, 
                        AzPmat *m_inp) const {
    const char *eyec = "AzpPatchDflt::downward"; 
    if (is_asis) {
      m_inp->set(m_out); 
      return; 
    }

    int occ = patch_size()*cc; 
    AzX::throw_if((m_out->rowNum() != occ), eyec, "Conflict in patch length"); 
    int isize = input_region()->size(); 
    int osize = output_region()->size(); 
    AzX::throw_if((m_out->colNum() % osize != 0), eyec, "Conflict in # of patches"); 
    int data_num = m_out->colNum() / osize; 

    int rnum = cc; 
    int cnum = isize*data_num; 
    m_inp->reform(rnum, cnum); 
    if (is_allinone) {
      m_inp->fill(m_out); 
      return; 
    }
    app.add_with_map(data_num, m_out, m_inp, cc, &pia2_inp2out);     
  }  
  
  /*------------------------------------------------------------*/     
  virtual void reset() {
    i_region.reset(); o_region.reset(); 
    is_asis = is_allinone = false; 
    cc = 0; 
    tmpl.reset(0,0); 
    ia_x0.reset(); ia_y0.reset();    
    pia2_inp2out.reset(); pia2_out2inp.reset(); 
  }
  
  /*------------------------------------------------------------*/     
  virtual AzpPatch_ *clone() const {
    AzpPatchDflt *o = new AzpPatchDflt(); 
    o->reset(this); 
    return o; 
  }
  /*------------------------------------------------------------*/     
  virtual void reset(const AzpPatchDflt *i) {
    p = i->p; 
    
    i_region.reset(&i->i_region); 
    o_region.reset(&i->o_region); 
    is_asis = i->is_asis; 
    is_allinone = i->is_allinone; 
    cc = i->cc; 
    pia2_inp2out.reset(&i->pia2_inp2out); 
    pia2_out2inp.reset(&i->pia2_out2inp);     
    
    tmpl = i->tmpl;  
    ia_x0.reset(&i->ia_x0); 
    ia_y0.reset(&i->ia_y0); 
  }
  /*------------------------------------------------------------*/     
  virtual void write(AzFile *file) {
    p.write(file); 
    AzTools::write_header(file, version, reserved_len);    
    i_region.write(file); 
    o_region.write(file); 
    file->writeBool(is_asis); 
    file->writeBool(is_allinone); 
    file->writeInt(cc); 
    pia2_inp2out.write(file); 
    pia2_out2inp.write(file); 
    
    tmpl.write(file); 
    ia_x0.write(file); 
    ia_y0.write(file);     
  }
  /*------------------------------------------------------------*/   
  virtual void read(AzFile *file) {
    p.read(file); 
    AzTools::read_header(file, reserved_len); 
    i_region.read(file); 
    o_region.read(file); 
    is_asis = file->readBool(); 
    is_allinone = file->readBool(); 
    cc = file->readInt(); 
    pia2_inp2out.read(file); 
    pia2_out2inp.read(file); 
    
    tmpl.read(file); 
    ia_x0.read(file); 
    ia_y0.read(file);    
  }  
  /*------------------------------------------------------------*/                         
  virtual void reset_for_pooling(
                       const AzxD *input,   
                       int pch_sz, int pch_step, int pch_padding, 
                       bool do_simple_grid, 
                       /*---  output  ---*/
                       AzPintArr2 *pia2_out2inp, 
                       AzPintArr2 *pia2_inp2out,
                       AzPintArr *pia_out2num) {
    p.setup(pch_sz, pch_step, pch_padding, do_simple_grid);                         
    AzOut dummy_out; int dummy_cc=1; 
    reset(dummy_out, input, dummy_cc, false, false);    
    mapping_for_pooling(pia2_out2inp, pia2_inp2out, pia_out2num); 
  }                         
  static void mapping_for_resnorm(const AzxD *region, int width, 
                                   AzPintArr2 *pia2_neighbors,
                                   AzPintArr2 *pia2_whose_neighbor, 
                                   AzPintArr *pia_neighsz); 
  
protected:   
  virtual void def_allinone(); 
  virtual void def_filters();  
  void set_dim_back();  
  void def_patches(int pch_xsz, int pch_xstride, int xpadding,
                   int pch_ysz, int pch_ystride, int ypadding, 
                   bool do_simple_grid);   
  void _set_simple_grids(int sz, int pch_sz, int step, 
                         AzIntArr *ia_p0) const {
    int pch_num = DIVUP(sz-pch_sz, step) + 1; 
    for (int px = 0; px < pch_num; ++px) ia_p0->put(px*step); 
  }                
  void _set_grids(int sz, int pch_sz, int stride, AzIntArr *ia_p0) const;   
  virtual void mapping_for_filtering(AzPintArr2 *pia2_out2inp, 
                                     AzPintArr2 *pia2_inp2out) const;   
  virtual void mapping_for_pooling(AzPintArr2 *pia2_out2inp, 
                       AzPintArr2 *pia2_inp2out,
                       AzPintArr *pia_out2num) const;                   
};      
#endif 