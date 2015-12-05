/* * * * *
 *  AzpLayerConn.hpp
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

#ifndef _AZP_LAYER_CONN_HPP_
#define _AZP_LAYER_CONN_HPP_

#include "AzpLayer.hpp"

/*------------------------------------------------------------*/
class AzpLayerInfo {
protected: 
  static const int reserved_len=64; 
  static const int version=0;  
public:
  int o_channels; 
  AzxD o_shape; 
  bool is_input_var, is_conn; 
  int below;  /* set only if it's a regular layer (i.e., is_conn=false) */
  int above; /* set only if it's a regular layer (i.e., is_conn=false) */
  int dsno; /* dataset#: set only if it's the lowest layer */
  
  AzpLayerInfo() : o_channels(-1), is_input_var(false), is_conn(false), below(-1), above(-1), dsno(-1) {}
  void reset() {
    o_channels = -1; 
    o_shape.reset(); 
    is_input_var = is_conn = false; 
    below = above = dsno = -1; 
  }
  
  void write(AzFile *file) {
    AzTools::write_header(file, version, reserved_len); 
    file->writeInt(o_channels); 
    o_shape.write(file); 
    file->writeBool(is_input_var);  
    file->writeBool(is_conn); 
    file->writeInt(below); 
    file->writeInt(above);     
    file->writeInt(dsno); 
  }
  void read(AzFile *file) {
    int my_version = AzTools::read_header(file, reserved_len); 
    o_channels = file->readInt(); 
    o_shape.read(file); 
    is_input_var = file->readBool();    
    is_conn = file->readBool(); 
    below = file->readInt(); 
    above = file->readInt();     
    dsno = file->readInt(); 
  }
}; 

/*------------------------------------------------------------*/
class AzpLayerConn_Work {
protected:
  AzPmat m_value; 
  AzPmatVar mv_value; 
  friend class AzpLayerConn; 
}; 
  
/*------------------------------------------------------------*/
class AzpLayerConn : public virtual /* implements */ AzpUpperLayer {
protected: 
  bool is_inp_var; 
  int layer_no; 
  AzIntArr ia_below; 
  AzIntArr ia_above; 
  AzIIarr iia_range_to_above; 
  AzIIarr iia_all_in_one; 

  /*---  work  ---*/
  AzPmat m_lossd_after; 
  AzPmatVar mv_lossd_after; 
  
  static const int reserved_len=64; 
  static const int version=0;   
  
public: 
  AzpLayerConn() : is_inp_var(false), layer_no(-1) {}
  
  virtual void write(AzFile *file) const {
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(is_inp_var); 
    file->writeInt(layer_no); 
    ia_below.writec(file); 
    ia_above.writec(file); 
    iia_range_to_above.write(file); 
    iia_all_in_one.write(file); 
  }
  virtual void read(AzFile *file) {
    int my_version = AzTools::read_header(file, reserved_len); 
    is_inp_var = file->readBool(); 
    layer_no = file->readInt(); 
    ia_below.read(file); 
    ia_above.read(file); 
    iia_range_to_above.read(file); 
    iia_all_in_one.read(file); 
  }
    
  void coldstart(const AzOut &_out, int lno, 
                 const AzDataArr<AzpLayerInfo> &layer_info, /* only set for the layers below */
                 const AzIntArr *_ia_below, const AzIntArr *_ia_above, 
                 /*---  output  ---*/
                 int *out_cc, AzxD *out_shape); 

  int output_channels() const {
    int num = iia_range_to_above.size(); 
    if (num <= 0) return 0; 
    int i1, i2; iia_range_to_above.get(num-1, &i1, &i2); 
    return i2; 
  }                 
                 
  const AzPmat *upward(const AzBaseArr<const AzPmat *> &amptr_out, bool is_test, 
                       AzpLayerConn_Work &wrk) const {
    return _upward<AzPmat>(amptr_out, &wrk.m_value); 
  }
  const AzPmatVar *upward_var(const AzBaseArr<const AzPmatVar *> &amptr_out, bool is_test, 
                          AzpLayerConn_Work &wrk) const {
    return _upward(amptr_out, &wrk.mv_value); 
  }             
  
  void downward_fixed(AzpLayers &lays, bool through_lm2=false); 
  void downward_var(AzpLayers &lays, bool through_lm2=false); 
  
  /*---  to implement AzpUpperLayer  ---*/
  bool is_variable_input() const { return is_inp_var; }
  virtual void get_lossd_after_fixed(AzPmat *m_lossd_a, bool through_lm2, int below) const {
    _get_lossd_after(m_lossd_a, NULL, through_lm2, below); 
  }
  virtual void get_lossd_after_var(AzPmatVar *mv_lossd_a, bool through_lm2, int below) const {
    _get_lossd_after(NULL, mv_lossd_a, through_lm2, below); 
  }        

protected:   
  template <class M>  
  const M *_upward(const AzBaseArr<const M *> &amptr_out, M *mat_value) const;  
  void _get_lossd_after(AzPmat *m_lossd_a, AzPmatVar *mv_lossd_a, bool through_lm2, int below) const; 
  
  const AzPmatVar *all_in_one_upward(const AzPmatVar *m, int index, AzPmatVar *_m) const {
    if (iia_all_in_one.size() <= 0) return m; 
    AzX::throw_if(true, "AzpLayerConn::all_in_one_upward(var)", "This shouldn't be called."); 
    return NULL; 
  }
  const AzPmat *all_in_one_upward(const AzPmat *m, int index, AzPmat *_m) const; 
  void all_in_one_downward(int index, AzPmat *m) const; 
}; 
#endif 
