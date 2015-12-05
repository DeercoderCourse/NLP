/* * * * *
 *  AzpLayerConn.cpp
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

#include "AzpLayerConn.hpp"

/*------------------------------------------------------------*/
void AzpLayerConn::coldstart(const AzOut &out, 
                             int lno, 
                             const AzDataArr<AzpLayerInfo> &info, /* layer output */
                             const AzIntArr *_ia_below, 
                             const AzIntArr *_ia_above, 
                             /*---  output  ---*/
                             int *out_cc, 
                             AzxD *out_shape)
{                             
  const char *eyec = "AzpLayerConn::coldstart"; 
  layer_no = lno; 
  ia_below.reset(_ia_below); 
  ia_above.reset(_ia_above); 
  AzX::throw_if((ia_below.size() <= 0), eyec, "No Input?!"); 
  AzX::throw_if((ia_above.size() <= 0), eyec, "No output?!"); 

  /*---  shape check  ---*/
  AzxD shape(&info[ia_below[0]]->o_shape); 
  bool do_all_in_one = false; 
  for (int ix = 0; ix < ia_below.size(); ++ix) {
    int below = ia_below.get(ix); 
    const AzxD *bshape = &info[below]->o_shape; 
    AzX::throw_if((shape.is_var() != bshape->is_var()), AzInputError, eyec, 
                  "Multiple inputs must be all variable-sized or all fixed-sized."); 
    if (!shape.is_var() && !shape.isSame(bshape)) { /* fixed-sized with different shapes */
      do_all_in_one = true; 
    }       
  }    
  if (do_all_in_one) {
    AzPrint::writeln(out, "  The connector will do \"all-in-one\" (concatenating all the pixels into one pixel) as input images have different shapes."); 
  }
  
  /*---  ---*/
  iia_range_to_above.reset();  
  iia_all_in_one.reset(); /* #pixels: before after */
  int cc = 0; 
  for (int ix = 0; ix < ia_below.size(); ++ix) {
    int below = ia_below.get(ix); 
    int last_cc = cc; 
    if (do_all_in_one) {
      int sz_before = info[below]->o_channels; 
      int sz_after = sz_before * info[below]->o_shape.size(); /* make one vector concatenating all pixels */
      iia_all_in_one.put(sz_before, sz_after); 
      cc += sz_after; 
    }
    else {
      cc += info[below]->o_channels; 
    }
    iia_range_to_above.put(last_cc, cc);    
  } 
  if (out_cc != NULL) *out_cc = cc; 
  if (out_shape != NULL) {
    if (do_all_in_one) out_shape->reset(shape.get_dim()); /* 1 x 1 */
    else               out_shape->reset(&shape); 
  }
  
  is_inp_var = shape.is_var(); 
}

/*------------------------------------------------------------*/
const AzPmat *AzpLayerConn::all_in_one_upward(const AzPmat *m, int index, AzPmat *_m) const
{
  if (iia_all_in_one.size() <= 0) return m; 
  
  int sz_before, sz_after; 
  iia_all_in_one.get(index, &sz_before, &sz_after); 
  AzX::throw_if((m->rowNum() != sz_before), "all_in_oneupward", "Wrong dimensionality."); 
  if (sz_before == sz_after) return m; 
  _m->set(m); 
  _m->change_dim(sz_after, m->size()/sz_after); 
  return _m; 
}

/*------------------------------------------------------------*/
void AzpLayerConn::all_in_one_downward(int index, AzPmat *m) const
{
  if (iia_all_in_one.size() <= 0) return; 

  int sz_before, sz_after; 
  iia_all_in_one.get(index, &sz_before, &sz_after); 
  AzX::throw_if((m->rowNum() != sz_after), "AzpLayerConn::all_in_one_downward", "Wrong dimensionality."); 
  if (sz_before == sz_after) return; 
  m->change_dim(sz_before, m->size()/sz_before); 
}

/*------------------------------------------------------------*/
template <class M> /* M: AzPmat, AzPmatVar */
const M *AzpLayerConn::_upward(const AzBaseArr<const M *> &amptr_out, 
                                    M *mat_value) const
{
  const char *eyec = "AzpLayerConn::upward"; 
  AzX::throw_if((ia_below.size() <= 0), eyec, "No layers below?"); 
  mat_value->reset();   

  if (ia_below.size() == 1) {
    int below = ia_below.get(0); 
    const M *m = amptr_out[below]; 
    return m; 
  }
  else {
    for (int ix = 0; ix < ia_below.size(); ++ix) {
      int row_begin = mat_value->rowNum(); 
      int below = ia_below.get(ix);   
      const M *m = amptr_out[below];    
      M _m; /* work */
      m = all_in_one_upward(m, ix, &_m);  /* change the shape if all-in-one */
      AzX::throw_if((!mat_value->can_rbind(m)), eyec, "Different shape.  rbind is not allowed."); 
      mat_value->rbind(m); 
      int row_end = mat_value->rowNum(); 
      int bx, ex; 
      iia_range_to_above.get(ix, &bx, &ex); 
      AzX::throw_if((bx != row_begin || ex != row_end), eyec, "unexpected #row"); 
    }
    return mat_value; 
  }
}                          
template const AzPmat *AzpLayerConn::_upward<AzPmat>(const AzBaseArr<const AzPmat *> &, AzPmat *) const; 
template const AzPmatVar *AzpLayerConn::_upward<AzPmatVar>(const AzBaseArr<const AzPmatVar *> &, AzPmatVar *) const; 

/*------------------------------------------------------------*/
void AzpLayerConn::downward_fixed(AzpLayers &lays, bool through_lm2)
{
  const char *eyec = "AzpLayerConn::downward_fixed"; 
  AzX::throw_if((ia_above.size() == 0), eyec, "No layers above"); 
  
  for (int ix = 0; ix < ia_above.size(); ++ix) {
    int above = ia_above.get(ix); 
    AzPmat m_temp; 
    lays[above]->get_lossd_after_fixed(&m_temp, through_lm2, layer_no); 
    if (ix == 0) m_lossd_after.set(&m_temp); 
    else         m_lossd_after.add(&m_temp); 
  }
}

/*------------------------------------------------------------*/
void AzpLayerConn::downward_var(AzpLayers &lays, bool through_lm2)
{
  const char *eyec = "AzpLayerConn::downward_var"; 
  AzX::throw_if((ia_above.size() == 0), eyec, "No layers above"); 

  for (int ix = 0; ix < ia_above.size(); ++ix) {
    int above = ia_above.get(ix); 
    AzPmatVar mv_temp; 
    lays[above]->get_lossd_after_var(&mv_temp, through_lm2, layer_no); 
    if (ix == 0) mv_lossd_after.set(&mv_temp); 
    else         mv_lossd_after.add(&mv_temp); 
  }
}

/*------------------------------------------------------------*/  
void AzpLayerConn::_get_lossd_after(AzPmat *m_lossd_a, AzPmatVar *mv_lossd_a, bool through_lm2, int below) const
{
  const char *eyec = "AzpLayerConn::_get_lossd_after"; 
  int index = ia_below.find(below); 
  AzX::throw_if((index < 0), eyec, "Invalid index?!");
  AzX::no_support(through_lm2, eyec, "Side network"); 
 
  int row_begin, row_end; 
  iia_range_to_above.get(index, &row_begin, &row_end); 
  int r_num = row_end - row_begin; 
  if (m_lossd_a != NULL) {
    m_lossd_a->reform(r_num, m_lossd_after.colNum()); 
    m_lossd_a->set_rowwise(0, r_num, &m_lossd_after, row_begin); 
    all_in_one_downward(index, m_lossd_a); /* change the shape if all-in-one */
  }
  else if (mv_lossd_a != NULL) {
    AzPmat m(r_num, mv_lossd_after.colNum()); 
    m.set_rowwise(0, r_num, mv_lossd_after.data(), row_begin); 
    mv_lossd_a->set(&m, mv_lossd_after.d_index());  
  }
}
