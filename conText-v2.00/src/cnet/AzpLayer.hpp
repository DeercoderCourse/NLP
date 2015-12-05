/* * * * *
 *  AzpLayer.hpp
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

#ifndef _AZP_LAYER_HPP_
#define _AZP_LAYER_HPP_

#include "AzPmat.hpp"
#include "AzHelp.hpp"
#include "AzpCompoSet_.hpp"
#include "AzpTimer_CNN.hpp"

class AzpLayerParam {
protected: 
  static const int version = 0; 
/*  static const int reserved_len = 64;  */
  static const int reserved_len = 63;  /* 04/14/2015: do_patch_later */
public: 
  int nodes; 
  bool do_activate_after_pooling; 
  bool do_dropout_later; /* true: apply dropout right after activation */
                         /* false: apply dropout to the input to the layer */
  bool do_average_spa; 
  AzBytArr s_weight_fn; 
  double weight_coeff; 
  bool do_patch_later;  /* make patches after weighting; only for variable-sized sparse input */

  void reset(const AzpLayerParam *inp) { *this = *inp; } 
  void write(AzFile *file) const {
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(do_patch_later); 
    file->writeBool(do_average_spa); 
    file->writeBool(do_activate_after_pooling); 
    file->writeInt(nodes); 
  }
  void read(AzFile *file) {
    AzTools::read_header(file, reserved_len);
    do_patch_later = file->readBool(); 
    do_average_spa = file->readBool();
    do_activate_after_pooling = file->readBool();  
    nodes = file->readInt(); 
  }
 
  AzpLayerParam() : nodes(0), do_activate_after_pooling(false), do_dropout_later(false), weight_coeff(1), 
                         do_average_spa(false), do_patch_later(false) {}                     
  void resetParam_top(const AzOut &out, AzParam &azp, const char *dflt_pfx, const char *pfx) {
    resetParam(out, azp, dflt_pfx, pfx, true); 
  }
  void resetParam(const AzOut &out, AzParam &azp, const char *dflt_pfx, const char *pfx, 
                  bool is_top=false) {
    resetParam(azp, dflt_pfx, is_top); 
    resetParam(azp, pfx, is_top); 
    checkParam(pfx, is_top); 
    printParam(out, pfx, is_top); 
  } 

  /*------------------------------------------------------------*/  
  #define kw_nodes "nodes="
  #define kw_do_activate_after_pooling "ActivateAfterPooling"
  #define kw_do_dropout_later "DropoutLater"
  #define kw_weight_fn "weight_fn="
  #define kw_weight_coeff "weight_coeff="
  #define kw_do_average_spa_old "AverageSpa"  
  #define kw_do_average_spa "Avg"
  #define kw_do_patch_later "PatchLater"  
  /*------------------------------------------------------------*/  
  void resetParam(AzParam &azp, const char *pfx, bool is_top=false) {                                          
    azp.reset_prefix(pfx); 
    if (!is_top) {
      azp.vInt(kw_nodes, &nodes); 
      azp.swOn(&do_activate_after_pooling, kw_do_activate_after_pooling); 
      azp.swOn(&do_dropout_later, kw_do_dropout_later); 
      azp.swOn(&do_average_spa, kw_do_average_spa_old); /* for compatibility */
      azp.swOn(&do_average_spa, kw_do_average_spa); 
      azp.swOn(&do_patch_later, kw_do_patch_later);          
    }
    azp.vStr(kw_weight_fn, &s_weight_fn); 
    if (s_weight_fn.length() > 0) {
      azp.vFloat(kw_weight_coeff, &weight_coeff);
    }
    azp.reset_prefix();        
  }
  void overwrite_sw(AzParam &azp, const char *kw, bool *sw) const {
    AzBytArr s_kw("Dont", kw); 
    if (*sw) azp.swOff(sw, s_kw.c_str());   
    else     azp.swOn(sw, kw); 
  }
  void resetParam_overwrite(AzParam &azp, const char *pfx, bool is_top=false) {
    if (is_top) return;     
    azp.reset_prefix(pfx);
    overwrite_sw(azp, kw_do_average_spa_old, &do_average_spa);     
    overwrite_sw(azp, kw_do_average_spa, &do_average_spa);      
    overwrite_sw(azp, kw_do_patch_later, &do_patch_later);        
    overwrite_sw(azp, kw_do_activate_after_pooling, &do_activate_after_pooling);       
    overwrite_sw(azp, kw_do_dropout_later, &do_dropout_later);      
    azp.reset_prefix();        
  }  
  void printHelp(AzHelp &h) const {
    h.item_required(kw_nodes, "Number of weight vectors (or neurons)."); 
    /* kw_do_activate_after_pooling kw_do_dropout_later kw_weight_fn kw_weight_coeff kw_do_average_spa */
  }
  void checkParam(const char *pfx, bool is_top=false) const {
    const char *eyec = "AzpLayerParam::checkParam"; 
    if (!is_top) {
      AzXi::throw_if_nonpositive(nodes, eyec, kw_nodes, pfx); 
    }
    if (s_weight_fn.length() > 0) {
      AzXi::throw_if_nonpositive(weight_coeff, eyec, kw_weight_coeff, pfx); 
    }
  }
  void printParam(const AzOut &out, const char *pfx, bool is_top=false) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    o.reset_prefix(pfx);    
    if (!is_top) {
      o.printV(kw_nodes, nodes); 
      o.printSw(kw_do_activate_after_pooling, do_activate_after_pooling); 
      o.printSw(kw_do_dropout_later, do_dropout_later); 
      o.printSw(kw_do_average_spa, do_average_spa);
      o.printSw(kw_do_patch_later, do_patch_later);             
    }
    o.printV_if_not_empty(kw_weight_fn, s_weight_fn); 
    if (s_weight_fn.length() > 0) {
      o.printV(kw_weight_coeff, weight_coeff); 
    }   
    o.printEnd(); 
  }   
}; 

/*------------------------------------------------------------*/
class AzpUpperLayer { /* interface */
public:
  virtual bool is_variable_input() const = 0; 
  virtual void get_lossd_after_fixed(AzPmat *m_lossd_a, bool through_lm2, int below) const = 0; 
  virtual void get_lossd_after_var(AzPmatVar *mv_lossd_a, bool through_lm2, int below) const = 0; 
}; 
                    
/*------------------------------------------------------------*/
class AzpLayer : public virtual /* implements */ AzpUpperLayer {
protected:
  AzpLayerParam lap; 
  
  /*---  for lm2  ---*/
  bool using_lm2; 

  /*---  ---*/
  AzPmat m_value;  /* set by upward: output from this layer, i.e., values after pooling */
                   /* z: the input to this layer */
                   /* must be kept until the upper layer uses this */
  AzPmat m_lossd;  /* set by downward: derivatives of the loss w.r.t. w dot z */
                   /* must be kept until the lower layer uses this */

  const AzpUpperLayer *upper; 

  int layer_no; 
  AzOut out; 
  AzxD xd_out; /* set this at coldstart/warmstart */

  /*---  variable-length input/output  ---*/
  bool is_var, is_spavar; 
  AzPmatVar mv_lossd, mv_value; 

  /*---  keep data for updateDelta  ---*/
  AzPmat m_x, m_lm2x; 
  AzPmatSpa ms_x; 
  
  AzPmatVar mv_x, mv_lm2x; 
  AzPmatSpaVar msv_x; 
  /*-----------------------------------*/
  
  bool do_topthru; 
  
  AzPmat m_one; /* for word2vec */

  AzBytArr s_pfx; /* work */
  AzpLayerCompoPtrs cs; 
  
  /*---  Use with a lot of caution  ---*/
  AzpLm *linmod_u() { return cs.weight->linmod_u(); }  
  AzpWeight_ *wei_u() { return cs.weight; }
  /*-----------------------------------*/
  friend class AzpCNet2; 
  friend class AzpCNet2_svrg;   
  friend class AzpCNet2_pre; 
  friend class AzpCNet3; 
  friend class AzpCNet3_pre; 
  
  static const int version = 0; 
  static const int reserved_len = 64;  
public:
  AzpLayer() : upper(NULL), layer_no(-1), out(log_out), 
               is_var(false), is_spavar(false), using_lm2(false), do_topthru(false) {} 
  ~AzpLayer() {}
 
  void printHelp(AzHelp &h) const { 
    cs.check_if_ready("AzpLayer::printHelp");  
    lap.printHelp(h); 
    cs.dropout->printHelp(h); 
    cs.patch->printHelp(h); 
    cs.patch_var->printHelp(h); 
    cs.weight->printHelp(h); 
    cs.activ->printHelp(h); 
    cs.pool->printHelp(h); 
    cs.pool_var->printHelp(h); 
    cs.resnorm->printHelp(h); 
  }
  
  void reset(const AzpCompoSet_ *cset) {
    cs.reset(cset); 
  }
  const AzpLayerCompoPtrs *components() const { return &cs; }
  void resetOut(AzOut &_out) { out = _out; }
  
  bool is_input_var() const { return is_var; }

  const AzpLm *linmod() const { return cs.weight->linmod(); }
  const AzpWeight_ *wei() const { return cs.weight; }

  void show_stat(AzBytArr &s) const; 
  
  void write(const char *fn) { AzFile::write(fn, this); }
  void read(const char *fn) { AzFile::read(fn, this); }
  void write(AzFile *file) {
    cs.check_if_ready("write"); 
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(do_topthru); 
    file->writeBool(using_lm2); 
    file->writeBool(is_var); 
    file->writeInt(layer_no); 
    lap.write(file); 
    cs.write(file); 
  }
  void read(AzFile *file) {
    cs.check_if_ready("read"); 
    AzTools::read_header(file, reserved_len);     
    do_topthru = file->readBool(); 
    using_lm2 = file->readBool(); 
    is_var = file->readBool(); 
    layer_no = file->readInt(); 
    lap.read(file); 
    cs.read(file); 
  }
  void reset_upper(const AzpUpperLayer *my_upper) { upper = my_upper; } /* use this with caution */
  virtual void reset(const AzpLayer *inp, const AzpUpperLayer *my_upper) {
    prohibit_lm2("AzpLayer::reset(inp,upper)"); 
    upper = my_upper; 
    cs.reset(&inp->cs); 
    lap.reset(&inp->lap); 
    m_value.set(&inp->m_value); 
    m_lossd.set(&inp->m_lossd); 
    layer_no = inp->layer_no; 
    out = inp->out; 
    xd_out.reset(&inp->xd_out); 
    
    /*---  ---*/
    using_lm2 = inp->using_lm2; 
  
    is_var = inp->is_var; 
    is_spavar = inp->is_spavar; 

    m_x.set(&inp->m_x); 
    ms_x.set(&inp->ms_x); 
    m_lm2x.set(&inp->m_lm2x);     
    mv_x.set(&inp->mv_x); 
    msv_x.set(&inp->msv_x); 
    mv_lm2x.set(&inp->mv_lm2x); 
    
    mv_lossd.set(&inp->mv_lossd); 
    mv_value.set(&inp->mv_value); 
    
    cs.check_if_ready("reset(copy)"); 
  }

  virtual inline int nodeNum() const {
    return cs.weight->classNum(); 
  }
  virtual inline int output_channels() const {
    int cc = (lap.do_patch_later) ? cs.patch_var->patch_length() : cs.weight->classNum(); 
    return cs.activ->output_channels(cc); 
  }
  virtual inline int dim_filtered() const {
    prohibit_lm2("AzpLayer::dim_filtered"); 
    return cs.weight2->get_dim(); 
  }
  virtual inline const AzPmat *data_filtered() const {
    prohibit_lm2("AzpLayer::data_filtered"); 
    return &m_x; /* valid only right after upward */
  }
  virtual inline void output_shape(AzxD *xd_output) const {
    xd_output->reset(&xd_out); 
  }
  virtual inline bool isTopLayer() const {
    return (upper == NULL) ? true : false; 
  }
  virtual inline void makeTop() {
    upper = NULL; 
  }
  virtual inline const AzpActiv_ *ptr_to_activ() const {
    return cs.activ; 
  }
  virtual void coldstart_nontop(const AzOut &_out, AzParam &azp, 
                 bool is_spa, int lno, int channels,
                 const AzxD *input, 
                 const AzpUpperLayer *_upper, 
                 AzxD *output=NULL, const char *_pfx=NULL); 
  virtual void coldstart_top(const AzOut &_out, AzParam &azp, 
                  int lno, int channels, const AzxD *input, int class_num, 
                  bool do_asis=false, const char *_pfx=NULL);                      
  virtual void coldstart_topthru(const AzOut &_out, AzParam &azp, int lno, 
                             int channels, const AzxD *input, int class_num); 
  virtual void coldstart_lm2side(AzParam &azp, 
                                 int inp_dim, /* input dimensionality */
                                 const char *_pfx=NULL); 
  virtual void warmstart_lm2side(AzParam &azp, 
                                 int inp_dim, /* input dimensionality */
                                 bool for_testonly=false, const char *_pfx=NULL); 
  virtual void warmstart(const AzOut &_out, AzParam &azp, 
                 int lno, int channels, const AzxD *input, 
                 const AzpUpperLayer *inp_upper, 
                 int class_num, /* only for the top layer */
                 AzxD *output=NULL, bool for_testonly=false, 
                 bool do_overwrite_layerno=false, /* for doing "plus" with "KeepTop" */
                 bool do_overwrite_param=false, 
                 const char *_pfx=NULL); 
  /*---  returns #channels of output  ---*/                 
  virtual void coldstart_var(const AzOut &_out, AzParam &azp,                    
                             bool is_spa, int lno, int channels, 
                             const AzpUpperLayer *inp_upper, 
                             /*---  output  ---*/
                             AzxD *output=NULL, /* output region */      
                             const char *_pfx=NULL); 
  /*---  returns #channels of output  ---*/
  virtual void warmstart_var(const AzOut &_out, AzParam &azp, /* not tested */
                             bool is_spa, int lno, int channels, 
                             const AzpUpperLayer *inp_upper, 
                             AzxD *output=NULL, /* output region */
                             bool for_testonly=false, 
                             bool do_overwrite_layerno=false, 
                             bool do_overwrite_param=false, 
                             const char *_pfx=NULL); 

  void setup_for_reg_L2init() {
    cs.weight->setup_for_reg_L2init(); 
    if (using_lm2) cs.weight2->setup_for_reg_L2init(); 
  }  
  void check_for_reg_L2init() const {
    cs.weight->check_for_reg_L2init(); 
    if (using_lm2) cs.weight2->check_for_reg_L2init(); 
  } 
  
  const AzPmat *upward(bool is_test, const AzpData_X *below, const AzPmat *m_forlm2=NULL) {
    AzX::throw_if((below == NULL), "AzpLayer::upward", "No input"); 
    if      (below->is_dense())  return upward(is_test, below->dense(), m_forlm2); 
    else if (below->is_sparse()) return upward(is_test, below->sparse(), m_forlm2); 
    else {                       AzX::throw_if(true, "AzpLayer::upward", "No input data"); return NULL; }
  }  
  const AzPmatVar *upward_var(bool is_test, const AzpData_X *below, 
                              const AzPmatVar *mv_forlm2=NULL, /* for semi only */
                              bool do_skip_pooling=false, bool do_skip_resnorm=false) /* true only for analysis */ {
    AzX::throw_if((below == NULL), "AzpLayer::upward_var", "No input"); 
    if (below->is_spavar())   return upward_var(is_test, below->spavar(), mv_forlm2, do_skip_pooling, do_skip_resnorm);   
    else if (below->is_var()) return upward_var(is_test, below->var(), mv_forlm2, do_skip_pooling, do_skip_resnorm); 
    else {                     AzX::throw_if(true, "AzpLayer::upward_var", "No input data"); return NULL; }
  }
                             
  virtual const AzPmat *upward(bool is_test, const AzPmat *m_below, const AzPmat *m_forlm2=NULL); 
  virtual const AzPmat *upward(bool is_test, const AzPmatSpa *m_below, const AzPmat *m_forlm2=NULL);    
  const AzPmatVar *upward_var(bool is_test, const AzPmatSpaVar *msv_below, 
                              const AzPmatVar *mv_forlm2=NULL, 
                              bool do_skip_pooling=false, bool do_skip_resnorm=false) {
    return upward_var(is_test, NULL, msv_below, mv_forlm2, do_skip_pooling, do_skip_resnorm); 
  }  
  const AzPmatVar *upward_var(bool is_test, const AzPmatVar *mv_below, 
                              const AzPmatVar *mv_forlm2=NULL, 
                              bool do_skip_pooling=false, bool do_skip_resnorm=false) {                              
    return upward_var(is_test, mv_below, NULL, mv_forlm2, do_skip_pooling, do_skip_resnorm); 
  }

  virtual void downward(const AzPmat *m_loss_deriv=NULL, bool through_lm2=false) { 
    if (is_var) downward_var(through_lm2); 
    else        downward_fixed(m_loss_deriv, through_lm2); 
  }
  virtual void updateDelta(int d_num, bool do_update_lm=true, bool do_update_activ=true) {
    if (is_var) updateDelta_var(d_num, do_update_lm, do_update_activ); 
    else        updateDelta_fixed(d_num, do_update_lm, do_update_activ); 
  }
  
  virtual void flushDelta(bool do_update_lm=true, bool do_update_activ=true); 
  inline virtual void end_of_epoch() {
    if (do_topthru) return; 
    cs.weight->end_of_epoch(); 
    if (using_lm2) cs.weight2->end_of_epoch(); 
    cs.activ->end_of_epoch(); 
  }

  /*----------------*/  
  virtual void patch_only(const AzPmat *m_below, AzPmat *m_out) const; /* for auto encoder */

  /*----------------*/  
  inline virtual double regloss(double *iniloss) const {
    double loss = cs.weight->regloss(iniloss); 
    if (using_lm2) loss += cs.weight2->regloss(iniloss); 
    return loss; 
  }          
  
  inline void multiply_to_stepsize(double factor, const AzOut *out=NULL) {
    cs.weight->multiply_to_stepsize(factor, out); 
    if (using_lm2) cs.weight2->multiply_to_stepsize(factor, out); 
  }
  inline void set_momentum(double newmom, const AzOut *out=NULL) {
    cs.weight->set_momentum(newmom, out); 
    if (using_lm2) cs.weight2->set_momentum(newmom, out); 
  }

  /*---  to check memory usage  ---*/
  inline int show_size(AzBytArr &s) const { 
    int sz = 0; 
    s << "layer#" << layer_no << ":"; 
    s << "m_value:" << m_value.size() << ";"; sz += m_value.size(); 
    s << "m_lossd:" << m_lossd.size() << ";"; sz += m_lossd.size();  
    s << "m_x:" << m_x.size() << ";"; sz += m_x.size(); 
    s << "total:" << sz << ";"; 
    return sz; 
  }

  /*---  use this when just testing, i.e., not training  ---*/
  void release_memory_for_update() {
    m_x.reset(); 
    m_lm2x.reset();
    ms_x.reset();     
    mv_x.reset(); 
    mv_lm2x.reset(); 
    msv_x.reset(); 
  }

  /*---  to implement AzpUpperLayer  ---*/
  bool is_variable_input() const { return is_var; }
  void get_lossd_after_fixed(AzPmat *m_lossd_a, bool through_lm2, int below) const;  
  void get_lossd_after_var(AzPmatVar *mv_lossd_a, bool through_lm2, int below) const; 
  
protected:  
  const char *default_prefix(const AzParam &azp) const; 
  const AzPmatVar *upward_var(bool is_test, const AzPmatVar *mv_below, const AzPmatSpaVar *msv_below, 
                              const AzPmatVar *mv_forlm2, 
                              bool do_skip_pooling=false, bool do_skip_resnorm=false); /* for analysis only */
  virtual void downward_fixed(const AzPmat *m_loss_deriv, bool through_lm2); /* ignored by hidden layers */
  void downward_var(bool through_lm2); 
  virtual void updateDelta_fixed(int d_num, bool do_update_lm, bool do_update_activ); 
  void updateDelta_var(int d_num, bool do_update_lm, bool do_update_activ);  
    
  void prohibit_lm2(const char *eyec) const {
    AzX::throw_if(using_lm2, eyec, "lm2 is prohibited"); 
  }
  template <class M> /* M: AzPmat | AzPmatSpa */
  void check_input(const M *m0, const AzPmat *m_forlm2, const char *eyec) const {
    AzX::throw_if((m0 == NULL), eyec, "No input data"); 
    check_forlm2(m_forlm2, eyec); 
  }
  template <class M> /* M: AzPmat | AzPmatVar */
  void check_forlm2(const M *m_forlm2, const char *eyec) const {
    AzX::throw_if((using_lm2 && m_forlm2 == NULL), eyec, "No input data for lm2"); 
    AzX::throw_if((!using_lm2 && m_forlm2 != NULL), eyec, "Input data was given to unused lm2"); 
  }
  void average_spa(const AzPmatSpaVar *m_x, AzPmat *m);   
  virtual void init_weights(); 

  int num_data() const {
    if (isTopLayer()) return m_x.colNum(); 
    else              return m_x.colNum() / cs.pool->input_size();     
  }
  
  int reset_patch_var(bool is_spa, int channels); /* returns dimensionality after filtering */   
  void show_image_dim(const AzxD *pooled) const; 
  void show_weight_dim(const AzpWeight_ *wei, const char *header=NULL) const;   
  const char *gen_prefix(const char *pfx_str, AzBytArr &s_pfx); 
  void check_for_var(const AzPmatVar *mv_below, const AzPmatSpaVar *msv_below, const AzPmatVar *m_forlm2, const char *eyec) const; 

  inline void activ_upward(bool is_test, AzPmat *m) {  
    cs.activ->upward(is_test, m);   timestampD(AzpTimer_CNN_type::ld_UActiv);
    if (do_dropout_later()) {
      cs.dropout->upward(is_test, m); timestampD(AzpTimer_CNN_type::ld_UDropout);
    }
  }
  inline void activ_downward(AzPmat *m) {  
    if (do_dropout_later()) {
      cs.dropout->downward(m); timestampD(AzpTimer_CNN_type::ld_DDropout);
    }
    cs.activ->downward(m); timestampD(AzpTimer_CNN_type::ld_DActiv);
  }  
  inline void activ_updateDelta(int d_num) {  
    cs.activ->updateDelta(d_num); 
  }  
  inline void activ_show_stat(AzBytArr &s) const {
    cs.activ->show_stat(s); 
    cs.dropout->show_stat(s); 
  }
  inline void activ_flushDelta() {
    cs.activ->flushDelta(); 
  }
  
  void reset_timestamp() const; 
  void reset_timestampD() const; 
  void timestamp(AzpTimer_CNN_type::l_type typ) const; 
  void timestampD(AzpTimer_CNN_type::ld_type typ) const; 
  
  bool do_dropout_first() const { /* apply dropout to the input to the layer */
    return (cs.dropout->is_active() && !lap.do_dropout_later); 
  }
  bool do_dropout_later() const { /* apply dropout to the results of activation */
    return (cs.dropout->is_active() && lap.do_dropout_later); 
  }
}; 

/*------------------------------------------------------------*/
class  AzpLayers {
protected: 
  AzDataArr<AzpLayer> layers; 
public: 
  inline virtual AzpLayer *point_u(int idx) { return layers.point_u(idx); }
  inline virtual const AzpLayer *point(int idx) const { return layers.point(idx); }
  inline int size() const { return layers.size(); }
  AzpLayers() {}
  void reset() { layers.reset(); }
  void reset(const AzpCompoSet_ *cset) {
    AzX::throw_if((cset == NULL), "AzpLayers::reset(compo)", "No component set"); 
    for (int lx = 0; lx < size(); ++lx) point_u(lx)->reset(cset); 
  }  
  inline void reset(const AzpCompoSet_ *cset, int layer_num) {   
    reset(layer_num); 
    reset(cset); 
  }
  void write(AzFile *file) {
    file->writeInt(size()); 
    for (int lx = 0; lx < size(); ++lx) point_u(lx)->write(file); 
  }
  void read(const AzpCompoSet_ *cset, AzFile *file) {
    AzX::throw_if((cset == NULL), "AzpLayers::read", "No component set");   
    int sz = file->readInt(); 
    reset(sz); 
    reset(cset); 
    for (int lx = 0; lx < size(); ++lx) point_u(lx)->read(file); 
  }
  const AzpLayer *operator[](int lx) const { return point(lx); };  
  AzpLayer *operator()(int lx) { return point_u(lx); };   
protected:   
  inline virtual void reset(int layer_num) { 
    layers.reset(layer_num); 
  }  
}; 
#endif
