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

#include "AzpCNet3_multi.hpp"

using namespace AzpTimer_CNN_type; 
extern AzpTimer_CNN *timer; 

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::coldstart(const AzpData_ *trn, 
                               AzParam &azp)
{
  const char *eyec = "AzpCNet3_multi::coldstart"; 

  int layer_num = hid_num+1; /* +1 for the output layer */
  AzDataArr<AzIntArr>aia_below, aia_above; 
  order_layers(layer_num, iia_layer_conn, &ia_layer_order, aia_below, aia_above); 
  insert_connectors(ia_layer_order, aia_below, aia_above, conn); 

  AzX::throw_if((ia_layer_order.size() != aia_below.size() || ia_layer_order.size() != aia_above.size()), 
                eyec, "number conflict"); 
  lays.reset(cs, layer_num);  
  int conn_num = ia_layer_order.size() - layer_num; 

  conn.reset(layer_num + conn_num); 
  layer_info.reset(layer_num + conn_num); 
  
  AzxD data_shape; 
  trn->get_info(&data_shape); 

  int ox = 0;   
  for ( ; ox < ia_layer_order.size(); ++ox) {
    int lx = ia_layer_order.get(ox); 
    AzpLayerInfo *li = layer_info(lx); 
    li->reset(); 
    if (lx >= layer_num) { /* connector */
      AzBytArr s("Cold-starting connector#"); s << lx << "  "; 
      show_below_above(*aia_below[lx], *aia_above[lx], s); 
      AzPrint::writeln(out, s.c_str());
      int out_cc; 
      AzxD out_shape; 
      conn(lx)->coldstart(out, lx, layer_info, aia_below[lx], aia_above[lx], 
                          &out_cc, &out_shape); 
      li->o_channels = out_cc; 
      li->o_shape.reset(&out_shape); 
      li->is_conn = true; 
      if (!out_shape.is_valid()) {
        li->is_input_var = true; 
      }
    }
    else { /* regular layer */
      int cc;  /* #channels */
      bool is_spa = false; 
      AzxD shape(&data_shape);  /* input shape */
      if (aia_below[lx]->size() == 0) { /* the bottom layer */
        int dflt_dsno = 0; 
        li->dsno = get_dsno(azp, trn, lx, dflt_dsno, out); 
        cc = trn->channelNum(li->dsno); 
        is_spa = trn->is_sparse_x(); 
      }
      else if (aia_below[lx]->size() == 1) { /* not the bottom */
        int below = aia_below[lx]->get(0); 
        cc = layer_info[below]->o_channels; 
        shape.reset(&layer_info[below]->o_shape);         
        li->below = below; 
        li->dsno = -1; 
      }
      else {
        AzX::throw_if(true, eyec, "a regular layer should have exactly one input"); 
      }     
      
      if (aia_above[lx]->size() == 0) { /* top layer */
        /*---  top layer, which only takes fixed-sized input  ---*/
        li->above = -1; 
        AzX::throw_if((!shape.is_valid()), AzInputError, eyec, "Input to the top layer must be fixed-size"); 
        if (do_topthru) {
          AzPrint::writeln(out, "Cold-starting the topthru layer"); 
          lays(lx)->coldstart_topthru(out, azp, hid_num, cc, &shape, class_num); 
        }
        else {
          AzPrint::writeln(out, "Cold-starting the top layer"); 
          lays(lx)->coldstart_top(out, azp, hid_num, cc, &shape, class_num); 
        }
      }
      else { /* hidden layer */
        AzX::throw_if((aia_above[lx]->size() != 1), eyec, "a regular non-top layer should have exactly one output");    
        li->above = aia_above[lx]->get(0); 
        const AzpUpperLayer *upper; 
        if (li->above >= layer_num) upper = conn[li->above];
        else                        upper = lays[li->above]; 
        if (!shape.is_valid()) { /* variable-sized input */
          AzPrint::writeln(out, "Cold-starting (variable-size input) layer#", lx); 
          init_layer(azp, true, upper, is_spa, lx, cc, &shape);                        
          li->is_input_var = true; 
        }
        else {
          AzPrint::writeln(out, "Cold-starting layer#", lx);
          AzxD next_shape;      
          init_layer(azp, false, upper, is_spa, lx, cc, &shape);           
        } 
        li->o_channels = lays[lx]->output_channels();     
        li->o_shape.reset(&shape); 
      }
    }
  }  
  
  lays_setup_for_reg_L2init(); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::warmstart(const AzpData_ *trn, 
                               AzParam &azp, 
                               bool for_testonly)
{
  const char *eyec = "AzpCNet3_multi::warmstart"; 
  AzX::no_support(do_topthru, eyec, "warmstart with topthru"); 
  AzX::throw_if((lays.size() != hid_num+1), AzInputError, eyec, "#layer doesn't match"); 
  int layer_num = hid_num+1; /* +1 for the output layer */

  AzxD data_shape; 
  trn->get_info(&data_shape); 

  int ox = 0;   
  for ( ; ox < ia_layer_order.size(); ++ox) {
    int lx = ia_layer_order.get(ox); 
    const AzpLayerInfo *li = layer_info(lx); 
    if (lx >= layer_num) { /* connector */
      AzBytArr s("Warm-starting connector#"); s << lx << "  "; 
    }
    else { /* regular layer */
      int cc;  /* #channels */
      bool is_spa = false; 
      AzxD shape(&data_shape);  /* input shape */
      if (li->below < 0) { /* the bottom layer */
        cc = trn->channelNum(li->dsno); 
        is_spa = trn->is_sparse_x(); 
        print_dsno(lx, li->dsno, out); /* display dataset# */
      }
      else { /* not the bottom */
        cc = layer_info[li->below]->o_channels; 
        shape.reset(&layer_info[li->below]->o_shape);         
      }
      
      if (ox == ia_layer_order.size()-1) { /* top layer */
        AzPrint::writeln(out, "Warm-starting the top layer"); 
        lays(lx)->warmstart(out, azp, lx, cc, &shape, NULL, class_num, NULL, for_testonly); 
      }
      else { /* hidden layer */    
        const AzpUpperLayer *upper; 
        if (li->above >= layer_num) upper = conn[li->above];
        else                        upper = lays[li->above]; 
        if (shape.is_var()) { /* variable-sized input */
          AzPrint::writeln(out, "Warm-starting (variable-size input) layer#", lx); 
          lays(lx)->warmstart_var(out, azp, is_spa, lx, cc, upper, &shape, for_testonly);     
        }
        else {
          AzPrint::writeln(out, "Warm-starting layer#", lx);
          lays(lx)->warmstart(out, azp, lx, cc, &shape, upper, -1, NULL, for_testonly);
        } 
      }
    }
  }  
  
  lays_check_for_reg_L2init();   
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::order_layers(int layer_num, 
                          const AzIIarr &iia_conn, 
                          /*---  output  ---*/
                          AzIntArr *ia_order, 
                          AzDataArr<AzIntArr> &aia_below, 
                          AzDataArr<AzIntArr> &aia_above) const
{
  const char *eyec = "AzpCNet3_multi::order_layers"; 
  AzIntArr ia_done(layer_num, 0); 
  int *done = ia_done.point_u(); 
  aia_below.reset(layer_num); 
  aia_above.reset(layer_num); 
  for (int ix = 0; ix < iia_conn.size(); ++ix) {
    int below, above; 
    iia_conn.get(ix, &below, &above); 
    aia_below(above)->put(below); 
    aia_above(below)->put(above); 
  }

  ia_order->reset(); 
  int num = 0; 
  for ( ; num < layer_num; ) {
    int org_num = num; 
    for (int lx = 0; lx < layer_num; ++lx) {
      if (done[lx] == 1) continue; 
      bool is_ready = true; 
      for (int bx = 0; bx < aia_below[lx]->size(); ++bx) {
        int below = aia_below[lx]->get(bx); 
        if (done[below] != 1) {
          is_ready = false; 
          break; 
        }
      }
      if (is_ready) {      
        ia_order->put(lx); 
        done[lx] = 1; 
        ++num; 
        break; 
      }
    }
    AzX::throw_if((num == org_num), AzInputError, eyec, "deadlock"); /* if not making a progress, there may be a loop */
  }  
  
  int top_num = 0, bottom_num = 0; 
  int top_lx = -1; 
  for (int lx = 0; lx < ia_order->size(); ++lx) {
    if (aia_above[lx]->size() <= 0) {
      top_lx = lx; 
      ++top_num; 
    }
  }
  AzX::throw_if((top_num != 1), AzInputError, eyec, "more than one top layer or no top layer"); 
  AzX::throw_if((top_lx != ia_order->get(ia_order->size()-1)), eyec, "Something is wrong with the top layer index"); 
}  

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::insert_connectors(AzIntArr &ia_order,  /* inout */
                               AzDataArr<AzIntArr> &aia_below, /* inout */
                               AzDataArr<AzIntArr> &aia_above, /* inout */
                               AzDataArr<AzpLayerConn> &conn)
const                               
{
  const char *eyec = "AzpCNet3_multi::insert_connectors"; 

  int layer_num = ia_order.size();
  /*---  count connectors to be inserted  ---*/
  int conn_num = 0; 
  for (int lx = 0; lx < layer_num; ++lx) {
    if (aia_below[lx]->size() > 1) ++conn_num; 
    if (aia_above[lx]->size() > 1) ++conn_num; 
  }
  
  /*---  copy the current edges  ---*/
  AzDataArr<AzIntArr> aia_b(layer_num + conn_num); 
  AzDataArr<AzIntArr> aia_a(layer_num + conn_num); 
  conn.reset(layer_num + conn_num); 
  for (int lx = 0; lx < layer_num; ++lx) {
    aia_b(lx)->reset(aia_below[lx]); 
    aia_a(lx)->reset(aia_above[lx]); 
  }

  /*---  insert connection where multiple input/output  ---*/
  AzIntArr ia_o; 
  int cx = layer_num; 
  for (int ix = 0; ix < ia_order.size(); ++ix) {
    int lx = ia_order.get(ix); 
    if (aia_b[lx]->size() > 1) { /* multiple inputs */
      aia_b(cx)->reset(aia_b[lx]); 
      aia_a(cx)->put(lx); 
      for (int ix = 0; ix < aia_b[cx]->size(); ++ix) {
        int below = aia_b[cx]->get(ix);        
        int count = aia_a(below)->replace(lx, cx); 
        AzX::throw_if((count != 1), eyec, "something is wrong"); 
      }
      aia_b(lx)->reset(); 
      aia_b(lx)->put(cx); 
      ia_o.put(cx);       
      ++cx; 
    }

    ia_o.put(lx); 
    
    if (aia_above[lx]->size() > 1) {
      aia_b(cx)->put(lx); 
      aia_a(cx)->reset(aia_above[lx]); 
      for (int ix = 0; ix < aia_a[cx]->size(); ++ix) {
        int above = aia_a[cx]->get(ix); 
        int count = aia_b(above)->replace(lx, cx); 
        AzX::throw_if((count != 1), eyec, "something is wrong-2"); 
      }
      aia_a(lx)->reset(); 
      aia_a(lx)->put(cx); 
      ia_o.put(cx); 
      ++cx; 
    }
  }
  
  /*---  output  ---*/
  aia_below.reset(&aia_b); 
  aia_above.reset(&aia_a); 
  ia_order.reset(&ia_o); 
}

/* global: ia_layer_order, layer_info */
/*------------------------------------------------------------*/ 
void AzpCNet3_multi::up_down(const AzpData_ *trn, 
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
      if (do_update && (arr_doUpdate == NULL || arr_doUpdate[lx] == 1)) { /* needs update */        
        lays(lx)->downward(&m_loss_deriv); 
        lays(lx)->updateDelta(d_num, do_update_lm, do_update_act);         
      }  
      else if (li->below >= 0) { /* no need to update, but there is a layer below */
        lays(lx)->downward(&m_loss_deriv); 
      }      
    }
  }
  timestamp(t_Downward); 
  lays_flushDelta();   timestamp(t_Flush);   
  for_sparse_y_term(); timestamp(t_DataY); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::apply(const AzpData_ *tst, 
                      const int dx_begin, int d_num, 
                      AzPmat *m_top_out)
{
  const char *eyec = "AzpCNet3::up_down"; 
  reset_timestamp(); 
  bool is_test = true; 
  AzDataArr<AzpData_X> data(tst->datasetNum()); 
  for (int dsno = 0; dsno < tst->datasetNum(); ++dsno) {
    tst->gen_data(dx_begin, d_num, data(dsno), dsno); 
  }  

  up_with_conn(is_test, data, m_top_out); 
  timestamp(t_Test); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::up_with_conn(bool is_test, 
                        const AzDataArr<AzpData_X> &data, 
                        AzPmat *m_top_out)
{                        
  AzDataArr<AzpLayerConn_Work> connw(conn.size()); 
  AzBaseArr<const AzPmatVar *> amptr_outvar(layer_info.size(), NULL); 
  AzBaseArr<const AzPmat *> amptr_outfix(layer_info.size(), NULL);   

  /*---  upward: variable size  ---*/ 
  int ox = 0; 
  for (ox = 0; ox < ia_layer_order.size(); ++ox) {
    int lx = ia_layer_order.get(ox); 
    const AzpLayerInfo *li = layer_info[lx]; 
    if (!li->is_input_var) break; 
    
    const AzPmatVar *mv_out = NULL;     
    if (li->is_conn)        mv_out = conn[lx]->upward_var(amptr_outvar, is_test, *connw(lx)); 
    else if (li->below < 0) mv_out = lays(lx)->upward_var(is_test, data[li->dsno]); 
    else                    mv_out = lays(lx)->upward_var(is_test, amptr_outvar[li->below]); 

    if (li->o_shape.is_valid()) amptr_outfix(lx, mv_out->data()); 
    else                        amptr_outvar(lx, mv_out);     
  }
  /*---  upward: fixed size  ---*/
  for ( ; ox < ia_layer_order.size(); ++ox) {
    int lx = ia_layer_order.get(ox);   
    const AzpLayerInfo *li = layer_info[lx];     
    const AzPmat *m_out = NULL;     
    if (li->is_conn)        m_out = conn[lx]->upward(amptr_outfix, is_test, *connw(lx)); 
    else if (li->below < 0) m_out = lays(lx)->upward(is_test, data[li->dsno]); 
    else {
      if (fdp != NULL) fdp->fdump(lx, amptr_outfix[li->below]); 
      m_out = lays(lx)->upward(is_test, amptr_outfix[li->below]); 
    }
    amptr_outfix(lx, m_out);     
  }
  int top_lx = top_lay_ind(); 
  m_top_out->set(amptr_outfix[top_lx]); 
}

/*------------------------------------------------------------*/ 
/*------------------------------------------------------------*/ 
void AzpCNet3_multi::resetParam(AzParam &azp, bool is_warmstart)
{
  const char *eyec = "AzpCNet3_multi::resetParam"; 

  AzpCNet3::resetParam(azp, is_warmstart); 
  if (!is_warmstart) resetParam_conn(azp, iia_layer_conn); 
  
  AzX::throw_if((s_save_lay0_fn.length() > 0), AzInputError, eyec, kw_save_lay0_fn, 
       " can be used only with the basic single-connection configuration."); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::printParam(const AzOut &out) const
{
  AzpCNet3::printParam(out); 
  printParam_conn(out); 
}

/*------------------------------------------------------------*/ 
#define kw_conn "conn"
#define conn_dlm '-'
#define kw_top "top"
/* input: hid_num */
/* output: sp_conn */
void AzpCNet3_multi::resetParam_conn(AzParam &azp, AzIIarr &iia_conn)
{
  const char *eyec = "AzpCNet3_multi::resetParam_conn"; 
  iia_conn.reset(); 
  sp_conn.size(); 

  int top = hid_num; 

  int ix; 
  for (ix = 0; ; ++ix) {
    AzBytArr s_conn; 
    AzBytArr s_kw(kw_conn); s_kw << ix << "="; 
    azp.vStr(s_kw.c_str(), &s_conn); 
    if (s_conn.length() <= 0) break; 
    
    AzStrPool sp(32,32); 
    AzTools::getStrings(s_conn.point(), s_conn.length(), conn_dlm, &sp); 

    AzX::throw_if((sp.size() != 2), AzInputError, "Expected the format like n-m for", kw_conn); 
    int below = parse_layerno(sp.c_str(0), hid_num); 
    int above = parse_layerno(sp.c_str(1), hid_num); 
    AzX::throw_if((below == top), AzInputError, eyec, "No edge is allowed to go out of the top layer"); 
    iia_conn.put(below, above); 
    sp_conn.put(&s_conn); 
  }
  
  /*---  default  ---*/
  if (iia_conn.size() == 0) {
    for (int lx = 0; lx < hid_num; ++lx) {
      int below = lx, above = lx + 1; 
      iia_conn.put(below, above); 
      AzBytArr s_conn; s_conn << below << conn_dlm << above; 
      sp_conn.put(s_conn.c_str()); 
    }
  }
}

/*------------------------------------------------------------*/ 
/* input: sp_conn */
void AzpCNet3_multi::printParam_conn(const AzOut &out) const
{
  AzPrint o(out); 
  for (int ix = 0; ix < sp_conn.size(); ++ix) {
    AzBytArr s_kw(kw_conn); s_kw << ix << "="; 
    o.printV(s_kw.c_str(), sp_conn.c_str(ix)); 
  }
  o.printEnd(); 
}

/*------------------------------------------------------------*/ 
int AzpCNet3_multi::parse_layerno(const char *str, int hid_num) const
{
  const char *eyec = "AzpCNet3_multi::parse_layerno"; 
  int layer_no = -1; 
  if (strcmp(str, kw_top) == 0) layer_no = hid_num; 
  else {
    AzX::throw_if((*str < '0' || *str > '9'), AzInputError, eyec, "Invalid layer#", str); 
    layer_no = atol(str); 
    AzX::throw_if((layer_no < 0 || layer_no > hid_num), AzInputError, eyec, "layer# is out of range", str); 
  }
  return layer_no; 
}

/*------------------------------------------------------------*/ 
int AzpCNet3_multi::get_dsno(AzParam &azp, const AzpData_ *trn, int layer_no, int dflt_dsno, const AzOut &out) const 
{
  AzBytArr s_pfx; 
  if (layer_no == top_lay_ind()) s_pfx.reset("top_"); 
  else                           s_pfx << layer_no; 
  int dsno = getParam_dsno(azp, s_pfx.c_str(), dflt_dsno); 
  checkParam_dsno(trn, s_pfx.c_str(), dsno); 
  printParam_dsno(out, s_pfx.c_str(), dsno); 
  return dsno; 
}  

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::print_dsno(int layer_no, int dsno, const AzOut &out) const 
{
  AzBytArr s_pfx; 
  if (layer_no == top_lay_ind()) s_pfx.reset("top_"); 
  else                           s_pfx << layer_no; 
  printParam_dsno(out, s_pfx.c_str(), dsno); 
} 
 
/*------------------------------------------------------------*/ 
#define kw_dsno "dataset_no="
/*------------------------------------------------------------*/ 
int AzpCNet3_multi::getParam_dsno(AzParam &azp, const char *pfx, int dflt_dsno) const 
{
  azp.reset_prefix(pfx); 
  int dsno = dflt_dsno; 
  azp.vInt(kw_dsno, &dsno); 
  azp.reset_prefix(); 
  if (dsno < 0) dsno = dflt_dsno;  /* if omitted, use dataset#0 */
  return dsno; 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::printParam_dsno(const AzOut &out, const char *pfx, int dsno) const 
{
  AzPrint o(out, pfx); 
  o.printV(kw_dsno, dsno); 
  o.printEnd(); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3_multi::checkParam_dsno(const AzpData_ *trn, const char *pfx, int dsno) const 
{
  if (dsno >= trn->datasetNum()) {
    AzBytArr s_kw(pfx); s_kw << kw_dsno << dsno; 
    AzX::throw_if(true, AzInputError, "AzpCNet3_multi::checkParam_dsno", "out of range: ", s_kw.c_str()); 
  }
}  