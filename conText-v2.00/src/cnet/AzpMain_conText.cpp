/* * * * *
 *  AzpMain_conText.cpp
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

#include "AzpMain_conText.hpp"
#include "AzParam.hpp"
#include "AzpLm.hpp"
#include "AzHelp.hpp"
#include "AzPmatSpa.hpp"

/*---  global (declared in AzPmat)  ---*/
extern AzPdevice dev; 
extern bool __doDebug; 
extern bool __doTry; 
extern int max_threads; 
extern int max_blocks; 

/*---  global (declared in AzpCNet3)  ---*/
extern AzByte param_dlm; 

/*------------------------------------------------------------*/ 
#define kw_do_debug "Debug"
#define kw_do_try "Try"
#define kw_not_doLog "DontLog"
#define kw_doLog "Log"
#define kw_doDump "Dump"
/*------------------------------------------------------------*/ 
class AzpMain_conText_Param_ {
public: 
  bool doLog, doDump; 
  
  AzpMain_conText_Param_() : doLog(true), doDump(false) {}
  virtual void resetParam(AzParam &azp) = 0; 
  virtual void printParam(const AzOut &out) const = 0; 
  virtual void reset(AzParam &azp, const AzOut &out) {
    print_hline(out);      
    AzPrint::writeln(out, azp.c_str()); 
    print_hline(out);  
    resetParam(azp); 
    printParam(out); 
  }
  /*------------------------------------------------------------*/   
  void print_hline(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint::writeln(out, "--------------------");  
  }  
protected:   
  void _resetParam(AzParam &azp) {
    const char *eyec = "AzpMain_conText_Param::_resetParam"; 
    azp.swOn(&__doDebug, kw_do_debug);   
    azp.swOn(&__doTry, kw_do_try);   
    AzPmatSpa_flags::resetParam(azp); 
    dev.resetParam(azp);    
  }
  /*------------------------------------------------------------*/ 
  void _printParam(AzPrint &o) const {
    o.printSw(kw_do_debug, __doDebug); 
    o.printSw(kw_do_try, __doTry); 
    AzPmatSpa_flags::printParam(o); 
    dev.printParam(o); 
  } 
  /*------------------------------------------------------------*/ 
  static void _printHelp(AzHelp &h) {
    dev.printHelp(h);  
    AzPmatSpa_flags::printHelp(h);      
  } 
  
  /*------------------------------------------------*/
  void setupLogDmp(AzParam &azp) {
    azp.swOff(&doLog, kw_not_doLog); 
    azp.swOn(&doDump, kw_doDump); 
  
    log_out.reset(NULL); 
    dmp_out.reset(NULL); 
    if (doLog) log_out.setStdout(); 
    if (doDump) dmp_out.setStderr(); 
  }
  void printLogDmp(AzPrint &o) const {
    o.printSw(kw_doLog, doLog); 
    o.printSw(kw_doDump, doDump); 
  }
}; 

/*------------------------------------------------------------*/ 
#define kw_eval_fn "evaluation_fn="
#define kw_fn_for_warmstart "fn_for_warmstart="
#define kw_do_no_test "NoTest"
#define kw_cnet_ext "extension="  
/*------------------------------------------------------------*/ 
class AzpMain_conText_cnn_Param : public virtual AzpMain_conText_Param_ {
public:
  AzpDataSetDflt ds; 
  AzBytArr s_fn_for_warmstart, s_eval_fn, s_cnet_ext; 
  bool do_no_test; 
  AzpMain_conText_cnn_Param(AzParam &p, const AzOut &out) : do_no_test(false) {
    reset(p, out); 
  }
  AzpMain_conText_cnn_Param() : do_no_test(false) {} /* use this for displaing help */
  void resetParam(AzParam &p) {
    _resetParam(p); 
    p.swOn(&do_no_test, kw_do_no_test, false); 
    bool do_train = true, is_there_y = true; 
    ds.resetParam(p, do_train, !do_no_test, is_there_y);
    p.vStr(kw_fn_for_warmstart, &s_fn_for_warmstart); 
    p.vStr(kw_eval_fn, &s_eval_fn); 
    p.vStr(kw_cnet_ext, &s_cnet_ext);  
    setupLogDmp(p);   
  }
  void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    o.ppBegin("AzpMain_conText::cnn", "\"cnn\""); 
    ds.printParam(out); 
    o.printV_if_not_empty(kw_cnet_ext, s_cnet_ext);   
    o.printV_if_not_empty(kw_fn_for_warmstart, s_fn_for_warmstart); 
    o.printV_if_not_empty(kw_eval_fn, s_eval_fn); 
    o.printSw(kw_do_no_test, do_no_test); 
    printLogDmp(o); 
    _printParam(o);  
    o.printEnd(); 
  }  
  void printHelp(AzHelp &h) const {
    bool do_train = true, do_test = true, is_there_y = true; 
    ds.printHelp(h, do_train, do_test, is_there_y);
    /* kw_cnet_ext kw_fn_for_warmstart */
    h.item(kw_eval_fn, "Write performance results to this file in the csv format."); 
    h.item(kw_do_no_test, "Do no testing.  Training only."); 
    _printHelp(h); 
  } 
}; 

/*------------------------------------------------*/
bool AzpMain_conText::printHelp_cnn_if_needed(const AzOut &out, AzParam &azp) {
  if (!azp.needs_help()) return false; 
  AzHelp h(out); h.writeln("cnn: training and optional testing."); h.writeln(""); 
  AzpMain_conText_cnn_Param p; p.printHelp(h); 
  AzpCNet3 cnet(compo_set(azp));
  AzpCNet3_multi cnet_multi(compo_set(azp)); 
  AzpCNet3_multi_side cnet_multi_side(compo_set(azp)); 
  bool dont_throw = true; 
  AzBytArr s_cnet_ext; 
  azp.vStr(kw_cnet_ext, &s_cnet_ext); 
  AzpCNet3 *cnet_ptr = choose_cnet(s_cnet_ext, &cnet, &cnet_multi, &cnet_multi_side, dont_throw); 
  cnet_ptr->printHelp_training(h); 
  h.end(); 
  return true; 
}

/*------------------------------------------------*/
void AzpMain_conText::cnn(int argc, const char *argv[]) 
{
  const char *eyec = "AzpMain_conText::cnn"; 
  AzParam azp(param_dlm, argc, argv); 
  if (printHelp_cnn_if_needed(log_out, azp)) return; 
  AzpMain_conText_cnn_Param p(azp, log_out); 

  /*---  read data  ---*/
  p.ds.reset_data(log_out); 
  
  AzBytArr s; s << "#train=" << p.ds.trn_data()->dataNum(); 
  if (p.ds.tst_data() != NULL) {
    s << ", #test=" << p.ds.tst_data()->dataNum(); 
  }
  AzTimeLog::print("Start ... ", s.c_str(), log_out); 
  p.print_hline(log_out); 

  /*---  set up evaluation file if specified  ---*/
  AzOut eval_out; 
  AzOfs ofs; 
  AzOut *eval_out_ptr = reset_eval(p.s_eval_fn, ofs, eval_out); 

  /*---  set up components  ---*/
  if (p.ds.trn_data()->isRegression()) AzPrint::writeln(log_out, "... regression ... "); 
  if (p.ds.trn_data()->isMulticat()) AzPrint::writeln(log_out, "... multi-cat ... "); 
  
  AzpCNet3 cnet(compo_set(azp));
  AzpCNet3_multi cnet_multi(compo_set(azp)); 
  AzpCNet3_multi_side cnet_multi_side(compo_set(azp)); 
  AzpCNet3 *cnet_ptr = choose_cnet(p.s_cnet_ext, &cnet, &cnet_multi, &cnet_multi_side); 
  
  /*---  read NN if specified  ---*/  
  if (p.s_fn_for_warmstart.length() > 0) {
    AzTimeLog::print("Reading for warm-start: ", p.s_fn_for_warmstart.c_str(), log_out); 
    cnet_ptr->read(p.s_fn_for_warmstart.c_str());   
  } 
  
  /*---  training and test  ---*/
  clock_t clocks = 0; 
  clock_t b_clk = clock(); 
  cnet_ptr->training(eval_out_ptr, azp, p.ds.trn_data(), p.ds.tst_data(), p.ds.tst_data2()); 
  clocks += (clock() - b_clk); 
  AzTimeLog::print("Done ...", log_out); 
  
  if (ofs.is_open()) ofs.close(); 
  
  show_elapsed(log_out, clocks); 
}

/*------------------------------------------------------------*/ 
#define kw_mod_fn "model_fn="
#define kw_pred_fn "prediction_fn="
#define kw_do_text "WriteText"
/*------------------------------------------------------------*/ 
class AzpMain_conText_cnn_predict_Param : public virtual AzpMain_conText_Param_ {
public:
  AzpDataSetDflt ds; 
  AzBytArr s_mod_fn, s_pred_fn, s_cnet_ext; 
  bool do_text; 
  /*------------------------------------------------*/
  AzpMain_conText_cnn_predict_Param() : do_text(false) {} /* use this only for displaying help */
  AzpMain_conText_cnn_predict_Param(AzParam &p, const AzOut &out) : do_text(false) {
    reset(p, out); 
  }
  void resetParam(AzParam &p) {
    const char *eyec = "AzpMain_conText_cnn_predict_Param::resetParam"; 
    _resetParam(p);   
    bool do_train = false, do_test = true, is_there_y = false; 
    ds.resetParam(p, do_train, do_test, is_there_y); 
    p.vStr(kw_mod_fn, &s_mod_fn);  
    p.vStr(kw_pred_fn, &s_pred_fn);   
    p.vStr(kw_cnet_ext, &s_cnet_ext); 
    p.swOn(&do_text, kw_do_text);
    AzXi::throw_if_empty(&s_mod_fn, eyec, kw_mod_fn); 
    AzXi::throw_if_empty(&s_pred_fn, eyec, kw_pred_fn); 
    setupLogDmp(p); 
  }
  /*------------------------------------------------*/
  void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    ds.printParam(out); 
    o.printV(kw_mod_fn, s_mod_fn); 
    o.printV(kw_pred_fn, s_pred_fn); 
    o.printSw(kw_do_text, do_text); 
    o.printV_if_not_empty(kw_cnet_ext, s_cnet_ext); 
    printLogDmp(o);  
    _printParam(o); 
    o.printEnd(); 
  }
  /*------------------------------------------------*/
  void printHelp(AzHelp &h) const {  
    bool do_train = false, do_test = true, is_there_y = false; 
    ds.printHelp(h, do_train, do_test, is_there_y);  
    h.item_required(kw_mod_fn, "Path to the model file (input)."); 
    h.item_required(kw_pred_fn, "Path to the prediction file (output)."); 
    h.item(kw_do_text, "Write the prediction values in the text format, one data point per line.", "binary format");     
    h.item(kw_cnet_ext, "Extension.  This must be the same as training.  ");     
    _printHelp(h);    
  }
}; 

/*------------------------------------------------*/
bool AzpMain_conText::printHelp_cnn_predict_if_needed(const AzOut &out, AzParam &azp) {
  if (!azp.needs_help()) return false;   
  AzHelp h(out); h.writeln("cnn_predict: apply the trained model to new data.", 3); h.writeln(""); 
  AzpMain_conText_cnn_predict_Param p; p.printHelp(h); 
  AzpCNet3 cnet(compo_set(azp)); cnet.printHelp_predict(h); 
  h.end(); 
  return true; 
}

/*------------------------------------------------------------*/ 
void AzpMain_conText::cnn_predict(int argc, const char *argv[]) 
{
  const char *eyec = "AzpMain_conText::cnn_predict"; 
  AzParam azp(param_dlm, argc, argv); 
  if (printHelp_cnn_predict_if_needed(log_out, azp)) return;   
  AzpMain_conText_cnn_predict_Param p(azp, log_out); 

  AzpCNet3 cnet(compo_set(azp)); 
  AzpCNet3_multi cnet_multi(compo_set(azp)); 
  AzpCNet3_multi_side cnet_multi_side(compo_set(azp)); 
  AzpCNet3 *cnet_ptr = choose_cnet(p.s_cnet_ext, &cnet, &cnet_multi, &cnet_multi_side); 
  
  AzTimeLog::print("Reading: ", p.s_mod_fn.c_str(), log_out); 
  cnet_ptr->read(p.s_mod_fn.c_str()); 

  p.ds.reset_data(log_out, cnet_ptr->classNum());  

  AzTimeLog::print("Predicting ... ", log_out); 
  AzDmatc m_pred; 
  clock_t b_clk = clock();
  cnet_ptr->predict(azp, p.ds.tst_data(), NULL, NULL, &m_pred); 
  clock_t clocks = clock() - b_clk; 
  show_elapsed(log_out, clocks);
  
  if (p.do_text) {
    AzTimeLog::print("Writing (text) ", p.s_pred_fn.c_str(), log_out); 
    int digits = 7; 
    m_pred.writeText(p.s_pred_fn.c_str(), digits); 
  }
  else {
    AzTimeLog::print("Writing (binary) ", p.s_pred_fn.c_str(), log_out); 
    m_pred.write(p.s_pred_fn.c_str()); 
  }  
  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------------------*/ 
class AzpMain_conText_write_embedded_Param : public virtual AzpMain_conText_Param_ {
public:
  AzpDataSetDflt ds; 
  AzBytArr s_mod_fn; 
  /*------------------------------------------------*/
  AzpMain_conText_write_embedded_Param() {} /* use this only for displaying help */
  AzpMain_conText_write_embedded_Param(AzParam &p, const AzOut &out) {
    reset(p, out); 
  }
  void resetParam(AzParam &p) {
    const char *eyec = "AzpMain_conText_write_embedded_Param::resetParam"; 
    _resetParam(p);   
    bool do_train = false, do_test = true, is_there_y = false; 
    ds.resetParam(p, do_train, do_test, is_there_y); 
    p.vStr(kw_mod_fn, &s_mod_fn);  
    AzXi::throw_if_empty(&s_mod_fn, eyec, kw_mod_fn); 
  }
  /*------------------------------------------------*/
  void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    ds.printParam(out); 
    o.printV(kw_mod_fn, s_mod_fn); 
    _printParam(o); 
    o.printEnd(); 
  }
  /*------------------------------------------------*/
  void printHelp(AzHelp &h) const {  
    bool do_train = false, do_test = true, is_there_y = false; 
    ds.printHelp(h, do_train, do_test, is_there_y);  
    h.item_required(kw_mod_fn, "Path to the model file (input)."); 
    _printHelp(h);    
  }  
}; 

/*------------------------------------------------*/
bool AzpMain_conText::printHelp_write_embedded_if_needed(const AzOut &out, AzParam &azp) {
  if (!azp.needs_help()) return false;   
  AzHelp h(out); h.writeln("write_embedded: write output of region embedding of layer-0 to a file.  One vector per region, typically.", 3); h.writeln(""); 
  AzpMain_conText_write_embedded_Param p; p.printHelp(h); 
  AzpCNet3 cnet(compo_set(azp)); cnet.printHelp_write_embedded(h); 
  h.end(); 
  return true; 
}

/*------------------------------------------------------------*/ 
void AzpMain_conText::write_embedded(int argc, const char *argv[]) 
{
  const char *eyec = "AzpMain_conText::write_embedded"; 
  AzParam azp(param_dlm, argc, argv); 
  if (printHelp_write_embedded_if_needed(log_out, azp)) return; 
  AzpMain_conText_write_embedded_Param p(azp, log_out); 

  AzpCNet3 cnet(compo_set(azp)); 
  AzTimeLog::print("Reading: ", p.s_mod_fn.c_str(), log_out); 
  cnet.read(p.s_mod_fn.c_str()); 

  p.ds.reset_data(log_out, cnet.classNum());  
  cnet.write_embedded(azp, p.ds.tst_data());   
}

/*------------------------------------------------------------*/ 
class AzpMain_conText_write_features_Param : public virtual AzpMain_conText_Param_ {
public:
  AzpDataSetDflt ds; 
  AzBytArr s_mod_fn, s_out_fn, s_cnet_ext; 
  int lay_no; 
  /*------------------------------------------------*/
  AzpMain_conText_write_features_Param() : lay_no(-1) {} /* use this only for displaying help */
  AzpMain_conText_write_features_Param(AzParam &p, const AzOut &out) : lay_no(-1) {
    reset(p, out); 
  }
  #define kw_out_fn "feature_fn="
  #define kw_lay_no "layer="
  void resetParam(AzParam &p) {
    const char *eyec = "AzpMain_conText_internal_features_Param::resetParam"; 
    _resetParam(p);   
    bool do_train = false, do_test = true, is_there_y = false; 
    ds.resetParam(p, do_train, do_test, is_there_y); 
    p.vStr(kw_cnet_ext, &s_cnet_ext);     
    p.vStr(kw_mod_fn, &s_mod_fn);  
    p.vStr(kw_out_fn, &s_out_fn); 
    p.vInt(kw_lay_no, &lay_no); 
    AzXi::throw_if_empty(&s_mod_fn, eyec, kw_mod_fn); 
    AzXi::throw_if_empty(&s_out_fn, eyec, kw_out_fn); 
  }
  /*------------------------------------------------*/
  void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    ds.printParam(out); 
    o.printV(kw_cnet_ext, s_cnet_ext); 
    o.printV(kw_mod_fn, s_mod_fn); 
    o.printV(kw_out_fn, s_out_fn); 
    o.printV(kw_lay_no, lay_no); 
    _printParam(o); 
    o.printEnd(); 
  }
  /*------------------------------------------------*/
  void printHelp(AzHelp &h) const {  
    bool do_train = false, do_test = true, is_there_y = false; 
    ds.printHelp(h, do_train, do_test, is_there_y);  
    h.item_required(kw_mod_fn, "Path to the model file (input)."); 
    h.item_required(kw_out_fn, "Path to the file to which internal feature will be written (output)."); 
    h.item(kw_lay_no, "Internal features produced as input to this layer will be written to the designated file.  Default: top layer.  The layer must be a middle or the top layer with fixed-sized input.  Otherwise, it would result in either an error or output of size 0.");
    h.item(kw_cnet_ext, "Extension.  This must be the same as training.  "); 
    _printHelp(h);    
  }  
}; 

/*------------------------------------------------*/
bool AzpMain_conText::printHelp_write_features_if_needed(const AzOut &out, AzParam &azp) {
  if (!azp.needs_help()) return false;   
  AzHelp h(out); h.writeln("write_features: write internal features to a file.", 3); h.writeln(""); 
  AzpMain_conText_write_features_Param p; p.printHelp(h); 
  AzpCNet3 cnet(compo_set(azp)); cnet.printHelp_write_features(h); 
  h.end(); 
  return true; 
}

/*------------------------------------------------------------*/ 
void AzpMain_conText::write_features(int argc, const char *argv[]) 
{
  const char *eyec = "AzpMain_conText::write_features"; 
  AzParam azp(param_dlm, argc, argv); 
  if (printHelp_write_features_if_needed(log_out, azp)) return; 
  AzpMain_conText_write_features_Param p(azp, log_out); 
  AzpCNet3 cnet(compo_set(azp)); 
  AzpCNet3_multi cnet_multi(compo_set(azp)); 
  AzpCNet3_multi_side cnet_multi_side(compo_set(azp)); 
  AzpCNet3 *cnet_ptr = choose_cnet(p.s_cnet_ext, &cnet, &cnet_multi, &cnet_multi_side);   

  AzTimeLog::print("Reading: ", p.s_mod_fn.c_str(), log_out); 
  cnet_ptr->read(p.s_mod_fn.c_str()); 

  p.ds.reset_data(log_out, cnet_ptr->classNum());  
  cnet_ptr->write_features(azp, p.ds.tst_data(), p.s_out_fn.c_str(), p.lay_no); 
}

/*------------------------------------------------------------*/ 
/*------------------------------------------------------------*/ 
class AzpMain_conText_write_word_mapping_Param : public virtual AzpMain_conText_Param_ {
public:
  AzBytArr s_cnet_ext, s_mod_fn, s_lay0_fn, s_wmap_fn; 
  int dsno; 
    
  /*------------------------------------------------*/
  AzpMain_conText_write_word_mapping_Param(AzParam &azp, const AzOut &out) : dsno(-1) {
    if (azp.needs_help()) {
      printHelp(out); 
      AzX::throw_if(true, AzNormal, "", "");       
    }    
    reset(azp, out); 
  }
  #define kw_lay0_fn "layer0_fn="
  #define kw_wmap_fn "word_map_fn="
  #define kw_dsno "dsno="
  void resetParam(AzParam &p) {
    const char *eyec = "AzpMain_conText_write_word_mapping::resetParam"; 
    _resetParam(p);   
    p.vStr(kw_lay0_fn, &s_lay0_fn); 
    if (s_lay0_fn.length() <= 0) {
      p.vStr(kw_mod_fn, &s_mod_fn);  
      p.vStr(kw_cnet_ext, &s_cnet_ext); 
      dsno = 0; /* default */
      p.vInt(kw_dsno, &dsno); 
      AzXi::throw_if_negative(dsno, eyec, kw_dsno); 
    }
    p.vStr(kw_wmap_fn, &s_wmap_fn); 
    AzBytArr s("No input: either "); s << kw_mod_fn << " or " << kw_lay0_fn << " is required."; 
    AzX::throw_if((s_mod_fn.length() <= 0 && s_lay0_fn.length() <= 0), eyec, s.c_str()); 
    AzXi::throw_if_empty(s_wmap_fn, eyec, kw_wmap_fn); 
  }
  /*------------------------------------------------*/
  void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    o.printV_if_not_empty(kw_mod_fn, s_mod_fn); 
    o.printV_if_not_empty(kw_cnet_ext, s_cnet_ext); 
    o.printV(kw_dsno, dsno); 
    o.printV_if_not_empty(kw_lay0_fn, s_lay0_fn); 
    o.printV(kw_wmap_fn, s_wmap_fn);    
    _printParam(o); 
    o.printEnd(); 
  }
  /*------------------------------------------------*/
  void printHelp(const AzOut &out) const {  
    AzHelp h(out); 
    h.writeln("write_word_mapping: write the word mapping information saved in either a model file or a layer-0 file.", 3); h.writeln(""); 
    h.item(kw_mod_fn, "Path to the model file (input).  Either this or layer0_fn is required."); 
    h.item(kw_lay0_fn, "Path to the layer-0 file (input).  Either this or model_fn is required. "); 
    h.item_required(kw_wmap_fn, "Path to the file the word mappin will be written to (output).");       
    h.item(kw_dsno, "Data# if model_fn is specified.", "0"); 
    h.item(kw_cnet_ext, "Extention name if the model_fn is specified.  This must be the same as training."); 
    _printHelp(h); 
    h.end();     
  }   
}; 

/*------------------------------------------------------------*/ 
void AzpMain_conText::write_word_mapping(int argc, const char *argv[]) 
{
  const char *eyec = "AzpMain_conText::write_word_mapping"; 
  AzParam azp(param_dlm, argc, argv); 
  AzpMain_conText_write_word_mapping_Param p(azp, log_out); 

  if (p.s_lay0_fn.length() > 0) {
    AzpCNet3 cnet(compo_set(azp));  
    azp.check(log_out); 
    cnet.write_word_mapping_in_lay0(p.s_lay0_fn.c_str(), p.s_wmap_fn.c_str()); 
  }
  else {     
    AzpCNet3 cnet(compo_set(azp));  
    AzpCNet3_multi cnet_multi(compo_set(azp)); 
    AzpCNet3_multi_side cnet_multi_side(compo_set(azp)); 
    AzpCNet3 *cnet_ptr = choose_cnet(p.s_cnet_ext, &cnet, &cnet_multi, &cnet_multi_side); 
    AzTimeLog::print("Reading: ", p.s_mod_fn.c_str(), log_out); 
    cnet_ptr->read(p.s_mod_fn.c_str()); 
    azp.check(log_out);
    cnet_ptr->write_word_mapping(p.s_wmap_fn.c_str(), p.dsno); 
  }
  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------------------*/ 
/*------------------------------------------------------------*/ 
#define kw_wvecbin_fn "wordvec_bin_fn="
#define kw_wvectxt_fn "wordvec_txt_vec_fn="
#define kw_wvecdic_fn "wordvec_txt_words_fn="
#define kw_do_ignore_dup "IgnoreDupWords"
#define kw_wvec_rand_param "rand_param="
#define kw_w_fn "weight_fn="
#define kw_do_verbose "Verbose"
#define kw_rand_seed "random_seed="
#define kw_xdic_fn "word_map_fn="    
/*------------------------------------------------------------*/ 
class AzMain_conText_adapt_word_vectors_Param : public virtual AzpMain_conText_Param_ {
public: 
  AzBytArr s_w_fn, s_wvecbin_fn, s_wvectxt_fn, s_wvecdic_fn, s_xdic_fn; 
  int rand_seed; 
  double wvec_rand_param; 
  bool do_verbose, do_ignore_dup; 
  AzMain_conText_adapt_word_vectors_Param(AzParam &azp, const AzOut &out) : 
    wvec_rand_param(-1), do_verbose(false), rand_seed(-1), do_ignore_dup(false) {
    if (azp.needs_help()) {
      printHelp(out); 
      AzX::throw_if(true, AzNormal, "", "");       
    }
    reset(azp, out); 
  }
  void resetParam(AzParam &p) {
    const char *eyec = "AzMain_conText_adapt_word_vectors_Param::resetParam";  
    _resetParam(p);     
    p.vStr(kw_w_fn, &s_w_fn);   
    p.vStr(kw_wvecbin_fn, &s_wvecbin_fn); 
    if (s_wvecbin_fn.length() <= 0) {
      p.vStr(kw_wvectxt_fn, &s_wvectxt_fn); 
      p.vStr(kw_wvecdic_fn, &s_wvecdic_fn);     
    }
    p.vStr(kw_xdic_fn, &s_xdic_fn); 
    p.vInt(kw_rand_seed, &rand_seed); 
    p.vFloat(kw_wvec_rand_param, &wvec_rand_param); 
    p.swOn(&do_ignore_dup, kw_do_ignore_dup); 
    p.swOn(&do_verbose, kw_do_verbose); 
    if (s_wvecbin_fn.length() > 0); 
    else if (s_wvectxt_fn.length() > 0) AzXi::throw_if_empty(s_wvecdic_fn, eyec, kw_wvecdic_fn);        
    else {
      AzBytArr s("To specify word vectors, either a binary file ("); s << kw_wvecbin_fn << ") or a pair of text files ("; 
      s << kw_wvectxt_fn << " and " << kw_wvecdic_fn << ") must be given."; 
      AzX::throw_if(true, AzInputError, eyec, s.c_str()); 
    }   
    AzXi::throw_if_empty(s_xdic_fn, eyec, kw_xdic_fn); 
    AzXi::throw_if_empty(s_w_fn, eyec, kw_w_fn);   
    if (rand_seed != -1) AzXi::throw_if_nonpositive(rand_seed, eyec, kw_rand_seed); 
  }
  void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    o.printV(kw_w_fn, s_w_fn); 
    o.printV_if_not_empty(kw_wvecbin_fn, s_wvecbin_fn); 
    o.printV_if_not_empty(kw_wvectxt_fn, s_wvectxt_fn); 
    o.printV_if_not_empty(kw_wvecdic_fn, s_wvecdic_fn);     
    o.printV(kw_xdic_fn, s_xdic_fn); 
    o.printV(kw_rand_seed, rand_seed); 
    o.printV(kw_wvec_rand_param, wvec_rand_param); 
    o.printSw(kw_do_ignore_dup, do_ignore_dup); 
    o.printSw(kw_do_verbose, do_verbose);
    _printParam(o);  
    o.printEnd(); 
  }
  static void printHelp(const AzOut &out) {   
    AzHelp h(out); h.begin("", "", "");  h.nl(); 
    h.writeln("adapt_word_vectors: convert a word vector file to a \"weight file\" so that the word vectors can be used to produce input to CNN training.\n", 3); 
    h.item(kw_wvecbin_fn, "Path to a binary file containing word vectors and words in the word2vec format (input).  Either this or a pair of text files (wordvec_txt_vec_fn and wordvec_txt_words_fn below) is requried."); 
    h.item(kw_wvectxt_fn, "Path to the word vector text file (input).  One line per vector.  Vector components should be delimited by space."); 
    h.item(kw_wvecdic_fn, "Path to the word vector word file (input).  One line per word."); 
    h.item_required(kw_w_fn, "Path to the weight file to be generated (output).");     
    h.item_required(kw_xdic_fn, "Path to the word-mapping file (*.xtext) generated by \"prepText gen_regions\".  Word vectors will be sorted in the order of this file so that the dimensions of the resulting weights will correctly correspond to the dimensions of the region vectors generated by \"gen_regions\".");     
    h.item(kw_rand_seed, "Random number generator seed."); 
    h.item(kw_wvec_rand_param, "x: scale of initialization.  If the word-mapping file (word_map_fn) contains words for which word vectors are not given, the word vectors for these unknown words will be randomly set by Gaussian distribution with zero mean with standard deviation x.", "0");     
    /* do_verbose */
    h.item(kw_do_ignore_dup, "Ignore it if there are duplicated words associated with word vectors.  If this is not turned on, the process will be terminated on the detection of duplicated words."); 
    h.end(); 
  }   
}; 

/*------------------------------------------------*/
void AzpMain_conText::adapt_word_vectors(int argc, const char *argv[]) 
{
  const char *eyec = "AzMain_conText::adapt_word_vectors"; 
  AzParam azp(param_dlm, argc, argv);
  AzMain_conText_adapt_word_vectors_Param p(azp, log_out);   
  azp.check(log_out); 
  
  if (p.rand_seed > 0) srand(p.rand_seed);  
  
  AzTimeLog::print("Reading vectors ... ", log_out); 
  AzDic xdic(p.s_xdic_fn.c_str()), wvecdic; 
  AzDmat m_wvec; 
  if (p.s_wvecbin_fn.length() > 0) read_word2vec_bin(p.s_wvecbin_fn.c_str(), wvecdic, m_wvec, p.do_ignore_dup); 
  else {
    wvecdic.reset(p.s_wvecdic_fn.c_str(), p.do_ignore_dup); 
    AzTextMat::readMatrix(p.s_wvectxt_fn.c_str(), &m_wvec); 
  }
  AzTimeLog::print("Processing ... ", log_out); 
  bool do_die_if = true; 
  int mapped = AzDic::rearrange_cols(xdic, wvecdic, m_wvec, do_die_if); 
  if (p.do_verbose) {
    AzBytArr s; s << wvecdic.size() << " -> " << xdic.size() << ": mapped=" << mapped; 
    AzTimeLog::print(s.c_str(), log_out); 
  }  
  if (p.wvec_rand_param > 0) {
    AzTimeLog::print("Random initialization of unknown words ... ", log_out); 
    zerovec_to_randvec(p.wvec_rand_param, m_wvec); 
  }
  
  AzDmat m_w; m_wvec.transpose_to(&m_w); m_wvec.destroy(); 
  
  AzBytArr s("Writing "); s << p.s_w_fn.c_str() << ": " << m_w.rowNum() << " x " << m_w.colNum();
  if (p.s_w_fn.endsWith("pmat")) {
    AzTimeLog::print(s.c_str(), " (binary: pmat)", log_out); 
    AzPmat mp(&m_w); 
    mp.write(p.s_w_fn.c_str());  
  }
  else if (p.s_w_fn.endsWith("dmat")) {
    AzTimeLog::print(s.c_str(), " (binary: dmat)", log_out); 
    m_w.write(p.s_w_fn.c_str());  
  }  
  else {
    AzTimeLog::print(s.c_str(), " (text)", log_out); 
    int digits = 7; 
    m_w.writeText(p.s_w_fn.c_str(), digits);    
  }
  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------*/ 
void AzpMain_conText::zerovec_to_randvec(double wvec_rand_param, AzDmat &m_wvec) const
{
  AzRandGen rg; 
  for (int col = 0; col < m_wvec.colNum(); ++col) {
    if (!m_wvec.isZero(col)) continue;    
    rg.gaussian(wvec_rand_param, m_wvec.col_u(col)); 
  }
}

/*--------------------------------------------------------------*/ 
/*  read word2vec word vectors as word2vec word-analogy.c does  */
/*--------------------------------------------------------------*/ 
void AzpMain_conText::read_word2vec_bin(const char *fn, AzDic &dic, AzDmat &m_wvec, bool do_ignore_dup) {
  const char *eyec = "AzpMain_conText::read_word2vec_bin"; 
  AzBytArr s_err("An error was encountered while reading "); 
  s_err << fn << ".  The file is either corrupted or not in the word2vec binary format."; 

  AzFile file(fn); file.open("rb"); 
  FILE *f = file.ptr(); 
  long long words, size; 
  if (fscanf(f, "%lld", &words) != 1) throw new AzException(AzInputError, eyec, s_err.c_str()); 
  if (fscanf(f, "%lld", &size) != 1) throw new AzException(AzInputError, eyec, s_err.c_str()); 
  int row_num = Az64::to_int(size, "dimensionality of word2vec vectors"); 
  int col_num = Az64::to_int(words, "number of word2vec vectors"); 
  m_wvec.reform(row_num, col_num); 
  char word[4096];  /* assume no words/phrases are longer than this */
  AzStrPool sp; 
  for (int b = 0; b < words; b++) {
    char ch; 
    if (fscanf(f, "%s%c", word, &ch) != 2) throw new AzException(AzInputError, eyec, s_err.c_str()); 
    sp.put(word); 
    for (int a = 0; a < size; a++) {
      float val; 
      if (fread(&val, sizeof(float), 1, f) != 1) throw new AzException(AzInputError, eyec, s_err.c_str()); 
      m_wvec.set(a, b, val);  
    }
  }
  dic.reset(&sp, do_ignore_dup);   
  file.close(); 
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
void AzpMain_conText::show_elapsed(const AzOut &out, 
                             clock_t clocks) const
{
  double seconds = (double)clocks / (double)CLOCKS_PER_SEC; 
  AzPrint::writeln(out, "elapsed: ", seconds); 
}

/*------------------------------------------------*/
AzOut *AzpMain_conText::reset_eval(const AzBytArr &s_eval_fn, 
                                AzOfs &ofs, 
                                AzOut &eval_out) 
{
  AzOut *eval_out_ptr = NULL;  
  if (s_eval_fn.length() > 0) {
    ofs.open(s_eval_fn.c_str(), ios_base::out); 
    ofs.set_to(eval_out); 
    eval_out_ptr = &eval_out; 
  }
  return eval_out_ptr; 
}
