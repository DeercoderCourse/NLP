/* * * * *
 *  AzpCNet3_multi_side.hpp
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

#ifndef _AZP_CNET3_MULTI_SIDE_HPP_
#define _AZP_CNET3_MULTI_SIDE_HPP_

#include "AzpCNet3_multi.hpp"

/*------------------------------------------------------------*/
class _AzpSideInfo {
protected: 
/*  static const int reserved_len=16; */
/*  static const int reserved_len=15;  04/08/2015 */ 
  static const int reserved_len=14; /* 04/26/2015 */ 
  static const int version=0; 

public:
  bool do_rand, do_fixed, do_prepped; 
  AzBytArr s_fn; 
  int dsno, sideno; 
  _AzpSideInfo() : do_rand(false), do_fixed(false), do_prepped(false), dsno(-1), sideno(-1) {}
  void write(AzFile *file) {
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(do_fixed);    /* 04/26/2015 */
    file->writeBool(do_prepped);  /* 04/08/2015 */
    file->writeBool(do_rand); 
    s_fn.write(file); 
    file->writeInt(dsno); 
    file->writeInt(sideno); 
  }
  void read(AzFile *file) {
    AzTools::read_header(file, reserved_len); 
    do_fixed = file->readBool();    /* 04/26/2015 */    
    do_prepped = file->readBool();  /* 04/08/2015 */
    do_rand = file->readBool(); 
    s_fn.read(file); 
    dsno = file->readInt(); 
    sideno = file->readInt(); 
  }
}; 
class AzpSideInfo {
protected:  
  bool is_all_fixed; 
/*  static const int reserved_len=16; */
  static const int reserved_len=15; /* 04/26/2015 */
  static const int version=0;  
public:
  AzDataArr<_AzpSideInfo> a; 
  AzpSideInfo() : is_all_fixed(false) {}

  bool is_all_sides_fixed() const { return is_all_fixed; }
  int sideNum() const { return a.size(); }

  #define kw_side_num "num_sides="
  void resetParam(AzParam &azp, int layer_no, int dflt_dsno, AzOut &out, const AzpData_ *data,  
                  int &sx) { /* inout */
    AzBytArr s_pfx; s_pfx << layer_no;  
    azp.reset_prefix(s_pfx.c_str());
    int side_num = 0;     
    azp.vInt(kw_side_num, &side_num); 
    azp.reset_prefix(); 

    resetParam(azp, side_num, layer_no, dflt_dsno, sx); 
    printParam(out, layer_no); 
    checkParam(data, layer_no); 
  }
  void write(AzFile *file) {
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(is_all_fixed); /* 04/26/2015 */
    a.write(file); 
  }
  void read(AzFile *file) {
    AzTools::read_header(file, reserved_len); 
    is_all_fixed = file->readBool(); 
    a.read(file); 
  }

  const char *gen_prefix(int layer_no, int side_idx, AzBytArr &s) const {
    s.reset(); 
/*    if (sideNum() == 1 && side_idx == 0) s << layer_no << "side_";  removed for simplicity: 08/24/2015 */
/*    else                                 s << layer_no << "side" << side_idx << "_"; */
    s << layer_no << "side" << side_idx << "_";
    return s.c_str(); 
  }
  
protected:       
  #define kw_fn "fn="
  #define kw_dsno "dsno="    
  #define kw_do_rand "Random"  
  #define kw_do_prepped "Prepped"
  #define kw_do_fixed "Fixed"
  void resetParam(AzParam &azp, int side_num, int layer_no, int dflt_dsno, int &sx) {  
    a.reset(); 
    if (side_num <= 0) return;   
  
    a.reset(side_num); 
    is_all_fixed = true; 
    for (int ix = 0; ix < side_num; ++ix) {  
      a(ix)->sideno = sx++; 
      a(ix)->dsno = dflt_dsno;    
 
      AzBytArr s_pfx; 
      azp.reset_prefix(gen_prefix(layer_no, ix, s_pfx)); 
      /* 0num_sides=2 0side0_fn=... 0side1_fn=...  */
      azp.swOn(&a(ix)->do_rand, kw_do_rand); 
      azp.swOn(&a(ix)->do_prepped, kw_do_prepped); 
      if (!a[ix]->do_rand && !a[ix]->do_prepped) azp.vStr(kw_fn, &a(ix)->s_fn);         
      azp.vInt(kw_dsno, &a(ix)->dsno);
      if (a[ix]->do_prepped) a(ix)->do_fixed = true; /* must be fixed if prepped */
      else                   azp.swOn(&a(ix)->do_fixed, kw_do_fixed);
      if (!a[ix]->do_rand && a[ix]->s_fn.length() <= 0) {
        AzBytArr s_weight_fn; azp.vStr(kw_weight_fn, &s_weight_fn); /* read by AzpLayerParam again */
        if (s_weight_fn.length() > 0) a(ix)->do_rand = true; 
      }
      azp.reset_prefix();
      if (!a[ix]->do_fixed) is_all_fixed = false; 
    }
  }
  void printParam(AzOut &out, int layer_no) const {
    if (a.size() <= 0) return; 
    AzPrint o(out); 
    for (int ix = 0; ix < a.size(); ++ix) {
      AzBytArr s_pfx; 
      o.reset_prefix(gen_prefix(layer_no, ix, s_pfx)); 
      o.printV(kw_fn, a[ix]->s_fn); 
      o.printV(kw_dsno, a[ix]->dsno); 
      o.printSw(kw_do_rand, a[ix]->do_rand); 
      o.printSw(kw_do_prepped, a[ix]->do_prepped); 
      o.printSw(kw_do_fixed, a[ix]->do_fixed); 
      o.reset_prefix(); 
    }
    if (is_all_fixed) {
      AzPrint::writeln(out, ""); 
      AzPrint::writeln(out, "   All side layer weights are fixed."); 
    }
    o.printEnd();     
  }
  void checkParam(const AzpData_ *data, int layer_no) const {
    const char *eyec = "AzpSideInfo::checkParam"; 
    for (int ix = 0; ix < a.size(); ++ix) {
      AzBytArr s_pfx; 
      const char *my_pfx = gen_prefix(layer_no, ix, s_pfx); 
      AzXi::invalid_input((a[ix]->dsno < 0 || a[ix]->dsno >= data->datasetNum()), 
                             eyec, kw_dsno, my_pfx); 
      if (!a[ix]->do_rand && !a[ix]->do_prepped) AzXi::throw_if_empty(a[ix]->s_fn, eyec, kw_fn, my_pfx);
      AzXi::throw_if_both((a[ix]->do_rand && a[ix]->do_prepped), eyec, kw_do_rand, kw_do_prepped, my_pfx); 
    }
  }
}; 

/*------------------------------------------------------------*/
class AzpCNet3_multi_side : public virtual AzpCNet3_multi {
protected:
  AzpLayers sidelays; 
  AzDataArr<AzpSideInfo> side_info; 
  AzDataArr<AzpLayerConn> side_conn; 
  
  bool has_side(int layer_no) const {
    if (layer_no < 0 || layer_no >= side_info.size()) return false; 
    return (side_info[layer_no]->sideNum() > 0); 
  }
  
  static const int reserved_len=64; 
  static const int version=0;   
public:
  AzpCNet3_multi_side(const AzpCompoSet_ *_cs) : AzpCNet3(_cs), AzpCNet3_multi(_cs) {}
  ~AzpCNet3_multi_side() {}
 
  virtual void write(AzFile *file) {
    AzpCNet3_multi::write(file); 
    AzTools::write_header(file, version, reserved_len); 
    sidelays.write(file); 
    side_info.write(file); 
    side_conn.write(file); 
  }
  virtual void read(AzFile *file) {
    AzpCNet3_multi::read(file); 
    int my_version = AzTools::read_header(file, reserved_len); 
    sidelays.read(cs, file); 
    side_info.read(file); 
    side_conn.read(file); 
  }
 
protected:
  /*---  override ...  ---*/  
  virtual void coldstart(const AzpData_ *trn, AzParam &azp); 
  virtual void warmstart(const AzpData_ *trn, AzParam &azp, bool for_testonly=false); 

  virtual void up_down(const AzpData_ *trn, const int *dxs, int d_num, 
                       double *out_loss=NULL, bool do_update=true); 
  virtual void up_with_conn(bool is_test, const AzDataArr<AzpData_X> &data, AzPmat *m_top_out);   
  
  virtual void lays_check_for_reg_L2init() const {
    AzpCNet3::lays_check_for_reg_L2init(); 
    for (int lx = 0; lx < sidelays.size(); ++lx) sidelays[lx]->check_for_reg_L2init(); 
  }   
  virtual void lays_flushDelta() {
    AzpCNet3::lays_flushDelta(); 
    for (int lx = 0; lx < lays.size(); ++lx) {
      if (!has_side(lx)) continue; 
      if (arr_doUpdate != NULL && arr_doUpdate[lx] != 1) continue; 
      for (int ix = 0; ix < side_info[lx]->sideNum(); ++ix) {
        int sx = side_info[lx]->a[ix]->sideno; 
        sidelays(sx)->flushDelta(do_update_lm, do_update_act); 
      }
    }
  }
  virtual void lays_end_of_epoch() {
    AzpCNet3::lays_end_of_epoch(); 
    for (int lx = 0; lx < sidelays.size(); ++lx) {
      if (!has_side(lx)) continue; 
      if (arr_doUpdate != NULL && arr_doUpdate[lx] != 1) continue;   
      for (int ix = 0; ix < side_info[lx]->sideNum(); ++ix) {
        int sx = side_info[lx]->a[ix]->sideno;
        sidelays(sx)->end_of_epoch(); 
      }
    }   
  }
  virtual void lays_multiply_to_stepsize(double coeff, const AzOut *out) {
    AzpCNet3::lays_multiply_to_stepsize(coeff, out); 
    for (int lx = 0; lx < sidelays.size(); ++lx) sidelays(lx)->multiply_to_stepsize(coeff, out);
  }  
  virtual void lays_set_momentum(double newmom, const AzOut *out) {
    AzpCNet3::lays_set_momentum(newmom, out); 
    for (int lx = 0; lx < sidelays.size(); ++lx) sidelays(lx)->set_momentum(newmom, out);     
  }
  virtual double lays_regloss(double *out_iniloss=NULL) const {
    double iniloss = 0; 
    double loss = AzpCNet3::lays_regloss(&iniloss); 
    for (int lx = 0; lx < sidelays.size(); ++lx) loss += sidelays[lx]->regloss(&iniloss); 
    if (out_iniloss != NULL) *out_iniloss = iniloss; 
    return loss;    
  }
  virtual void lays_show_stat(AzBytArr &s) const {
    for (int lx = 0; lx < lays.size(); ++lx) {
      s << "[" << lx << "]"; 
      lays[lx]->show_stat(s); 
      for (int ix = 0; ix < side_info[lx]->sideNum(); ++ix) {
        if (side_info[lx]->a[ix]->do_prepped) continue; 
        int sx = side_info[lx]->a[ix]->sideno; 
        s << "[" << lx << "side" << ix << "]"; 
        sidelays[sx]->show_stat(s); 
      }
    }
  }
  virtual int lays_show_size(AzBytArr &s) const {
    int sz = 0; 
    for (int lx = 0; lx < lays.size(); ++lx) {
      sz += lays[lx]->show_size(s); 
      for (int ix = 0; ix < side_info[lx]->sideNum(); ++ix) {
        if (side_info[lx]->a[ix]->do_prepped) continue;         
        int sx = side_info[lx]->a[ix]->sideno; 
        s << "[side" << ix << "]"; 
        sz += sidelays[lx]->show_size(s); 
      }
    }
    return sz; 
  }  
  
  /*---  new method  ---*/
  int reset_side_conn(int lx, const AzpData_ *data); 
  void side_coldstart(int lx, AzParam &azp, const AzpData_ *data); 
  void side_warmstart(int lx, AzParam &azp, const AzpData_ *data, bool for_testonly); 
}; 
#endif
