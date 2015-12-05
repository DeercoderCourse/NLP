/* * * * *
 *  AzpLayer.cpp    
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

#include "AzpLayer.hpp"
#include "AzTools.hpp"
#include "AzTextMat.hpp" /* to read external weights */

#include "AzpTimer_CNN.hpp"
using namespace AzpTimer_CNN_type; 
extern AzpTimer_CNN *timer; 

#define kw_dflt_old "default_"
#define kw_dflt_pfx ""

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
void AzpLayer::show_image_dim(const AzxD *pooled) const 
{
  /*---  show "image" dimensions  ---*/    
  if (!cs.patch->is_convolution()) {
    AzPrint::writeln(out, "   ---- fully-connected ----"); 
    cs.patch->show_input("   input: ", out);     
  }
  else {
    AzPrint::writeln(out, "   --- filering/pooling  ---"); 
    cs.patch->show_input("   input: ", out); 
    cs.patch->show_output("   after patch generation: ", out); 
    AzX::throw_if((pooled == NULL), "AzpLayer::show_image_dim", "No pooling_map"); 
    pooled->show("   after pooling: ", out); 
  } 
}

/*------------------------------------------------------------*/
void AzpLayer::show_weight_dim(const AzpWeight_ *wei, const char *header) 
const
{
  if (header != NULL && strlen(header) > 0) {
    AzPrint::writeln(out, header);  
  }
  AzPrint::writeln(out, "   --------  weights  --------"); 
  AzPrint::writeln(out, "   input dim: ", wei->get_dim()); 
  AzPrint::writeln(out, "   output dim: ", wei->classNum()); 
  AzPrint::writeln(out, "   #weights: ", wei->num_weights()); 
  AzPrint::writeln(out, "   ---------------------------");
}  

/*------------------------------------------------------------*/  
const char *AzpLayer::gen_prefix(const char *pfx_str, 
                                 AzBytArr &s_pfx)
{                               
  if (pfx_str != NULL && *pfx_str >= '0' && *pfx_str <= '9') {
    AzPrint::writeln(out, "   Using the given prefix as it starts with a digit ..."); 
    s_pfx.reset(pfx_str); 
    return s_pfx.c_str(); 
  }
  if (isTopLayer()) s_pfx.reset("top_"); 
  else {
    s_pfx.reset(); 
    s_pfx << layer_no << pfx_str; 
  }
  return s_pfx.c_str(); 
}
const char *AzpLayer::default_prefix(const AzParam &azp) const {
  AzBytArr s(azp.c_str()); 
  if (s.contains(kw_dflt_old)) return kw_dflt_old;  /* for compatibility */
  return kw_dflt_pfx; 
}
                               
/*------------------------------------------------------------*/
void AzpLayer::coldstart_nontop(const AzOut &_out, AzParam &azp,                    
                             bool is_spa, /* input data is sparse */                             
                             int lno, 
                             int cc, /* the number of channels */
                             const AzxD *input, 
                             const AzpUpperLayer *_upper, 
                             AzxD *output,
                             const char *_pfx)
{
  const char *eyec = "AzpLayer::coldstart_nontop"; 
  cs.check_if_ready(eyec); 
  out = _out; 
  upper = _upper; 
  AzX::throw_if((upper == NULL), eyec, "No upper layer"); 
  layer_no = lno; 
  const char *pfx = gen_prefix(_pfx, s_pfx); 
  const char *pfx_dflt = default_prefix(azp); 
  
  /*---  get parameters  ---*/
  lap.resetParam(out, azp, pfx_dflt, pfx); 
  cs.patch->resetParam(out, azp, pfx_dflt, pfx); 
  cs.weight->resetParam(out, azp, pfx_dflt, pfx); 
  cs.activ->resetParam(out, azp, pfx_dflt, pfx);
  cs.dropout->resetParam(out, azp, pfx_dflt, pfx);   
  cs.pool->resetParam(out, azp, pfx_dflt, pfx); 
  cs.resnorm->resetParam(out, azp, pfx_dflt, pfx); 
    
  /*---  set up patch generator  ---*/
  cs.patch->reset(out, input, cc, is_spa, is_var); 
  
  /*---  set up weights  ---*/
  int inp_dim = cs.patch->patch_length();     
  AzxD patched(cs.patch->output_region()); /* shape of output image */
  int loc_num = patched.size(); 
  cs.weight->reset(loc_num, lap.nodes, inp_dim, is_spa, is_var);
  
  /*---  set up activation  ---*/
  cs.activ->reset(out);   
  cs.dropout->reset(out); 
  
  /*---  set up pooling  ---*/
  AzxD pooled; 
  cs.pool->reset(out, &patched, &pooled); 
    
  /*---  set up response normalization  ---*/
  cs.resnorm->reset(out, &pooled); 
 
  /*---  ---*/
  xd_out.reset(&pooled); 
  show_image_dim(&pooled);    

  show_weight_dim(cs.weight); 
  init_weights(); 
  if (output != NULL) output->reset(&xd_out); 
}

/*------------------------------------------------------------*/
void AzpLayer::coldstart_top(const AzOut &_out, AzParam &azp,                    
                             int lno, 
                             int cc, /* the number of channels */
                             const AzxD *input, 
                             int class_num,
                             bool do_asis, 
                             const char *_pfx)
{
  const char *eyec = "AzpLayer::coldstart_top"; 
  cs.check_if_ready(eyec); 
  out = _out; 
  upper = NULL; 
  layer_no = lno; 
  const char *pfx = gen_prefix(_pfx, s_pfx); 
  const char *pfx_dflt = default_prefix(azp); 
  
  lap.resetParam_top(out, azp, pfx_dflt, pfx); 
  cs.weight->resetParam(out, azp, pfx_dflt, pfx); 
  cs.dropout->resetParam(out, azp, pfx_dflt, pfx);  
  
  /*---  set up patch generator for no patch generation  ---*/
  if (do_asis) cs.patch->setup_asis(out, input, cc); 
  else         cs.patch->setup_allinone(out, input, cc);  /* one big patch per data */
  int inp_dim = cs.patch->patch_length(); 
  lap.nodes = class_num; 
  bool is_spa = false; 
  cs.weight->reset(1, class_num, inp_dim, is_spa, is_var); 

  /*---  set up activation for no activation  ---*/
  cs.activ->setup_no_activation(); 
  cs.dropout->reset(out); 
    
  AzPrint::writeln(out, "   ------  top layer  ------"); 
  cs.patch->show_input("   input: ", out); 

  show_weight_dim(cs.weight); 
  init_weights(); 
  xd_out.reset(input->get_dim()); 
}

/*------------------------------------------------------------*/
void AzpLayer::coldstart_topthru(
                             const AzOut &_out, AzParam &azp,                 
                             int lno, 
                             int cc, /* the number of channels */
                             const AzxD *input, 
                             int class_num)
{
  const char *eyec = "AzpLayer::coldstart_topthru"; 
  cs.check_if_ready(eyec);   
  out = _out; 
  do_topthru = true; 
  is_var=is_spavar=using_lm2=false;
  upper = NULL; 
  layer_no = lno; 
  cs.patch->setup_allinone(out, input, cc);  /* one big patch */ 
  int dim = cs.patch->patch_length(); 
  if (dim != class_num) {
    AzBytArr s("dim="); s << dim << " class_num=" << class_num; 
    AzX::throw_if(true, eyec, "dimensionality conflict", s.c_str()); 
  }
  AzPrint::writeln(out, "   ------  top-through layer  ------");  
}
  
/*---------------------------------------------------------------*/
void AzpLayer::coldstart_lm2side(AzParam &azp,            
                             int inp_dim, /* input dimensionality */
                             const char *_pfx)
{
  cs.check_if_ready("coldstart_lms2side"); 
  using_lm2 = true; 
  AzPrint::writeln(out, "   -----       lm2 for side      -----"); 

  const char *pfx = gen_prefix(_pfx, s_pfx); 
  const char *pfx_dflt = default_prefix(azp); 
  cs.weight2->resetParam(out, azp, pfx_dflt, pfx); 
  cs.weight2->force_no_intercept(); /* since lm has intercept ... */
  cs.weight2->reset(-1, lap.nodes, inp_dim, false, is_var); /* not sparse */
  cs.weight2->initWeights(); 
  show_weight_dim(cs.weight2);
}

/*---------------------------------------------------------------*/
void AzpLayer::warmstart_lm2side(AzParam &azp, 
                             int inp_dim, /* input dimensionality */
                             bool for_testonly,
                             const char *_pfx)
{
  const char *eyec = "AzpLayer::warmstart_lm2side"; 
  cs.check_if_ready(eyec);   
  AzX::throw_if((!using_lm2), eyec, "lm2 wasn't expected"); 
  AzPrint::writeln(out, "   -----       lm2 for side      -----"); 

  if (!for_testonly) {
    const char *pfx = gen_prefix(_pfx, s_pfx); 
    const char *pfx_dflt = default_prefix(azp); 
    bool is_warmstart = true; 
    cs.weight2->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
    cs.weight2->force_no_intercept(); /* since lm has intercept ... */
  }
  AzX::throw_if((cs.weight2->get_dim() != inp_dim), eyec, "dim conflict"); 
  show_weight_dim(cs.weight2); 
}

/*------------------------------------------------------------*/
void AzpLayer::warmstart(const AzOut &_out, AzParam &azp, 
                             int lno, 
                             int cc, /* the number of channels */
                             const AzxD *input, 
                             const AzpUpperLayer *inp_upper, 
                             int class_num, /* only for the top layer */
                             AzxD *output, 
                             bool for_testonly, 
                             bool do_overwrite_layerno,
                             bool do_overwrite_param, 
                             const char *_pfx)
{
  const char *eyec = "AzpLayer::warmstart"; 
  cs.check_if_ready(eyec);   
  out = _out; 
  AzX::throw_if(do_topthru, eyec, "warmstart is not supported with topthru"); 
  upper = inp_upper; 
  if (do_overwrite_layerno) {
    layer_no = lno; 
  }
  AzX::throw_if((layer_no != lno), eyec, "layer# conflict"); 
  const char *pfx = gen_prefix(_pfx, s_pfx); 
  const char *pfx_dflt = default_prefix(azp); 
  
  bool is_warmstart = true;    
  if (!for_testonly) {
    cs.weight->resetParam(out, azp, pfx_dflt, pfx, is_warmstart);     
  }
  if (do_overwrite_param) {
    lap.resetParam_overwrite(azp, pfx);  
  }
  cs.patch->resetParam(out, azp, pfx_dflt, pfx, is_warmstart);     
  cs.activ->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
  cs.dropout->resetParam(out, azp, pfx_dflt, pfx, is_warmstart);  

  AzBytArr s_lno("layer#="); s_lno.cn(layer_no); 
  if (!cs.patch->isSameInput(input, cc)) {
    AzBytArr s("Conflict in the input: "); s.c(&s_lno); 
    AzX::throw_if(true, AzInputError, eyec, s.c_str()); 
  }
  
  if (isTopLayer()) {
    cs.activ->reset(out); 
    cs.dropout->reset(out);     
    int inp_dim = cs.patch->patch_length(); 
    AzX::throw_if((lap.nodes != class_num), AzInputError, eyec, "Conflict btw # of weight vectors and #class in the top layer"); 
    AzX::throw_if((cs.weight->classNum() != class_num || cs.weight->get_dim() != inp_dim), 
                  AzInputError, eyec, "Conflict in the weight dimensions in the top layer");    
    AzPrint::writeln(out, "   ------  top layer  ------"); 
    cs.patch->show_input("   input: ", out);     
    xd_out.reset(input->get_dim()); 
  }
  else {
    lap.printParam(out, pfx); 
    cs.pool->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
    cs.resnorm->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 

    int inp_dim = cs.patch->patch_length(); 
    AzX::throw_if((cs.weight->get_dim() != inp_dim && cs.weight->classNum() != lap.nodes), 
                  AzInputError, eyec, "Conflict in the weight dimensions", s_lno.c_str());
    AzxD pooled; 
    cs.pool->output(&pooled); 
    xd_out.reset(&pooled); 
    cs.resnorm->reset(out, &pooled); 
    cs.activ->reset(out); 
    cs.dropout->reset(out);     

    show_image_dim(&pooled);   
  }

  show_weight_dim(cs.weight); 
  if (output != NULL) output->reset(&xd_out);  
}

/*------------------------------------------------------------*/  
 void AzpLayer::patch_only(const AzPmat *m_below, AzPmat *m_out) const
{
  cs.patch->upward(false, m_below, m_out); 
}
 
/*------------------------------------------------------------*/
void AzpLayer::coldstart_var(const AzOut &_out, AzParam &azp,                   
                             bool is_spa, 
                             int lno, 
                             int cc, /* the number of channels */
                             const AzpUpperLayer *_upper, 
                             AzxD *output, /* output region */
                             const char *_pfx)
{
  const char *eyec = "AzpLayer::coldstart_var";
  cs.check_if_ready(eyec);   
  out = _out; 
  AzX::throw_if((_upper == NULL), eyec, "layer with variable-size data cannot be the top layer"); 
  using_lm2 = false; 
  is_var = true; 
  upper = _upper; 
  layer_no = lno; 
  
  /*---  get parameters  ---*/
  const char *pfx = gen_prefix(_pfx, s_pfx);     
  const char *pfx_dflt = default_prefix(azp); 
  cs.weight->resetParam(out, azp, pfx_dflt, pfx); 
  lap.resetParam(out, azp, pfx_dflt, pfx); 
  cs.patch_var->resetParam(out, azp, pfx_dflt, pfx); 
  cs.activ->resetParam(out, azp, pfx_dflt, pfx); 
  cs.dropout->resetParam(out, azp, pfx_dflt, pfx);    
  cs.pool_var->resetParam(out, azp, pfx_dflt, pfx); 
  cs.resnorm->resetParam(out, azp, pfx_dflt, pfx); 
  
  /*---  set up patch generation, activation, pooling, and response normalization  ---*/
  if (lap.do_patch_later) {
    if (is_spa) is_spavar = true; 
    cs.weight->reset(-1, lap.nodes, cc, is_spa, is_var);
    cs.patch_var->reset(out, lap.nodes, false);     
  }
  else {
    int dim = reset_patch_var(is_spa, cc);     
    cs.weight->reset(-1, lap.nodes, dim, is_spa, is_var); 
  }
  cs.activ->reset(out); 
  cs.dropout->reset(out); 
  cs.pool_var->reset(out); 
  cs.resnorm->reset(out, NULL);     

  show_weight_dim(cs.weight);  

  /*---  output size  ---*/
  cs.pool_var->output(&xd_out); 
  if (output != NULL) output->reset(&xd_out); 
  
  /*---  initialize weights  ---*/
  init_weights(); 
}

/*------------------------------------------------------------*/
int AzpLayer::reset_patch_var(bool is_spa, int cc)
{
  cs.patch_var->reset(out, cc, is_spa);   
  if (is_spa) {
    is_spavar = true; 
    return cc; 
  }
  else {
    return cs.patch_var->patch_length(); 
  }
}

/*------------------------------------------------------------*/
void AzpLayer::warmstart_var(const AzOut &_out, AzParam &azp, 
                             bool is_spa,                              
                             int lno, 
                             int cc, /* the number of channels */
                             const AzpUpperLayer *_upper, 
                             AzxD *output, /* output region */
                             bool for_testonly, 
                             bool do_overwrite_layerno, 
                             bool do_overwrite_param, 
                             const char *_pfx)
{
  const char *eyec = "AzpLayer::warmstart_var"; 
  cs.check_if_ready(eyec);   
  out = _out; 
  is_var = true; 
  AzX::throw_if((_upper == NULL), eyec, "a layer with variable-sized data cannot be the top layer"); 
  upper = _upper; 
  if (do_overwrite_layerno) {
    layer_no = lno; 
  }
  if (layer_no != lno) {
    AzBytArr s("mine:"); s << layer_no << " given:" << lno; 
    AzX::throw_if(true, eyec, "layer# conflict", s.c_str()); 
  }
  const char *pfx = gen_prefix(_pfx, s_pfx);   
  const char *pfx_dflt = default_prefix(azp); 
  bool is_warmstart = true;    
  if (!for_testonly) {
    cs.weight->resetParam(out, azp, pfx_dflt, pfx, is_warmstart);     
  }
  if (do_overwrite_param) {
    lap.resetParam_overwrite(azp, pfx); 
    cs.patch_var->resetParam(out, azp, pfx_dflt, pfx, false);     
  }
  else {
    cs.patch_var->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
  }
  cs.activ->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
  cs.dropout->resetParam(out, azp, pfx_dflt, pfx, is_warmstart);    

  AzBytArr s_lno("layer#="); s_lno.cn(layer_no); 
  lap.printParam(out, pfx); 
  cs.pool_var->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
  cs.resnorm->resetParam(out, azp, pfx_dflt, pfx, is_warmstart); 
  if (lap.do_patch_later) {
    AzX::throw_if((cs.weight->get_dim() != cc && cs.weight->classNum() != lap.nodes), 
                  AzInputError, eyec, "Conflict in the weight dimensions", s_lno.c_str());        
    cs.patch_var->reset(out, lap.nodes, false); 
    if (is_spa) is_spavar = true; 
  }
  else {
    int dim = reset_patch_var(is_spa, cc); 
    AzX::throw_if((cs.weight->get_dim() != dim && cs.weight->classNum() != lap.nodes), 
                  AzInputError, eyec, "Conflict in the weight dimensions", s_lno.c_str());
  }
  cs.activ->reset(out); 
  cs.dropout->reset(out);   
  cs.pool_var->reset(out); 
  cs.resnorm->reset(out, NULL); 

  show_weight_dim(cs.weight);  

  cs.pool_var->output(&xd_out); 
  if (output != NULL) output->reset(&xd_out); 
}

/*------------------------------------------------------------*/  
/* fixed-sized dense input */
const AzPmat *AzpLayer::upward(bool is_test, const AzPmat *m_below, const AzPmat *m_forlm2)
{
  const char *eyec = "AzpLayer::upward"; 
  reset_timestamp(); reset_timestampD(); 
  check_input(m_below, m_forlm2, eyec);   
  AzX::no_support(lap.do_patch_later, eyec, "PatchLater with fixed-sized input");   
  int ll = layer_no; 
  if (do_dropout_first()) {
    AzPmat m_do(m_below); 
    cs.dropout->upward(is_test, &m_do);         timestampD(ld_UDropout); 
    cs.patch->upward(is_test, &m_do, &m_x);     timestampD(ld_UPatch);
  } 
  else {
    cs.patch->upward(is_test, m_below, &m_x);   timestampD(ld_UPatch); 
  }
  
  if (do_topthru) {
    m_value.set(&m_x); 
  }
  else if (isTopLayer()) {
    cs.weight->upward(is_test, &m_x, &m_value); timestampD(ld_ULmod); 
    activ_upward(is_test, &m_value);    
  }
  else {
    AzPmat m_v; 
    cs.weight->upward(is_test, &m_x, &m_v);     timestampD(ld_ULmod); 
    if (using_lm2) {
      m_lm2x.set(m_forlm2); /* save this for downward */ 
      AzPmat m_lm2v; 
      cs.weight2->upward(is_test, m_forlm2, &m_lm2v);   timestampD(ld_ULmod); 
      m_v.add(&m_lm2v); 
    }    
    if (!lap.do_activate_after_pooling) activ_upward(is_test, &m_v);
    cs.pool->upward(is_test, &m_v, &m_value);   timestampD(ld_UPool); 
    if (lap.do_activate_after_pooling) activ_upward(is_test, &m_value);
    cs.resnorm->upward(is_test, &m_value);              timestampD(ld_UResnorm); 
  }
  timestamp((is_test) ? l_Apply : l_Upward); 
  return &m_value; 
}

/*------------------------------------------------------------*/  
/* fixed-sized sparse input */
const AzPmat *AzpLayer::upward(bool is_test, const AzPmatSpa *m_below, const AzPmat *m_forlm2)
{
  const char *eyec = "AzpLayer::upward(sparse)"; 
  AzX::no_support(do_dropout_first(), eyec, "dropout on sparse input"); 
  AzX::no_support(lap.do_patch_later, eyec, "PatchLater with fixed-sized input");  
  reset_timestamp(); reset_timestampD(); 
  check_input(m_below, m_forlm2, "AzpLayer::upward(sparse)");   
  if (do_topthru) {
    m_value.set(&m_x); 
  }  
  else if (isTopLayer()) {
    ms_x.set(m_below); /* save this for downward */
    cs.weight->upward(is_test, m_below, &m_value);    timestampD(ld_ULmod); 
    activ_upward(is_test, &m_value);     
  }
  else {
    AzPmat m_v; 
    ms_x.set(m_below); /* save this for downward */
    cs.weight->upward(is_test, m_below, &m_v);        timestampD(ld_ULmod); 
    if (using_lm2) {
      m_lm2x.set(m_forlm2); /* save this for downward */ 
      AzPmat m_lm2v; 
      cs.weight2->upward(is_test, m_forlm2, &m_lm2v); timestampD(ld_ULmod); 
      m_v.add(&m_lm2v); 
    }  
    if (!lap.do_activate_after_pooling) activ_upward(is_test, &m_v); 
    cs.pool->upward(is_test, &m_v, &m_value); timestampD(ld_UPool);
    if (lap.do_activate_after_pooling)  activ_upward(is_test, &m_v); 
    cs.resnorm->upward(is_test, &m_value);            timestampD(ld_UResnorm); 
  }
  timestamp((is_test) ? l_Apply : l_Upward);   
  return &m_value; 
}
 
/*------------------------------------------------------------*/  
void AzpLayer::get_lossd_after_fixed(AzPmat *m_lossd_a, bool through_lm2, int below) const
{
  if (through_lm2) {
    cs.weight2->downward(&m_lossd, m_lossd_a);  timestampD(ld_DLmod); 
  }
  else { 
    AzPmat m_unapply; 
    if (do_topthru) m_unapply.set(&m_lossd); 
    else            cs.weight->downward(&m_lossd, &m_unapply);  /* m_lossd #col: #pixels before pooling */ 
    timestampD(ld_DLmod); 
    cs.patch->downward(&m_unapply, m_lossd_a); timestampD(ld_DPatch); 
    if (do_dropout_first()) cs.dropout->downward(m_lossd_a);     
  }
}

/*------------------------------------------------------------*/  
/* fixed-sized input */
void AzpLayer::downward_fixed(const AzPmat *m_loss_deriv, bool through_lm2) /* ignored by hidden layers */
{
  const char *eyec = "AzpLayer::downward_fixed"; 
  reset_timestamp();  reset_timestampD(); 
  AzX::throw_if(is_var, eyec, "is_var is ON?!"); 
  
  int ll = layer_no; 
  m_lossd.zeroOut(); 
  if (isTopLayer()) {  /* output layer */
    AzX::throw_if((m_loss_deriv == NULL), eyec, "output layer needs loss_deriv input"); 
    m_lossd.set(m_loss_deriv);   
    if (!do_topthru) activ_downward(&m_lossd); 
  }
  else {
    AzPmat m_lossd_after; 
    upper->get_lossd_after_fixed(&m_lossd_after, through_lm2, layer_no); 
    cs.resnorm->downward(&m_lossd_after);                 timestampD(ld_DResnorm); 
    if (lap.do_activate_after_pooling) activ_downward(&m_lossd_after); 
    cs.pool->downward(&m_lossd_after, &m_lossd);  timestampD(ld_DPool); 
    if (!lap.do_activate_after_pooling) activ_downward(&m_lossd);
  }
  timestamp(l_Downward);  
}

/*------------------------------------------------------------*/  
void AzpLayer::updateDelta_fixed(int d_num, bool do_update_lm, bool do_update_activ)
{
  AzX::throw_if((is_var), "AzpLayer::updateDelta_fixed", "is_var is ON?!"); 
  reset_timestamp(); 
  if (do_topthru) return; 
  if (do_update_lm) {
    int width = (isTopLayer()) ? 1 : cs.pool->input_size();  /* 1 unless it's convolutonal */
    if (m_x.colNum() == width*d_num) cs.weight->updateDelta(d_num, &m_x, &m_lossd);     
    else if (ms_x.colNum() == d_num) cs.weight->updateDelta(d_num, &ms_x, &m_lossd);   
    else {
      AzX::throw_if(true, "AzpLayer::updateDelta", "#columns of m_x or ms_x is wrong"); 
    }
    if (using_lm2) {
      cs.weight2->updateDelta(d_num, &m_lm2x, &m_lossd); 
    } 
  }
  if (do_update_activ) {
    activ_updateDelta(d_num); 
  }
  timestamp(l_UpdateDelta);   
}

/*------------------------------------------------------------*/  
void AzpLayer::flushDelta(bool do_update_lm, bool do_update_activ)
{
  if (do_topthru) return; 
  reset_timestamp(); 
  if (do_update_lm) {
    cs.weight->flushDelta(); 
    if (using_lm2) cs.weight2->flushDelta(); 
  }
  if (do_update_activ) {
    activ_flushDelta(); 
  }
  timestamp(l_Flush); 
}

/*------------------------------------------------------------*/  
void AzpLayer::init_weights() 
{
  if (lap.s_weight_fn.length() <= 0) {
    cs.weight->initWeights();   
  }
  else {
    const char *w_fn = lap.s_weight_fn.c_str(); 
    AzPrint::writeln(out, "To set external weights reading: ", w_fn); 
    AzpLm_Untrainable lm_inp; 
    if (AzBytArr::endsWith(w_fn, ".lm")) {
      lm_inp.read(w_fn); 
    }
    else if (AzBytArr::endsWith(w_fn, "pmat")) {
      AzPmat m_w(w_fn);
      AzPmat m_i(1, m_w.colNum()); 
      lm_inp.reset(&m_w, &m_i); 
    }      
    else {
      AzDmat m_w; 
      if (AzBytArr::endsWith(w_fn, "dmat")) m_w.read(w_fn); 
      else                                  AzTextMat::readMatrix(w_fn, &m_w); 
      AzDmat m_i(1, m_w.colNum()); 
      lm_inp.reset(&m_w, &m_i); 
    }     
    AzBytArr s("  "); s << " (" << lm_inp.nodeNum() << " nodes)";
    AzPrint::writeln(out, s); 
    cs.weight->initWeights(&lm_inp, lap.weight_coeff); 
  }
}

/*------------------------------------------------------------*/  
void AzpLayer::show_stat(AzBytArr &s) const {
  if (do_topthru) {
    s.c("[topthru]"); 
    return; 
  }
  cs.weight->show_stat(s); 
  if (using_lm2) {
    s.c(";-lm2-;"); 
    cs.weight2->show_stat(s); 
  }
  activ_show_stat(s); 
}

/*------------------------------------------------------------*/  
/*---           for variable-length input/output           ---*/
/*------------------------------------------------------------*/  
void AzpLayer::check_for_var(const AzPmatVar *mv_below, const AzPmatSpaVar *msv_below, 
                             const AzPmatVar *mv_forlm2, 
                             const char *eyec) const
{
  AzX::throw_if((isTopLayer() || !is_var), eyec, "This is not a var layer."); 
  if (is_spavar) { /* sparse input */
    AzX::throw_if((mv_below != NULL || msv_below == NULL), eyec, "check_ptrs_var failed.  is_spavar=ON"); 
  }
  else { /* dense input */
    AzX::throw_if((mv_below == NULL || msv_below != NULL), eyec, "check_ptrs_var failed. is spavar=OFF"); 
  }
  check_forlm2(mv_forlm2, eyec); 
}                                    

/*------------------------------------------------------------*/  
const AzPmatVar *AzpLayer::upward_var(bool is_test, const AzPmatVar *mv_below, /* dense&var from the layer below */
                                      const AzPmatSpaVar *msv_below, /* sparse&var, original input */
                                      const AzPmatVar *mv_forlm2,  
                                      bool do_skip_pooling, /* for analysis only */
                                      bool do_skip_resnorm) /* for analysis only */                                 
{
  const char *eyec = "AzpLayer::upward_var"; 
  reset_timestamp(); reset_timestampD(); 
  check_for_var(mv_below, msv_below, mv_forlm2, eyec); 
  AzX::no_support((using_lm2 && lap.do_average_spa), eyec, "AverageSpa with lm2"); 
  mv_x.reset(); msv_x.reset(); mv_lm2x.reset(); 
  AzPmat m_v; 
  const AzPintArr *d_index = NULL; 
  if (mv_below != NULL) { /* dense input */
    if (do_dropout_first()) {
      AzPmatVar mv_do(mv_below); 
      cs.dropout->upward(is_test, mv_do.data_u());  timestampD(ld_UDropout);
      cs.patch_var->upward(is_test, &mv_do, &mv_x);  /* save this for updateDelta */
                                                    timestampD(ld_UPatch); 
    }
    else {
      cs.patch_var->upward(is_test, mv_below, &mv_x);  /* save this for updateDelta */
                                                      timestampD(ld_UPatch);
    }
    cs.weight->upward(is_test, mv_x.data(), &m_v);  timestampD(ld_ULmod);
    d_index = mv_x.d_index(); 
  }
  else if (msv_below != NULL) { /* sparse input */
    AzX::no_support(do_dropout_first(), eyec, "dropout on sparse input");  
    /* no support for patch generation with sparse input */
    cs.weight->upward(is_test, msv_below->data(), &m_v);  timestampD(ld_ULmod);
    msv_x.set(msv_below); /* save this for updateDelta */
    if (lap.do_average_spa) average_spa(msv_below, &m_v); 
    d_index = msv_below->d_index(); 
  }
    
  if (using_lm2) {  
    AzPmat m_v2; 
    mv_lm2x.set(mv_forlm2); /* save this for updateDelta */
    cs.weight2->upward(is_test, mv_forlm2->data(), &m_v2); timestampD(ld_ULmod);
    m_v.add(&m_v2);    
  }
  AzPmatVar mv_v; mv_v.set(&m_v, d_index); 
  if (lap.do_patch_later) { /* patch after weighting */
    AzX::no_support(msv_below == NULL, eyec, "PatchLater with non-sparse variable-sized input");     
    AzPmatVar mv(&mv_v); 
    cs.patch_var->upward(is_test, &mv, &mv_v); 
  }  
  if (!lap.do_activate_after_pooling) {
    activ_upward(is_test, mv_v.data_u()); 
  }
  if (do_skip_pooling) mv_value.set(&mv_v); /* for analysis only */
  else {
    cs.pool_var->upward(is_test, &mv_v, &mv_value);   timestampD(ld_UPool); 
  }
  if (lap.do_activate_after_pooling) {  
    activ_upward(is_test, mv_value.data_u()); 
  }
  if (!do_skip_resnorm) { /* true only for analysis */
    cs.resnorm->upward(is_test, mv_value.data_u());   timestampD(ld_UResnorm); 
  }
  
  timestamp((is_test) ? l_Apply : l_Upward);   
  return &mv_value; 
}

/*------------------------------------------------------------*/ 
void AzpLayer::average_spa(const AzPmatSpaVar *m_x, AzPmat *m) 
{
  if (m_one.rowNum() != m_x->rowNum() || m_one.colNum() != 1) {
    m_one.reform(m_x->rowNum(), 1); 
    m_one.set(1); 
  }
  AzPmat m_scale;
  AzPs::prod(&m_scale, &m_one, m_x->data(), true, false); 
  bool do_inv = true; 
  m->multiply_eachcol(&m_scale, do_inv);  /* divide each column by sum over rows */
}
  
/*------------------------------------------------------------*/  
void AzpLayer::get_lossd_after_var(AzPmatVar *mv_lossd_a, bool through_lm2, int below) const
{
  AzPmat m_unapply; 
  if (through_lm2) cs.weight2->downward(mv_lossd.data(), &m_unapply); 
  else             cs.weight->downward(mv_lossd.data(), &m_unapply); 
  AzPmatVar mv_unapply; mv_unapply.set(&m_unapply, mv_lossd.d_index()); 
  if (is_spavar || through_lm2) mv_lossd_a->set(&mv_unapply); 
  else                          cs.patch_var->downward(&mv_unapply, mv_lossd_a); 
  if (do_dropout_first()) {
    cs.dropout->downward(mv_lossd_a->data_u()); 
  }
}

/*------------------------------------------------------------*/  
/* variable-sized input */
void AzpLayer::downward_var(bool through_lm2)
{
  const char *eyec = "AzpLayer::downward_var"; 
  reset_timestamp();  reset_timestampD(); 
  AzX::throw_if((isTopLayer() || !is_var), eyec, "This is not a var layer."); 
  
  AzPmatVar mv_lossd_after; 
  if (upper->is_variable_input()) {
    upper->get_lossd_after_var(&mv_lossd_after, through_lm2, layer_no); 
  }
  else {
    AzPmat m_lossd_after; 
    upper->get_lossd_after_fixed(&m_lossd_after, through_lm2, layer_no);  
    mv_lossd_after.set(&m_lossd_after, cs.pool_var->out_ind());     
  }
 
  cs.resnorm->downward(mv_lossd_after.data_u());     timestampD(ld_DResnorm); 
  if (lap.do_activate_after_pooling) {
    activ_downward(mv_lossd_after.data_u());
  }
  cs.pool_var->downward(&mv_lossd_after, &mv_lossd); timestampD(ld_DPool);
  if (!lap.do_activate_after_pooling) {
    activ_downward(mv_lossd.data_u());          
  }
  if (lap.do_patch_later) {
    AzPmatVar mv(&mv_lossd); 
    cs.patch_var->downward(&mv, &mv_lossd);          timestampD(ld_DPatch); 
  }
  timestamp(l_Downward); 
}

/*------------------------------------------------------------*/  
/* variable-sized input */
void AzpLayer::updateDelta_var(int d_num, bool do_update_lm, bool do_update_activ)
{
  const char *eyec = "AzpLayer::updateDelta_var"; 
  reset_timestamp(); 
  AzX::throw_if((!is_var), eyec, "this is not a var layer"); 
  if (do_update_lm) {
    if (is_spavar) { /* sparse input */
      AzX::throw_if((msv_x.rowNum() <= 0), eyec, "No sparse input has been saved.");     
      if (lap.do_average_spa) {
        AzPmat m(mv_lossd.data()); 
        average_spa(&msv_x, &m); 
        cs.weight->updateDelta(d_num, msv_x.data(), &m); 
      }      
      else {
        cs.weight->updateDelta(d_num, msv_x.data(), mv_lossd.data()); 
      }
    }
    else { /* dense input */
      AzX::throw_if((mv_x.rowNum() <= 0), eyec, "No dense input has been saved.");  
      cs.weight->updateDelta(d_num, mv_x.data(), mv_lossd.data()); 
    }
    
    if (using_lm2) {
      AzX::throw_if((mv_lm2x.size() <= 0), eyec, "No input to lm2 has been saved."); 
      cs.weight2->updateDelta(d_num, mv_lm2x.data(), mv_lossd.data());  /* dense input */
    }
  }
  if (do_update_activ) {
    activ_updateDelta(d_num); 
  }
  timestamp(l_UpdateDelta); 
}

/*------------------------------------------------------------*/  
void AzpLayer::reset_timestamp() const {
  if (timer != NULL) timer->reset_Layer(); 
}
void AzpLayer::reset_timestampD() const {
  if (timer != NULL) timer->reset_LayerDetail(); 
}
void AzpLayer::timestamp(AzpTimer_CNN_type::l_type typ) const {
  if (timer != NULL) timer->stamp_Layer(typ, layer_no); 
}
void AzpLayer::timestampD(AzpTimer_CNN_type::ld_type typ) const {
  if (timer !=  NULL) timer->stamp_LayerDetail(typ, layer_no); 
}
