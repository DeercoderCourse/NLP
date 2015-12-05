/* * * * *
 *  AzpCNet3_multi_side.cpp
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

#include "AzpCNet3_multi_side.hpp"

using namespace AzpTimer_CNN_type; 
extern AzpTimer_CNN *timer; 

/*------------------------------------------------------------*/ 
void AzpCNet3_multi_side::coldstart(const AzpData_ *trn, 
                                    AzParam &azp)
{
  AzpCNet3_multi::coldstart(trn, azp); 
  side_info.reset(layer_info.size()); 
  
  int side_num = 0; 
  for (int lx = 0; lx < hid_num; ++lx) {
    side_info(lx)->resetParam(azp, lx, layer_info[lx]->dsno, out, trn, side_num);   
  }  
  
  sidelays.reset(cs, side_num); 
  side_conn.reset(side_info.size()); 
  for (int lx = 0; lx < hid_num; ++lx) {
    if (has_side(lx)) side_coldstart(lx, azp, trn);      
  }  
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi_side::warmstart(const AzpData_ *trn, 
                                    AzParam &azp, 
                                    bool for_testonly)
{
  AzpCNet3_multi::warmstart(trn, azp, for_testonly); 
  for (int lx = 0; lx < hid_num; ++lx) {
    if (has_side(lx)) side_warmstart(lx, azp, trn, for_testonly);      
  }  
}

/* global: ia_layer_order, layer_info */
/*------------------------------------------------------------*/ 
void AzpCNet3_multi_side::up_down(const AzpData_ *trn, 
                      const int *dxs, int d_num, 
                      double *out_loss, 
                      bool do_update)
{
  const char *eyec = "AzpCNet3::up_down"; 
  reset_timestamp(); 

  AzPmatSpa m_spa_y; 
  const AzPmatSpa *mptr_spa_y = for_sparse_y_init(trn, dxs, d_num, &m_spa_y); timestamp(t_DataY);
  
  bool is_test = false; 

  /*---  upward  ---*/  
  AzDataArr<AzpData_X> data(trn->datasetNum()); 
  for (int dsno = 0; dsno < trn->datasetNum(); ++dsno) {
    trn->gen_data(dxs, d_num, data(dsno), dsno); 
  }
  timestamp(t_DataX); 

  AzPmat m_top_out; 
  up_with_conn(is_test, data, &m_top_out); 
  timestamp(t_Upward); 

  /*---  downward  ---*/
  AzPmat m_loss_deriv; 
  nco.loss->get_loss_deriv(&m_top_out, trn, dxs, d_num, &m_loss_deriv, out_loss, mptr_spa_y); 
  for (int ox = ia_layer_order.size() - 1; ox >= 0; --ox) {
    int lx = ia_layer_order.get(ox); 
    const AzpLayerInfo *li = layer_info[lx]; 
    if (li->is_conn) {
      if (li->is_input_var) conn(lx)->downward_var(lays); 
      else                  conn(lx)->downward_fixed(lays); 
    }
    else {
      lays(lx)->downward(&m_loss_deriv); 
      if (has_side(lx) && !side_info[lx]->is_all_sides_fixed()) {
        bool through_lm2 = true;     
        if (lays[lx]->is_input_var()) side_conn(lx)->downward_var(lays, through_lm2);
        else                          side_conn(lx)->downward_fixed(lays, through_lm2);     
        for (int ix = 0; ix < side_info[lx]->sideNum(); ++ix) {   
          if (side_info[lx]->a[ix]->do_fixed) continue; 
          int sx = side_info[lx]->a[ix]->sideno;
          sidelays(sx)->downward(NULL); 
        }              
      } 
      if (do_update && (arr_doUpdate == NULL || arr_doUpdate[lx] == 1)) {
        lays(lx)->updateDelta(d_num, do_update_lm, do_update_act);        
        for (int ix = 0; ix < side_info[lx]->sideNum(); ++ix) {   
          if (side_info[lx]->a[ix]->do_fixed) continue;         
          int sx = side_info[lx]->a[ix]->sideno;
          sidelays(sx)->updateDelta(d_num, do_update_lm, do_update_act); 
        }       
      }      
      
    }
    if (arr_doUpdate != NULL && lx <= min_focus) break; 
  }
  timestamp(t_Downward); 
  
  lays_flushDelta();   timestamp(t_Flush); 
  for_sparse_y_term(); timestamp(t_DataY); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi_side::up_with_conn(bool is_test, 
                        const AzDataArr<AzpData_X> &data, 
                        AzPmat *m_top_out)
{                        
  AzDataArr<AzpLayerConn_Work> connw(conn.size()); 
  AzBaseArr<const AzPmatVar *> amvptr(layer_info.size(), NULL); 
  AzBaseArr<const AzPmat *> amptr(layer_info.size(), NULL);   
   
  AzDataArr<AzpLayerConn_Work> side_connw(side_conn.size()); 
  AzBaseArr<const AzPmatVar *> side_amvptr(sidelays.size(), NULL); 
  AzBaseArr<const AzPmat *> side_amptr(sidelays.size(), NULL); 
  
  /*---  upward: variable size  ---*/ 
  int ox = 0; 
  for (ox = 0; ox < ia_layer_order.size(); ++ox) {
    int lx = ia_layer_order.get(ox); 
    const AzpLayerInfo *li = layer_info[lx]; 
    if (!li->is_input_var) break; 

    const AzPmatVar *mv_forlm2 = NULL; 
    const AzpSideInfo *si = side_info[lx];
    if (si->sideNum() > 0) {
      for (int ix = 0; ix < si->sideNum(); ++ix) {
        int sx = si->a[ix]->sideno; 
        if (si->a[ix]->do_prepped) side_amvptr(sx, data[si->a[ix]->dsno]->var()); 
        else                       side_amvptr(sx, sidelays(sx)->upward_var(is_test, data[si->a[ix]->dsno])); 
      }    
      mv_forlm2 = side_conn(lx)->upward_var(side_amvptr, is_test, *side_connw(lx));    
    }    
    
    const AzPmatVar *mv_out = NULL;   
    if (li->is_conn)        mv_out = conn[lx]->upward_var(amvptr, is_test, *connw(lx)); 
    else if (li->below < 0) mv_out = lays(lx)->upward_var(is_test, data[li->dsno], mv_forlm2); 
    else                    mv_out = lays(lx)->upward_var(is_test, amvptr[li->below], mv_forlm2); 

    if (li->o_shape.is_valid()) amptr(lx, mv_out->data()); 
    else                        amvptr(lx, mv_out);     
  }
  /*---  upward: fixed size  ---*/
  for ( ; ox < ia_layer_order.size(); ++ox) {
    int lx = ia_layer_order.get(ox);   
    const AzpLayerInfo *li = layer_info[lx];     
    
    const AzPmat *m_forlm2 = NULL;    
    const AzpSideInfo *si = side_info[lx];
    if (si->sideNum() > 0) {
      for (int ix = 0; ix < si->sideNum(); ++ix) {
        int sx = si->a[ix]->sideno; 
        if (si->a[ix]->do_prepped) side_amptr(sx, data[si->a[ix]->dsno]->dense()); 
        else                       side_amptr(sx, sidelays(sx)->upward(is_test, data[si->a[ix]->dsno])); 
      }
      m_forlm2 = side_conn(lx)->upward(side_amptr, is_test, *side_connw(lx));    
    }  
    
    const AzPmat *m_out = NULL;     
    if (li->is_conn)        m_out = conn[lx]->upward(amptr, is_test, *connw(lx)); 
    else if (li->below < 0) m_out = lays(lx)->upward(is_test, data[li->dsno], m_forlm2); 
    else { 
      if (fdp != NULL) fdp->fdump(lx, amptr[li->below]); /* no support for writing direct input from a side layer */
      m_out = lays(lx)->upward(is_test, amptr[li->below], m_forlm2); 
    }
    amptr(lx, m_out);     
  }
  int top_lx = top_lay_ind(); 
  m_top_out->set(amptr[top_lx]); 
}

/*------------------------------------------------------------*/ 
int AzpCNet3_multi_side::reset_side_conn(int lx, const AzpData_ *data)
{
  AzDataArr<AzpLayerInfo> li(sidelays.size()); 
  const AzpSideInfo *si = side_info[lx]; 
  for (int ix = 0; ix < si->sideNum(); ++ix) {
    int sx = si->a[ix]->sideno; 
    if (si->a[ix]->do_prepped) {
      data->get_info(&li(sx)->o_shape); 
      li(sx)->o_channels = data->channelNum(si->a[ix]->dsno); 
    }
    else {
      sidelays[sx]->output_shape(&li(sx)->o_shape); 
      li(sx)->o_channels = sidelays[sx]->output_channels();     
    }
  }  
  AzxD out_shape; 
  AzIntArr ia_a, ia_b; 
  for (int ix = 0; ix < si->sideNum(); ++ix) ia_b.put(si->a[ix]->sideno); /* below */ 
  ia_a.put(lx); /* above */
  int o_channels; 
  side_conn(lx)->coldstart(out, lx, li, &ia_b, &ia_a, &o_channels, NULL);                            
  return o_channels; 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi_side::side_coldstart(int lx, AzParam &azp, 
                                  const AzpData_ *data)
{                  
  const char *eyec = "AzpCNet3_multi_side::side_coldstart"; 

  bool is_spa_org = data->is_sparse_x(); 
  bool is_var = lays[lx]->is_input_var(); 
  AzpSideInfo *si = side_info(lx); 
  
  AzxD org_shape; 
  data->get_info(&org_shape); 
  AzX::no_support((is_var && org_shape.is_valid()), eyec, 
      "A side-layer with fixed-sized input attached to a layer with variable-sized input"); 
  AzX::no_support((!is_var && !org_shape.is_valid()), eyec, 
      "A side-layer with variable-sized input attached to a layer with fixed-sized input"); 

  AzpUpperLayer *upper = side_conn(lx); 
  for (int ix = 0; ix < si->sideNum(); ++ix) {
    int sx = si->a[ix]->sideno; 
    int side_cc = data->channelNum(si->a[ix]->dsno); 
    AzpLayer *side = sidelays(sx); 
    AzBytArr s_no; 
    const char *pfx = si->gen_prefix(lx, ix, s_no); 
    if (si->a[ix]->do_rand) {
      AzPrint::writeln(out, "Cold-starting side-layer: ", s_no.c_str()); 
      /*---  AzpLayer needs sx for downward  ---*/
      if (is_var) side->coldstart_var(out, azp, is_spa_org, sx, side_cc, upper, NULL, pfx); 
      else        side->coldstart_nontop(out, azp, false, sx, side_cc, &org_shape, upper, NULL, pfx); 
    }
    else if (si->a[ix]->do_prepped) { side = NULL; }
    else {
      const char *side_fn = si->a[ix]->s_fn.c_str(); 
      AzPrint::writeln(out, "Reading: ", side_fn); 
      if (AzBytArr::endsWith(side_fn, azp_lay0_ext)) {
        AzDicc side_dic; read_lay0(side_fn, side_dic, *side); 
        AzTimeLog::print("Checking word-mapping ... ", out); 
        AzX::throw_if(!side_dic.is_same(data->get_dic(si->a[ix]->dsno)), AzInputError, eyec, 
                      side_fn, ": Word-mapping check failed.  Word-mapping of the input data to the side layer must be the same as the one used for training the side layer.");
        side->reset_upper(upper); 
      }
      else if (AzBytArr::endsWith(side_fn, ".lay0")) { /* for compatibility */
        side->read(side_fn); 
        side->reset_upper(upper); 
      }      
      else { /* for compatibility */
        AzpCNet3 side_nn(cs); side_nn.read(side_fn); 
        side->reset(side_nn.layer(0), upper); 
      }
      AzPrint::writeln(out, "Warm-starting side-layer: ", s_no.c_str()); 
      bool do_ow_lno = true;  /* overwrite layer# */
      bool do_ow_param = true; /* allow to overwrite some of the parameters */
      if (is_var) side->warmstart_var(out, azp, is_spa_org, sx, side_cc, upper, NULL, false, do_ow_lno, do_ow_param, pfx); 
      else        side->warmstart(out, azp, sx, side_cc, &org_shape, upper, -1, NULL, false, do_ow_lno, do_ow_param, pfx); 
    }
  
    if (side != NULL) side->setup_for_reg_L2init();           
  }  
  
  int o_channels = reset_side_conn(lx, data);   

  lays(lx)->coldstart_lm2side(azp, o_channels);
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi_side::side_warmstart(int lx, AzParam &azp, 
                                  const AzpData_ *data, 
                                  bool for_testonly)                                  
{                  
  const char *eyec = "AzpCNet3_multi_side::side_warmstart"; 

  bool is_spa_org = data->is_sparse_x(); 
  bool is_var = lays[lx]->is_input_var(); 
  const AzpSideInfo *si = side_info(lx); 
  
  AzxD org_shape; 
  data->get_info(&org_shape); 

  AzpUpperLayer *upper = side_conn(lx); 
  for (int ix = 0; ix < si->sideNum(); ++ix) {
    int sx = si->a[ix]->sideno; 
    int side_cc = data->channelNum(si->a[ix]->dsno); 
    AzpLayer *side = sidelays(sx); 
    AzBytArr s_no; 
    const char *pfx = si->gen_prefix(lx, ix, s_no); 
    AzPrint::writeln(out, "Warm-starting side-layer: ", s_no.c_str()); 
    bool do_ow_lno = false, do_ow_param = false; 
    if (si->a[ix]->do_prepped) { side = NULL; }
    if (is_var) side->warmstart_var(out, azp, is_spa_org, sx, side_cc, upper, NULL, for_testonly, do_ow_lno, do_ow_param, pfx); 
    else        side->warmstart(out, azp, sx, side_cc, &org_shape, upper, -1, NULL, for_testonly, do_ow_lno, do_ow_param, pfx); 
    if (side != NULL) side->check_for_reg_L2init();      
  }  
  int o_channels = side_conn[lx]->output_channels(); 
  lays(lx)->warmstart_lm2side(azp, o_channels, for_testonly);
}
