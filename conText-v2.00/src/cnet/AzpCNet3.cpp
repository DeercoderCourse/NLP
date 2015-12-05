/* * * * *
 *  AzpCNet3.cpp
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

#include "AzpCNet3.hpp"
#include "AzPrint.hpp"
#include "AzTools.hpp"
#include "AzMomentSch.hpp"
#include "AzpTimer_CNN.hpp"

using namespace AzpTimer_CNN_type; 

/*---  global  ---*/
AzpTimer_CNN my_timer, *timer = NULL; 
AzByte param_dlm = ' '; 
/*----------------*/

/*---  to check memory usage  ---*/
static bool do_check_size = false; 

/*------------------------------------------------------------*/ 
void AzpCNet3::coldstart(const AzpData_ *trn, 
                         AzParam &azp)
{
  const char *eyec = "AzpCNet3::coldstart"; 

  lays.reset(cs, hid_num+1);  /* +1 for the output layer */

  AzxD sizes; 
  trn->get_info(&sizes); 
  int cc = trn->channelNum(); 

  bool is_spa = trn->is_sparse_x(); 
  int lx; 
  /*---  hidden layers with variable-sized input  ---*/
  for (lx = 0; lx < hid_num; ++lx) {
    if (sizes.is_valid()) break; /* output of the previous layer is fixed-size */
    AzPrint::writeln(out, "Cold-starting (variable-size input) layer#", lx);  
    init_layer(azp, true, lays[lx+1], is_spa, lx, cc, &sizes); 
    cc = lays[lx]->output_channels(); 
    is_spa = false; 
  }  
  /*---  hidden layers with fixed-sized input  ---*/
  for ( ; lx < hid_num; ++lx) {
    AzPrint::writeln(out, "Cold-starting layer#", lx);   
    init_layer(azp, false, lays[lx+1], is_spa, lx, cc, &sizes); 
    cc = lays[lx]->output_channels();     
  }
  
  /*---  top layer, which only takes fixed-sized input  ---*/
  AzpLayer *top_lay = lays(hid_num); 
  if (!sizes.is_valid()) { /* variable-sized */
    AzX::throw_if((!trn->is_vg_y()), AzInputError, eyec, "Input to the top layer must be fixed-size"); 
    AzPrint::writeln(out, "Cold-starting the top layer with variable-sized input/output"); 
    bool do_asis = true; 
    top_lay->coldstart_top(out, azp, hid_num, cc, &sizes, class_num, do_asis);     
  }   
  else if (do_topthru) {
    AzPrint::writeln(out, "Cold-starting the topthru layer"); 
    top_lay->coldstart_topthru(out, azp, hid_num, cc, &sizes, class_num); 
  }
  else {
    AzPrint::writeln(out, "Cold-starting the top layer"); 
    top_lay->coldstart_top(out, azp, hid_num, cc, &sizes, class_num); 
  }
  
  lays_setup_for_reg_L2init(); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::init_layer(AzParam &azp, bool is_var, 
                          const AzpUpperLayer *upper, 
                          bool is_spa, int lx, int cc, 
                          AzxD *sizes, /* inout */
                          bool for_testonly)
{
  AzBytArr s_nn_fn; 
  get_nn_fn(azp, out, lx, s_nn_fn); 
  if (s_nn_fn.length() <= 0) { /* initialize in a regular way */
    if (is_var) lays(lx)->coldstart_var(out, azp, is_spa, lx, cc, upper, sizes);          
    else {
      AzxD next_sizes; 
      lays(lx)->coldstart_nontop(out, azp, is_spa, lx, cc, sizes, upper, &next_sizes);   
      sizes->reset(&next_sizes); 
    }
  }  
  else { /* initialize by a nn file */
    AzTimeLog::print("Reading ", s_nn_fn.c_str(), out); 
    AzpCNet3 nn(cs); nn.read(s_nn_fn.c_str()); 
    lays(lx)->reset(nn.lays[0], upper); 
    bool do_ow_lno = true, do_ow_param = true; 
    if (is_var) { /* variable-sized input */
      lays(lx)->warmstart_var(out, azp, is_spa, lx, cc, upper, sizes, for_testonly, do_ow_lno, do_ow_param);    
    }
    else { /* fixed-sized input */
      AzxD next_sizes; 
      lays(lx)->warmstart(out, azp, lx, cc, sizes, upper, -1, &next_sizes, for_testonly, do_ow_lno, do_ow_param); 
      sizes->reset(&next_sizes);       
    }
  }
}

/*------------------------------------------------------------*/ 
void AzpCNet3::warmstart(
                        const AzpData_ *trn, 
                        AzParam &azp,
                        bool for_testonly)
{
  const char *eyec = "AzpCNet3::warmstart"; 

  AzX::no_support(do_topthru, eyec, "warmstart with topthru"); 
  AzX::throw_if((lays.size() != hid_num+1), AzInputError, eyec, "#layer doesn't match"); 
  AzxD shape;  
  trn->get_info(&shape); 
  bool is_spa = trn->is_sparse_x(); 
  int cc = trn->channelNum(); 
  
  /*---  hidden layers with variable-length input  ---*/
  int lx;   
  for (lx = 0; lx < hid_num; ++lx) {
    if (shape.is_valid()) break; /* output of the previous layer is fixed-size */
    AzPrint::writeln(out, "Warm-starting (variable-sized input) layer#", lx);      
    lays(lx)->warmstart_var(out, azp, is_spa, lx, cc, lays[lx+1], &shape, for_testonly);
    cc = lays[lx]->output_channels();
    is_spa = false; 
  }
    
  /*---  hidden layers with fixed-sized input  ---*/
  AzxD next_shape; 
  for ( ; lx < hid_num; ++lx) {
    AzPrint::writeln(out, "Warm-starting layer#", lx); 
    lays(lx)->warmstart(out, azp, lx, cc, &shape, lays[lx+1], -1, &next_shape, for_testonly); 
    cc = lays[lx]->output_channels(); 
    shape.reset(&next_shape); 
  }
  AzPrint::writeln(out, "Warm-starting the top layer"); 
  lays(hid_num)->warmstart(out, azp, hid_num, cc, &shape, NULL, class_num, &next_shape, for_testonly); 
  
  lays_check_for_reg_L2init();   
}

/*------------------------------------------------------------*/ 
void AzpCNet3::training(AzParam &azp, 
                      AzpData_ *trn, 
                      const AzpData_ *tst, /* may be NULL (starting from 1/24/2014) */
                      const AzpData_ *tst2)
{
  const char *eyec = "AzpCNet3::training"; 
  bool is_warmstart = false; 

  /*---  check the data signature; determine whether this is warm-start  ---*/
  if (lays.size() > 0) {
    is_warmstart = true;    
    check_data_signature(trn, "training data");
    AzTimeLog::print("Warm-start ... ", out); 
  }
  else {
    trn->signature(&s_data_sign); 
  }
  AzTimeLog::print("Data signature: ", s_data_sign.c_str(), out); 
  if (tst != NULL) check_data_signature(tst, "test data");
  if (tst2 != NULL) check_data_signature(tst2, "test data2");

  /*---  check #class (this must be done before resetParam) ---*/
  class_num = trn->classNum(); 
  AzTimeLog::print("#class=", class_num, out); 
  AzX::throw_if((tst != NULL && tst->classNum() != class_num), 
                AzInputError, eyec, "training data and test data have differnt number of classes"); 
  
  /*---  read parameters  ---*/ 
  resetParam(azp, is_warmstart); 
  printParam(out); 

  /*---  reset random number generator  ---*/
  if (rseed > 0) { /* added for co-training on 9/16/2014 */
    srand(rseed); 
    rng.reset_seed(rseed); /* used for many-class sparse y */
  }
  AzX::no_support((trn->base() != NULL && class_num > 1), eyec, "base"); 
 
  nco.loss->check_target(out, trn); 
 
  /*---  initialize layers  ---*/
  if (!is_warmstart) coldstart(trn, azp); 
  else               warmstart(trn, azp); 
  azp.check(out); 

  /*---  check word-mapping set consistency  ---*/
  check_word_mapping(is_warmstart, trn, tst, tst2);  /* added 8/19/2015 */
  
  /*---  set timer  ---*/
  timer = NULL; 
  if (do_timer) {
    timer = &my_timer; 
    timer->reset(hid_num); 
  }  
  
  /*---  setup mini-batches  ---*/
  dataseq.init(trn, out); 
  
  /*---  training  ---*/
  sup_training(trn, tst, tst2); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::check_word_mapping(bool is_warmstart, const AzpData_ *trn, const AzpData_ *tst, const AzpData_ *tst2)
{
  const char *eyec = "AzpCNet3::check_word_mapping"; 
  if (!is_warmstart) {
    AzX::throw_if((trn == NULL), eyec, "No training data?!"); 
    do_ds_dic = true; 
    ds_dic.reset(trn->datasetNum()); 
    for (int ix = 0; ix < trn->datasetNum(); ++ix) {
      ds_dic(ix)->reset(trn->get_dic(ix)); 
    }
  }  
  if (!do_ds_dic) return; 

  AzTimeLog::print("Checking word-mapping ... ", out); 
  AzX::throw_if((trn == NULL && tst == NULL), eyec, "No data?!"); 
  int ds_num = (trn != NULL) ? trn->datasetNum() : tst->datasetNum(); 
  AzX::throw_if((ds_dic.size() != ds_num), eyec, "# of word-mapping sets does not match the number saved with the model."); 
  AzX::throw_if((tst != NULL && tst->datasetNum() != ds_num), eyec, "# of word-mapping sets of test data is wrong."); 
  AzX::throw_if((tst2 != NULL && tst2->datasetNum() != ds_num), eyec, "# of word-mapping sets of 2nd test data is wrong.");   
  for (int ix = 0; ix < ds_num; ++ix) {
    AzBytArr s_err("Word-mapping check failed."); 
    if (ds_num > 1) s_err << "  data#=" << ix; 
    if (trn != NULL)  AzX::throw_if(!ds_dic[ix]->is_same(trn->get_dic(ix)), eyec, "Training data: ", s_err.c_str()); 
    if (tst != NULL)  AzX::throw_if(!ds_dic[ix]->is_same(tst->get_dic(ix)), eyec, "Test data: ", s_err.c_str()); 
    if (tst2 != NULL) AzX::throw_if(!ds_dic[ix]->is_same(tst2->get_dic(ix)), eyec, "2nd test data: ", s_err.c_str()); 
  }             
}

/*------------------------------------------------------------*/ 
void AzpCNet3::sup_training_loop(AzpData_ *trn,  /* not const b/c of first|next_batch */
                                const AzpData_ *tst, 
                                const AzpData_ *tst2)
{
  AzTimeLog::print("supervised training: #hidden=", lays.size()-1, out);   

  reset_stepsize(&sssch, &momsch);  
  trn->first_batch(); 
  show_layer_stat(); 
  
  int ite;
  for (ite = init_ite; ite < ite_num; ++ite) {
    reset_timestamp(); 
    int data_size = trn->dataNum();  /* size of this data batch */
    const int *dxs = dataseq.gen(trn, data_size); 
    double tr_loss = 0; 
    int ix; 
    for (ix = 0; ix < data_size; ix += minib) {
      int d_num = MIN(minib, data_size - ix);
      if (do_discard && ix > 0 && d_num < minib) continue; 
      up_down(trn, dxs+ix, d_num, &tr_loss);    
      if (do_check_size && ix == 0) show_size();     
      show_progress(ix+d_num, dx_inc, data_size, tr_loss); 
      if (dx_inc > 0 && (ix+d_num)%(dx_inc*10) == 0 && ix+minib < data_size) {
        show_layer_stat(); 
      }
    }
    lays_end_of_epoch();    
    show_layer_stat(); 
    tr_loss /= (double)data_size; 
   
    /*-----  test  -----*/
    test_save(trn, tst, tst2, -1, ite, tr_loss); 
    
    if (timer != NULL) {
      timer->show(out); 
      timer->set_zero(); 
    }

    change_stepsize_if_required(&sssch, &momsch, ite);    
    
    trn->next_batch(); 
  }

  if (ite_num == 0) save(0); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::sup_training(AzpData_ *trn, 
                            const AzpData_ *tst, 
                            const AzpData_ *tst2)
{
  if (s_focus_layers.length() <= 0 || !s_focus_layers.contains(focus_dlm_outer)) {
    /*---  training in a single session  ---*/
    set_focus_layers(s_focus_layers);   
    sup_training_loop(trn, tst, tst2);
  }
  else {
    /*---  training in multiple sessions  ---*/
    /*---  e.g., s_focus_layers="0-2_0-1-2" means                     ---*/
    /*---  Update layer#0 and #2, and then update layer#0,#1, and #2  ---*/
    AzBytArr s_save_fn_org(&s_save_fn);
    AzStrPool sp_focus(32,32); 
    AzTools::getStrings(s_focus_layers.c_str(), focus_dlm_outer, &sp_focus); 
    for (int ix = 0; ix < sp_focus.size(); ++ix) {
      const char *focus_str = sp_focus.c_str(ix); 
      AzPrint::writeln(out, " "); 
      AzPrint::writeln(out, "---------------------------------------------"); 
      AzBytArr s; s << "Session " << ix+1 << "/" << sp_focus.size(); 
      AzTimeLog::print(s.c_str(), out); 
      s_save_fn.reset(&s_save_fn_org); s_save_fn << ".lbl" << focus_str;  
      AzBytArr s_focus(focus_str); 
      set_focus_layers(s_focus); 
      sup_training_loop(trn, tst, tst2);     
    }
    s_save_fn.reset(&s_save_fn_org); 
  }
  /*---  fine tuning if requested  ---*/
  if (fine_ite_num > 0) {
    fine_tuning(trn, tst, tst2); 
  }
} 

/*------------------------------------------------------------*/ 
void AzpCNet3::fine_tuning(AzpData_ *trn, 
                           const AzpData_ *tst, 
                           const AzpData_ *tst2)
{
  if (fine_ite_num <= 0) return; 

  AzPrint::writeln(out, " "); 
  AzPrint::writeln(out, "---------------------------------------------");   
  AzTimeLog::print("Fine tuning ... ", out); 
  int org_ite_num = ite_num; 
  ite_num = fine_ite_num; 
  AzStepszSch org_sssch = sssch; 
  AzMomentSch org_momsch = momsch; 
  sssch = fine_sssch; 
  momsch = fine_momsch; 
  
  s_focus_layers.reset();  /* update all the layers */
  set_focus_layers(s_focus_layers); 
  sup_training_loop(trn, tst, tst2);   
  
  ite_num = org_ite_num;   
  sssch = org_sssch; 
  momsch = org_momsch; 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::reset_stepsize(AzStepszSch *sssch, AzMomentSch *momsch)
{
  sssch->init(); 
  AzTimeLog::print("Resetting step-sizes to s0 (initial value) ...", out); 
  double coeff = 1; 
  const AzOut *myout = (dmp_out.isNull()) ? NULL : &out;   
  lays_multiply_to_stepsize(coeff, myout); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::change_stepsize_if_required(AzStepszSch *sssch, AzMomentSch *momsch, int ite)
{
  double coeff = sssch->get_stepsize_coeff(ite+1); 
  if (coeff > 0) {
    AzBytArr s("Setting step-sizes to s0 times "); s.cn(coeff); 
    AzTimeLog::print(s, out); 
    const AzOut *myout = (dmp_out.isNull()) ? NULL : &out; 
    lays_multiply_to_stepsize(coeff, myout); 
  }
  double newmom = momsch->get_momentum(ite+1); 
  if (newmom > 0) {
    AzBytArr s("Setting momentum to "); s.cn(newmom); 
    AzTimeLog::print(s, out); 
    const AzOut *myout = (dmp_out.isNull()) ? NULL : &out; 
    lays_set_momentum(newmom, myout); 
  }
}
  
/*------------------------------------------------------------*/ 
void AzpCNet3::test_save(const AzpData_ *trn, 
                       const AzpData_ *tst, 
                       const AzpData_ *tst2, 
                       double tr_y_avg, 
                       int ite,
                       double tr_loss) /* no reg */                    
{
  double iniloss = 0, regloss = -1; 
  double perf=-1, perf2=-1, exact_tr_loss=-1, test_loss = -1, test_loss2 = -1; 
  int myite = ite - init_ite; 
  AzBytArr s_pf; 
  if (myite == 0 || ite == ite_num-1 || test_interval <= 0 || (myite+1)%test_interval == 0) {
    if (tst != NULL) { /* test on first data if any */
      perf = test(tst, &test_loss, &s_pf); 
    }
    if (tst2 != NULL) { /* test on second data if any */
      perf2 = test(tst2, &test_loss2); 
    }
    if (do_exact_trnloss) { /* show exact training loss */
      regloss = lays_regloss(&iniloss);    /* regularization term */
      exact_tr_loss = get_loss_noreg(trn); /* reg.term is not included */
    }  
  }
  
  show_perf(s_pf, ite, regloss, iniloss, tr_loss, exact_tr_loss, test_loss, perf, test_loss2, perf2);   

  if (save_interval > 0 && (myite+1)%save_interval == 0 || ite == ite_num - 1) save(ite+1); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::save(int iteno)
{
  if (s_save_fn.length() > 0) {
    AzBytArr s_fn(s_save_fn); s_fn << ".ite" << iteno; 
    AzTimeLog::print("Saving the model to ", s_fn.c_str(), out); 
    write(s_fn.c_str()); 
  }
  if (s_save_lay0_fn.length() > 0) {
    AzBytArr s_fn(s_save_lay0_fn); s_fn << ".ite" << iteno << azp_lay0_ext; 
    AzTimeLog::print("Saving Layer-0 to ", s_fn.c_str(), out); 
    write_lay0(s_fn.c_str()); 
  }
}

/*------------------------------------------------------------*/ 
void AzpCNet3::write_lay0(const char *fn) {
  AzX::throw_if((!do_ds_dic || ds_dic.size() <= 0), "AzpCNet3::write_lay0", 
                "Layer-0 cannot be saved without word-mapping info.  If this model was trained with an old version, re-train with the latest version so that word-mapping info is saved with the model."); 
  write_lay0(fn, *ds_dic(0), *lays(0));   
}

/*------------------------------------------------------------*/ 
/* static */
void AzpCNet3::write_lay0(const char *fn, 
                          AzDicc &lay0_dic, 
                          AzpLayer &lay0) {
  AzFile file(fn); file.open("wb"); 
  lay0_dic.write(&file); 
  lay0.write(&file); 
  file.close(true); 
}
/*------------------------------------------------------------*/ 
/* static */
void AzpCNet3::read_lay0(const char *fn, 
                         AzDicc &lay0_dic, 
                         AzpLayer &lay0) {
  AzFile file(fn); file.open("rb"); 
  lay0_dic.read(&file); 
  lay0.read(&file); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::write_word_mapping_in_lay0(const char *lay0_fn, const char *dic_fn) {
  AzDicc lay0_dic; 
  AzpLayer lay0; lay0.reset(cs); 
  read_lay0(lay0_fn, lay0_dic, lay0); 
  lay0_dic.writeText(dic_fn); 
}
 
/*------------------------------------------------------------*/ 
void AzpCNet3::write_word_mapping(const char *fn, int dsno) const {
  if (!do_ds_dic) {
    AzPrint::writeln(out, "No word-mapping info is saved with this model."); 
    return; 
  }
  AzX::throw_if((dsno < 0 || dsno >= ds_dic.size()), "AzpCNet3::write_word_mapping", "data# is out of range"); 
  ds_dic[dsno]->writeText(fn); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::show_perf(const AzBytArr &s_pf, int ite, 
                       double regloss, double iniloss, 
                       double loss, double exact_loss,  /* reg.term is not included */
                       double test_loss, 
                       double perf, 
                       double test_loss2, double perf2, 
                       AzBytArr *s_out) 
const 
{
  int width = (do_less_verbose) ? 6 : 15; 
  /*---  loss and performance  ---*/
  AzBytArr s(" ite,"); s << ite+1 << ","; 
  s << loss; /* loss estimate: reg.term is not included */
  if (exact_loss >= 0) {
    s << ", trn-loss,"; s.cn(exact_loss,width); /* reg.term is not included */
    if (regloss >= 0) {
      s << ","; s.cn(regloss,width); 
    }
  }
  if (test_loss >= 0) {
    s.c(", test-loss,"); s.cn(test_loss,width); 
  }
  if (perf >= 0)       s << ", perf:" << s_pf.c_str() << "," << perf; 
  if (perf2 >= 0)      s << "," << perf2 << "," << test_loss2; 
  if (do_show_iniloss) s << ",i," << iniloss;  /* for reg_L2init */
  
  /*---  ---*/
  if (s_out != NULL) s_out->c(&s); 
  else {
    AzTimeLog::print(s, out); 
    if (perf >= 0 || exact_loss > 0) AzPrint::writeln(eval_out, s); 
  }
}

/*------------------------------------------------------------*/ 
void AzpCNet3::show_layer_stat(const char *str) const
{
  if (do_less_verbose) return; 
  AzBytArr s; 
  if (str != NULL) s.c(str); 
  lays_show_stat(s); 
  AzPrint::writeln(out, s); 
  AzPrint::writeln(out, ""); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::up_down(const AzpData_ *trn, 
                      const int *dxs, 
                      int d_num, 
                      double *out_loss, 
                      bool do_update)
{
  const char *eyec = "AzpCNet3::up_down"; 
  reset_timestamp(); 

  AzPmatSpa m_spa_y; const AzPmatSpa *mptr_spa_y = NULL; 
  AzPmat m_den_y; const AzPmat *mptr_den_y = NULL; 
  if (do_y_try) mptr_den_y = for_sparse_y_init2(trn, dxs, d_num, &m_den_y);
  else          mptr_spa_y = for_sparse_y_init(trn, dxs, d_num, &m_spa_y);
  timestamp(t_DataY); 

  /*---  upward  ---*/  
  AzpData_X data; 
  trn->gen_data(dxs, d_num, &data); timestamp(t_DataX); 
  bool is_test = false;   
  const AzPmat *m_out = up(trn->is_vg_x(), data, is_test); 
  timestamp(t_Upward); 

  /*---  downward  ---*/
  AzPmat m_loss_deriv; 
  if (do_y_try) nco.loss->get_loss_deriv(m_out, trn, dxs, d_num, &m_loss_deriv, out_loss, NULL, mptr_den_y);  
  else          nco.loss->get_loss_deriv(m_out, trn, dxs, d_num, &m_loss_deriv, out_loss, mptr_spa_y); 
  for (int lx = lays.size() - 1; lx >= 0; --lx) {
    lays(lx)->downward(&m_loss_deriv);   
    if (do_update && (arr_doUpdate == NULL || arr_doUpdate[lx] == 1)) {
      lays(lx)->updateDelta(d_num, do_update_lm, do_update_act);         
    }    
    if (arr_doUpdate != NULL && lx <= min_focus) break; 
  }  
  timestamp(t_Downward); 
  lays_flushDelta();   timestamp(t_Flush);   
  for_sparse_y_term(); timestamp(t_DataY); 
}

/*------------------------------------------------------------*/ 
const AzPmat *AzpCNet3::up(
                  bool is_vg_x, 
                  const AzpData_X &data, 
                  bool is_test)
{
  /*---  upward  ---*/   
  const AzPmat *m_inout = NULL;  
  int lx = 0; 
  if (is_vg_x) {
    const AzPmatVar *mv_inout = NULL; 
    mv_inout = lays(0)->upward_var(is_test, &data); 
    for (lx = 1; lx < lays.size(); ++lx) {
      if (!lays[lx]->is_input_var()) break; 
      mv_inout = lays(lx)->upward_var(is_test, mv_inout); 
    }
    m_inout = mv_inout->data(); 
  }
  else {
    m_inout = lays(0)->upward(is_test, &data);    
    lx = 1; 
  }
  for ( ; lx < lays.size(); ++lx) {
    if (fdp != NULL) fdp->fdump(lx, m_inout); 
    m_inout = lays(lx)->upward(is_test, m_inout); 
  }
  return m_inout; 
}

/*------------------------------------------------------------*/ 
const AzPmatSpa *AzpCNet3::for_sparse_y_init(
                           const AzpData_ *trn, 
                           const int *dxs, int d_num, 
                           AzPmatSpa *m_y) /* output */
{
  const char *eyec = "AzpCNet3::for_sparse_y_init"; 
  if (!do_partial_y) return NULL; 
  AzX::no_support(!trn->is_sparse_y(), eyec, "zero_target_ratio/partial_y with non-sparse targets");  

  AzSmat ms_y;   
  trn->gen_targets(dxs, d_num, &ms_y);  
  AzIntArr ia_p2w, ia_w2p; 
  pick_classes(&ms_y, zerotarget_ratio, &ia_p2w, &ia_w2p, m_y); 
  if (ia_p2w.size() > 0) {
    topLayer_u()->linmod_u()->reset_partial(&ia_p2w, &ia_w2p); 
    return m_y;  
  } 
  return NULL; 
}

/*------------------------------------------------------------*/ 
const AzPmat *AzpCNet3::for_sparse_y_init2(
                           const AzpData_ *trn, 
                           const int *dxs, int d_num, 
                           AzPmat *m_y) /* output */
{
  const char *eyec = "AzpCNet3::for_sparse_y_init2"; 
  if (!do_partial_y) return NULL; 
  AzX::no_support(!trn->is_sparse_y(), eyec, "zero_target_ratio/partial_y with non-sparse targets");  
  
  AzSmat ms_y;   
  trn->gen_targets(dxs, d_num, &ms_y);  
  if (ms_y.colNum() <= 1 || ms_y.rowNum() <= 0) return NULL; 
                
  double nega_val = -1e-10; 
  int cnum = ms_y.colNum(); 
  AzDvect v(ms_y.rowNum()); 
  for (int col = 0; col < cnum; ++col) v.add(ms_y.col(col)); 
  AzIntArr ia_p2w; 
  v.nonZeroRowNo(&ia_p2w); 
  if (ia_p2w.size() <= 0) { /* If all zero, keep one row. */
    ia_p2w.put(0); 
  }
  
  AzIntArr ia_w2p(ms_y.rowNum(), -1); 
  AzDvect vv(ia_p2w.size()); 
  for (int px = 0; px < ia_p2w.size(); ++px) {
    int wx = ia_p2w[px]; 
    ia_w2p(wx, px); 
    vv.set(px, v.get(wx)); 
  }
  
  ms_y.change_rowno(ia_p2w.size(), &ia_w2p);  
  bool do_gen_rowindex = false; 
  AzPmatSpa m_y_spa; m_y_spa.set(&ms_y, do_gen_rowindex); 
  if (zerotarget_ratio <= 0) {
    AzPs::set(m_y, &m_y_spa); 
  }
  else {
    vv.multiply((double)zerotarget_ratio/(double)cnum);   
    m_y->reform(ms_y.rowNum(), ms_y.colNum());  
    rng.uniform_01(m_y);   
    m_y->mark_le_rowth(&vv, (AzFloat)nega_val);   
    AzPs::add(m_y, &m_y_spa); 
  }

  topLayer_u()->linmod_u()->reset_partial(&ia_p2w, &ia_w2p); 
  return m_y; 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::for_sparse_y_term()
{
  if (do_partial_y) topLayer_u()->linmod_u()->reset_partial();   
}

/*------------------------------------------------------------*/ 
double AzpCNet3::test(const AzpData_ *data, 
                     double *out_loss,
                     AzBytArr *s_perf_str)
{
  const char *eyec = "AzpCNet3::test"; 
  AzPmat m_p; 
  apply_all2(data, &m_p); 
  AzBytArr s; 
  return nco.loss->test_eval(data, &m_p, out_loss, out, s_perf_str); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::apply(const AzpData_ *tst, 
                      const int dx_begin, int d_num, 
                      AzPmat *m_top_out)
{
  reset_timestamp(); 
  
  AzpData_X data; 
  tst->gen_data(dx_begin, d_num, &data); 
  bool is_test = true;  
  const AzPmat *m_out = up(tst->is_vg_x(), data, is_test); 
  m_top_out->set(m_out); 
  
  timestamp(t_Test); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::apply_all2(
                        const AzpData_ *data, 
                        AzPmat *m_p)
{
  int data_num = data->dataNum(); 
  m_p->reform(class_num, data_num); 
  for (int dx = 0; dx < data_num; dx += tst_minib) {
    int d_num = MIN(tst_minib, data_num - dx); 
    AzPmat m_out; 
    apply(data, dx, d_num, &m_out); 
    m_p->set(dx, d_num, &m_out); 
  }
}

/*------------------------------------------------------------*/ 
double AzpCNet3::get_loss_noreg(const AzpData_ *data)
{
  int data_num = data->dataNum(); 
  double loss = 0; 
  for (int dx = 0; dx < data_num; dx += tst_minib) {
    int d_num = MIN(tst_minib, data_num - dx); 
  
    AzPmat m_out; 
    apply(data, dx, d_num, &m_out);  
    loss += nco.loss->get_loss(data, dx, d_num, &m_out); 
  }
  if (data_num > 0) {
    loss /= (double)data_num; 
  }
  return loss; 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::show_progress(int num, int inc, int data_size, 
                            double loss_sum) const
{    
  if (do_less_verbose && (inc <= 0 || inc > data_size)) return; 
  int width = (do_less_verbose) ? 6 : 20; 
           
  if (inc > 0 && num%inc == 0 || num == data_size || 
      do_discard && data_size - num < minib) {       
    double loss_avg = loss_sum / (double)num; 
    AzBytArr s("... "); s << num << ": "; s.cn(loss_avg,width); /* loss estimate */
    AzTimeLog::print(s, out); 
    AzPrint(out, " "); 
  }
}

/*------------------------------------------------------------*/ 
/*------------------------------------------------------------*/ 
#define kw_save_interval "save_interval="
#define kw_save_fn "save_fn="
#define kw_test_interval "test_interval="
#define kw_hid_num "layers="

#define kw_ite_num     "num_iterations="
#define kw_minib       "mini_batch_size="
#define kw_tst_minib   "test_mini_batch_size="
#define kw_rseed       "random_seed="
#define kw_init_ite    "initial_iteration="
  
#define kw_dx_inc "inc="
#define kw_do_exact_trnloss "ExactTrainingLoss"

#define kw_do_timer "Timer"

#define kw_focus_layers "focus_layers="
#define kw_dont_update_lm "DontUpdateWeights"
#define kw_do_discard "Discard"
#define kw_do_dw "UseDataWeights"

#define kw_do_topthru "TopThru"
#define kw_zerotarget_ratio "zero_Y_ratio="
#define kw_do_partial_y "PartialY"
#define kw_do_y_try "Ytry"

#define kw_fine_pfx "fine_"
#define kw_fine_ite_num "fine_num_iterations="
#define kw_do_less_verbose "LessVerbose"

/*------------------------------------------------------------*/ 
void AzpCNet3::resetParam(AzParam &azp, bool is_warmstart)
{
  const char *eyec = "AzpCNet3::resetParam"; 

  if (!is_warmstart) { /* cold-start */
    azp.swOn(&do_topthru, kw_do_topthru);   
    azp.vInt(kw_hid_num, &hid_num);   
  }
  else { /* warm-start */
    azp.vInt(kw_init_ite, &init_ite);   
  }
  /*---  which layer to train  ---*/
  azp.vStr(kw_focus_layers, &s_focus_layers); 

  /*---  ---*/
  azp.vStr(kw_save_fn, &s_save_fn); 
  azp.vStr(kw_save_lay0_fn, &s_save_lay0_fn);   
  azp.vInt(kw_save_interval, &save_interval); 
  azp.vInt(kw_test_interval, &test_interval); 

  azp.vInt(kw_ite_num, &ite_num);  
  azp.vInt(kw_minib, &minib); 
  AzXi::throw_if_nonpositive(minib, eyec, kw_minib); 

  azp.vInt(kw_rseed, &rseed); 
  azp.vInt(kw_dx_inc, &dx_inc); 
  azp.swOn(&do_exact_trnloss, kw_do_exact_trnloss); 
  
  bool dont_update_lm = false; 
  azp.swOn(&dont_update_lm, kw_dont_update_lm, false); 
  do_update_lm = do_update_act = !dont_update_lm; 

  azp.swOn(&do_discard, kw_do_discard); 
#if 0   
  azp.swOn(&do_partial_y, kw_do_partial_y);   
  if (do_partial_y) {
    azp.vInt(kw_zerotarget_ratio, &zerotarget_ratio);     
    azp.swOn(&do_y_try, kw_do_y_try); 
  }
#else
  /*---  simplifying the interface: 8/24/2015 */
  azp.vInt(kw_zerotarget_ratio, &zerotarget_ratio);
  do_partial_y = (zerotarget_ratio >= 0); 
#endif

  azp.swOn(&do_less_verbose, kw_do_less_verbose); 
  
  _resetParam(azp);  /* read parameters used both train_test and test */

  /*---  loss  ---*/
  nco.loss->resetParam(out, azp, is_warmstart); 
  nco.loss->check_losstype(out, class_num); 
  
  /*---  stepsize scheduling  ---*/
  sssch.resetParam(azp); 
  
  /*---  momentum scheduling  ---*/
  momsch.resetParam(azp); 

  /*---  data sequence generation  ---*/  
  dataseq.resetParam(azp); 
  
  /*---  fine tuning (typically) after layer-by-layer training  ---*/
  if (s_focus_layers.length() > 0) {
    _resetParam_fine(azp, is_warmstart); 
  }
}

/*------------------------------------------------------------*/ 
#define kw_nn_fn "nn_fn="
void AzpCNet3::get_nn_fn(AzParam &azp, const AzOut &out, int lx, AzBytArr &s_nn_fn) const
{
  AzBytArr s_pfx; s_pfx << lx; 
  azp.reset_prefix(s_pfx.c_str()); 
  azp.vStr(kw_nn_fn, &s_nn_fn); 
  azp.reset_prefix();   

  if (s_nn_fn.length() > 0) {
    AzPrint o(out); 
    o.reset_prefix(s_pfx.c_str()); 
    o.printV(kw_nn_fn, s_nn_fn); 
    o.printEnd(); 
  }  
}

/*------------------------------------------------------------*/ 
void AzpCNet3::_resetParam_fine(AzParam &azp, bool is_warmstart)
{
  /*---  fine tuning (typically) after layer-by-layer training  ---*/
  fine_ite_num = -1; 
  azp.vInt(kw_fine_ite_num, &fine_ite_num); 
  if (fine_ite_num > 0) {
    fine_sssch.resetParam(azp, kw_fine_pfx); 
    fine_momsch.resetParam(azp, kw_fine_pfx); 
  }  
}

/*------------------------------------------------------------*/ 
void AzpCNet3::printParam(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.ppBegin("AzpCNet3", ""); 
  o.printV(kw_hid_num, hid_num); 
  o.printV_if_not_empty(kw_focus_layers, s_focus_layers); 
  
  o.printV(kw_save_fn, s_save_fn); 
  o.printV_if_not_empty(kw_save_lay0_fn, s_save_lay0_fn); 
  o.printV(kw_save_interval, save_interval); 

  o.printV(kw_init_ite, init_ite); 
  o.printV(kw_test_interval, test_interval); 

  o.printV(kw_ite_num, ite_num); 
  o.printV(kw_rseed, rseed); 
  o.printV(kw_dx_inc, dx_inc); 

  o.printSw(kw_do_exact_trnloss, do_exact_trnloss); 

  o.printV(kw_minib, minib); 

  o.printSw(kw_dont_update_lm, !do_update_lm); 
  o.printSw(kw_do_discard, do_discard); 
  o.printSw(kw_do_less_verbose, do_less_verbose); 
  o.printSw(kw_do_partial_y, do_partial_y);
  o.printV(kw_zerotarget_ratio, zerotarget_ratio);   
  o.printSw(kw_do_y_try, do_y_try); 
  
  /*---  loss  ---*/
  nco.loss->printParam(out); 
  
  /*---  step-size scheduling  ---*/
  sssch.printParam(out); 
  
  /*---  momentum scheduling  ---*/
  momsch.printParam(out); 
  
  /*---  data sequence generation  ---*/
  dataseq.printParam(out); 
  
  _printParam(o); /* show parameters used both for train_test and test */
  
  /*---  fine tuning  ---*/
  _printParam_fine(o); 

  o.ppEnd(); 
}
 
/*------------------------------------------------------------*/ 
void AzpCNet3::printHelp(AzHelp &h) const {
  h.item_required(kw_hid_num, "Number of hidden layers (i.e., excluding the top layer)"); 
  h.item_required(kw_ite_num, "Number of epochs."); 
  h.item(kw_focus_layers, "Layers whose weights should be updated.", "All"); 
  h.item(kw_save_fn, "Path to the file the model is saved to for warm-start later."); 
  h.item(kw_save_lay0_fn, "Path to the file Layer-0 is saved to, for use in semi-supervised learning later."); 
  h.item(kw_save_interval, "n: the model is saved to a file after every n epochs."); 
  h.item(kw_test_interval, "m: tested after every m epochs."); 
  h.item(kw_rseed, "Random seed"); 
  h.item(kw_dx_inc, "t: Show progress after going through t data points.  Useful when data is large or when training is time-consuming."); 
  h.item(kw_do_exact_trnloss, "Compute and display training loss.  If this switch is off, estimated loss is shown instead."); 
  h.item(kw_minib, "Mini-batch size.", 100); 
  h.item(kw_do_less_verbose, "Don't display too much information.");   
  /* kw_init_ite kw_dont_update_lm kw_do_discard kw_do_partial_y kw_zerotarget_ratio */
  nco.loss->printHelp(h); 
  sssch.printHelp(h);
  /* momsch.printHelp(h); */
  /* dataseq.printHelp(h); */
  
  _printHelp(h);
  /* _printHelp_fine(h); */

  int ind = 3; 
  h.writeln("------------------------------------------------------------------------", ind); 
  h.writeln("The following parameters can be specified for a specific layer by attaching the layer id in front of the keywords, e.g., \"0nodes=200\".  Without a layer id, the parameter is applied to all the layers.  If there are both, the parameter with a layer id (e.g., \"0nodes=200\") supersedes the one without the id (e.g., \"nodes=100\").", ind); 
  h.writeln("------------------------------------------------------------------------", ind); 
  AzpLayer lay; lay.reset(cs); 
  lay.printHelp(h);   
}

/*------------------------------------------------------------*/ 
void AzpCNet3::_printParam_fine(AzPrint &o) const
{
  /*---  fine tuning  ---*/
  o.printV(kw_fine_ite_num, fine_ite_num); 
  if (fine_ite_num > 0) {
    fine_sssch.printParam(out, kw_fine_pfx); 
    fine_momsch.printParam(out, kw_fine_pfx); 
  } 
}

/*------------------------------------------------------------*/ 
/* parameters used for both train_test and test */
void AzpCNet3::_resetParam(AzParam &azp)
{
  const char *eyec = "AzpCNet3::_resetParam"; 
  azp.vInt(kw_tst_minib, &tst_minib); 
  AzXi::throw_if_nonpositive(tst_minib, eyec, kw_tst_minib); 

  azp.swOn(&do_timer, kw_do_timer); 
}

/*------------------------------------------------------------*/ 
/* parameters used for both train_test and test */
void AzpCNet3::_printParam(AzPrint &o) const {
  o.printV(kw_tst_minib, tst_minib);  
  o.printSw(kw_do_timer, do_timer);  
}
void AzpCNet3::_printHelp(AzHelp &h) const {
  h.item(kw_tst_minib, "Mini batch-size for parallel processing for testing.", 100); 
  /* kw_do_timer */
}
 
/*------------------------------------------------------------*/ 
void AzpCNet3::resetParam_test(AzParam &azp) {
  const char *eyec = "AzpCNet3::resetParam_test";
  _resetParam(azp); 
}  

/*------------------------------------------------------------*/ 
void AzpCNet3::printParam_test(const AzOut &out) const {
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.ppBegin("AzpCNet3", "test");
  _printParam(o); 
  o.ppEnd(); 
}
void AzpCNet3::printHelp_test(AzHelp &h) const {
  _printHelp(h); 
}
 
/*------------------------------------------------------------*/ 
/* output: ia_doUpdate, arr_doUpdate */
void AzpCNet3::set_focus_layers(const AzBytArr &s_focus) /* which layers to train */
{
  ia_doUpdate.reset(); 
  arr_doUpdate = NULL; 

  if (s_focus.length() <= 0) return; 
  if (s_focus.compare("all") == 0 || s_focus.compare("All") == 0) {
    return; 
  }
  
  AzStrPool sp; 
  AzTools::getStrings(s_focus.c_str(), focus_dlm_inner, &sp); 
  ia_doUpdate.reset(hid_num+1, 0); 
  int ix; 
  for (ix = 0; ix < sp.size(); ++ix) {
    int lx = atol(sp.c_str(ix)); 
    AzX::throw_if((lx < 0 || lx >= ia_doUpdate.size()), AzInputError, "AzpCNet3::set_focus_layers", kw_focus_layers, " out of range"); 
    ia_doUpdate.update(lx, 1); 
  }
  arr_doUpdate = ia_doUpdate.point(); 
  
  min_focus = -1; 
  int count = 0; 
  for (ix = 0; ix < ia_doUpdate.size(); ++ix) {
    if (arr_doUpdate[ix] == 1) {
      if (min_focus < 0) min_focus = ix; 
      ++count; 
    }
  }
  AzBytArr s(&s_focus); s << " -- updating " << count << " layer(s) (min_focus=" << min_focus << ") ..."; 
  AzPrint::writeln(out, s); 
}

/*------------------------------------------------------------*/ 
double AzpCNet3::predict(AzParam &azp, 
                      const AzpData_ *tst,
                      /*---  output  ---*/
                      double *out_loss, 
                      AzDmat *m_pred, 
                      AzDmatc *mc_pred, 
                      AzPmat *m_p) /* for cotr */
/*loc const loc*/
{
  const char *eyec = "AzpCNet3::predict"; 

  AzX::throw_if((out_loss != NULL && (m_pred != NULL || mc_pred != NULL || m_p != NULL)), 
                AzInputError, eyec, "Only one of them can be returned: loss or predictions"); 
  AzX::throw_if((lays.size() <= 0), eyec, "No neural net to test"); 
  
  /*---  check the data signature  ---*/
  AzBytArr s_sign; 
  tst->signature(&s_sign); 
  if (!tst->isSignatureCompatible(&s_data_sign, &s_sign)) {
    AzPrint::writeln(out, "model: ", s_data_sign.c_str()); 
    AzPrint::writeln(out, "data: ", s_sign.c_str()); 
    AzX::throw_if(true, AzInputError, eyec, "data signature mismatch btw the net and test data"); 
  }

  /*---  read parameters  ---*/
  resetParam_test(azp); 
  printParam_test(out); 
 
  /*---  set timer  ---*/
  timer = NULL; 
  if (do_timer) {
    timer = &my_timer; 
    timer->reset(hid_num); 
  }

  AzX::throw_if((tst->base() != NULL), AzInputError, eyec, "This version does not support base"); 
  
  /*---  initialize layers  ---*/
  bool for_testonly = true; 
  warmstart(tst, azp, for_testonly); 
  
  azp.check(out); 

  bool is_warmstart = true; 
  check_word_mapping(is_warmstart, NULL, tst, NULL); 
  
  if (out_loss != NULL) {
    return test(tst, out_loss);  
  }
  else {
    int data_num = tst->dataNum(); 
    int inc = data_num/50, milestone = inc; 
    if (m_pred != NULL) m_pred->reform(class_num, data_num); 
    if (mc_pred != NULL)    mc_pred->reform(class_num, data_num); 
    if (m_p != NULL) m_p->reform(class_num, data_num); 
    int dx; 
    for (dx = 0; dx < data_num; dx += tst_minib) {
      AzTools::check_milestone(milestone, dx, inc); 
      int d_num = MIN(tst_minib, data_num - dx); 
      AzPmat m_out; 
      apply(tst, dx, d_num, &m_out);   
      if (m_pred != NULL) { 
        AzDmat m; m_out.get(&m); 
        m_pred->set(dx, dx+d_num, &m); 
      }
      if (mc_pred != NULL) {
        m_out.copy_to(mc_pred, dx); 
      }
      if (m_p != NULL) m_p->set(dx, d_num, &m_out); 
    }
    AzTools::finish_milestone(milestone);  
    return -1; 
  }
}

/*------------------------------------------------------------*/  
void AzpCNet3::reset_timestamp() const {
  if (timer != NULL) timer->reset_Total(); 
}
void AzpCNet3::timestamp(AzpTimer_CNN_type::t_type typ) const {
  if (timer != NULL) timer->stamp_Total(typ); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::check_data_signature(const AzpData_ *data, const char *msg, bool do_loose) const 
{
  AzBytArr s_sign; 
  data->signature(&s_sign);  
  if (do_loose) {
    if (!data->isSignatureCompatible(&s_data_sign, &s_sign)) {
      AzPrint::writeln(out, "model: ", s_data_sign); 
      AzPrint::writeln(out, "data: ", s_sign); 
      AzX::throw_if(true, AzInputError, "AzpCNet3::check_data_signature", "data signature loose check failed: ", msg);       
    }
  }
  else {
    if (s_data_sign.compare(&s_sign) != 0) {
      AzPrint::writeln(out, "model: ", s_data_sign); 
      AzPrint::writeln(out, "data: ", s_sign);       
      AzX::throw_if(true, AzInputError, "AzpCNet3::check_data_signature", "data signature check failed: ", msg);   
    }
  }
}

/*------------------------------------------------------------*/ 
/* Assume that ms_y is binary and 0 represents "negative"     */
/* returning empty ia_p2w means use ms_y asis                 */
void AzpCNet3::pick_classes(AzSmat *ms_y, /* inout */
                           int nega_count, 
                           /*---  output  ---*/
                           AzIntArr *ia_p2w, 
                           AzIntArr *ia_w2p,
                           AzPmatSpa *m_y) const
{
  ia_p2w->reset(); 
  ia_w2p->reset(); 
  bool do_gen_rowindex = false;
  if (ms_y->colNum() <= 1 || ms_y->rowNum() <= 0) {
    return;  /* keep target asis */
  }
  AzIntArr ia_nzrows;  
  AzDataArr<AzIFarr> aifa; 
  if (nega_count <= 0) {
    ms_y->nonZeroRowNo(&ia_nzrows); 
  }
  else {
    double nega_val = -1e-10; 
    int cnum = ms_y->colNum(); 
    aifa.reset(cnum); 

    for (int col = 0; col < cnum; ++col) {   
      ms_y->col(col)->nonZero(aifa(col));  /* appended */
      AzIntArr ia_nzr; 
      ms_y->col(col)->nonZeroRowNo(&ia_nzr); 
      ia_nzrows.concat(&ia_nzr); 
      for (int ix = 0; ix < ia_nzr.size(); ++ix) {
        int row = ia_nzr[ix]; 
        for (int jx = 0; jx < nega_count; ) {
          if (jx+1 >= cnum) break; 
          int cx = AzTools::big_rand() % cnum; 
          if (cx != col) {
            aifa(cx)->put(row, nega_val); 
            ++jx; 
          }
        }
      }
    }
    for (int col = 0; col < cnum; ++col) {
      aifa(col)->squeeze_Max();          
    }  
    ia_nzrows.unique();  
  }
  
  /*---  only keep non-zero rows  ---*/  
  ia_p2w->reset(&ia_nzrows); 
  if (ia_p2w->size() <= 0) {  /* if all zero, keep one row. */
    ia_p2w->put(0); 
  }
  ia_w2p->reset(ms_y->rowNum(), -1); 
  for (int px = 0; px < ia_p2w->size(); ++px) (*ia_w2p)((*ia_p2w)[px], px); 
  
  if (nega_count <= 0) {
    ms_y->change_rowno(ia_p2w->size(), ia_w2p); 
    m_y->set(ms_y, do_gen_rowindex); 
  }
  else {  
    m_y->set(aifa, ia_p2w->size(), ia_w2p, do_gen_rowindex);    
  }
}

/*--- for analysis ---*/
/*------------------------------------------------------------*/ 
class AzpCNet3_write_embedded_Param {
public:
  AzBytArr s_feat_fn; 
  int feat_top_num; 
  bool do_skip_pooling, do_skip_resnorm; 
    
  AzpCNet3_write_embedded_Param() : feat_top_num(-1), do_skip_pooling(true), do_skip_resnorm(false) {}
  
  /*------------------------------------------------*/
  #define kw_feat_top_num "num_top="
  #define kw_feat_fn "embed_fn="
  #define kw_dont_skip_pooling "DontSkipPooling"
  #define kw_do_skip_pooling "SkipPooling"  
  #define kw_do_skip_resnorm "SkipResnorm"
  /*------------------------------------------------*/
  void resetParam(AzParam &p) {
    const char *eyec = "AzpCNet3_write_embedded_Param::resetParam"; 
    p.vInt(kw_feat_top_num, &feat_top_num); 
    p.vStr(kw_feat_fn, &s_feat_fn); 
    p.swOff(&do_skip_pooling, kw_dont_skip_pooling); 
    p.swOn(&do_skip_resnorm, kw_do_skip_resnorm); 

    AzXi::throw_if_empty(s_feat_fn, eyec, kw_feat_fn); 
  }
  void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_feat_top_num, feat_top_num); 
    o.printV(kw_feat_fn, s_feat_fn); 
    o.printSw(kw_do_skip_pooling, do_skip_pooling); 
    o.printSw(kw_do_skip_resnorm, do_skip_resnorm);     
    o.printEnd(); 
  }
  static void printHelp(AzHelp &h) {
    h.item(kw_feat_top_num, "k: If specified, only the k largest components are retained in each vector while the rest are set to zero.  Use this only when the components are guaranteed to be non-negative."); 
    h.item(kw_feat_fn, "Path to the output file."); 
    h.item(kw_do_skip_resnorm, "Skip response normalization."); 
    /* kw_dont_skip_pooling */
  }
}; 

void AzpCNet3::printHelp_write_embedded(AzHelp &h) const {
  AzpCNet3_write_embedded_Param::printHelp(h); 
  printHelp_test(h); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::write_embedded(AzParam &azp, const AzpData_ *tst)
{
  const char *eyec = "ApzCnet3::write_embedded"; 

  /*---  check the data signature  ---*/
  bool do_loose = true; 
  check_data_signature(tst, "write_embedded,test data", do_loose);

  /*---  read parameters  ---*/
  AzpCNet3_write_embedded_Param p; 
  p.resetParam(azp); 
  resetParam_test(azp); 
  p.printParam(out); 
  printParam_test(out); 

  /*---  initialize layers  ---*/
  bool for_testonly = true; 
  warmstart(tst, azp, for_testonly); 
  azp.check(out); 

  bool is_warmstart = true; 
  check_word_mapping(is_warmstart, NULL, tst, NULL); 
  
  _write_embedded(tst, p.feat_top_num, p.s_feat_fn.c_str(), p.do_skip_pooling, p.do_skip_resnorm);  
}

/*------------------------------------------------------------*/ 
void AzpCNet3::_write_embedded(const AzpData_ *tst, 
                       int feat_top_num, const char *fn, 
                       bool do_skip_pooling, bool do_skip_resnorm)
{
  const char *eyec = "AzpCNet3::_write_embedded"; 
  AzX::throw_if(!lays[0]->is_input_var(), eyec, "variable-sized input was expected"); 
  
  int data_num = tst->dataNum(); 
  AzTimeLog::print("Processing ... #data=", data_num, out); 
  
  AzMats_file<AzSmat> mfile(fn, data_num); 
  int inc = MAX(1, data_num/50), milestone = inc; 
  for (int dx = 0; dx < data_num; dx += tst_minib) {
    AzTools::check_milestone(milestone, dx, inc); 
    int d_num = MIN(tst_minib, data_num - dx); 
    AzpData_X data; 
    tst->gen_data(dx, d_num, &data);
    bool is_test = true; 
    const AzPmatVar *mv = lays(0)->upward_var(is_test, &data, NULL, do_skip_pooling, do_skip_resnorm); 

    AzDmatc mdc; mdc.reform(mv->rowNum(), mv->colNum()); 
    mv->data()->copy_to(&mdc, 0); 
    AzIFarr ifa; ifa.prepare(mdc.rowNum()); 
    for (int ix = 0; ix < d_num; ++ix) {   
      int col0, col1; 
      mv->get_begin_end(ix, col0, col1); 
      AzSmat ms(mdc.rowNum(), col1-col0); 
      for (int col = col0; col < col1; ++col) {
        const AZ_MTX_FLOAT *vals = mdc.rawcol(col); 
        ifa.reset_norelease(); 
        for (int row = 0; row < mdc.rowNum(); ++row) {
          if (vals[row] == 0) continue; 
          AzX::no_support((vals[row] < 0 && feat_top_num > 0), eyec, "num_top with possibily negative activation"); 
          ifa.put(row, vals[row]); 
        }
        if (feat_top_num > 0 && ifa.size() > feat_top_num) {
          ifa.sort_Float(false); 
          ifa.cut(feat_top_num);           
        }
        ms.col_u(col-col0)->load(&ifa); 
      }
      mfile.write(&ms); 
    }  
  }
  mfile.done(); 
  
  AzTools::finish_milestone(milestone); 
  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------------------*/ 
void AzpCNet3::write_features(AzParam &azp, 
                      const AzpData_ *tst,
                      const char *out_fn, 
                      int lx) 
{
  const char *eyec = "AzpCNet3::write_features"; 
  AzBytArr s_sign; 
  tst->signature(&s_sign); 
  if (!tst->isSignatureCompatible(&s_data_sign, &s_sign)) {
    AzPrint::writeln(out, "model: ", s_data_sign.c_str()); 
    AzPrint::writeln(out, "data: ", s_sign.c_str()); 
    AzX::throw_if(true, AzInputError, eyec, "data signature mismatch btw the net and test data"); 
  }
  resetParam_test(azp); 
  printParam_test(out); 
  timer = NULL; 
  bool for_testonly = true; 
  warmstart(tst, azp, for_testonly); 
  azp.check(out); 
  bool is_warmstart = true; 
  check_word_mapping(is_warmstart, NULL, tst, NULL); 

  int layer_no = (lx < 0) ? top_lay_ind() : lx; 
  AzBytArr s_lay, s_caution; 
  if (layer_no == top_lay_ind()) s_lay.reset("the top layer"); 
  else {
    s_lay.reset("Layer-");  s_lay << layer_no;
    AzX::throw_if((layer_no < 0 || layer_no >= lays.size()), AzInputError, eyec, s_lay.c_str(), "does not exist.");   
    s_caution.reset("!CAUTION!: features are written only if the designated layer is a middle or the top layer with fixed-sized input.  Otherwise, size of output will be zero."); 
  }  
  
  AzpCNet3_fdump fdump(out_fn, layer_no); 
  fdp = &fdump; 
  int data_num = tst->dataNum(); 
  int inc = data_num/50, milestone = inc; 
  AzBytArr s("Writing to "); s << out_fn << ": input to " << s_lay.c_str() << " ... "; 
  AzTimeLog::print(s.c_str(), out); 
  if (s_caution.length() > 0) AzPrint::writeln(out, s_caution); 
  for (int dx = 0; dx < data_num; dx += tst_minib) {
    AzTools::check_milestone(milestone, dx, inc); 
    int d_num = MIN(tst_minib, data_num - dx); 
    AzPmat m_out; apply(tst, dx, d_num, &m_out);   
  }
  AzTools::finish_milestone(milestone); 
  fdump.term(); 
  fdp = NULL; 
  AzTimeLog::print("Done ... ", out); 
}
