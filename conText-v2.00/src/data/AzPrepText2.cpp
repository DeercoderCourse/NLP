/* * * * *
 *  AzPrepText2.cpp 
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

#include "AzPrepText2.hpp"
#include "AzTools.hpp"
#include "AzParam.hpp"
#include "AzPrint.hpp"
#include "AzTools_text.hpp"
#include "AzTextMat.hpp"
#include "AzRandGen.hpp"
#include "AzHelp.hpp"

static AzByte param_dlm = ' ', file_mark='@'; 
static bool do_paramChk = true; 

/*-------------------------------------------------------------------------*/
class AzPrepText2_Param_ {
public: 
  AzPrepText2_Param_() {}
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
class AzPrepText2_gen_regions_unsup_Param : public virtual AzPrepText2_Param_ {
public: 
  AzBytArr s_xtyp, s_xdic_fn, s_ydic_fn, s_inp_fn, s_txt_ext, s_rnm; 
  AzBytArr s_batch_id, s_x_ext, s_y_ext; 
  int dist, min_x, min_y; 
  int pch_sz, pch_step, padding; 
  bool do_lower, do_utf8dashes; 
  bool do_nolr; /* only when bow */
  bool do_no_skip; 

  #define kw_bow "Bow"
  #define kw_seq "Seq"   
  AzPrepText2_gen_regions_unsup_Param(int argc, const char *argv[], const AzOut &out) 
    : dist(-1), min_x(1), min_y(1), pch_sz(-1), pch_step(1), padding(-1), 
      s_x_ext(".xsmat"), s_y_ext(".ysmat"), s_xtyp(kw_bow), 
      do_lower(false), do_utf8dashes(false), do_nolr(false), do_no_skip(false) {
    reset(argc, argv, out); 
  }

  /*-------------------------------------------------------------------------*/  
  #define kw_xtyp "x_type="
  #define kw_xdic_fn "x_vocab_fn="    
  #define kw_ydic_fn "y_vocab_fn="
  #define kw_inp_fn "input_fn="  
  #define kw_rnm "region_fn_stem="
  #define kw_txt_ext "text_fn_ext="  
  #define kw_pch_sz "patch_size="
  #define kw_pch_step "patch_stride="
  #define kw_padding "padding="
  #define kw_min_x "min_x="
  #define kw_min_y "min_y="
  #define kw_dist "dist="
  #define kw_do_lower "LowerCase"
  #define kw_do_utf8dashes "UTF8"  
  #define kw_do_nolr "MergeLeftRight"
  #define kw_batch_id "batch_id="
  #define kw_x_ext "x_fn_ext="    
  #define kw_y_ext "y_fn_ext="   
  #define kw_do_no_skip "NoSkip"
 
  #define xtext_ext ".xtext"
  #define ytext_ext ".ytext"
 
  #define help_rnm "Pathname stem of the region vector file and target file (output).  To make the pathnames, the respective extensions will be attached."
  #define help_do_lower "Convert upper-case to lower-case characters."
  #define help_do_utf8dashes "Convert UTF8 en dash, em dash, single/double quotes to ascii characters."
  #define help_do_nolr "Do not distinguish the target regions on the left and right."
  #define help_xtyp "Vector representation for X (region vectors).  Bow | Seq"
  /*-------------------------------------------------------------------------*/
  void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText2_gen_regions_unsup_Param::resetParam"; 
    azp.vStr(kw_xtyp, &s_xtyp); 
    azp.vStr(kw_xdic_fn, &s_xdic_fn);    
    azp.vStr(kw_ydic_fn, &s_ydic_fn);       
    azp.vStr(kw_inp_fn, &s_inp_fn);      
    azp.vStr(kw_rnm, &s_rnm);  
    azp.vStr(kw_txt_ext, &s_txt_ext);  
    azp.vInt(kw_pch_sz, &pch_sz);      
    azp.vInt(kw_pch_step, &pch_step);   
    azp.vInt(kw_padding, &padding);   
    azp.vInt(kw_dist, &dist);   
    azp.swOn(&do_lower, kw_do_lower); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes);     
    azp.swOn(&do_nolr, kw_do_nolr); 
    azp.vStr(kw_batch_id, &s_batch_id); 
    azp.vStr(kw_x_ext, &s_x_ext); 
    azp.vStr(kw_y_ext, &s_y_ext); 
    azp.swOn(&do_no_skip, kw_do_no_skip, false); 
    if (do_no_skip) {
      min_x = min_y = 0;       
    }
    else {
      azp.vInt(kw_min_x, &min_x);   
      azp.vInt(kw_min_y, &min_y);      
    }

    AzStrPool sp_typ(10,10); sp_typ.put(kw_bow, kw_seq); 
    AzXi::check_input(s_xtyp, &sp_typ, eyec, kw_xtyp);     
    AzXi::throw_if_empty(s_xdic_fn, eyec, kw_xdic_fn);  
    AzXi::throw_if_empty(s_ydic_fn, eyec, kw_ydic_fn);     
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn);      
    AzXi::throw_if_empty(s_rnm, eyec, kw_rnm);      
    AzXi::throw_if_nonpositive(pch_sz, eyec, kw_pch_sz); 
    AzXi::throw_if_nonpositive(pch_step, eyec, kw_pch_step); 
    AzXi::throw_if_negative(padding, eyec, kw_padding); 
    AzXi::throw_if_nonpositive(dist, eyec, kw_dist); 
    AzStrPool sp_x_ext(10,10); sp_x_ext.put(".xsmat", ".xsmatvar"); 
    AzStrPool sp_y_ext(10,10); sp_y_ext.put(".ysmat", ".ysmatvar"); 
    AzXi::check_input(s_x_ext, &sp_x_ext, eyec, kw_x_ext);     
    AzXi::check_input(s_y_ext, &sp_y_ext, eyec, kw_y_ext);         
  }
  void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_xtyp, s_xtyp); 
    o.printV(kw_xdic_fn, s_xdic_fn); 
    o.printV(kw_ydic_fn, s_ydic_fn);       
    o.printV(kw_inp_fn, s_inp_fn);      
    o.printV(kw_rnm, s_rnm);  
    o.printV_if_not_empty(kw_txt_ext, s_txt_ext);  
    o.printV(kw_pch_sz, pch_sz);      
    o.printV(kw_pch_step, pch_step);   
    o.printV(kw_padding, padding);   
    o.printV(kw_min_x, min_x);   
    o.printV(kw_min_y, min_y);   
    o.printV(kw_dist, dist);        
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printSw(kw_do_nolr, do_nolr); 
    o.printV_if_not_empty(kw_batch_id, s_batch_id); 
    o.printV(kw_x_ext, s_x_ext); 
    o.printV(kw_y_ext, s_y_ext); 
    o.printSw(kw_do_no_skip, do_no_skip); 
    o.printEnd(); 
  } 
  void printHelp(const AzOut &out) const {
    AzHelp h(out); h.begin("", "", "");  h.nl(); 
    h.writeln("To generate from unlabeled data a region file and target file for tv-embedding learning with unsupervised target.  The task is to predict adjacent regions based on the current region.\n", 3);
    h.item_required(kw_xtyp, help_xtyp, kw_bow); 
    h.item_required(kw_inp_fn, "Path to the input token file or the list of token files.  If the filename ends with \".lst\", the file should be the list of token filenames.  The input file(s) should contain one document per line, and each document should be tokens delimited by space."); 
    h.item_required(kw_rnm, help_rnm); 
    h.item_required(kw_xdic_fn, "Path to the vocabulary file generated by \"gen_vocab\", used for X (region vectors)."); 
    h.item_required(kw_ydic_fn, "Path to the vocabulary file generated by \"gen_vocab\", used for Y (target).");    
    h.item_required(kw_pch_sz, "Region size."); 
    h.item_required(kw_pch_step, "Region stride.", "1"); 
    h.item_required(kw_padding, "Padding size.  Region size minus one is recommended.");     
    h.item_required(kw_dist, "Size of adjacent regions used to produce Y (target).");         
    h.item(kw_do_lower, help_do_lower);     
    h.item(kw_do_utf8dashes, help_do_utf8dashes); 
    h.item(kw_do_nolr, help_do_nolr); 
    /* txt_ext, x_ext, y_ext, do_no_skip, min_x, min_y */
    h.end(); 
  }   
}; 

/*-------------------------------------------------------------------------*/
/* Note: X and Y use different dictionaries */
void AzPrepText2::gen_regions_unsup(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText2::gen_regions_unsup"; 
  AzPrepText2_gen_regions_unsup_Param p(argc, argv, out);   
  check_batch_id(p.s_batch_id);     
 
  bool do_xseq = p.s_xtyp.equals(kw_seq); 
  bool do_skip_stopunk = (do_xseq) ? false : true; 
  
  AzDic ydic(p.s_ydic_fn.c_str()); 
  int ydic_nn = ydic.get_max_n(); 
  AzPrint::writeln(out, "y dic n=", ydic_nn); 
  AzX::throw_if((ydic.size() <= 0), AzInputError, eyec, "No Y (target) vocabulary."); 
  
  AzDic xdic(p.s_xdic_fn.c_str()); 
  int xdic_nn = xdic.get_max_n(); 
  AzPrint::writeln(out, "x dic n=", xdic_nn);   
  AzX::throw_if((xdic.size() <= 0), AzInputError, eyec, "No vocabulary.");   
  AzX::no_support((xdic_nn > 1 && do_xseq), eyec, "X with multi-word vocabulary and Seq option");    

  /*---  scan files to determine buffer size and #data  ---*/
  AzOut noout; 
  AzStrPool sp_list; 
  AzIntArr ia_data_num; 
  int buff_size = AzTools_text::scan_files_in_list(p.s_inp_fn.c_str(), p.s_txt_ext.c_str(), 
                                                   noout, &sp_list, &ia_data_num);   
  int data_num = ia_data_num.sum(); 
  
  /*---  read data and generate features  ---*/
  AzDataArr<AzSmat> am_x(data_num), am_y(data_num); 
  
  buff_size += 256; 
  AzBytArr s_buff; 
  AzByte *buff = s_buff.reset(buff_size, 0); 
  int no_data = 0, data_no = 0, cnum = 0, cnum_before_reduce = 0; 
  int l_dist = -p.dist, r_dist = p.dist; 

  AzIntArr ia_xnn; for (int ix = 1; ix <= xdic_nn; ++ix) ia_xnn.put(ix); 
  AzIntArr ia_ynn; for (int ix = 1; ix <= ydic_nn; ++ix) ia_ynn.put(ix); 
  
  for (int fx = 0; fx < sp_list.size(); ++fx) { /* for each file */
    AzBytArr s_fn(sp_list.c_str(fx), p.s_txt_ext.c_str()); 
    const char *fn = s_fn.c_str(); 
    AzTimeLog::print(fn, out);   
    AzFile file(fn); 
    file.open("rb"); 
    int num_in_file = ia_data_num.get(fx); 
    int inc = num_in_file / 50, milestone = inc; 
    int dx = 0; 
    for ( ; ; ++dx) {  /* for each doc */
      AzTools::check_milestone(milestone, dx, inc); 
      int len = file.gets(buff, buff_size); 
      if (len <= 0) break; 
      
      /*---  X  ---*/
      AzIntArr ia_pos;    
      AzBytArr s_data(buff, len); 
      int my_len = s_data.length();
      
      bool do_allow_zero = false; 
      if (p.do_no_skip) {
        do_allow_zero = true; 
        do_skip_stopunk = false; 
      }
      int xtok_num = 0; 
      if (xdic_nn > 1) { /* n-grams */
        AzDataArr<AzIntArr> aia_xtokno; 
        AzTools_text::tokenize(s_data.point_u(), my_len, &xdic, ia_xnn, p.do_lower, p.do_utf8dashes, aia_xtokno);  
        if (aia_xtokno.size() > 0) xtok_num = aia_xtokno[0]->size();        
        gen_X_ngram_bow(ia_xnn, aia_xtokno, xdic.size(), p.pch_sz, p.pch_step, p.padding, do_allow_zero, 
                        am_x.point_u(data_no), &ia_pos); 
      }
      else { /* words */
        AzIntArr ia_xtokno;        
        int nn = 1; 
        AzTools_text::tokenize(s_data.point_u(), my_len, &xdic, nn, p.do_lower, p.do_utf8dashes, &ia_xtokno);         
        xtok_num = ia_xtokno.size(); 
        if (do_xseq) gen_X_seq(ia_xtokno, xdic.size(), p.pch_sz, p.pch_step, p.padding, 
                               do_allow_zero, do_skip_stopunk, am_x.point_u(data_no), &ia_pos);       
        else         gen_X_bow(ia_xtokno, xdic.size(), p.pch_sz, p.pch_step, p.padding, 
                               do_skip_stopunk, am_x.point_u(data_no), &ia_pos); 
      }
      if (am_x.point(data_no)->colNum() <= 0) {
        ++no_data; 
        continue; 
      }
      
      /*---  Y  ---*/
      s_data.reset(buff, len); 
      my_len = s_data.length();        

      if (ydic_nn > 1) { /* n-grams */
        AzDataArr<AzIntArr> aia_ytokno; 
        AzTools_text::tokenize(s_data.point_u(), my_len, &ydic, ia_ynn, p.do_lower, p.do_utf8dashes, aia_ytokno);  
        int ytok_num = (aia_ytokno.size() > 0) ? aia_ytokno[0]->size() : 0; 
        AzX::throw_if((xtok_num != ytok_num), eyec, "conflict in the numbers of X tokens and Y tokens"); 
        gen_Y_ngram_bow(ia_ynn, aia_ytokno, ydic.size(), ia_pos, 
                        p.pch_sz, l_dist, r_dist, p.do_nolr, am_y.point_u(data_no));   
      }
      else { /* words */
        int nn = 1; 
        AzIntArr ia_ytokno; 
        AzTools_text::tokenize(s_data.point_u(), my_len, &ydic, nn, p.do_lower, p.do_utf8dashes, &ia_ytokno);  
        AzX::throw_if((xtok_num != ia_ytokno.size()), eyec, "conflict in the numbers of X tokens and Y tokens"); 
        if (p.do_nolr) gen_Y_nolr(ia_ytokno, ydic.size(), ia_pos, 
                                  p.pch_sz, l_dist, r_dist, am_y.point_u(data_no));  
        else           gen_Y(ia_ytokno, ydic.size(), ia_pos, 
                             p.pch_sz, l_dist, r_dist, am_y.point_u(data_no));      
      }

      cnum_before_reduce += am_x.point(data_no)->colNum(); 
      reduce_xy(p.min_x, p.min_y, am_x.point_u(data_no), am_y.point_u(data_no)); 
      if (am_x.point(data_no)->colNum() <= 0) {
        ++no_data; 
        continue; 
      }
      cnum += am_x.point(data_no)->colNum(); 
      ++data_no;         
    } /* for each doc */
    AzTools::finish_milestone(milestone); 
    AzBytArr s("   #data="); s << data_no << " no_data=" << no_data << " #col=" << cnum; AzPrint::writeln(out, s); 
  } /* for each file */
  AzBytArr s("#data="); s << data_no << " no_data=" << no_data << " #col=" << cnum << " #col_all=" << cnum_before_reduce;
  AzPrint::writeln(out, s);   
    
  const char *outnm = p.s_rnm.c_str(); 
  AzTimeLog::print("Generating X ... ", out);  
  write_XY(am_x, data_no, p.s_batch_id, outnm, p.s_x_ext.c_str(), &xdic, xtext_ext); 
  AzTimeLog::print("Generating Y ... ", out);  
  write_XY(am_y, data_no, p.s_batch_id, outnm, p.s_y_ext.c_str(), &ydic, ytext_ext);   

  AzTimeLog::print("Done ... ", out); 
}

/*-------------------------------------------------------------------------*/
void AzPrepText2::write_XY(AzDataArr<AzSmat> &am_xy, /* destroyed to save memory */
                           int data_num, 
                           const AzBytArr &s_batch_id, 
                           const char *outnm, const char *xy_ext, 
                           const AzDic *xy_dic, /* may be NULL */
                           const char *xy_dic_ext) const {
  AZint8 e_num = 0; 
  for (int dx = 0; dx < data_num; ++dx) e_num += am_xy[dx]->nonZeroNum(); 
   
  int r_num = 0; 
  if (Az64::can_be_int(e_num)) r_num = _write_XY<AzSmatcVar>(am_xy, data_num, s_batch_id, outnm, xy_ext); 
  else {
    AzBytArr s("!WARNING!: # of non-zero components exceeded 2GB ("); 
    s << (double)e_num/(double)AzSigned32Max << " times larger).  Either divide data or use \"datatype=sparse_large\" (or \"sparse_large_multi\") for training/testing."; 
    AzPrint::writeln(out, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"); 
    AzPrint::writeln(out, s.c_str()); 
    AzPrint::writeln(out, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"); 
    r_num = _write_XY<AzSmatVar>(am_xy, data_num, s_batch_id, outnm, xy_ext); 
  }

  _write_XY_dic(xy_dic, r_num, outnm, xy_dic_ext);   
}  

/*-------------------------------------------------------------------------*/
template <class Vmat> /* Vmat: AzSmatVar | AzSmatcVar */
int AzPrepText2::_write_XY(AzDataArr<AzSmat> &am_xy, /* destroyed to save memory */
                           int data_num, 
                           const AzBytArr &s_batch_id, 
                           const char *outnm, const char *xy_ext) const {                          
  Vmat mv_xy; mv_xy.transfer_from(&am_xy, data_num); am_xy.reset(); 
  AzBytArr s_xy(": "); AzTools::show_smat_stat(*mv_xy.data(), s_xy); 

  AzBytArr s_xy_fn(outnm, xy_ext); 
  if (s_batch_id.length() > 0) s_xy_fn << "." << s_batch_id.c_str(); 
  const char *xy_fn = s_xy_fn.c_str(); 
  AzTimeLog::print(xy_fn, s_xy.c_str(), out); 
  if (AzBytArr::endsWith(xy_ext, "smat")) mv_xy.write_matrix(xy_fn); 
  else                                    mv_xy.write(xy_fn);  
  return mv_xy.rowNum(); 
}  
template int AzPrepText2::_write_XY<AzSmatVar>(AzDataArr<AzSmat> &, int, const AzBytArr &, const char *, const char *) const; 
template int AzPrepText2::_write_XY<AzSmatcVar>(AzDataArr<AzSmat> &, int, const AzBytArr &, const char *, const char *) const; 

/*-------------------------------------------------------------------------*/
void AzPrepText2::_write_XY_dic(const AzDic *dic, int x_row_num, const char *nm, const char *ext) const
{
  const char *eyec = "AzPrepText2::_write_XY_dic"; 
  if (dic == NULL) return; 
  int pch_sz = x_row_num / dic->size(); 
  AzX::throw_if((x_row_num % dic->size() != 0), eyec, "Conflict in #row and size of vocabulary"); 
  AzBytArr s_fn(nm, ext); 
  if (pch_sz > 1) {
    AzDic seq_dic; 
    gen_seq_dic(dic, pch_sz, &seq_dic); 
    seq_dic.writeText(s_fn.c_str()); 
  }
  else {
    dic->writeText(s_fn.c_str()); 
  }
} 
  
/*-------------------------------------------------------------------------*/
void AzPrepText2::check_batch_id(const AzBytArr &s_batch_id) const
{
  const char *eyec = "AzPrepText::check_batch_id"; 
  if (s_batch_id.length() <= 0) return; 
  AzBytArr s(kw_batch_id); s << " should look like \"1of5\""; 
  const char *batch_id = s_batch_id.c_str(); 
  const char *of_str = strstr(batch_id, "of"); 
  AzX::throw_if((of_str == NULL), AzInputError, eyec, s.c_str()); 
  for (const char *wp = batch_id; wp < batch_id+s_batch_id.length(); ++wp) {
    if (wp >= of_str && wp < of_str+2) continue; 
    AzX::throw_if((*wp < '0' || *wp > '9'), AzInputError, eyec, s.c_str());  
  }
  int batch_no = atol(batch_id); 
  int batch_num = atol(of_str+2); 
  AzX::throw_if((batch_no < 1 || batch_no > batch_num), AzInputError, eyec, 
                s.c_str(), " batch# must start with 1 and must not exceed the number of batches. "); 
} 

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_Y_ngram_bow(const AzIntArr &ia_nn, 
                             const AzDataArr<AzIntArr> &aia_tokno, int dic_sz, 
                             const AzIntArr &ia_pos, 
                             int xpch_sz, /* patch size used to generate X */
                             int min_dist, int max_dist, 
                             bool do_nolr, 
                             AzSmat *m_y) const
{
  const char *eyec = "AzPrepText2::gen_Y_ngram_bow"; 
  int t_num = aia_tokno[0]->size(); 
  if (do_nolr) m_y->reform(dic_sz, ia_pos.size());
  else         m_y->reform(dic_sz*2, ia_pos.size()); /* *2 for left and right */
  for (int ix = 0; ix < ia_pos.size(); ++ix) {
    int xtx0 = ia_pos.get(ix); 
    int xtx1 = xtx0 + xpch_sz; 
    
    AzIntArr ia_ctx;     
    int base = xtx0+min_dist;  
    for (int nx = 0; nx < aia_tokno.size(); ++nx) {
      const AzIntArr *ia_tokno = aia_tokno[nx];      
      int nn = ia_nn[nx]; 
      for (int tx = MAX(0,base); tx <= MIN(t_num,xtx0)-nn; ++tx) {         
        int tokno = ia_tokno->get(tx); 
        if (tokno >= 0) ia_ctx.put(tokno); 
      }
    }
   
    base = xtx1; 
    for (int nx = 0; nx < aia_tokno.size(); ++nx) {  
      const AzIntArr *ia_tokno = aia_tokno[nx];     
      int nn = ia_nn[nx];   
      for (int tx = MAX(0,base); tx <= MIN(t_num,xtx1+max_dist)-nn; ++tx) {           
        int tokno = ia_tokno->get(tx); 
        if (tokno >= 0) {
          if (do_nolr) ia_ctx.put(tokno); 
          else         ia_ctx.put(dic_sz+tokno); 
        }
      }
    }
    ia_ctx.unique();
    m_y->col_u(ix)->load(&ia_ctx, 1); 
  }
}

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_seq_dic(const AzDic *inp_dic, int pch_sz,
                                   AzDic *out_dic) const
{
  if (inp_dic == NULL) return; 
  for (int px = 0; px < pch_sz; ++px) {
    AzBytArr s_pref; s_pref.cn(px); s_pref.c(":"); 
    AzDic tmp_dic; tmp_dic.reset(inp_dic); 
    tmp_dic.add_prefix(s_pref.c_str()); 
    if (px == 0) out_dic->reset(&tmp_dic); 
    else         out_dic->append(&tmp_dic); 
  }
}   

/*-------------------------------------------------------------------------*/
void AzPrepText2::reduce_xy(int min_x, int min_y, 
                         AzSmat *m_x, AzSmat *m_y, 
                         bool do_keep_one) const /* for para vec, we want to keep 
                                                    at least one column for each doc */
{
  if (m_x->colNum() <= 0) return; 
  AzIntArr ia_cols; 
  for (int col = 0; col < m_x->colNum(); ++col) {
    if (m_x->col(col)->nonZeroRowNum() >= min_x && 
        m_y->col(col)->nonZeroRowNum() >= min_y) {
      ia_cols.put(col); 
    }
  }
  if (ia_cols.size() == m_x->colNum()) return; 
  if (do_keep_one && ia_cols.size() <= 0) ia_cols.put(0); 
  m_x->reduce(ia_cols.point(), ia_cols.size()); 
  m_y->reduce(ia_cols.point(), ia_cols.size()); 
}                         

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_X_seq(const AzIntArr &ia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_allow_zero, bool do_skip_stopunk, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const /* patch position: may be NULL */
{
  const char *eyec = "AzPrepText2::gen_X_seq"; 
  AzX::throw_if_null(m_feat, eyec, "m_feat"); 
  AzX::no_support(do_skip_stopunk, eyec, "variable strides with Seq"); 
  int t_num; 
  const int *tokno = ia_tokno.point(&t_num); 
    
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
    tx0 += pch_step; 
  }
  m_feat->resize(col);   
}

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_X_ngram_bow(const AzIntArr &ia_nn, 
                       const AzDataArr<AzIntArr> &aia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_skip_stopunk, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const /* patch position: may be NULL */
{
  const char *eyec = "AzPrepText2::gen_X_ngram_bow"; 
  AzX::throw_if_null(m_feat, eyec, "m_feat"); 
  int t_num = aia_tokno[0]->size(); 
    
  int pch_num = DIVUP(t_num+padding*2-pch_sz, pch_step) + 1; 
  m_feat->reform(dic_sz, pch_num);   
  if (ia_pos != NULL) ia_pos->reset(); 

  int col = 0; 
  int tx0 = -padding; 
  for (int pch_no = 0; pch_no < pch_num; ++pch_no) {
    int tx1 = tx0 + pch_sz; 
    
    AzIntArr ia_rows; 
    for (int nx = 0; nx < aia_tokno.size(); ++nx) {
      int nn = ia_nn[nx]; 
      const AzIntArr *ia_tokno = aia_tokno[nx]; 
      for (int tx = MAX(0, tx0); tx <= MIN(t_num, tx1)-nn; ++tx) {
        if ((*ia_tokno)[tx] >= 0) ia_rows.put((*ia_tokno)[tx]); 
      }
    }
    ia_rows.unique();  /* sorting too */
    
    if (!do_skip_stopunk || ia_rows.size() > 0) {   
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
void AzPrepText2::gen_X_bow(const AzIntArr &ia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_skip_stopunk, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const /* patch position: may be NULL */
{
  const char *eyec = "AzPrepText2::gen_X_bow"; 
  AzX::throw_if_null(m_feat, eyec, "m_feat"); 
  int t_num; 
  const int *tokno = ia_tokno.point(&t_num); 
    
  int pch_num = DIVUP(t_num+padding*2-pch_sz, pch_step) + 1; 
  m_feat->reform(dic_sz, pch_num);   
  if (ia_pos != NULL) ia_pos->reset(); 
  
  int col = 0; 
  int tx0 = -padding; 
  for (int pch_no = 0; pch_no < pch_num; ++pch_no) {
    int tx1 = tx0 + pch_sz; 
    
    AzIntArr ia_rows; 
    for (int tx = MAX(0, tx0); tx < MIN(t_num, tx1); ++tx) {
      if (tokno[tx] >= 0) ia_rows.put(tokno[tx]); 
    }
    if (!do_skip_stopunk || ia_rows.size() > 0) {
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
      for (tx = tx1; tx < t_num; ++tx) if (tx >= 0 && tokno[tx] >= 0) break; 
      int dist1 = tx-tx1+1; /* to get a new word, we have to slide a window this much */
      dist = MIN(dist0, dist1); 
    }
    tx0 += MAX(dist, pch_step); 
  }
  m_feat->resize(col);     
} 

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_Y(const AzIntArr &ia_tokno, int dic_sz, 
                        const AzIntArr &ia_pos, 
                        int xpch_sz, /* patch size used to generate X */
                        int min_dist, int max_dist, 
                        AzSmat *m_y) const
{
  AzX::throw_if_null(m_y, "AzPrepText2::gen_Y", "m_y"); 
  int t_num; 
  const int *tokno = ia_tokno.point(&t_num); 

  m_y->reform(dic_sz*2, ia_pos.size()); /* *2 for left and right */
  for (int ix = 0; ix < ia_pos.size(); ++ix) {
    int xtx0 = ia_pos.get(ix); 
    int xtx1 = xtx0 + xpch_sz; 
        
    AzIntArr ia_ctx0, ia_ctx1;      
    for (int tx = MAX(0,xtx0+min_dist); tx < MIN(t_num,xtx0); ++tx) if (tokno[tx] >= 0) ia_ctx0.put(tokno[tx]); 
    ia_ctx0.unique(); 

    for (int tx = MAX(0,xtx1); tx < MIN(t_num,xtx1+max_dist); ++tx) if (tokno[tx] >= 0) ia_ctx1.put(tokno[tx]); 
    ia_ctx1.unique(); 
    ia_ctx1.add(dic_sz); 
    
    AzIntArr ia_ctx; 
    ia_ctx.concat(&ia_ctx0); 
    ia_ctx.concat(&ia_ctx1); 
    if (ia_ctx.size() > 0) {
      ia_ctx.unique(); 
      m_y->col_u(ix)->load(&ia_ctx, 1); 
    }
  }
}

/*-------------------------------------------------------------------------*/
/* don't distinguish left and right */
void AzPrepText2::gen_Y_nolr(const AzIntArr &ia_tokno, int dic_sz, 
                             const AzIntArr &ia_pos, 
                             int xpch_sz, /* patch size used to generate X */
                             int min_dist, int max_dist, 
                             AzSmat *m_y) const
{
  AzX::throw_if_null(m_y, "AzPrepText2::gen_Y_nolr", "m_y"); 
  int t_num; 
  const int *tokno = ia_tokno.point(&t_num); 

  m_y->reform(dic_sz, ia_pos.size());
  for (int ix = 0; ix < ia_pos.size(); ++ix) {
    int xtx0 = ia_pos.get(ix); 
    int xtx1 = xtx0 + xpch_sz; 
    
    AzIntArr ia_ctx;         
    for (int tx = MAX(0,xtx0+min_dist); tx < MIN(t_num,xtx0); ++tx) if (tokno[tx] >= 0) ia_ctx.put(tokno[tx]); 
    for (int tx = MAX(0,xtx1); tx < MIN(t_num,xtx1+max_dist); ++tx) if (tokno[tx] >= 0) ia_ctx.put(tokno[tx]); 
    ia_ctx.unique(); 
    m_y->col_u(ix)->load(&ia_ctx, 1); 
  }
}
 
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
class AzPrepText2_gen_regions_parsup_Param : public virtual AzPrepText2_Param_ {
public: 
  AzBytArr s_xtyp, s_xdic_fn, s_inp_fn, s_txt_ext, s_rnm; 
  AzBytArr s_batch_id, s_x_ext, s_y_ext;
  AzBytArr s_feat_fn; 
  int dist, min_x, min_y; 
  int pch_sz, pch_step, padding; 
  int f_pch_sz, f_pch_step, f_padding; 
  bool do_lower, do_utf8dashes; 
  bool do_nolr, do_binarize; 
  int top_num_each, top_num_total; 
  double scale_y, min_yval; 

  AzPrepText2_gen_regions_parsup_Param(int argc, const char *argv[], const AzOut &out) 
    : dist(0), min_x(1), min_y(1), pch_sz(-1), pch_step(1), padding(-1),
      s_x_ext(".xsmat"), s_y_ext(".ysmat"),     
      top_num_each(-1), top_num_total(-1), 
      f_pch_sz(-1), f_pch_step(-1), f_padding(-1), 
      do_lower(false), do_utf8dashes(false), do_nolr(false), do_binarize(false), 
      scale_y(-1), min_yval(-1) {
    reset(argc, argv, out); 
  }

  /*-------------------------------------------------------------------------*/  
  #define kw_feat_fn "embed_fn="
  #define kw_top_num_each "num_top_each="
  #define kw_top_num_total "num_top="  
  #define kw_do_binarize "BinarizeY"
  #define kw_scale_y "scale_y="
  #define kw_min_yval "min_yval="
  #define kw_f_pch_sz "f_patch_size="
  #define kw_f_pch_step "f_patch_stride="
  #define kw_f_padding "f_padding="  
  /*-------------------------------------------------------------------------*/
  void resetParam(AzParam &azp) {
    const char *eyec = "AzPrepText2_gen_regions_parsup_Param::resetParam"; 
    azp.vInt(kw_top_num_each, &top_num_each); 
    azp.vInt(kw_top_num_total, &top_num_total);     
    azp.vStr(kw_feat_fn, &s_feat_fn); 
    azp.vStr(kw_xtyp, &s_xtyp); 
    azp.vStr(kw_xdic_fn, &s_xdic_fn);         
    azp.vStr(kw_inp_fn, &s_inp_fn);      
    azp.vStr(kw_rnm, &s_rnm);  
    azp.vStr(kw_txt_ext, &s_txt_ext);  
    azp.vInt(kw_pch_sz, &pch_sz);      
    azp.vInt(kw_pch_step, &pch_step);   
    azp.vInt(kw_padding, &padding);
    
    f_pch_sz = pch_sz; 
    f_pch_step = 1; 
    f_padding = f_pch_sz - 1; 
    azp.vInt(kw_f_pch_sz, &f_pch_sz);      
    azp.vInt(kw_f_pch_step, &f_pch_step);   
    azp.vInt(kw_f_padding, &f_padding);
    
    azp.vInt(kw_min_x, &min_x);   
    azp.vInt(kw_min_y, &min_y);   
    azp.vInt(kw_dist, &dist);   
    azp.swOn(&do_lower, kw_do_lower); 
    azp.swOn(&do_utf8dashes, kw_do_utf8dashes);     
    azp.swOn(&do_nolr, kw_do_nolr); 
    azp.vStr(kw_batch_id, &s_batch_id); 
    azp.vStr(kw_x_ext, &s_x_ext); 
    azp.vStr(kw_y_ext, &s_y_ext);     
    azp.swOn(&do_binarize, kw_do_binarize); 
    if (!do_binarize) {
      azp.vFloat(kw_scale_y, &scale_y); 
    }
    azp.vFloat(kw_min_yval, &min_yval); 
    
    AzXi::throw_if_empty(s_feat_fn, eyec, kw_feat_fn);      
    AzXi::throw_if_empty(s_xtyp, eyec, kw_xtyp); 
    AzXi::throw_if_empty(s_xdic_fn, eyec, kw_xdic_fn);     
    AzXi::throw_if_empty(s_inp_fn, eyec, kw_inp_fn);      
    AzXi::throw_if_empty(s_rnm, eyec, kw_rnm);      
    AzXi::throw_if_nonpositive(pch_sz, eyec, kw_pch_sz); 
    AzXi::throw_if_nonpositive(pch_step, eyec, kw_pch_step); 
    AzXi::throw_if_negative(padding, eyec, kw_padding); 
    AzXi::throw_if_nonpositive(f_pch_sz, eyec, kw_f_pch_sz); 
    AzXi::throw_if_nonpositive(f_pch_step, eyec, kw_f_pch_step); 
    AzXi::throw_if_negative(f_padding, eyec, kw_f_padding);     
    AzXi::throw_if_nonpositive(dist, eyec, kw_dist); 
    if (f_pch_sz > dist) {
      AzBytArr s(kw_f_pch_sz); s << " must be no greater than " << kw_dist << "."; 
      AzX::throw_if(true, AzInputError, eyec, s.c_str()); 
    }
  }
  void printParam(const AzOut &out) const { 
    AzPrint o(out); 
    o.printV(kw_top_num_each, top_num_each); 
    o.printV(kw_top_num_total, top_num_total);     
    o.printV(kw_feat_fn, s_feat_fn); 
    o.printV(kw_xtyp, s_xtyp); 
    o.printV(kw_xdic_fn, s_xdic_fn);     
    o.printV(kw_inp_fn, s_inp_fn);      
    o.printV(kw_rnm, s_rnm);  
    o.printV_if_not_empty(kw_txt_ext, s_txt_ext);  
    o.printV(kw_pch_sz, pch_sz);      
    o.printV(kw_pch_step, pch_step);   
    o.printV(kw_padding, padding);   
    o.printV(kw_f_pch_sz, f_pch_sz);      
    o.printV(kw_f_pch_step, f_pch_step);   
    o.printV(kw_f_padding, f_padding); 
    o.printV(kw_min_x, min_x);   
    o.printV(kw_min_y, min_y);   
    o.printV(kw_dist, dist);        
    o.printSw(kw_do_lower, do_lower); 
    o.printSw(kw_do_utf8dashes, do_utf8dashes); 
    o.printSw(kw_do_nolr, do_nolr); 
    o.printV_if_not_empty(kw_batch_id, s_batch_id); 
    o.printV(kw_x_ext, s_x_ext); 
    o.printV(kw_y_ext, s_y_ext);     
    o.printSw(kw_do_binarize, do_binarize); 
    o.printV(kw_scale_y, scale_y);
    o.printV(kw_min_yval, min_yval); 
    o.printEnd(); 
  }
  void printHelp(const AzOut &out) const {    
    AzHelp h(out); h.begin("", "", "");  h.nl(); 
    h.writeln("To generate from unlabeled data a region file and target file for tv-embedding learning with partially-supervised target.  The task is to predict the internal features obtained by applying a supervised model to adjacent regions, based on the current region.\n", 3); 
    h.item_required(kw_xtyp, help_xtyp, kw_bow); 
    h.item_required(kw_inp_fn, "Path to the input token file or the list of token files.  If the filename ends with \".lst\", the file should be the list of token filenames.  The input file(s) should contain one document per line, and each document should be tokens delimited by space."); 
    h.item_required(kw_rnm, help_rnm); 
    h.item_required(kw_xdic_fn, "Path to the vocabulary file generated by \"gen_vocab\", used for X (region vectors).");   
    h.item_required(kw_pch_sz, "Region size."); 
    h.item_required(kw_pch_step, "Region stride.", "1"); 
    h.item_required(kw_padding, "Padding size.  Region size minus one is recommended.");   
    h.item_required(kw_feat_fn, "Pathname to the internal-feature file.");     
    h.item_required(kw_f_pch_sz, "Region size used to generate the internal-feature file."); 
    h.item_required(kw_f_pch_step, "Region stride used to generate the internal-feature file."); 
    h.item_required(kw_f_padding, "Padding size used to generate the internal-feature file.");      
    h.item_required(kw_dist, "Size of adjacent regions used to produce Y (target).");         
    h.item(kw_top_num_total, "Number of the larget internal feature components to retain.  The rest will be set to zero."); 
    h.item(kw_do_lower, help_do_lower);     
    h.item(kw_do_utf8dashes, help_do_utf8dashes); 
    h.item(kw_do_nolr, help_do_nolr); 
    /* txt_ext, x_ext, y_ext, do_no_skip, min_x, min_y, top_num_each */
    h.end(); 
  }   
}; 

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_regions_parsup(int argc, const char *argv[]) const
{
  const char *eyec = "AzPrepText2::gen_regions_parsup"; 
  AzPrepText2_gen_regions_parsup_Param p(argc, argv, out);   
  check_batch_id(p.s_batch_id); 
  
  AzMats_file<AzSmat> mfile; 
  int feat_data_num = mfile.reset_for_read(p.s_feat_fn.c_str()); 

  AzStrPool sp_typ(10,10); sp_typ.put(kw_bow, kw_seq); 
  AzXi::check_input(p.s_xtyp.c_str(), &sp_typ, eyec, kw_xtyp);  
  bool do_xseq = p.s_xtyp.equals(kw_seq); 
  bool do_skip_stopunk = (do_xseq) ? false : true; 

  AzDic dic(p.s_xdic_fn.c_str()); 
  AzX::throw_if((dic.size() <= 0), AzInputError, eyec, "No vocabulary"); 
  
  /*---  scan files to determine buffer size and #data  ---*/
  AzOut noout; 
  AzStrPool sp_list; 
  AzIntArr ia_data_num; 
  int buff_size = AzTools_text::scan_files_in_list(p.s_inp_fn.c_str(), p.s_txt_ext.c_str(), 
                                                   noout, &sp_list, &ia_data_num);   
  int data_num = ia_data_num.sum(); 
  AzX::throw_if ((data_num != feat_data_num), eyec, "#data mismatch"); 
  
  /*---  read data and generate features  ---*/
  AzDataArr<AzSmat> am_x(data_num), am_y(data_num); 
  
  buff_size += 256; 
  AzBytArr s_buff; 
  AzByte *buff = s_buff.reset(buff_size, 0); 
  int no_data = 0, data_no = 0, cnum = 0, cnum_before_reduce = 0; 
  feat_info fi[2]; 
  for (int fx = 0; fx < sp_list.size(); ++fx) { /* for each file */
    AzBytArr s_fn(sp_list.c_str(fx), p.s_txt_ext.c_str()); 
    const char *fn = s_fn.c_str(); 
    AzTimeLog::print(fn, log_out);   
    AzFile file(fn); 
    file.open("rb"); 
    int num_in_file = ia_data_num.get(fx); 
    int inc = num_in_file / 50, milestone = inc; 
    int dx = 0; 
    for ( ; ; ++dx) {  /* for each doc */
      AzTools::check_milestone(milestone, dx, inc); 
      int len = file.gets(buff, buff_size); 
      if (len <= 0) break; 
     
      /*---  X  ---*/
      AzBytArr s_data(buff, len); 
      int my_len = s_data.length();
      AzIntArr ia_tokno;        
      int nn = 1; 
      AzTools_text::tokenize(s_data.point_u(), my_len, &dic, nn, p.do_lower, p.do_utf8dashes, &ia_tokno);        
      
      AzIntArr ia_pos; 
      bool do_allow_zero = false;       
      if (do_xseq) gen_X_seq(ia_tokno, dic.size(), p.pch_sz, p.pch_step, p.padding, 
                             do_allow_zero, do_skip_stopunk, am_x.point_u(data_no), &ia_pos);       
      else         gen_X_bow(ia_tokno, dic.size(), p.pch_sz, p.pch_step, p.padding, 
                             do_skip_stopunk, am_x.point_u(data_no), &ia_pos); 
      AzSmat m_feat; 
      mfile.read(&m_feat);      
      if (am_x.point(data_no)->colNum() <= 0) {
        ++no_data; 
        continue; 
      }
      if (p.top_num_each > 0 || p.top_num_total > 0 || p.scale_y > 0) {
        double min_ifeat = m_feat.min(); 
        AzX::no_support((min_ifeat < 0), eyec, "Negative values for internal-feature components."); 
      }
 
      /*---  Y (ifeat: internal features generated by a supervised model) ---*/ 
      gen_Y_ifeat(p.top_num_each, p.top_num_total, &m_feat, &ia_tokno, &ia_pos, 
                  p.pch_sz, -p.dist, p.dist, p.do_nolr, 
                  p.f_pch_sz, p.f_pch_step, p.f_padding, 
                  am_y.point_u(data_no), fi); 
      if (p.min_yval > 0) {
        am_y.point_u(data_no)->cut(p.min_yval); 
      }                                         
      cnum_before_reduce += am_x.point(data_no)->colNum(); 
      reduce_xy(p.min_x, p.min_y, am_x.point_u(data_no), am_y.point_u(data_no));              
      if (am_x.point(data_no)->colNum() <= 0) {
        ++no_data; 
        continue; 
      }
      cnum += am_x.point(data_no)->colNum(); 
      ++data_no;         
    } /* for each doc */
    AzTools::finish_milestone(milestone); 
    AzBytArr s("   #data="); s << data_no << " no_data=" << no_data << " #col=" << cnum; 
    AzPrint::writeln(out, s); 
  } /* for each file */
  mfile.done();   

  AzBytArr s("#data="); s << data_no << " no_data=" << no_data << " #col=" << cnum << " #col_all=" << cnum_before_reduce;        
  AzPrint::writeln(out, s); 
  s.reset("all:"); fi[0].show(s); AzPrint::writeln(out, s); 
  s.reset("top:"); fi[1].show(s); AzPrint::writeln(out, s); 

  if (p.do_binarize) {
    AzTimeLog::print("Binarizing Y ... ", log_out); 
    for (int dx = 0; dx < data_no; ++dx) am_y(dx)->binarize(); /* (x>0) ? 1 : (x<0) ? -1 : 0 */
  }
  else if (p.scale_y > 0) {
    double max_top = fi[1].max_val; 
    double scale = 1; 
    if (max_top < p.scale_y) for ( ; ; scale *= 2) if (max_top*scale >= p.scale_y) break; 
    if (max_top > p.scale_y*2) for ( ; ; scale /= 2) if (max_top*scale <= p.scale_y*2) break; 
    s.reset("Multiplying Y with "); s << scale; AzPrint::writeln(out, s); 
    for (int dx = 0; dx < data_no; ++dx) am_y(dx)->multiply(scale); 
  }  
  
  const char *outnm = p.s_rnm.c_str(); 
  AzTimeLog::print("Generating X ... ", out);  
  write_XY(am_x, data_no, p.s_batch_id, outnm, p.s_x_ext.c_str(), &dic, xtext_ext); 
  AzTimeLog::print("Generating Y ... ", out);  
  write_XY(am_y, data_no, p.s_batch_id, outnm, p.s_y_ext.c_str());   
}

/*-------------------------------------------------------------------------*/
void AzPrepText2::gen_Y_ifeat(int top_num_each, int top_num_total, const AzSmat *m_feat, 
                              const AzIntArr &ia_tokno, const AzIntArr &ia_pos, 
                              int xpch_sz, int min_dist, int max_dist, 
                              bool do_nolr, 
                              int f_pch_sz, int f_pch_step, int f_padding, 
                              AzSmat *m_y, 
                              feat_info fi[2]) const
{
  const char *eyec = "AzPrepText2::gen_neigh_topfeat"; 
  AzX::throw_if_null(m_feat, eyec, "m_feat"); 
  AzX::throw_if_null(m_y, eyec, "m_y"); 
  int t_num; 
  const int *tokno = ia_tokno.point(&t_num); 

  int feat_sz = m_feat->rowNum(); 
  int f_pch_num = DIVUP(t_num+f_padding*2-f_pch_sz, f_pch_step) + 1; 
  if (m_feat->colNum() != f_pch_num) {
    AzBytArr s("#patch mismatch: Expcected: "); s << f_pch_num << " Actual: " << m_feat->colNum(); 
    AzX::throw_if(true, AzInputError, eyec, s.c_str()); 
  }
  
  if (do_nolr) m_y->reform(feat_sz,    ia_pos.size()); 
  else         m_y->reform(feat_sz*2,  ia_pos.size()); 
  for (int ix = 0; ix < ia_pos.size(); ++ix) {
    int xtx0 = ia_pos[ix]; 
    int xtx1 = xtx0 + xpch_sz; 
     
    AzIFarr ifa_ctx; 
    int offs = 0;     
    for (int tx = xtx0+min_dist; tx < xtx0; ++tx) {
      if (tx + f_pch_sz > xtx0) break; 
      set_ifeat(m_feat, top_num_each, (tx+f_padding)/f_pch_step, offs, &ifa_ctx, fi); 
    }
    
    if (!do_nolr) offs = feat_sz; 
    for (int tx = xtx1; tx < xtx1+max_dist; ++tx) {
      if (tx + f_pch_sz > xtx1+max_dist) break; 
      set_ifeat(m_feat, top_num_each, (tx+f_padding)/f_pch_step, offs, &ifa_ctx, fi); 
    }
    ifa_ctx.squeeze_Max(); 
    if (top_num_total > 0 && ifa_ctx.size() > top_num_total) {
      ifa_ctx.sort_Float(false); 
      ifa_ctx.cut(top_num_total);       
    }
    m_y->col_u(ix)->load(&ifa_ctx); 
  }
}                                   

/*-------------------------------------------------------------------------*/
void AzPrepText2::set_ifeat(const AzSmat *m_feat, int top_num, 
                 int col, int offs, AzIFarr *ifa_ctx, feat_info fi[2]) const
{                 
  if (col < 0 || col >= m_feat->colNum()) return; 
  AzIFarr ifa; m_feat->col(col)->nonZero(&ifa); 
  fi[0].update(ifa); 
  if (top_num > 0 && ifa.size() > top_num) {
    ifa.sort_FloatInt(false); /* descending order */
    ifa.cut(top_num); 
  }
  fi[1].update(ifa); 
  if (offs == 0) ifa_ctx->concat(&ifa); 
  else {
    for (int ix = 0; ix < ifa.size(); ++ix) {
      int row = ifa.getInt(ix); 
      double val = ifa.get(ix); 
      ifa_ctx->put(row+offs, val); 
    }    
  }
}   
