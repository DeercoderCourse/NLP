/* * * * *
 *  AzPrepText.cpp 
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

#include "AzParam.hpp"
#include "AzTools.hpp"
#include "AzTools_text.hpp"
#include "AzPrepText.hpp"
#include "AzHelp.hpp"

static AzByte param_dlm = ' ', file_mark='@'; 
static bool do_paramChk = true; 

class AzPrepText_Param_ {
public: 
  AzPrepText_Param_() {}
  virtual void reset(int argc, const char *argv[], const AzOut &out) {
    if (argc < 2) {
      if (argc >= 1) {
        AzPrint::writeln(out, ""); 
        AzPrint::writeln(out, argv[0]); /* action */
      }    
      printHelp(out); 
      throw new AzException(AzNormal, "", ""); 
    }
    const char *action = argv[0]; 
    AzParam azp(param_dlm, argc-1, argv+1); 
    AzPrint::writeln(out, ""); 
    AzPrint::writeln(out, "--------------------------------------------------");     
    AzPrint::writeln(out, action, " ", azp.c_str()); 
    AzPrint::writeln(out, "--------------------------------------------------"); 
    resetParam(azp); 
    printParam(out); 
    azp.check(out); 
  }
  virtual void resetParam(AzParam &azp) = 0; 
  virtual void printParam(const AzOut &out) const = 0; 
  virtual void printHelp(const AzOut &out) const {  /* derived classes should override this */
    AzPrint::writeln(out, "Help is not ready yet."); 
  }  
}; 

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/ 
class AzPrepText_gen_vocab_Param : public virtual AzPrepText_Param_ {
public:
  AzBytArr s_inp_fn, s_voc_fn, s_stop_fn, s_txt_ext; 
  bool do_lower, do_remove_number, do_utf8dashes, do_write_count; 
  int min_count, nn, max_num; 

  AzPrepText_gen_vocab_Param(int argc, const char *argv[], const AzOut &out)
     : do_lower(false), do_remove_number(false), do_utf8dashes(false), min_count(-1), nn(1), 
       max_num(-1), do_write_count(false) {
    reset(argc, argv, out); 
  }
  
  /*-----------------------------------------------*/
  #define kw_inp_fn "input_fn="
  #define kw_voc_fn "vocab_fn="
  #define kw_do_lower "LowerCase"
  #define kw_min_count "min_word_count="
  #define kw_do_remove_number "RemoveNumbers"
  #define kw_txt_ext "text_fn_ext="
  #define kw_nn "n="
  #define kw_stop_fn "stopword_fn="
  #define kw_do_utf8dashes "UTF8"
  #define kw_max_num "max_vocab_size="
  #define kw_do_write_count "WriteCount"
  
  #define help_do_lower "Convert upper-case to lower-case characters."
  #define help_do_utf8dashes "Convert UTF8 en dash, em dash, single/double quotes to ascii characters."
  /*-------------------------------------------------------------------------*/
  bool is_list(const AzBytArr &s) const {
    return (s.endsWith(".lst")); 
  }
  virtual void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_gen_vocab_Param::resetParam"; 
    azp.vStr(kw_inp_fn, &s_inp_fn); 
    azp.vStr(kw_txt_ext, &s_txt_ext);   
    azp.vStr(kw_voc_fn, &s_voc_fn); 
    azp.swOn(&do_lower, kw_do_lower); 
    azp.vStr(kw_stop_fn, &s_stop_fn);     
    azp.vInt(kw_min_count, &min_count); 
    azp.vInt(kw_max_num, &max_num); 
    azp.vInt(kw_nn, &nn); 
    azp.swOn(&do_remove_number, kw_do_remove_number); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes); 
    azp.swOn(&do_write_count, kw_do_write_count); 
  
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn); 
    AzXi::throw_if_empty(s_voc_fn, eyec, kw_voc_fn); 
  }
  
  virtual void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_inp_fn, s_inp_fn); 
    o.printV_if_not_empty(kw_txt_ext, s_txt_ext); 
    o.printV(kw_voc_fn, s_voc_fn);    
    o.printV_if_not_empty(kw_stop_fn, s_stop_fn); 
    o.printV(kw_min_count, min_count); 
    o.printV(kw_max_num, max_num); 
    o.printV(kw_nn, nn);     
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_remove_number, do_remove_number); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printSw(kw_do_write_count, do_write_count); 
    o.printEnd(); 
  }
            
  void printHelp(const AzOut &out) const {
    AzHelp h(out); 
    h.begin("", "", ""); 
    h.item_required(kw_inp_fn, "Path to the input token file or the list of token files.  If the filename ends with \".lst\", the file should be the list of token filenames.  The input file(s) should contain one document per line, and each document should be tokens delimited by space."); 
    h.item(kw_txt_ext, "Filename extension of input token files."); 
    h.item_required(kw_voc_fn, "Path to the file that the vocabulary is written to."); 
    h.item(kw_stop_fn, "Path to a stopword file.  The words in this file will be excluded."); 
    h.item(kw_min_count, "Minimum word counts to be included in the vocabulary file.", "No limit"); 
    h.item(kw_max_num, "Maximum number of words to be included in the vocabulary file.  The most frequent ones will be included.", "No limit"); 
    h.item(kw_do_lower, help_do_lower);     
    h.item(kw_do_utf8dashes, help_do_utf8dashes); 
    h.item(kw_do_remove_number, "Exclude words that contain numbers.");     
    h.item(kw_do_write_count, "Write word counts as well as the words to the vocabulary file."); 

    h.item(kw_nn, "n for n-grams.  E.g., if n=3, only tri-grams are included.");   
    h.end(); 
  }    
};                      
                  
/*-------------------------------------------------------------------------*/
void AzPrepText::gen_vocab(int argc, const char *argv[]) const
{
  AzPrepText_gen_vocab_Param p(argc, argv, out);  
  
  AzStrPool sp_voc[256]; 
  for (int vx = 0; vx < 256; ++vx) {
    sp_voc[vx].reset(100000,20); 
  }
  
  AzStrPool *sp_stop = NULL, _sp_stop; 
  if (p.s_stop_fn.length() > 0) {
    AzTools::readList(p.s_stop_fn.c_str(), &_sp_stop); 
    _sp_stop.commit(); 
    if (_sp_stop.size() > 0) sp_stop = &_sp_stop; 
  }  

  AzIntArr ia_data_num; 
  AzStrPool sp_list; 
  int buff_size = AzTools_text::scan_files_in_list(p.s_inp_fn.c_str(), p.s_txt_ext.c_str(), 
                                                   out, &sp_list, &ia_data_num); 
  buff_size += 256;  /* just in case */
  AzBytArr s_buff; 
  AzByte *buff = s_buff.reset(buff_size, 0);                                                    

  for (int fx = 0; fx < sp_list.size(); ++fx) {
    AzBytArr s_fn(sp_list.c_str(fx), p.s_txt_ext.c_str()); 
    const char *fn = s_fn.c_str();   
    AzTimeLog::print(fn, out); 
    AzFile file(fn); file.open("rb"); 
    for ( ; ; ) {
      int len = file.gets(buff, buff_size); 
      if (len <= 0) break;
    
      AzStrPool sp;     
      AzTools_text::tokenize(buff, len, p.do_utf8dashes, p.do_lower, &sp); 
      for (int wx = 0; wx < sp.size(); ++wx) {
        int index = gen_1byte_index(&sp, wx); 
        put_in_voc(p.nn, sp_voc[index], sp, wx, 1, sp_stop, p.do_remove_number); 
      }
    }
    int num = 0; 
    for (int vx = 0; vx < 256; ++vx) {
      sp_voc[vx].commit(); 
      num += sp_voc[vx].size(); 
    }    
    AzTimeLog::print(" ... size: ", num, out); 
  }
  if (p.min_count > 1) {
    int num = 0; 
    for (int vx = 0; vx < 256; ++vx) {
      sp_voc[vx].reduce(p.min_count); 
      num += sp_voc[vx].size(); 
    }
    AzTimeLog::print("Removed <min_count -> ", num, out);     
  }
  AzTimeLog::print("Merging ... ", out); 
  for (int vx = 1; vx < 256; ++vx) {
    sp_voc[0].put(&sp_voc[vx]); 
    sp_voc[vx].reset(); 
  }
  sp_voc[0].commit(); 
  AzTimeLog::print("Writing to ", p.s_voc_fn.c_str(), out); 
  int sz = write_vocab(p.s_voc_fn.c_str(), &sp_voc[0], p.max_num, p.min_count, p.do_write_count); 
  AzTimeLog::print("Done: size=", sz, out); 
}

/*-------------------------------------------------------------------------*/
void AzPrepText::put_in_voc(int nn, 
                  AzStrPool &sp_voc, 
                  const AzStrPool &sp_words, 
                  int wx, /* position in sp_words */
                  AZint8 count, 
                  const AzStrPool *sp_stop, 
                  bool do_remove_number) const
{
  if (wx < 0 || wx+nn > sp_words.size()) return; 
  if (nn == 1) {
    if (sp_stop != NULL && sp_stop->find(sp_words.c_str(wx)) >= 0 ||       
        do_remove_number && strpbrk(sp_words.c_str(wx), "0123456789") != NULL) {}
    else sp_voc.put(sp_words.c_str(wx), count); 
  }
  else {
    AzBytArr s_ngram;   
    bool do_n_remove_nonalpha = false; 
    sp_words.compose_ngram(s_ngram, wx, nn, sp_stop, do_remove_number, do_n_remove_nonalpha); 
    if (s_ngram.length() > 0) {
      sp_voc.put(s_ngram.c_str(), count); 
    }
  }
} 

/*-------------------------------------------------------------------------*/
/* static */
AzByte AzPrepText::gen_1byte_index(const AzStrPool *sp_words, int wx)
{
  if (wx < 0 || wx >= sp_words->size()) return 0; 
  int len; 
  const AzByte *ptr = sp_words->point(wx, &len); 
  if (len == 0) return 0; 
  return (AzByte)((int)(*ptr) + (int)(*(ptr+len/2))); 
} 

/*-------------------------------------------------------------*/
int AzPrepText::write_vocab(const char *fn, const AzStrPool *sp, 
                            int max_num, 
                            int min_count, 
                            bool do_write_count)
{
  AzFile file(fn); file.open("wb"); 
  AzIFarr ifa_ix_count; ifa_ix_count.prepare(sp->size()); 
  for (int ix = 0; ix < sp->size(); ++ix) {
    double count = (double)sp->getCount(ix); 
    if (count >= min_count) ifa_ix_count.put(ix, count); 
  } 
  ifa_ix_count.sort_FloatInt(false, true); /* float: descending, int: ascending */
  ifa_ix_count.cut(max_num); 
  for (int ix = 0; ix < ifa_ix_count.size(); ++ix) {
    int idx; 
    ifa_ix_count.get(ix, &idx); 
    AzBytArr s(sp->c_str(idx)); 
    if (do_write_count) s << '\t' << sp->getCount(idx); 
    s.nl(); s.writeText(&file);  
  }
  file.close(true); 
  return ifa_ix_count.size(); 
}
  
/*-------------------------------------------------------------------------*/
#define x_ext ".xsmatvar"
#define xtext_ext ".xtext"
/*-------------------------------------------------------------------------*/
class AzPrepText_gen_regions_Param : public virtual AzPrepText_Param_ {
public:
  bool do_bow, do_skip_stopunk; 
  AzBytArr s_inp_fn, s_txt_ext, s_cat_ext, s_voc_fn, s_cat_dic_fn, s_rnm, s_y_ext; 
  bool do_lower, do_utf8dashes; 
  int pch_sz, pch_step, padding; 
  bool do_allow_zero, do_allow_multi, do_allow_nocat;  
  bool do_ignore_bad; 
  bool do_region_only, do_write_pos; 
  AzBytArr s_batch_id; 
  AzBytArr s_inppos_fn; 
  bool do_unkw; 
  
  AzPrepText_gen_regions_Param(int argc, const char *argv[], const AzOut &out) 
    : do_bow(false), do_skip_stopunk(false), do_lower(false), pch_sz(-1), pch_step(1), padding(0), 
      do_allow_zero(false), do_allow_multi(false), do_allow_nocat(false), do_utf8dashes(false), 
      do_region_only(false), s_y_ext(".y"), do_write_pos(false), do_ignore_bad(false), do_unkw(false) {
    reset(argc, argv, out); 
  }      
  /*-------------------------------------------------------------------------*/
  #define kw_do_region_only "RegionOnly"
  #define kw_cat_ext "label_fn_ext="
  #define kw_cat_dic_fn "label_dic_fn="
  #define kw_rnm "region_fn_stem="
  #define kw_pch_sz "patch_size="
  #define kw_pch_step "patch_stride="
  #define kw_padding "padding="
  #define kw_do_allow_zero "AllowZeroRegion"
/*  #define kw_do_allow_multi "AllowMulti"  defined in AzTools_text */
  #define kw_do_bow_old "Bow-convolution"
  #define kw_do_bow "Bow"  
  #define kw_do_skip_stopunk "VariableStride"
  #define kw_y_ext "y_ext="   
  #define kw_batch_id "batch_id="
  #define kw_inppos_fn "input_pos_fn="
  #define kw_do_write_pos "WritePositions"
  #define kw_do_ignore_bad "ExcludeMultiNo"
  #define kw_do_unkw "Unkw"
  /*-------------------------------------------------------------------------*/  
  virtual void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_gen_regions_Param::resetParam";   
    azp.swOn(&do_region_only, kw_do_region_only); 
    if (!do_region_only) {
      azp.vStr(kw_cat_ext, &s_cat_ext);     
      azp.vStr(kw_cat_dic_fn, &s_cat_dic_fn);     
    }
    azp.swOn(&do_bow, kw_do_bow_old); /* for compatibility */
    azp.swOn(&do_bow, kw_do_bow);
    azp.vStr(kw_inp_fn, &s_inp_fn); 
    azp.vStr(kw_txt_ext, &s_txt_ext); 
    azp.vStr(kw_voc_fn, &s_voc_fn); 
    azp.vStr(kw_rnm, &s_rnm); 
    azp.swOn(&do_lower, kw_do_lower); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes); 
    azp.vInt(kw_pch_sz, &pch_sz); 
    azp.swOn(&do_allow_multi, kw_do_allow_multi); 
    do_allow_nocat = do_allow_multi; 
    azp.swOn(&do_ignore_bad, kw_do_ignore_bad); 
    azp.swOn(&do_write_pos, kw_do_write_pos); 
    azp.vStr(kw_y_ext, &s_y_ext); 
    azp.vStr(kw_batch_id, &s_batch_id); 
    
    azp.vStr(kw_inppos_fn, &s_inppos_fn); /* positions of regions for which vectors are generated */
    if (s_inppos_fn.length() <= 0) {
      azp.vInt(kw_pch_step, &pch_step); 
      azp.vInt(kw_padding, &padding); 
      azp.swOn(&do_allow_zero, kw_do_allow_zero); 
      azp.swOn(&do_skip_stopunk, kw_do_skip_stopunk); 
    } 
    else {
      pch_step = padding = -1;  /* these won't be used.  Setting -1 to avoid displaying */ 
    }
    azp.swOn(&do_unkw, kw_do_unkw); 
    
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn); 
    if (!do_region_only) {   
      AzXi::throw_if_empty(s_cat_ext, eyec, kw_cat_ext);     
      AzXi::throw_if_empty(s_cat_dic_fn, eyec, kw_cat_dic_fn);  
    }
    AzXi::throw_if_empty(s_voc_fn, eyec, kw_voc_fn);   
    AzXi::throw_if_empty(s_rnm, eyec, kw_rnm);     
    AzXi::throw_if_empty(s_y_ext, eyec, kw_y_ext);         
    AzXi::throw_if_nonpositive(pch_sz, kw_pch_sz, eyec); 
    if (s_inppos_fn.length() <= 0) {
      AzXi::throw_if_nonpositive(pch_step, kw_pch_step, eyec); 
      AzXi::throw_if_negative(padding, kw_padding, eyec);        
    }
  }
  virtual void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printSw(kw_do_region_only, do_region_only); 
    o.printSw(kw_do_bow, do_bow); 
    o.printV(kw_inp_fn, s_inp_fn); 
    o.printV(kw_txt_ext, s_txt_ext); 
    o.printV(kw_cat_ext, s_cat_ext);   
    o.printV(kw_cat_dic_fn, s_cat_dic_fn);    
    o.printV(kw_voc_fn, s_voc_fn); 
    o.printV(kw_rnm, s_rnm); 
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printV(kw_pch_sz, pch_sz); 
    o.printV(kw_pch_step, pch_step); 
    o.printV(kw_padding, padding); 
    o.printSw(kw_do_skip_stopunk, do_skip_stopunk);       
    o.printSw(kw_do_allow_zero, do_allow_zero); 
    o.printSw(kw_do_allow_multi, do_allow_multi);  
    o.printSw(kw_do_ignore_bad, do_ignore_bad); 
    o.printSw(kw_do_write_pos, do_write_pos); 
    o.printV(kw_y_ext, s_y_ext); 
    o.printV_if_not_empty(kw_batch_id, s_batch_id); 
    o.printV_if_not_empty(kw_inppos_fn, s_inppos_fn); 
    o.printSw(kw_do_unkw, do_unkw);     
    o.printEnd(); 
  }

  #define help_inp_fn "Input filename without extension or the list of input filenames.  If it ends with \".lst\" it should contain the list of input filenames without extension."
  #define help_txt_ext "Filename extension of the tokenized text file(s)."
  #define help_cat_ext "Filename extension of the label file(s).  Required for target file generation."
  #define help_cat_dic_fn "Path to the label dictionary file (input).  The file should list up the labels used in the label files, one per each line.  Required for target file generation."
  #define help_voc_fn "Path to the vocabulary file generated by \"gen_vocab\" (input)."
  #define help_y_ext "Filename extension of the target file (output).  \".y\" | \".ysmat\".  Use \".ysmat\" when the number of classes is large."
  #define help_batch_id "Batch ID, e.g., \"1of5\" (the first batch out of 5), \"2of5\" (the second batch out of 5).  Specify this when making multiple files for one dataset."
  void printHelp(const AzOut &out) const {
    AzHelp h(out); 
    h.item_required(kw_inp_fn, help_inp_fn); 
    h.item_required(kw_txt_ext, help_txt_ext); 
    h.item_required(kw_cat_ext, help_cat_ext); 
    h.item_required(kw_cat_dic_fn, help_cat_dic_fn); 
    h.item_required(kw_voc_fn, help_voc_fn); 
    h.item_required(kw_rnm, "Pathname stem of the region vector file, target file, and vocabulary file (output).  To make the pathnames, the respective extensions will be attached."); 
    h.item(kw_y_ext, help_y_ext, ".y"); 
    h.item_required(kw_pch_sz, "Region size."); 
    h.item_required(kw_pch_step, "Region stride.", "1"); 
    h.item_required(kw_padding, "Padding size.  Region size minus one is recommended.", "0"); 
    h.item(kw_do_bow, "Generate region vectors for a Bow-convolution layer."); 
    h.item(kw_do_skip_stopunk, "Take variable strides.");      
    h.item(kw_do_lower, help_do_lower); 
    h.item(kw_do_utf8dashes, help_do_utf8dashes); 
    h.item(kw_do_allow_multi, "Allow multiple labels per data point for multi-label classification.  Data points without any label are also allowed.");  
    h.item(kw_do_allow_zero, "Allow zero vector to be a region vector."); 
    h.item(kw_do_region_only, "Generate a region file only.  Do not generate a target file.");
      
    h.item(kw_batch_id, help_batch_id); 
    h.end(); 
  }   
}; 

/*-------------------------------------------------------------------------*/
int AzPrepText::add_unkw(AzDic &dic) const
{
  AzStrPool sp_words(&dic.ref()); 
  AzBytArr s("__UNK__"); 
  for ( ; ; ) {
    if (dic.find(s.c_str()) < 0) break; 
    s << "_"; 
  }
  AzTimeLog::print("Adding ", s.c_str(), out); 
  int id = sp_words.put(&s); 
  dic.reset(&sp_words); 
  return id; 
}

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_regions(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText::gen_regions"; 

  AzPrepText_gen_regions_Param p(argc, argv, out);   
  check_y_ext(p.s_y_ext, eyec); 
  check_batch_id(p.s_batch_id); 
  AzDic dic_word(p.s_voc_fn.c_str()); /* read the vocabulary set */
  if (dic_word.size() <= 0) {
    throw new AzException(AzInputError, eyec, "empty dic: ", p.s_voc_fn.c_str()); 
  }
  int unkw_id = (p.do_unkw) ? add_unkw(dic_word) : -1; 
  
  int max_nn = dic_word.get_max_n(); 
  AzIntArr ia_nn; 
  if (max_nn != 1) {
    AzBytArr s("Vocabulary with "); s << max_nn << "-grams"; 
    AzPrint::writeln(out, s.c_str()); 
    AzX::no_support((max_nn == 0), eyec, "Empty vocabulary"); 
    for (int ix = 1; ix <= max_nn; ++ix) ia_nn.put(ix); 
  }
  AzDataArr<AzIntArr> aia_inppos;   
  bool do_pos = false; 
  if (p.s_inppos_fn.length() > 0) {
    AzTimeLog::print("Reading ", p.s_inppos_fn.c_str(), out); 
    AzFile::read(p.s_inppos_fn.c_str(), &aia_inppos);
    do_pos = true; 
  }
  AzX::no_support((p.do_unkw && (max_nn != 1 || !p.do_bow)), eyec, "Unkw with n-gram or sequential ...."); 

  AzDic dic_cat; 
  if (!p.do_region_only) dic_cat.reset(p.s_cat_dic_fn.c_str());  /* read categories */

  /*---  scan files to determine buffer size and #data  ---*/
  AzStrPool sp_list; 
  AzIntArr ia_data_num; 
  int buff_size = AzTools_text::scan_files_in_list(p.s_inp_fn.c_str(), p.s_txt_ext.c_str(), 
                                                   out, &sp_list, &ia_data_num); 
  int data_num = ia_data_num.sum(); 
  AzDataArr<AzIntArr> aia_outpos(data_num); 
  
  /*---  read data and generate features  ---*/
  AzSmat m_cat; 
  if (!p.do_region_only) m_cat.reform(dic_cat.size(), data_num); 
  AzSmat m_feat; 
  AzDataArr<AzSmat> am_feat(data_num); 
  AzX::throw_if((do_pos && aia_inppos.size() != data_num), AzInputError, eyec, kw_inppos_fn, "#data mismatch");  

  buff_size += 256; 
  AzBytArr s_buff; 
  AzByte *buff = s_buff.reset(buff_size, 0); 
  int no_cat = 0, multi_cat = 0; 
  int data_no = 0; 
  for (int fx = 0; fx < sp_list.size(); ++fx) { /* for ecah file */
    AzBytArr s_txt_fn(sp_list.c_str(fx), p.s_txt_ext.c_str()); 
    const char *fn = s_txt_fn.c_str(); 
    AzStrPool sp_cat;     
    if (!p.do_region_only) {
      AzBytArr s_cat_fn(sp_list.c_str(fx), p.s_cat_ext.c_str()); 
      AzTools::readList(s_cat_fn.c_str(), &sp_cat); 
    }
    AzTimeLog::print(fn, out);   
    AzFile file(fn); 
    file.open("rb"); 
    int num_in_file = 0; 
    for ( ; ; ++num_in_file) {  /* for each document */
      int len = file.gets(buff, buff_size); 
      if (len <= 0) break; 

      /*---  categories  ---*/      
      if (!p.do_region_only) {
        if (num_in_file >= sp_cat.size()) {
          throw new AzException(AzInputError, eyec, "#data mismatch: btw text file and cat file"); 
        }
        AzBytArr s_cat;     
        sp_cat.get(num_in_file, &s_cat);
         
        AzIntArr ia_cats; 
        AzBytArr s_err; 
        AzTools_text::parse_cats(&s_cat, '|', p.do_allow_multi, p.do_allow_nocat, 
                                 &dic_cat, &ia_cats, multi_cat, no_cat, s_err); 
        if (s_err.length() > 0) {
          if (p.do_ignore_bad) continue; 
          throw new AzException(AzInputError, eyec, s_err.c_str()); 
        }
        m_cat.col_u(data_no)->load(&ia_cats, 1);                               
      }
           
      /*---  text  ---*/
      AzSmat *m_feat = am_feat(data_no); 
      AzIntArr *ia_opos = aia_outpos(data_no); 
      if (max_nn > 1) { /* bi-gram, tri-grams ... */
        AzDataArr<AzIntArr> aia_xtokno; 
        AzTools_text::tokenize(buff, len, &dic_word, ia_nn, p.do_lower, p.do_utf8dashes, aia_xtokno);  
        if (do_pos) gen_bow_ngram_regions2(aia_xtokno, ia_nn, dic_word.size(), p.pch_sz, aia_inppos[data_no], m_feat, ia_opos);       
        else        gen_bow_ngram_regions(aia_xtokno, ia_nn, dic_word.size(), p.pch_sz, p.pch_step, p.padding, 
                                          p.do_skip_stopunk, p.do_allow_zero, m_feat, ia_opos);    
      }
      else {  /* uni-gram */
        AzIntArr ia_tokno; 
        int nn = 1; 
        AzTools_text::tokenize(buff, len, &dic_word, nn, p.do_lower, p.do_utf8dashes, &ia_tokno);                 
        if (p.do_bow) { /* Bow */
          if (do_pos) gen_bow_regions2(&ia_tokno, dic_word.size(), p.pch_sz, *aia_inppos[data_no], m_feat, ia_opos, unkw_id); 
          else        gen_bow_regions(&ia_tokno, dic_word.size(), p.pch_sz, p.pch_step, p.padding, 
                                      p.do_skip_stopunk, p.do_allow_zero, m_feat, ia_opos, unkw_id);      
        }
        else { /* Sequential */
          if (do_pos) gen_nobow_regions2(&ia_tokno, dic_word.size(), p.pch_sz, *aia_inppos[data_no], m_feat, ia_opos); 
          else        gen_nobow_regions(&ia_tokno, dic_word.size(), p.pch_sz, p.pch_step, p.padding, 
                                        p.do_allow_zero, p.do_skip_stopunk, m_feat, ia_opos); 
        }                                      
      }
      ++data_no;
    } /* for each doc */
    if (!p.do_region_only && num_in_file != sp_cat.size()) {
      throw new AzException(AzInputError, eyec, "#data mismatch2: btw text file and cat file"); 
    }       
  } /* for each file */
  if (!p.do_region_only) m_cat.resize(data_no); 
  cout << "#data=" << data_no << " no-cat=" << no_cat << " multi-cat=" << multi_cat << endl; 
  
  /*---  write files  ---*/
  const char *outnm = p.s_rnm.c_str(); 
  AzBytArr s_x_fn(outnm, x_ext), s_xtext_fn(outnm, xtext_ext); 
  if (p.s_batch_id.length() > 0) s_x_fn << "." << p.s_batch_id.c_str(); 
  const char *x_fn = s_x_fn.c_str(), *xtext_fn = s_xtext_fn.c_str(); 
  AzSmatVar mv_feat; mv_feat.transfer_from(&am_feat, data_no); 
  check_size(*mv_feat.data()); 
  am_feat.reset(); 
  AzBytArr s("Writing "); s << x_fn << " ("; mv_feat.shape_str(s); s << ") ... "; 
  AzTimeLog::print(s.c_str(), out); 
  mv_feat.write(x_fn); 

  if (p.do_bow || p.pch_sz == 1) dic_word.writeText(xtext_fn); 
  else { 
    AzDic xdic_nobow; 
    gen_nobow_dic(&dic_word, &mv_feat, &xdic_nobow); 
    xdic_nobow.writeText(xtext_fn); 
  }

  if (!p.do_region_only) { /* labeled data */
    AzBytArr s_y_fn(outnm, p.s_y_ext.c_str()); 
    write_Y(m_cat, s_y_fn, &p.s_batch_id); 
  }
  
  if (p.do_write_pos) {
    AzBytArr s_pos_fn(outnm, ".pos"); 
    if (p.s_batch_id.length() > 0) s_pos_fn << "." << p.s_batch_id.c_str();
    AzFile::write(s_pos_fn.c_str(), &aia_outpos);   
  }
  AzTimeLog::print("Done ... ", out); 
}

/*-------------------------------------------------------------------------*/
template <class Mat> /* Mat: AzSmat | AzSmatc */
void AzPrepText::check_size(const Mat &m) const {
  if (Az64::can_be_int(m.elmNum())) return; 

  AzBytArr s("!WARNING!: # of non-zero components exceeded 2GB ("); s << (double)m.elmNum()/(double)AzSigned32Max; 
  s << " times larger).  Either divide data or use \"datatype=sparse_large\" (or \"sparse_large_multi\") for training/testing."; 
  AzPrint::writeln(out, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"); 
  AzPrint::writeln(out, s.c_str()); 
  AzPrint::writeln(out, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"); 
}
template void AzPrepText::check_size<AzSmat>(const AzSmat &m) const; 
template void AzPrepText::check_size<AzSmatc>(const AzSmatc &m) const; 

/*-------------------------------------------------------------------------*/
void AzPrepText::check_batch_id(const AzBytArr &s_batch_id) const
{
  const char *eyec = "AzPrepText::check_batch_id"; 
  if (s_batch_id.length() <= 0) return; 
  AzBytArr s(kw_batch_id); s << " should look like \"1of5\""; 
  const char *batch_id = s_batch_id.c_str(); 
  const char *of_str = strstr(batch_id, "of"); 
  if (of_str == NULL) {
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }
  const char *wp; 
  for (wp = batch_id; wp < batch_id+s_batch_id.length(); ++wp) {
    if (wp >= of_str && wp < of_str+2) continue; 
    if (*wp < '0' || *wp > '9') throw new AzException(AzInputError, eyec, s.c_str());  
  }
  int batch_no = atol(batch_id); 
  int batch_num = atol(of_str+2); 
  if (batch_no < 1 || batch_no > batch_num) {
    throw new AzException(AzInputError, eyec, s.c_str(), " batch# must start with 1 and must not exceed the number of batches. "); 
  }
}  

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_nobow_regions(const AzIntArr *ia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_allow_zero, 
                       bool do_skip_stopunk, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) /* patch position: may be NULL */
const                       
{
  const char *eyec = "AzPrepText::gen_nobow_regions"; 
  int t_num; 
  const int *tokno = ia_tokno->point(&t_num); 
    
  int pch_num = DIVUP(t_num+padding*2-pch_sz, pch_step) + 1; 
  m_feat->reform(dic_sz*pch_sz, pch_num);   
  if (ia_pos != NULL) ia_pos->reset(); 

  int col = 0; 
  int tx0 = -padding; 
  for (int pch_no = 0; pch_no < pch_num; ++pch_no) {
    int tx1 = tx0 + pch_sz; 
    
    AzSmat m; 
    for (int tx = tx0; tx < tx1; ++tx) {
      AzSmat m0(dic_sz, 1); 
      if (tx >= 0 && tx < t_num && tokno[tx] >= 0) {
        AzIntArr ia_row; ia_row.put(tokno[tx]); 
        m0.col_u(0)->load(&ia_row, 1); 
      }
      if (tx == tx0) m.set(&m0); 
      else           m.rbind(&m0); 
    }
    if (do_allow_zero || !m.isZero()) {
      m_feat->col_u(col)->set(m.col(0)); 
      if (ia_pos != NULL) ia_pos->put(tx0); 
      ++col; 
    }
   
    if (tx1 >= t_num+padding) break;

    int dist = 1; 
    if (do_skip_stopunk) {
      /*---  to avoid repeating the same bow  ---*/
      int tx; 
      for (tx = tx0; tx < t_num; ++tx) if (tx >= 0 && tokno[tx] >= 0) break;   
      int dist0 = tx-tx0+1; /* to lose a word, we have to slide a window this much */

      for (tx = tx1; tx < t_num; ++tx) if (tx >= 0 && tokno[tx] >= 0) break; /* corrected on 6/15/2014 */
      int dist1 = tx-tx1+1; /* to get a new word, we have to slide a window this much */
      dist = MIN(dist0, dist1); 
    }
    tx0 += MAX(dist, pch_step);     
  }
  m_feat->resize(col);    
}

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_nobow_regions2(const AzIntArr *ia_tokno, int dic_sz, 
                       int pch_sz, const AzIntArr &ia_tx0,  
                       /*---  output  ---*/
                       AzSmat *m_feat, AzIntArr *ia_pos) const {
  int t_num; const int *tokno = ia_tokno->point(&t_num); 
  m_feat->reform(dic_sz*pch_sz, ia_tx0.size()); 
  if (ia_pos != NULL) ia_pos->reset(); 
 
  for (int col = 0; col < ia_tx0.size(); ++col) {
    int tx0 = ia_tx0[col], tx1 = tx0 + pch_sz;  
    AzSmat m; 
    for (int tx = tx0; tx < tx1; ++tx) {
      AzSmat m0(dic_sz, 1); 
      if (tx >= 0 && tx < t_num && tokno[tx] >= 0) {
        AzIntArr ia_row; ia_row.put(tokno[tx]); 
        m0.col_u(0)->load(&ia_row, 1); 
      }
      if (tx == tx0) m.set(&m0); 
      else           m.rbind(&m0); 
    }
    m_feat->col_u(col)->set(m.col(0)); 
    if (ia_pos != NULL) ia_pos->put(tx0);     
  }
}

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_bow_regions(const AzIntArr *ia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_skip_stopunk, bool do_allow_zero, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos, /* patch position: may be NULL */
                       /*---  optional input  ---*/
                       int unk_id) 
const                       
{
  const char *eyec = "AzPrepText::gen_bow_regions"; 
  int t_num; 
  const int *tokno = ia_tokno->point(&t_num); 
    
  int pch_num = DIVUP(t_num+padding*2-pch_sz, pch_step) + 1; 
  m_feat->reform(dic_sz, pch_num);   
  if (ia_pos != NULL) ia_pos->reset(); 
  
  int pch_no; 
  int col = 0; 
  int tx0 = -padding; 
  for (pch_no = 0; pch_no < pch_num; ++pch_no) {
    int tx1 = tx0 + pch_sz; 
    
    AzIntArr ia_rows; 
    int tx; 
    for (tx = MAX(0, tx0); tx < MIN(t_num, tx1); ++tx) {
      if (tokno[tx] >= 0)   ia_rows.put(tokno[tx]); 
      else if (unk_id >= 0) ia_rows.put(unk_id); 
    }
    if (do_allow_zero || !do_skip_stopunk || ia_rows.size() > 0) {
      ia_rows.unique();  /* sorting too */
      m_feat->col_u(col)->load(&ia_rows, 1); 
      if (ia_pos != NULL) ia_pos->put(tx0); 
      ++col; 
    }
   
    if (tx1 >= t_num+padding) break;
    
    int dist = 1; 
    if (do_skip_stopunk) {
      /*---  to avoid repeating the same bow  ---*/
      int tx; 
      for (tx = tx0; tx < t_num; ++tx) if (tx >= 0 && tokno[tx] >= 0) break;   
      int dist0 = tx-tx0+1; /* to lose a word, we have to slide a window this much */

      tx = tx1; 
      for (tx = tx1; tx < t_num; ++tx) if (tx >= 0 && tokno[tx] >= 0) break; /* corrected on 6/15/2014 */
      int dist1 = tx-tx1+1; /* to get a new word, we have to slide a window this much */
      dist = MIN(dist0, dist1); 
    }
    tx0 += MAX(dist, pch_step); 
  }
  m_feat->resize(col);     
} 

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_bow_regions2(const AzIntArr *ia_tokno, int dic_sz, 
                       int pch_sz, const AzIntArr &ia_tx0,  
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos, /* patch position: may be NULL */
                       /*---  optional input  ---*/
                       int unk_id) const {
  int t_num; const int *tokno = ia_tokno->point(&t_num); 
  m_feat->reform(dic_sz, ia_tx0.size()); 
  if (ia_pos != NULL) ia_pos->reset(); 
  for (int col = 0; col < ia_tx0.size(); ++col) {
    int tx0 = ia_tx0[col], tx1 = tx0 + pch_sz; 
    AzIntArr ia_rows; 
    for (int tx = MAX(0, tx0); tx < MIN(t_num, tx1); ++tx) {
      if (tokno[tx] >= 0)   ia_rows.put(tokno[tx]); 
      else if (unk_id >= 0) ia_rows.put(unk_id); 
    }
    ia_rows.unique();  /* sorting too */
    m_feat->col_u(col)->load(&ia_rows, 1); 
    if (ia_pos != NULL) ia_pos->put(tx0); 
  }  
} 

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_bow_ngram_regions(const AzDataArr<AzIntArr> &aia_tokno, 
                       const AzIntArr &ia_nn, 
                       int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_skip_stopunk, bool do_allow_zero, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) /* patch position: may be NULL */
const                       
{
  const char *eyec = "AzPrepText::gen_bow_ngram_regions"; 
  int t_num = (aia_tokno.size() > 0) ? aia_tokno[0]->size() : 0; 
    
  int pch_num = DIVUP(t_num+padding*2-pch_sz, pch_step) + 1; 
  m_feat->reform(dic_sz, pch_num);   
  if (ia_pos != NULL) ia_pos->reset(); 
  
  int pch_no; 
  int col = 0; 
  int tx0 = -padding; 
  for (pch_no = 0; pch_no < pch_num; ++pch_no) {
    int tx1 = tx0 + pch_sz; 
    
    AzIntArr ia_rows; 
    int tx; 
    for (tx = MAX(0, tx0); tx < MIN(t_num, tx1); ++tx) {
      for (int nx = 0; nx < aia_tokno.size(); ++nx) {
        const AzIntArr *ia_tokno = aia_tokno[nx]; 
        if ((*ia_tokno)[tx] >= 0) ia_rows.put((*ia_tokno)[tx]); 
      }
    }
    if (do_allow_zero || !do_skip_stopunk || ia_rows.size() > 0) {
      ia_rows.unique();  /* sorting too */
      m_feat->col_u(col)->load(&ia_rows, 1); 
      if (ia_pos != NULL) ia_pos->put(tx0); 
      ++col; 
    }
   
    if (tx1 >= t_num+padding) break;
    
    int dist = 1; 
    if (do_skip_stopunk) {
      /*---  to avoid repeating the same bow  ---*/
      int tx; 
      for (tx = MAX(0,tx0); tx < t_num; ++tx) {
        int nx; 
        for (nx = 0; nx < aia_tokno.size(); ++nx) if (aia_tokno[nx]->get(tx) >= 0) break; 
        if (nx < aia_tokno.size()) break; 
      }
      int dist0 = tx-tx0+1; /* to lose a word, we have to slide a window this much */

      tx = tx1; 
      for (tx = MAX(0,tx1); tx < t_num; ++tx) {
        int nx; 
        for (nx = 0; nx < aia_tokno.size(); ++nx) if (aia_tokno[nx]->get(tx) >= 0) break; 
        if (nx < aia_tokno.size()) break; 
      }
      int dist1 = tx-tx1+1; /* to get a new word, we have to slide a window this much */
      dist = MIN(dist0, dist1); 
    }
    tx0 += MAX(dist, pch_step); 
  }
  m_feat->resize(col);     
} 

/*-------------------------------------------------------------------------*/
void AzPrepText::gen_bow_ngram_regions2(const AzDataArr<AzIntArr> &aia_tokno, 
                       const AzIntArr &ia_nn, 
                       int dic_sz, int pch_sz, 
                       const AzIntArr *ia_tx0, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const {
  const char *eyec = "AzPrepText::gen_bow_ngram_regions"; 
  if (ia_tx0 == NULL) throw new AzException(eyec, "null ptr"); 
  int t_num = (aia_tokno.size() > 0) ? aia_tokno[0]->size() : 0; 
  m_feat->reform(dic_sz, ia_tx0->size());   
  if (ia_pos != NULL) ia_pos->reset(); 
  for (int col = 0; col < ia_tx0->size(); ++col) {
    int tx0 = (*ia_tx0)[col]; 
    int tx1 = tx0 + pch_sz; 
    
    AzIntArr ia_rows; 
    for (int tx = MAX(0, tx0); tx < MIN(t_num, tx1); ++tx) {
      for (int nx = 0; nx < aia_tokno.size(); ++nx) {
        const AzIntArr *ia_tokno = aia_tokno[nx]; 
        if ((*ia_tokno)[tx] >= 0) ia_rows.put((*ia_tokno)[tx]); 
      }
    }
    ia_rows.unique();  /* sorting too */
    m_feat->col_u(col)->load(&ia_rows, 1); 
    if (ia_pos != NULL) ia_pos->put(tx0); 
  }  
} 

/*-------------------------------------------------------------------------*/
/* static */
void AzPrepText::gen_nobow_dic(const AzDic *inp_dic, int pch_sz, AzDic *out_dic)
{
  for (int px = 0; px < pch_sz; ++px) {
    AzBytArr s_pref; s_pref << px << ":"; 
    AzDic tmp_dic; tmp_dic.reset(inp_dic); 
    tmp_dic.add_prefix(s_pref.c_str()); 
    if (px == 0) out_dic->reset(&tmp_dic); 
    else         out_dic->append(&tmp_dic); 
  }
}  
    
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
class AzPrepText_show_regions_Param : public virtual AzPrepText_Param_ {
public:
  AzBytArr s_rnm; 
  bool do_wordonly; 
  AzPrepText_show_regions_Param(int argc, const char *argv[], const AzOut &out) 
    : do_wordonly(false) {
    reset(argc, argv, out); 
  } 
  #define kw_do_wordonly "ShowWordOnly"
  void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_show_regions_Param::resetParam"; 
    azp.vStr(kw_rnm, &s_rnm); 
    azp.swOn(&do_wordonly, kw_do_wordonly); 
    AzXi::throw_if_empty(s_rnm, eyec, kw_rnm); 
  }
  void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_rnm, s_rnm); 
    o.printSw(kw_do_wordonly, do_wordonly); 
    o.printEnd(); 
  }
  void printHelp(const AzOut &out) const {
    AzHelp h(out); 
    h.item_required(kw_rnm, "Filename stem of the region vector file generated by \"gen_regions\"."); 
    h.item(kw_do_wordonly, "Show words only without values."); 
    h.end(); 
  }
}; 

/*-------------------------------------------------------------------------*/
void AzPrepText::show_regions(int argc, const char *argv[]) const 
{
  AzPrepText_show_regions_Param p(argc, argv, out);   
  
  AzBytArr s_x_fn(p.s_rnm.c_str(), x_ext), s_xtext_fn(p.s_rnm.c_str(), xtext_ext); 
  AzSmatVar mv(s_x_fn.c_str()); 
  AzDic dic(s_xtext_fn.c_str()); 
  _show_regions(&mv, &dic, p.do_wordonly); 
}

/*-------------------------------------------------------------------------*/
void AzPrepText::_show_regions(const AzSmatVar *mv, 
                               const AzDic *dic,
                               bool do_wordonly) const
{
  const AzSmat *ms = mv->data(); 
  AzBytArr s; s << "#data=" << mv->dataNum() << " #col=" << ms->colNum(); 
  AzPrint::writeln(out, s); 
  for (int dx = 0; dx < mv->dataNum(); ++dx) {
    int col0 = mv->col_begin(dx), col1 = mv->col_end(dx); 
    s.reset("data#"); s << dx; s.nl(); 
    AzPrint::writeln(out, s); 
    int col; 
    for (col = col0; col < col1; ++col) {
      s.reset("  "); 
      AzTools_text::feat_to_text(ms->col(col), dic, do_wordonly, s); 
      AzPrint::writeln(out, s); 
    }
  }
} 
 
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
class AzPrepText_gen_nbw_Param : public virtual AzPrepText_Param_ {
public:
  AzBytArr s_trn_fn, s_txt_ext, s_cat_ext, s_cat_dic_fn, s_voc_fn; 
  AzBytArr s_nbw_fn; 
  double alpha; 
  bool do_lower, do_utf8dashes, do_ignore_bad; 
  
  AzPrepText_gen_nbw_Param(int argc, const char *argv[], const AzOut &out) 
    : alpha(1), do_lower(false), do_utf8dashes(false), do_ignore_bad(false) {
    reset(argc, argv, out); 
  }      
  /*-------------------------------------------------------------------------*/
  #define kw_trn_fn "train_fn="
  #define kw_nbw_fn "nbw_fn="
  #define kw_alpha "alpha="
  #define kw_cat_ext "label_fn_ext="
  #define kw_cat_dic_fn "label_dic_fn="
  /*-------------------------------------------------------------------------*/  
  virtual void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_gen_nbw_Param::resetParam"; 
    azp.vStr(kw_voc_fn, &s_voc_fn); 
    azp.vStr(kw_trn_fn, &s_trn_fn); 
    azp.vStr(kw_nbw_fn, &s_nbw_fn);     
    azp.vStr(kw_txt_ext, &s_txt_ext); 
    azp.vStr(kw_cat_ext, &s_cat_ext);     
    azp.vStr(kw_cat_dic_fn, &s_cat_dic_fn);     
    azp.vFloat(kw_alpha, &alpha); 
    azp.swOn(&do_lower, kw_do_lower); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes); 
    azp.swOn(&do_ignore_bad, kw_do_ignore_bad); 
    
    AzXi::throw_if_empty(s_voc_fn, eyec, kw_voc_fn); 
    AzXi::throw_if_empty(s_trn_fn, eyec, kw_trn_fn);    
    AzXi::throw_if_empty(s_nbw_fn, eyec, kw_nbw_fn);   
    AzXi::throw_if_empty(s_txt_ext, eyec, kw_txt_ext);  
    AzXi::throw_if_empty(s_cat_ext, eyec, kw_cat_ext);  
    AzXi::throw_if_empty(s_cat_dic_fn, eyec, kw_cat_dic_fn);    
  }
  virtual void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_voc_fn, s_voc_fn); 
    o.printV(kw_trn_fn, s_trn_fn); 
    o.printV(kw_nbw_fn, s_nbw_fn);     
    o.printV(kw_txt_ext, s_txt_ext);  
    o.printV(kw_cat_ext, s_cat_ext);   
    o.printV(kw_cat_dic_fn, s_cat_dic_fn);    
    o.printV(kw_alpha, alpha); 
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printSw(kw_do_ignore_bad, do_ignore_bad); 
    o.printEnd(); 
  } 
  
  void printHelp(const AzOut &out) const {  
    AzHelp h(out); 
    h.begin("", "", ""); 
    h.item_required(kw_voc_fn, help_voc_fn); 
    h.item_required(kw_trn_fn, help_inp_fn); 
    h.item_required(kw_nbw_fn, "Path to the file which the NB weights will be written to (output).  If it ends with \"dmat\", it is written in the dense matrix formatn and can be used in \"gen_nbw\".  If not, a text file is generated for browsing the NB-weights but not for use in \"gen_nbw\"."); 
    h.item_required(kw_txt_ext, help_txt_ext); 
    h.item_required(kw_cat_ext, help_cat_ext); 
    h.item_required(kw_cat_dic_fn, help_cat_dic_fn); 
    h.item(kw_alpha, "The value that should be added for smoothing", "1"); 
    h.item(kw_do_lower, help_do_lower);     
    h.item(kw_do_utf8dashes, help_do_utf8dashes);    
    h.end(); 
  }  
}; 
/*-------------------------------------------------------------------------*/
void AzPrepText::gen_nbw(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText::gen_nbw"; 

  AzPrepText_gen_nbw_Param p(argc, argv, out);   
  AzDic dic(p.s_voc_fn.c_str()); /* vocabulary */
  AzDic dic_cat(p.s_cat_dic_fn.c_str());  /* read categories */
  if (dic_cat.size() < 2) {
    throw new AzException(AzInputError, eyec, "#class must be no smaller than 2"); 
  }
  
  bool do_allow_multi = false, do_allow_nocat = false; 
  
  int nn = dic.get_max_n(); 
  AzPrint::writeln(out, "max n for n-gram in the vocabulary file: ", nn); 

  /*---  read training data  ---*/
  bool do_count_unk = false; 
  AzSmat m_trn_count, m_trn_cat; 
  AzTools_text::count_words_get_cats(out, p.do_ignore_bad, do_count_unk, 
        p.s_trn_fn.c_str(), p.s_txt_ext.c_str(), p.s_cat_ext.c_str(), 
        dic, dic_cat, nn, 
        do_allow_multi, do_allow_nocat, p.do_lower, p.do_utf8dashes, 
        &m_trn_count, &m_trn_cat); 

  AzDmat m_trn_sum(m_trn_count.rowNum(), m_trn_cat.rowNum()); 
  for (int col = 0; col < m_trn_count.colNum(); ++col) {
    int cat; 
    m_trn_cat.col(col)->max(&cat); 
    m_trn_sum.col_u(cat)->add(m_trn_count.col(col)); 
  }
  AzDmat m_val(dic.size(), dic_cat.size()); 
  for (int cat = 0; cat < dic_cat.size(); ++cat) {
    AzDvect v_posi(m_trn_sum.col(cat)); 
    AzDvect v_nega(m_trn_sum.rowNum()); 
    for (int nega = 0; nega < dic_cat.size(); ++nega) { /* one vs. others */
      if (nega == cat) continue; 
      v_nega.add(m_trn_sum.col(nega)); 
    }
    v_posi.add(p.alpha); v_posi.normalize1(); /* add smoothing factor and divide by sum */
    v_nega.add(p.alpha); v_nega.normalize1(); 
    double *val = m_val.col_u(cat)->point_u();  
    for (int row = 0; row < m_val.rowNum(); ++row) {
      val[row] = log(v_posi.get(row)/v_nega.get(row)); 
    }
  }  
  /*---  write nb-weights  ---*/
  if (p.s_nbw_fn.endsWith("dmat")) {
    AzTimeLog::print("Writing nbw (dmat): ", p.s_nbw_fn.c_str(), out); 
    m_val.write(p.s_nbw_fn.c_str()); 
  }
  else {
    AzTimeLog::print("Writing nbw (text): ", p.s_nbw_fn.c_str(), out); 
    int digits = 5; 
    m_val.writeText(p.s_nbw_fn.c_str(), digits); 
  }
  AzTimeLog::print("Done ... ", out); 
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
class AzPrepText_gen_nbwfeat_Param : public virtual AzPrepText_Param_ {
public:
  AzBytArr s_inp_fn, s_txt_ext, s_cat_ext, s_cat_dic_fn, s_voc_fn; 
  AzBytArr s_outnm, s_x_ext, s_y_ext, s_nbw_fn, s_batch_id; 
  bool do_lower, do_utf8dashes, do_no_cat, do_ignore_bad; 
  
  AzPrepText_gen_nbwfeat_Param(int argc, const char *argv[], const AzOut &out) 
    : do_lower(false), do_utf8dashes(false), do_no_cat(false), s_x_ext(".xsmatvar"), s_y_ext(".y"), do_ignore_bad(false) {
    reset(argc, argv, out); 
  }      
  /*-------------------------------------------------------------------------*/
  #define kw_inp_fn "input_fn="
  #define kw_outnm "output_fn_stem="
  #define kw_x_ext "x_ext="
  #define kw_do_no_cat "NoCat"
  #define help_x_ext "Extension of the feature file.  \".xsmatvar\" (binary; for CNN) | \".x\" (text format; for NN)."
  #define help_do_no_cat "Do not make a target file."
  #define help_outnm "Pathname stem of the feature file and target file (output).  To make the pathnames of the feature file and target file, the respective extensions will be attached."
  /*-------------------------------------------------------------------------*/  
  virtual void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_gen_nbwfeat_Param::resetParam"; 
    azp.vStr(kw_voc_fn, &s_voc_fn); 
    azp.vStr(kw_inp_fn, &s_inp_fn); 
    azp.vStr(kw_nbw_fn, &s_nbw_fn);     
    azp.vStr(kw_txt_ext, &s_txt_ext); 
    azp.vStr(kw_cat_ext, &s_cat_ext);     
    azp.vStr(kw_cat_dic_fn, &s_cat_dic_fn);     
    azp.vStr(kw_outnm, &s_outnm); 
    azp.vStr(kw_x_ext, &s_x_ext); 
    azp.vStr(kw_y_ext, &s_y_ext);     
    azp.swOn(&do_lower, kw_do_lower); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes); 
    azp.swOn(&do_no_cat, kw_do_no_cat, false); 
    azp.swOn(&do_ignore_bad, kw_do_ignore_bad); 
    azp.vStr(kw_batch_id, &s_batch_id); 

    AzXi::throw_if_empty(s_voc_fn, eyec, kw_voc_fn); 
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn); 
    AzXi::throw_if_empty(s_outnm, eyec, kw_outnm); 
    AzXi::throw_if_empty(s_x_ext, eyec, kw_x_ext); 
    AzXi::throw_if_empty(s_y_ext, eyec, kw_y_ext);      
    AzXi::throw_if_empty(s_nbw_fn, eyec, kw_nbw_fn);       
    if (!do_no_cat) AzXi::throw_if_empty(s_cat_ext, eyec, kw_cat_ext);  
    AzXi::throw_if_empty(s_cat_dic_fn, eyec, kw_cat_dic_fn);    
  }
  virtual void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_voc_fn, s_voc_fn); 
    o.printV(kw_inp_fn, s_inp_fn); 
    o.printV(kw_txt_ext, s_txt_ext);  
    o.printV(kw_cat_ext, s_cat_ext);   
    o.printV(kw_cat_dic_fn, s_cat_dic_fn);       
    o.printV(kw_nbw_fn, s_nbw_fn);     
    o.printV(kw_outnm, s_outnm); 
    o.printV(kw_x_ext, s_x_ext); 
    o.printV(kw_y_ext, s_y_ext);     
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printSw(kw_do_no_cat, do_no_cat); 
    o.printSw(kw_do_ignore_bad, do_ignore_bad); 
    o.printV_if_not_empty(kw_batch_id, s_batch_id); 
    o.printEnd(); 
  } 
  
  void printHelp(const AzOut &out) const {
    AzHelp h(out); 
    h.begin("", "", ""); 
    h.item_required(kw_voc_fn, help_voc_fn); 
    h.item_required(kw_inp_fn, help_inp_fn); 
    h.item_required(kw_nbw_fn, "Path to the file of the NB weights (input).  It should end with \"dmat\""); 
    h.item(kw_txt_ext, help_txt_ext); 
    h.item_required(kw_cat_ext, help_cat_ext); 
    h.item_required(kw_cat_dic_fn, help_cat_dic_fn); 
    h.item_required(kw_outnm, help_outnm); 
    h.item(kw_x_ext, help_x_ext, ".xsmatvar"); 
    h.item(kw_y_ext, help_y_ext, ".y"); 
    h.item(kw_do_lower, help_do_lower);     
    h.item(kw_do_utf8dashes, help_do_utf8dashes);    
    h.item(kw_do_no_cat, help_do_no_cat); 
    h.item(kw_batch_id, help_batch_id); 
    h.end(); 
  }   
}; 
/*-------------------------------------------------------------------------*/
void AzPrepText::gen_nbwfeat(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText::gen_nbwfeat"; 

  AzPrepText_gen_nbwfeat_Param p(argc, argv, out);  
  check_y_ext(p.s_y_ext, eyec);  
  check_batch_id(p.s_batch_id);   
  AzDic dic(p.s_voc_fn.c_str()); /* vocabulary */
  AzDic dic_cat(p.s_cat_dic_fn.c_str());  /* read categories */
  if (dic_cat.size() < 2) {
    throw new AzException(AzInputError, eyec, "#class must be no smaller than 2"); 
  }

  if (!p.s_nbw_fn.endsWith("dmat")) {
    throw new AzException(AzInputError, eyec, kw_nbw_fn, " should end with \"dmat\"");     
  }  
  AzTimeLog::print("Reading ", p.s_nbw_fn.c_str(), out); 
  AzDmat m_val;   
  m_val.read(p.s_nbw_fn.c_str()); 

  if (m_val.rowNum() != dic.size() || m_val.colNum() != dic_cat.size()) {
    AzBytArr s("Conflict in NB-weight dimensions.  Expected: "); 
    s << dic.size() << " x " << dic_cat.size() << "; actual: " << m_val.rowNum() << " x " << m_val.colNum(); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }
    
  bool do_allow_multi = false, do_allow_nocat = false; 
  int nn = dic.get_max_n(); 
  AzPrint::writeln(out, "max n for n-gram in the vocabulary file: ", nn); 
  
  /*---  read data  ---*/
  bool do_count_unk = false; 
  AzSmat m_count, m_cat; 
  AzTools_text::count_words_get_cats(out, p.do_ignore_bad, do_count_unk, 
        p.s_inp_fn.c_str(), p.s_txt_ext.c_str(), p.s_cat_ext.c_str(), 
        dic, dic_cat, nn, 
        do_allow_multi, do_allow_nocat, p.do_lower, p.do_utf8dashes, 
        &m_count, &m_cat, p.do_no_cat); 

  AzTimeLog::print("Binarizing ... ", out); 
  m_count.binarize(); /* binary features */

  /*---  multiply nb-weights and write files  ---*/
  for (int cat = 0; cat < dic_cat.size(); ++cat) {
    AzTimeLog::print("Cat", cat, out); 
    AzSmat m_feat(&m_count); 
    scale(&m_feat, m_val.col(cat));  /* multiply NB-weights */
 
    /*---  generate binary labels for one vs. others training  ---*/
    AzSmat m_bcat(2, m_cat.colNum()); 
    for (int col = 0; col < m_cat.colNum(); ++col) {
      if (m_cat.get(cat, col) == 1) m_bcat.set((int)0, (int)col, (double)1); /* positive */
      else                          m_bcat.set((int)1, (int)col, (double)1); /* negative */
    }
    
    /*---  write files  ---*/
    AzBytArr s_x_fn(p.s_outnm.c_str()), s_y_fn(p.s_outnm.c_str()); 
    if (dic_cat.size() != 2) {
      s_x_fn << ".cat" << cat; s_y_fn << ".cat" << cat; 
    }
    s_x_fn << p.s_x_ext.c_str(); 
    write_X(m_feat, s_x_fn, &p.s_batch_id); 

    s_y_fn << p.s_y_ext.c_str();
    write_Y(m_bcat, s_y_fn, &p.s_batch_id); 

    AzBytArr s_xtext_fn(p.s_outnm.c_str(), xtext_ext);     
    dic.writeText(s_xtext_fn.c_str(), false);     
    if (dic_cat.size() == 2) break; 
  }
  if (dic_cat.size() > 2) {
    AzBytArr s_y_fn(p.s_outnm.c_str(), p.s_y_ext.c_str()); 
    write_Y(m_cat, s_y_fn);       
  }
}

/*-------------------------------------------------------------------------*/ 
void AzPrepText::scale(AzSmat *ms, const AzDvect *v) 
{
  for (int col = 0; col < ms->colNum(); ++col) {
    AzIFarr ifa; 
    ms->col(col)->nonZero(&ifa); 
    AzIFarr ifa_new; ifa_new.prepare(ifa.size()); 
    for (int ix = 0; ix < ifa.size(); ++ix) {
      double val = ifa.get(ix); 
      int row = ifa.getInt(ix); 
      ifa_new.put(row, val*v->get(row)); 
    }
    ms->col_u(col)->load(&ifa_new); 
  }
}

/*-------------------------------------------------------------------------*/
void AzPrepText::smat_to_smatvar(const AzSmat *ms, AzSmatVar *msv)
{
  AzIntArr ia_colind; 
  for (int dx = 0; dx < ms->colNum(); ++dx) {
    ia_colind.put(dx); 
    ia_colind.put(dx+1); 
  }
  msv->reset(ms, &ia_colind); 
}  

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
class AzPrepText_gen_b_feat_Param : public virtual AzPrepText_Param_ {
public:
  AzBytArr s_inp_fn, s_txt_ext, s_cat_ext, s_cat_dic_fn, s_voc_fn; 
  AzBytArr s_outnm, s_x_ext, s_y_ext, s_batch_id; 
  bool do_log, do_bin, do_unit; 
  bool do_lower, do_utf8dashes, do_no_cat, do_ignore_bad; 
  
  AzPrepText_gen_b_feat_Param(int argc, const char *argv[], const AzOut &out) 
    : do_lower(false), do_utf8dashes(false), s_x_ext(".x"), s_y_ext(".y"), 
      do_no_cat(false), do_log(false), do_bin(false), do_unit(false), do_ignore_bad(false) {
    reset(argc, argv, out); 
  }      
  /*-------------------------------------------------------------------------*/   
  #define kw_outnm "output_fn_stem="
  #define kw_x_ext "x_ext="
  #define kw_do_no_cat "NoCat"  
  #define kw_do_log "LogCount"
  #define kw_do_bin "Binary"
  #define kw_do_unit "Unit"
  /*-------------------------------------------------------------------------*/  
  virtual void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_b_gen_b_feat_Param::resetParam"; 
    azp.vStr(kw_voc_fn, &s_voc_fn); 
    azp.vStr(kw_inp_fn, &s_inp_fn); 
    azp.vStr(kw_txt_ext, &s_txt_ext); 
    azp.vStr(kw_cat_ext, &s_cat_ext);     
    azp.vStr(kw_cat_dic_fn, &s_cat_dic_fn);     
    azp.vStr(kw_outnm, &s_outnm); 
    azp.vStr(kw_x_ext, &s_x_ext); 
    azp.vStr(kw_y_ext, &s_y_ext);     
    azp.swOn(&do_lower, kw_do_lower); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes); 
    azp.swOn(&do_no_cat, kw_do_no_cat, false); 
    azp.swOn(&do_log, kw_do_log); 
    azp.swOn(&do_bin, kw_do_bin); 
    azp.swOn(&do_unit, kw_do_unit); 
    azp.swOn(&do_ignore_bad, kw_do_ignore_bad); 
    azp.vStr(kw_batch_id, &s_batch_id); 

    AzXi::throw_if_empty(s_voc_fn, eyec, kw_voc_fn); 
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn); 
    AzXi::throw_if_empty(s_outnm, eyec, kw_outnm);  
    AzXi::throw_if_empty(s_x_ext, eyec, kw_x_ext);  
    AzXi::throw_if_empty(s_y_ext, eyec, kw_y_ext);         
    if (!do_no_cat) {
      AzXi::throw_if_empty(s_cat_ext, eyec, kw_cat_ext);  
      AzXi::throw_if_empty(s_cat_dic_fn, eyec, kw_cat_dic_fn);    
    }
    if (do_log && do_bin) {
      throw new AzException(AzInputError, eyec, "mutually exclusive switches: ", kw_do_log, kw_do_bin);  
    }
  }
  virtual void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_voc_fn, s_voc_fn); 
    o.printV(kw_inp_fn, s_inp_fn);    
    o.printV(kw_txt_ext, s_txt_ext);  
    o.printV(kw_cat_ext, s_cat_ext);   
    o.printV(kw_cat_dic_fn, s_cat_dic_fn);    
    o.printV(kw_outnm, s_outnm); 
    o.printV(kw_x_ext, s_x_ext);      
    o.printV(kw_y_ext, s_y_ext);     
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printSw(kw_do_no_cat, do_no_cat); 
    o.printSw(kw_do_log, do_log); 
    o.printSw(kw_do_bin, do_bin); 
    o.printSw(kw_do_unit, do_unit); 
    o.printSw(kw_do_ignore_bad, do_ignore_bad); 
    o.printV_if_not_empty(kw_batch_id, s_batch_id); 
    o.printEnd(); 
  } 
  
  void printHelp(const AzOut &out) const {
    AzHelp h(out); 
    h.begin("", "", ""); 
    h.item_required(kw_voc_fn, help_voc_fn); 
    h.item_required(kw_inp_fn, help_inp_fn); 
    h.item(kw_txt_ext, help_txt_ext); 
    h.item_required(kw_cat_ext, help_cat_ext); 
    h.item_required(kw_cat_dic_fn, help_cat_dic_fn); 
    h.item_required(kw_outnm, help_outnm); 
    h.item(kw_x_ext, help_x_ext, ".x"); 
    h.item(kw_y_ext, help_y_ext, ".y"); 
    h.item(kw_do_lower, help_do_lower);     
    h.item(kw_do_utf8dashes, help_do_utf8dashes);    
    h.item(kw_do_no_cat, help_do_no_cat); 
    h.item(kw_do_log, "Set components to log(x+1) where x is the frequency of the word in each document."); 
    h.item(kw_do_bin, "Set components to 1 if the word appears in the document; 0 otherwise."); 
    h.item(kw_do_unit, "Scale the feature vectors to unit vectors."); 
    h.item(kw_batch_id, help_batch_id); 
    h.end(); 
  }   
}; 
/*-------------------------------------------------------------------------*/
void AzPrepText::gen_b_feat(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText::gen_b_feat"; 

  AzPrepText_gen_b_feat_Param p(argc, argv, out);   
  check_y_ext(p.s_y_ext, eyec);  
  check_batch_id(p.s_batch_id); 
  AzDic dic(p.s_voc_fn.c_str()); /* vocabulary */
  AzDic dic_cat(p.s_cat_dic_fn.c_str());  /* read categories */
    
  bool do_allow_multi = false, do_allow_nocat = false; 
  int nn=dic.get_max_n(); 
  AzPrint::writeln(out, "max n for n-gram in the vocabulary file: ", nn); 
  
  /*---  read data  ---*/
  bool do_unk = false; 
  AzSmat m_feat, m_cat; 
  AzTools_text::count_words_get_cats(out, p.do_ignore_bad, do_unk, 
        p.s_inp_fn.c_str(), p.s_txt_ext.c_str(), p.s_cat_ext.c_str(), 
        dic, dic_cat, nn, 
        do_allow_multi, do_allow_nocat, p.do_lower, p.do_utf8dashes, 
        &m_feat, &m_cat, p.do_no_cat); 

  /*---  convert features if requested  ---*/
  if (p.do_bin) {
    AzTimeLog::print("Binarizing ... ", out); 
    m_feat.binarize(); /* binary features */
  }
  else if (p.do_log) {
    AzTimeLog::print("log(x+1) ... ", out); 
    m_feat.plus_one_log(); 
  }
  if (p.do_unit) {
    AzTimeLog::print("Converting to unit vectors ... ", out); 
    m_feat.normalize();  
  }

  /*---  write files  ---*/
  AzBytArr s_x_fn(p.s_outnm.c_str(), p.s_x_ext.c_str()); 
  write_X(m_feat, s_x_fn, &p.s_batch_id); 

  AzBytArr s_xtext_fn(p.s_outnm.c_str(), xtext_ext);   
  dic.writeText(s_xtext_fn.c_str(), false);     

  AzBytArr s_y_fn(p.s_outnm.c_str(), p.s_y_ext.c_str());    
  write_Y(m_cat, s_y_fn, &p.s_batch_id); 
}
 
/*-------------------------------------------------------------------------*/
void AzPrepText::write_X(AzSmat &m_x, const AzBytArr &s_x_fn, 
                         const AzBytArr *s_batch_id) /* may be NULL */
const                         
{
  AzBytArr s_fn(&s_x_fn); 
  if (s_batch_id != NULL && s_batch_id->length() > 0) s_fn << "." << s_batch_id->c_str(); 
  const char *fn = s_fn.c_str(); 
  AzBytArr s(": "); AzTools::show_smat_stat(m_x, s);  
  if (s_x_fn.endsWith("smatvar")) {
    AzSmatVar msv; 
    smat_to_smatvar(&m_x, &msv); 
    AzTimeLog::print(fn, s.c_str(), " (smatvar)", out);  
    msv.write(fn);    
  }
  else if (s_x_fn.endsWith("smat")) {
    AzTimeLog::print(fn, s.c_str(), " (smat)", out);  
    m_x.write(fn); 
  }
  else {
    int digits = 5; 
    bool do_sparse = true; 
    AzTimeLog::print(fn, s.c_str(), " (text)", out);  
    m_x.writeText(fn, digits, do_sparse); 
  }
}  

/*-------------------------------------------------------------------------*/
void AzPrepText::write_Y(AzSmat &m_y, 
                         const AzBytArr &s_y_fn, 
                         const AzBytArr *s_batch_id) /* may be NULL */
const
{
  check_size(m_y); 
  AzBytArr s(": "); AzTools::show_smat_stat(m_y, s);  
  AzBytArr s_fn(&s_y_fn); 
  if (s_batch_id != NULL && s_batch_id->length() > 0) s_fn << "." << s_batch_id->c_str(); 
  const char *fn = s_fn.c_str();     
  if (s_y_fn.endsWith("smat")) {
    AzTimeLog::print(fn, s.c_str(), " (smat)", out);
    m_y.write(fn); 
  }
  else {
    int digits=5; 
    bool do_sparse = true;   
    AzTimeLog::print(fn, s.c_str(), " (text)", out);
    m_y.writeText(fn, digits, do_sparse); 
  }
}   

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
#define kw_ext "ext="
#define kw_num "num_batches="
#define kw_split "split="
#define kw_seed "random_seed="
#define kw_id_fn "id_fn_stem="
/*-----------------------------------------------------------------*/
class AzPrepText_split_text_Param : public virtual AzPrepText_Param_ {
public: 
  AzBytArr s_inp_fn, s_ext, s_outnm, s_split, s_id_fn; 
  AzIntArr ia_ratio; 
  int num, seed; 
  AzPrepText_split_text_Param(int argc, const char *argv[], const AzOut &out) : num(0), seed(1) {
    reset(argc, argv, out); 
  }
  void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText_split_Param::resetParam"; 
    azp.vStr(kw_inp_fn, &s_inp_fn);
    azp.vStr(kw_ext, &s_ext);  
    azp.vStr(kw_outnm, &s_outnm);     
    azp.vStr(kw_split, &s_split); 
    if (s_split.length() <= 0) azp.vInt(kw_num, &num); 
    else {
      AzTools::getInts(s_split.c_str(), ':', &ia_ratio); 
      AzX::throw_if((ia_ratio.min() <= 0), eyec, kw_split, "must not include a non-positive value."); 
      num = ia_ratio.size(); 
    }
    azp.vInt(kw_seed, &seed); 
    azp.vStr(kw_id_fn, &s_id_fn); 
    AzXi::throw_if_nonpositive(num, eyec, kw_seed); 
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn);     
    AzXi::throw_if_empty(s_outnm, eyec, kw_outnm);      
  }
  void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_inp_fn, s_inp_fn);   
    o.printV_if_not_empty(kw_ext, s_ext);  
    o.printV(kw_outnm, s_outnm);       
    o.printV(kw_num, num); 
    o.printV_if_not_empty(kw_split, s_split); 
    o.printV(kw_seed, seed); 
    o.printV_if_not_empty(kw_id_fn, s_id_fn); 
    o.printEnd(); 
  }
 void printHelp(const AzOut &out) const {
    AzHelp h(out); 
    h.begin("", "", ""); 
    h.item_required(kw_inp_fn, "Input filename or the list of input filenames  If it ends with \".lst\" it should contain the list of input filenames.");  
    h.item(kw_ext, "Filename extension.  Optional."); 
    h.item_required(kw_outnm, help_outnm); 
    h.item_required(kw_num, "Number of partitions.");     
    h.item(kw_split, "How to split the data.  Use \":\" as a delimiter.  For example, \"4:1\" will split the data into four fifth and one fifth.  \"1:1:1\" will split the data into three partitions of the same size.    Required if \"num_batches=\" is not specified.");    
    h.item(kw_seed, "Seed for random number generation.", "1"); 
    h.item(kw_id_fn, "Used to make pathnames by attaching, e.g., \".1of5\" to write the data indexes of the resulting text files.  Optional."); 
    h.end(); 
  }     
}; 

/*-----------------------------------------------------------------*/
void AzPrepText::split_text(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText::split_text"; 
  AzPrepText_split_text_Param p(argc, argv, out);   

  srand(p.seed); 
  
  AzIntArr ia_data_num; 
  AzStrPool sp_list; 
  int buff_size = AzTools_text::scan_files_in_list(p.s_inp_fn.c_str(), p.s_ext.c_str(), 
                                                   out, &sp_list, &ia_data_num); 
  buff_size += 256;  /* just in case */
  AzBytArr s_buff; 
  AzByte *buff = s_buff.reset(buff_size, 0);                                                    

  int data_num = ia_data_num.sum(); 

  AzTimeLog::print("Assigning batch id: ", data_num, out); 
  AzIntArr ia_dx2group;  
  if (p.ia_ratio.size() <= 0) {
    /*---  divide into partitions of the same size approximately  ---*/
    ia_dx2group.reset(data_num, -1); 
    for (int dx = 0; dx < data_num; ++dx) {
      int gx = AzTools::rand_large() % p.num; 
      ia_dx2group(dx, gx);  
    }
  }
  else {
    /*---  use the specified ratio  ---*/
    double sum = p.ia_ratio.sum(); 
    AzIntArr ia_num; 
    for (int ix = 0; ix < p.ia_ratio.size(); ++ix) ia_num.put((int)floor((double)data_num * (double)p.ia_ratio[ix]/sum)); 
    int extra = data_num - ia_num.sum(); 
    for (int ix = 0; extra > 0; ix = (ix+1)*ia_num.size()) {
      ia_num.increment(ix); --extra; 
    }
    AzX::throw_if((ia_num.sum() != data_num), eyec, "Partition sizes do not add up ..."); 
    ia_num.print(log_out, "Sizes: "); 
    
    ia_dx2group.reset(); 
    for (int gx = 0; gx < ia_num.size(); ++gx) {
      for (int ix = 0; ix < ia_num[gx]; ++ix) ia_dx2group.put(gx); 
    }
    AzTools::shuffle2(ia_dx2group); 
  }

  AzTimeLog::print("Reading and writing ... ", out);   
  AzDataArr<AzFile> afile(p.num); 
  for (int gx = 0; gx < p.num; ++gx) {
    AzBytArr s_fn(&p.s_outnm); 
    s_fn << "." << gx+1 << "of" << p.num;     
    if (p.s_ext.length() > 0) s_fn << p.s_ext.c_str();     
    afile(gx)->reset(s_fn.c_str()); 
    afile(gx)->open("wb"); 
  }
  
  int dx = 0; 
  for (int fx = 0; fx < sp_list.size(); ++fx) {
    AzBytArr s_fn(sp_list.c_str(fx), p.s_ext.c_str()); 
    const char *fn = s_fn.c_str();   
    AzTimeLog::print(fn, out); 
    AzFile file(fn); file.open("rb"); 
    for ( ; ; ) {
      int len = file.gets(buff, buff_size); 
      if (len <= 0) break;  
  
      int gx = ia_dx2group[dx];     
      afile(gx)->writeBytes(buff, len); 
      ++dx; 
    }
  }
  for (int gx = 0; gx < p.num; ++gx) {
    afile(gx)->close(true);  
  }
  
  /*---  write data indexes to the id files  ---*/
  if (p.s_id_fn.length() > 0) {
    for (int gx = 0; gx < p.num; ++gx) {
      AzBytArr s_fn(p.s_id_fn.c_str()); s_fn << "." << gx+1 << "of" << p.num;  
      AzTimeLog::print("Writing data indexes to: ", s_fn.c_str(), out); 
      AzFile file(s_fn.c_str()); file.open("wb"); 
      for (int dx = 0; dx < data_num; ++dx) {
        if (ia_dx2group[dx] == gx) {
          AzBytArr s; s << dx; s.nl(); 
          s.writeText(&file); 
        }        
      }
      file.close(true); 
    }    
  }

  AzTimeLog::print("Done ... ", log_out); 
} 

/*-----------------------------------------------------------------*/
void AzPrepText::check_y_ext(const AzBytArr &s_y_ext, const char *eyec) const
{
  AzXi::check_input(s_y_ext, ".y", ".ysmat", eyec, kw_y_ext); 
}    
