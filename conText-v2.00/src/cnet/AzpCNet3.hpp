/* * * * *
 *  AzpCNet3.hpp
 *  Copyright (C) 2015 Rie Johnson
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
 
#ifndef _AZP_CNET3_HPP_
#define _AZP_CNET3_HPP_

#include "AzpLayer.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"
#include "AzpData_.hpp"
#include "AzStepszSch.hpp"
#include "AzMomentSch.hpp"
#include "AzpDataSeq.hpp"
#include "AzpCompoSet_.hpp"
#include "AzpTimer_CNN.hpp"
 
/*------------------------------------------------------------*/ 
class AzpCNet3_fdump {
protected: 
  int layer_no;   
  AzFile file; 
public:
  AzpCNet3_fdump(const char *fn, int lx) : layer_no(-1) { init(fn, lx); }
  void init(const char *fn, int lx) {
    layer_no = lx; file.reset(fn); file.open("wb"); 
  }
  void term() { file.close(true); }
  bool is_active(int lx) const { return (lx == layer_no); }
  inline void fdump(int lx, const AzPmat *m_feat) {
    if (lx != layer_no) return; 
    if (m_feat == NULL) return; 
    AzDmatc m(m_feat->rowNum(), m_feat->colNum()); 
    m_feat->copy_to(&m, 0); 
    const AZ_MTX_FLOAT *vals = m.rawcol(0); 
    file.writeBytes(vals, sizeof(vals[0])*m.size()); 
  }
}; 

/*------------------------------------------------------------*/
class AzpCNet3 {
protected:
  const AzpCompoSet_ *cs; 
  AzpNetCompoPtrs nco;  

  AzpLayers lays; 

  AzBytArr s_data_sign; 
  int class_num; /* 1: regression */  
  int hid_num; 
  
  AzOut out, eval_out; 
 
  int save_interval; 
/*  #define azp_lay0_ext ".lay0" */
  #define azp_lay0_ext ".layer0"  
  #define kw_save_lay0_fn "save_lay0_fn="
  AzBytArr s_save_fn, s_save_lay0_fn; 

  int test_interval, dx_inc; 
  int ite_num, minib, tst_minib, rseed, init_ite; 

  bool do_exact_trnloss, do_timer; 
  bool do_update_lm, do_update_act; 
  bool do_discard, do_show_iniloss;
  bool do_topthru;  

  int zerotarget_ratio; 
  bool do_partial_y; 
  bool do_y_try; 
  
  /*---   ---*/
  AzStepszSch sssch;  /* step-size scheduler */
  AzMomentSch momsch; /* momentum scheduler */
  AzpDataSeq dataseq;  /* data sequencer to make mini-batches */

  /*---  for layer-by-layer training  ---*/
  #define focus_dlm_inner  '-'
  #define focus_dlm_outer '_'  
  AzBytArr s_focus_layers; 
  int min_focus; 
  AzIntArr ia_doUpdate; 
  const int *arr_doUpdate; 
  
  /*---  for fine tuning (optional) after layer-by-layer training  ---*/
  int fine_ite_num;       /* # of iterations of fine tuning */
  AzStepszSch fine_sssch; /* step-size scheduler for fine tuning */
  AzMomentSch fine_momsch; /* momentum scheduler for fine tuning */
  
  bool do_less_verbose; 
  
  AzPrng rng; /* for sparse y */
  
  bool do_ds_dic; 
  AzDataArr<AzDicc> ds_dic; 

  AzpCNet3_fdump *fdp; 
  
/*  static const int reserved_len=256; */
  static const int reserved_len=255;  /* 8/19/2015: for do_ds_dic */
  static const int version=0;  
public:
  AzpCNet3(const AzpCompoSet_ *_cs) : dx_inc(-1), 
             zerotarget_ratio(-1), do_partial_y(false), do_y_try(false), 
             hid_num(0), class_num(1), save_interval(-1), test_interval(-1), out(log_out), 
             ite_num(0), minib(100), tst_minib(100), rseed(1), init_ite(0), 
             do_exact_trnloss(false), do_timer(false),  
             arr_doUpdate(NULL), min_focus(-1), 
             do_update_lm(true), do_update_act(true), do_discard(false), 
             do_topthru(false), do_show_iniloss(false), fine_ite_num(-1), 
             do_less_verbose(false), do_ds_dic(false), fdp(NULL) {
    AzX::throw_if((_cs == NULL), "AzpCNet3::constructor", "No component set"); 
    cs = _cs; 
    cs->check_if_ready("AzpCNet3::constructor"); 
    nco.reset(cs);   
  }
  ~AzpCNet3() {}

  void resetOut(const AzOut &_out) { out = _out; }
  virtual void write(const char *fn) {
    AzFile file(fn); file.open("wb");
    write(&file); file.close(true);
  }
  virtual void read(const char *fn) {
    AzFile file(fn); file.open("rb");
    read(&file); 
  }
  
  /*--------*/
  virtual void write(AzFile *file) {
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(do_ds_dic);          /* 8/19/2015 */
    if (do_ds_dic) ds_dic.write(file); /* 8/19/2015 */
    s_data_sign.write(file); 
    file->writeInt(class_num);
    file->writeBool(do_topthru);     
    nco.write(file);    
    lays.write(file); 
  }
  virtual void read(AzFile *file) {
    cs->check_if_ready("AzpCNet3::read"); 
    int my_version = AzTools::read_header(file, reserved_len); 
    do_ds_dic = file->readBool();       /* 8/19/2015 */
    if (do_ds_dic) ds_dic.read(file); /* 8/19/2015 */
    s_data_sign.read(file); 
    class_num = file->readInt(); 
    do_topthru = file->readBool(); 
    nco.read(file);    
    lays.read(cs, file);
    hid_num = lays.size() - 1; 
  }

  inline int classNum() const { return class_num; }
  inline virtual void training(
                      const AzOut *inp_eval_out, 
                      AzParam &azp,  
                      AzpData_ *trn, /* not const b/c of first|next_batch */
                      const AzpData_ *tst, 
                      const AzpData_ *tst2) {
    if (inp_eval_out != NULL) eval_out = *inp_eval_out; 
    training(azp, trn, tst, tst2); 
  }
  virtual void training(AzParam &azp, 
                      AzpData_ *trn, 
                      const AzpData_ *tst, 
                      const AzpData_ *tst2); 
          
  virtual double predict(AzParam &azp, 
                      const AzpData_ *tst,
                      /*--- output (only one of them should be not NULL) ---*/
                      double *out_loss=NULL, 
                      AzDmat *m_pred=NULL, 
                      AzDmatc *mc_pred=NULL, 
                      AzPmat *m_p=NULL
                      ); /*loc const loc*/
  virtual void write_embedded(AzParam &azp, const AzpData_ *tst); 
  virtual void write_features(AzParam &azp, const AzpData_ *tst, const char *out_fn, int lx); 
 
  const AzpLayer *layer(int lx) {
    AzX::throw_if((lx < 0 || lx >= lays.size()), "AzpCNet3::layer", "index is out of range"); 
    return lays[lx]; 
  }

  virtual void printHelp_training(AzHelp &h) const {
    printHelp(h); 
  }
  virtual void printHelp_predict(AzHelp &h) const {
    printHelp_test(h); 
  }
  virtual void printHelp_write_features(AzHelp &h) const {
    printHelp_test(h); 
  }
  virtual void printHelp_write_embedded(AzHelp &h) const; 
  
  virtual void write_word_mapping_in_lay0(const char *lay0_fn, const char *dic_fn); 
  virtual void write_word_mapping(const char *fn, int dsno) const;   
  
  /*---  for compatibility  ---*/
  void add_dic_to_nn(const AzpData_ *trn); 
  void add_dic_to_lay0(const char *old_fn, const AzpData_ *trn, const char *new_fn); 
  
protected:  
  virtual void _write_embedded(const AzpData_ *tst, int feat_top_num, const char *fn, bool do_skip_pooling, bool do_skip_resnorm); 
  virtual AzpLayer *topLayer_u() {
    AzX::throw_if((lays.size() <= 0), "AzpCNet3::topLayer_u", "No layer"); 
    return lays(lays.size()-1); 
  }
  virtual void coldstart(const AzpData_ *trn, AzParam &azp); 
  virtual void warmstart(const AzpData_ *trn, AzParam &azp, bool for_testonly=false); 
  virtual void up_down(const AzpData_ *trn, const int *dxs, int d_num, 
                       double *out_loss=NULL, bool do_update=true); 
  virtual const AzPmat *up(bool is_vg_x, const AzpData_X &data,  bool is_test); 
  virtual void apply(const AzpData_ *tst, const int dx_begin, int d_num, AzPmat *m_top_out); 
  virtual const AzPmatSpa *for_sparse_y_init(const AzpData_ *trn, const int *dxs, int d_num, AzPmatSpa *m_y); 
  virtual const AzPmat *for_sparse_y_init2(const AzpData_ *trn, const int *dxs, int d_num, AzPmat *m_y); 
  virtual void for_sparse_y_term(); 
  
  /*---  override these to add/remove hyper-parameters  ---*/
  virtual void resetParam(AzParam &azp, bool is_warmstart=false);
  virtual void printParam(const AzOut &out) const; 
  virtual void printHelp(AzHelp &h) const; 

  virtual void resetParam_test(AzParam &azp);
  virtual void printParam_test(const AzOut &out) const; 
  virtual void printHelp_test(AzHelp &h) const; 
  
  virtual void _resetParam(AzParam &azp); 
  virtual void _printParam(AzPrint &o) const; 
  virtual void _printHelp(AzHelp &h) const; 
  
  virtual void _resetParam_fine(AzParam &azp, bool is_warmstart=false); 
  virtual void _printParam_fine(AzPrint &o) const; 
  
  virtual void get_nn_fn(AzParam &azp, const AzOut &out, int lx, AzBytArr &s_nn_fn) const; 
  virtual void init_layer(AzParam &azp, bool is_var, 
                          const AzpUpperLayer *upper, 
                          bool is_spa, int lx, int cc, 
                          AzxD *sizes, /* inout */
                          bool for_testonly=false); 

  virtual void check_word_mapping(bool is_warmstart, const AzpData_ *trn, const AzpData_ *tst, const AzpData_ *tst2); 
  /*-------------------------------------------------------*/
  
  virtual void sup_training(AzpData_ *trn, const AzpData_ *tst, const AzpData_ *tst2); 
  virtual void sup_training_loop(AzpData_ *trn, const AzpData_ *tst, const AzpData_ *tst2);                         
  virtual void fine_tuning(AzpData_ *trn, const AzpData_ *tst, const AzpData_ *tst2); 
  virtual void test_save(const AzpData_ *trn, const AzpData_ *tst, const AzpData_ *tst2, 
                         double tr_y_avg, int ite, double tr_loss); 
  virtual void save(int ite_no);    
  virtual void write_lay0(const char *fn); 
  static void write_lay0(const char *fn, AzDicc &lay0_dic, AzpLayer &lay0); 
  static void read_lay0(const char *fn, AzDicc &lay0_dic, AzpLayer &lay0); 
  virtual void check_data_signature(const AzpData_ *data, const char *msg, bool do_loose=false) const; 
  virtual void set_focus_layers(const AzBytArr &s_focus); /* which layers to train */ 
  virtual void reset_stepsize(AzStepszSch *sssch, AzMomentSch *momsch); 
  virtual void change_stepsize_if_required(AzStepszSch *sssch, AzMomentSch *momsch, int ite); 
  virtual double get_loss_noreg(const AzpData_ *data); 
  virtual double test(const AzpData_ *data, double *out_loss, AzBytArr *s_pf=NULL);               
  virtual void apply_all2(const AzpData_ *data, AzPmat *m_out); 
                     
  virtual void pick_classes(AzSmat *ms_y, /* inout */
                            int nega_count, 
                            /*---  output  ---*/
                            AzIntArr *ia_p2w, AzIntArr *ia_w2p, AzPmatSpa *m_y) const; 
                     
  /*---  to display information  ---*/
  virtual void show_progress(int num, int inc, int data_size, double loss_sum) const; 
  virtual void show_layer_stat(const char *str=NULL) const;   
  virtual void show_perf(const AzBytArr &s_pf, int ite, 
                       double regloss, double iniloss, double loss, double exact_loss, 
                       double test_loss, double perf, double test_loss2, double perf2,
                       AzBytArr *s_out=NULL) const;  
  virtual void show_size() const {
    AzBytArr s; 
    int sz = lays_show_size(s); 
    s << "alltotal:" << sz; 
    AzPrint::writeln(out, s); 
  }
  virtual void reset_timestamp() const; 
  virtual void timestamp(AzpTimer_CNN_type::t_type typ) const; 
  
  /*--------------------------------------------------------------*/ 
  /*  Functions that go over all layers.                          */
  /*  For convenience for attching side layers in the extension.  */
  /*--------------------------------------------------------------*/   
  virtual void lays_setup_for_reg_L2init() {
    for (int lx = 0; lx < lays.size(); ++lx) lays(lx)->setup_for_reg_L2init();     
  }   
  virtual void lays_check_for_reg_L2init() const {
    for (int lx = 0; lx < lays.size(); ++lx) lays[lx]->check_for_reg_L2init(); 
  }   
  virtual void lays_flushDelta() {
    for (int lx = 0; lx < lays.size(); ++lx) {
      if (arr_doUpdate == NULL || arr_doUpdate[lx] == 1) lays(lx)->flushDelta(do_update_lm, do_update_act); 
    }
  }
  virtual void lays_end_of_epoch() {
    for (int lx = 0; lx < lays.size(); ++lx) {
      if (arr_doUpdate == NULL || arr_doUpdate[lx] == 1) lays(lx)->end_of_epoch(); 
    }   
  }
  virtual void lays_multiply_to_stepsize(double coeff, const AzOut *out) {
    for (int lx = 0; lx < lays.size(); ++lx) lays(lx)->multiply_to_stepsize(coeff, out);
  }  
  virtual void lays_set_momentum(double newmom, const AzOut *out) {
    for (int lx = 0; lx < lays.size(); ++lx) lays(lx)->set_momentum(newmom, out);     
  }
  virtual double lays_regloss(double *out_iniloss=NULL) const {
    double loss = 0, iniloss = 0; 
    for (int lx = 0; lx < lays.size(); ++lx) loss += lays[lx]->regloss(&iniloss); 
    if (out_iniloss != NULL) *out_iniloss = iniloss; 
    return loss;    
  }
  virtual void lays_show_stat(AzBytArr &s) const {
    for (int lx = 0; lx < lays.size(); ++lx) {
      s << "layer#" << lx << ":"; 
      lays[lx]->show_stat(s); 
    }
  }
  virtual int lays_show_size(AzBytArr &s) const {
    int sz = 0; 
    for (int lx = 0; lx < lays.size(); ++lx) sz += lays[lx]->show_size(s); 
    return sz; 
  }  
  virtual int top_lay_ind() const {
    return lays.size()-1;
  }
}; 
#endif
