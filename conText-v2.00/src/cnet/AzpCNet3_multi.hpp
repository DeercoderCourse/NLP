/* * * * *
 *  AzpCNet3_multi.hpp
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

#ifndef _AZP_CNET3_MULTI_HPP_
#define _AZP_CNET3_MULTI_HPP_

#include "AzpCNet3.hpp"
#include "AzpLayerConn.hpp"

/*------------------------------------------------------------*/
class AzpCNet3_multi : public virtual AzpCNet3 {
protected:
  AzIIarr iia_layer_conn; 
  AzIntArr ia_layer_order; 
  AzDataArr<AzpLayerInfo> layer_info; 
  AzDataArr<AzpLayerConn> conn; 

  AzStrPool sp_conn; 

  static const int reserved_len=64; 
  static const int version=0;   
public:
  AzpCNet3_multi(const AzpCompoSet_ *_cs) : AzpCNet3(_cs) {}
  ~AzpCNet3_multi() {}
 
  virtual void write(AzFile *file) {
    AzpCNet3::write(file); 
    AzTools::write_header(file, version, reserved_len); 
    iia_layer_conn.write(file); 
    ia_layer_order.writec(file); /* write2: const */
    layer_info.write(file); 
    conn.write(file); 
    sp_conn.writec(file); 
  }
  virtual void read(AzFile *file) {
    AzpCNet3::read(file); 
    int my_version = AzTools::read_header(file, reserved_len); 
    iia_layer_conn.read(file); 
    ia_layer_order.read(file); 
    layer_info.read(file); 
    conn.read(file); 
    sp_conn.read(file); 
  }
 
protected:
  /*---  override ...  ---*/  
  virtual void coldstart(const AzpData_ *trn, AzParam &azp); 
  virtual void warmstart(const AzpData_ *trn, AzParam &azp, bool for_testonly=false); 

  virtual void resetParam(AzParam &azp, bool is_warmstart=false);
  virtual void printParam(const AzOut &out) const; 

  virtual void up_down(const AzpData_ *trn, const int *dxs, int d_num, 
                       double *out_loss=NULL, bool do_update=true); 
  virtual const AzPmat *up(bool is_vg_x, const AzpData_X &data,  bool is_test) {
    AzX::throw_if(true, "AzpCNet3_multi::up", "Don't call this.  Call up_with_conn"); 
    return NULL; 
  }  
  virtual void apply(const AzpData_ *tst, const int dx_begin, int d_num, AzPmat *m_top_out); 
  
  /*---  functions to handle connections  ---*/
  virtual void up_with_conn(bool is_test, const AzDataArr<AzpData_X> &data, AzPmat *m_top_out);   
  virtual void order_layers(int layer_num, const AzIIarr &iia_conn, 
                    /*---  output  ---*/
                    AzIntArr *ia_order, AzDataArr<AzIntArr> &aia_below, AzDataArr<AzIntArr> &aia_above) const; 
  virtual void insert_connectors(AzIntArr &ia_order,  /* inout */
                         AzDataArr<AzIntArr> &aia_below, /* inout */
                         AzDataArr<AzIntArr> &aia_above, /* inout */
                         AzDataArr<AzpLayerConn> &aia_conn) const; /* output */  
  virtual void resetParam_conn(AzParam &azp, AzIIarr &iia_conn); 
  virtual void printParam_conn(const AzOut &out) const; 
  virtual int parse_layerno(const char *str, int hid_num) const; 
    
  void show_below_above(const AzIntArr &ia_below, const AzIntArr &ia_above, AzBytArr &s) const {
    s << "("; 
    for (int ix = 0; ix < ia_below.size(); ++ix) {
      if (ix > 0) s << ","; 
      s << ia_below.get(ix); 
    }
    s << ") -> ("; 
    for (int ix = 0; ix < ia_above.size(); ++ix) {
      if (ix > 0) s << ","; 
      s << ia_above.get(ix); 
    }    
    s << ")"; 
  }
  
  virtual int top_lay_ind() const {
    return ia_layer_order.get(ia_layer_order.size()-1); 
  }
  int get_dsno(AzParam &azp, const AzpData_ *trn, int layer_no, int dflt_dsno, const AzOut &out) const; 

  int getParam_dsno(AzParam &azp, const char *pfx, int dflt_dsno) const; 
  void printParam_dsno(const AzOut &out, const char *pfx, int dsno) const; 
  void checkParam_dsno(const AzpData_ *trn, const char *pfx, int dsno) const; 
  void print_dsno(int layer_no, int dsno, const AzOut &out) const; 
}; 
#endif
