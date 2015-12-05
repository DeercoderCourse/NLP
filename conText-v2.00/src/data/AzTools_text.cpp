/* * * * *
 *  AzTools_text.cpp 
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

#include "AzTools.hpp"
#include "AzTools_text.hpp"

/*-------------------------------------------------------------------------*/
void AzTools_text::tokenize(AzByte *data, int inp_len, /* used as work area */
                            bool do_utf8dashes, 
                            bool do_lower, 
                            AzStrPool *sp_tok)
{
  int len = inp_len;  
  if (do_utf8dashes) {
    len = replace_utf8dashes(data, len); 
  }
  if (do_lower) {
    AzBytArr s(data, len); s.lwr(); 
    memcpy(data, s.point(), len); 
  }
  sp_tok->reset(); 
  sp_tok->reset(len / 10, 10);  
  AzTools::getStrings(data, len, sp_tok); 
} 

/*-------------------------------------------------------------------------*/
void AzTools_text::tokenize(AzByte *buff, int &len, 
                       const AzDic *dic,
                       int nn, 
                       bool do_lower, bool do_utf8dashes, 
                       /*---  output  ---*/                       
                       AzIntArr *ia_tokno)
{
  const char *eyec = "AzTools_text::tokenize"; 
  AzStrPool sp_tok; 
  tokenize(buff, len, do_utf8dashes, do_lower, &sp_tok);   
  int t_num = sp_tok.size(); 
  if (dic != NULL && ia_tokno != NULL) {
    identify_tokens(&sp_tok, nn, dic, ia_tokno); 
    if (ia_tokno->size() != t_num) throw new AzException(eyec, "Conflict in uni #tokens"); 
  }
}    

/*-------------------------------------------------------------------------*/
void AzTools_text::tokenize(AzByte *buff, int &len, 
                       const AzDic *dic,
                       AzIntArr &ia_nn, 
                       bool do_lower, bool do_utf8dashes, 
                       /*---  output  ---*/                       
                       AzDataArr<AzIntArr> &aia_tokno)
{
  const char *eyec = "AzTools_text::tokenize(multi n)"; 
  AzStrPool sp_tok; 
  tokenize(buff, len, do_utf8dashes, do_lower, &sp_tok);   
  int t_num = sp_tok.size(); 
  aia_tokno.reset(ia_nn.size()); 
  for (int ix = 0; ix < ia_nn.size(); ++ix) {
    identify_tokens(&sp_tok, ia_nn[ix], dic, aia_tokno(ix)); 
    if (aia_tokno[ix]->size() != t_num) throw new AzException(eyec, "Conflict in #tokens"); 
  }
} 

/*-------------------------------------------------------------------------*/
void AzTools_text::identify_1gram(const AzStrPool *sp_tok, 
                       const AzDic *dic_word, 
                       AzIntArr *ia_tokno) /* output */
{
  ia_tokno->reset(sp_tok->size(), -1); 
  int *tokno = ia_tokno->point_u(); 
  for (int ix = 0; ix < sp_tok->size(); ++ix) tokno[ix] = dic_word->find(sp_tok->c_str(ix));  
}      

/*-------------------------------------------------------------------------*/
void AzTools_text::identify_ngram(const AzStrPool *sp_tok, 
                       int nn, 
                       const AzDic *dic_word, 
                       AzIntArr *ia_tokno) /* output */
{
  ia_tokno->reset(sp_tok->size(), -1); 
  int *tokno = ia_tokno->point_u(); 
  for (int ix = 0; ix < sp_tok->size(); ++ix) tokno[ix] = dic_word->find_ngram(sp_tok, ix, nn);  
} 
    
/*-------------------------------------------------------------------------*/
int AzTools_text::replace_utf8dashes(AzByte *data, int len) /* inout */
{
  const char *eyec = "AzTools_text:;replace_utf8dashes"; 
  AzBytArr s_mydata;
  AzByte *mydata = s_mydata.reset(len*2, 0); 
  AzByte *mywp = mydata;  
  /* 0xe28093: en dash (often used as in 1900-1935) */
  /* 0xe28094: em dash (long dash) */ 
  /* 0xe2809c: double quote begin -> [ " ]  */
  /* 0xe2809d: duoble quote end   -> [ " ]  */
  /* 0xe28098: single quote begin -> [ ' ]*/
  /* 0xe28099: single quote end -> [ 's] if ending a token; [ ' ] otherwise */
  const AzByte *data_end = data+len, *wp = data; 
  for ( ; wp < data_end; ) {
    const AzByte *ptr = (AzByte *)memchr(wp,  0xE2, data_end-wp); 
    if (ptr == NULL) {
      ptr = data_end; 
    }
    int mvlen = Az64::ptr_diff(ptr-wp, eyec); 
    if (mvlen > 0) {
      memcpy(mywp, wp, mvlen);  mywp += mvlen; /* string before 0xE2 */
    }
    if (ptr+3 <= data_end) {
      AzByte prevch = (ptr-1 >= data) ? *(ptr-1) : 0; 
      AzByte nextch = (ptr+3 < data_end) ? *(ptr+3) : 0; 
      AzBytArr s; 
      if (*(ptr+1) == 0x80) {
        if (*(ptr+2) == 0x93) { /* en dash */
          if (prevch>='0' && prevch<='9' && nextch>='0' && nextch<='9') s.c("-");  /* between digits */
          else                                                          s.c(" - "); 
        }
        else if (*(ptr+2) == 0x94)                          s.c(" - "); /* em dash */
        else if (*(ptr+2) == 0x98)                          s.c(" ' "); 
        else if (*(ptr+2) == 0x9c || *(ptr+2) == 0x9d)      s.c(" \" "); /* double quote */        
        else if (*(ptr+2) == 0x99) {
          if (ptr+5<=data_end && memcmp(ptr+3, "s ", 2)==0) s.c(" '");    
          else                                              s.c(" ' "); 
        }
        else s.c(ptr, 3); 
      }
      else s.c(ptr, 3); 
      memcpy(mywp, s.point(), s.length());  mywp += s.length(); 
      wp = ptr+3;     
    }
    else {
      mvlen = Az64::ptr_diff(data_end-ptr, eyec);
      if (mvlen > 0) {
        memcpy(mywp, ptr, mvlen); mywp += mvlen; 
      }
      wp = data_end; 
    }
  }
  int mydata_len = Az64::ptr_diff(mywp-mydata); 
  memcpy(data, mydata, mydata_len); 
  mydata[mydata_len] = '\0'; 
  return mydata_len; 
}

/*-------------------------------------------------------------------------*/
int AzTools_text::scan_files_in_list(const char *inp_fn, 
                                     const char *ext, 
                                     const AzOut &out,  
                                     /*---  output  ---*/
                                     AzStrPool *out_sp_list, /* may be NULL */
                                     AzIntArr *ia_data_num) /* may be NULL; number of docs in each file */
{
  AzStrPool sp_list; 
  if (AzBytArr::endsWith(inp_fn, ".lst")) AzTools::readList(inp_fn, &sp_list); 
  else                                    sp_list.put(inp_fn); 
  if (ia_data_num != NULL) ia_data_num->reset(); 
  int buff_size = 0; 
  for (int fx = 0; fx < sp_list.size(); ++fx) {
    AzBytArr s(sp_list.c_str(fx)); s.c(ext); 
    const char *fn = s.c_str(); 
    AzTimeLog::print("scanning ", fn, out); 
    AzIntArr ia_data_len; 
    AzFile::scan(fn, 1024*1024*100, &ia_data_len); 
    buff_size = MAX(buff_size, ia_data_len.max()); 
    if (ia_data_num != NULL) ia_data_num->put(ia_data_len.size()); 
  }  
  if (out_sp_list != NULL) out_sp_list->reset(&sp_list); 
  return buff_size; 
}

/*-------------------------------------------------------------------------*/
void AzTools_text::feat_to_text(const AzSvect *v_feat, 
                           const AzDic *dic_word, 
                           bool do_wordonly,                            
                           AzBytArr &s, /* output */
                           /*---  options mainly for bag-of-n-grams  ---*/
                           bool do_sort,                            
                           int print_max, 
                           char dlm) /* for n-grams */
{
  AzIFarr ifa_word_val; 
  v_feat->nonZero(&ifa_word_val); 
  if (do_sort) ifa_word_val.sort_Float(false); /* descending order */
  int ix; 
  for (ix = 0; ix < ifa_word_val.size(); ++ix) {
    if (print_max > 0 && ix >= print_max) break; 
    int word_no; 
    double val = ifa_word_val.get(ix, &word_no); 
    AzBytArr s_word(dic_word->c_str(word_no)); 
    s_word.replace(' ', dlm); 
    s << s_word.c_str(); 
    if (!do_wordonly) s << ":" << val; 
    s << " "; 
  }
  if (ix < ifa_word_val.size()) s << " ..."; 
} 


/*-------------------------------------------------------------------------*/
void AzTools_text::count_words_get_cats(const AzOut &out, bool do_ignore_bad, 
                            bool do_count_unk, 
                            const char *fn, const char *txt_ext, const char *cat_ext, 
                            const AzDic &dic, const AzDic &dic_cat, 
                            int max_nn, 
                            bool do_allow_multi, bool do_allow_nocat, 
                            bool do_lower, bool do_utf8dashes, 
                            AzSmat *m_count, AzSmat *m_cat, 
                            bool do_no_cat)
{
  const char *eyec = "AzTools_text::count_words_get_cats"; 
  if (do_no_cat) {
    AzTimeLog::print("Don't read cats",  out); 
  }
  /*---  scan training data files to determine buffer size and #data  ---*/
  AzStrPool sp_list; 
  AzIntArr ia_num; 
  int buff_size = scan_files_in_list(fn, txt_ext, out, &sp_list, &ia_num); 
  int data_num = ia_num.sum(); 
  
  /*---  read training data  ---*/
  m_cat->reform(dic_cat.size(), data_num);   
  int unk_idx = (do_count_unk) ? dic.size() : -1; 
  if (do_count_unk) m_count->reform(dic.size()+1, data_num); 
  else        m_count->reform(dic.size(), data_num); 
  
  buff_size += 256; 
  AzBytArr s_buff; 
  AzByte *buff = s_buff.reset(buff_size, 0); 
  int no_cat = 0, multi_cat = 0; 
  int data_no = 0; 
  for (int fx = 0; fx < sp_list.size(); ++fx) { /* for ecah file */
    AzBytArr s_txt_fn(sp_list.c_str(fx), txt_ext); 
    const char *fn = s_txt_fn.c_str(); 
    AzStrPool sp_cat;     
    if (!do_no_cat) {
      AzBytArr s_cat_fn(sp_list.c_str(fx), cat_ext); 
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
      if (!do_no_cat) {
        if (num_in_file >= sp_cat.size()) {
          throw new AzException(AzInputError, eyec, "#data mismatch: btw text file and cat file"); 
        }
        AzBytArr s_cat;     
        sp_cat.get(num_in_file, &s_cat);
         
        AzIntArr ia_cats; 
        AzBytArr s_err; 
        parse_cats(&s_cat, '|', do_allow_multi, do_allow_nocat, &dic_cat, &ia_cats, multi_cat, no_cat, s_err); 
        if (s_err.length() > 0) {
          if (do_ignore_bad) continue; 
          throw new AzException(AzInputError, eyec, s_err.c_str()); 
        }
        else  m_cat->col_u(data_no)->load(&ia_cats, 1);                               
      }
           
      /*---  text  ---*/
      int unk = 0; 
      AzStrPool sp_tok;               
      AzTools_text::tokenize(buff, len, do_utf8dashes, do_lower, &sp_tok); 
      AzIFarr ifa_count;  
      for (int ix = 0; ix < sp_tok.size(); ++ix) {   
        for (int nn = 1; nn <= max_nn; ++nn) {
          int id = dic.find_ngram(&sp_tok, ix, nn);  
          if (id >= 0) ifa_count.put(id, 1); 
          else if (sp_tok.size()-ix >= nn) ++unk; 
        }
      }
      ifa_count.squeeze_Sum(); 
      m_count->col_u(data_no)->load(&ifa_count);      
      if (unk_idx >= 0 && unk != 0) m_count->set(unk_idx, data_no, unk); 
      ++data_no;
    } /* for each doc */
    if (!do_no_cat && num_in_file != sp_cat.size()) {
      throw new AzException(AzInputError, eyec, "#data mismatch2: btw text file and cat file"); 
    }       
  } /* for each file */
  m_cat->resize(data_no); 
  m_count->resize(data_no); 
}

/*-------------------------------------------------------------------------*/
/* static */
void AzTools_text::parse_cats(const AzBytArr *s_cat, 
                         AzByte dlm,  /* e.g., | for, e.g., GSPO|M11|M12 */
                         bool do_allow_multicat, bool do_allow_nocat, 
                         const AzDic *dic_cat, AzIntArr *ia_cats, /*output */
                         int &multi_cat, int &no_cat, /* inout */
                         AzBytArr &s_err)
{                         
  s_err.reset(); 
  bool my_do_allow_multi = true; 
  _parse_cats(s_cat, dlm, my_do_allow_multi, dic_cat, ia_cats, multi_cat, no_cat); 
  int num = ia_cats->size(); 
  if (num == 1) return; 
  if (num == 0) {
    if (do_allow_nocat) return; 
    s_err.reset("For single-label classification, a data point without any label is not allowed.  Specify "); 
    s_err << kw_do_allow_nocat << " to allow no-label data points."; 
  }
  else if (num > 1) {
    if (do_allow_multicat) return;  
    s_err.reset("For single-label classification, a data point with multiple labels is not allowed.  Specify "); 
    s_err << kw_do_allow_multi << " to allow multi-label data points."; 
  }
} 

/*-------------------------------------------------------------------------*/
/* static */
void AzTools_text::_parse_cats(const AzBytArr *s_cat, 
                          AzByte dlm,  /* e.g., | for, e.g., GSPO|M11|M12 */
                          bool do_allow_multicat, 
                          const AzDic *dic_cat, 
                          AzIntArr *ia_cats, /*output */
                          int &multi_cat, /* inout */
                          int &no_cat) /* inout */
{      
  const char *eyec = "AzTools_text::_parse_cats";
  
  ia_cats->reset();
  if (s_cat->length() == 0) { /* no cat */
    ++no_cat; 
    return; 
  }  
  if (s_cat->contains(dlm)) {
    ++multi_cat; 
    if (!do_allow_multicat) return;
  }
  
  AzStrPool sp_cat; 
  AzTools::getStrings(s_cat->point(), s_cat->length(), dlm, &sp_cat); 
  int ix; 
  for (ix = 0; ix < sp_cat.size(); ++ix) {
    int cat = dic_cat->find(sp_cat.c_str(ix)); 
    if (cat < 0) {
      throw new AzException(AzInputError, eyec, "Undefined cat: ", sp_cat.c_str(ix)); 
    }
    ia_cats->put(cat); 
  }
  int old_num = ia_cats->size(); 
  ia_cats->unique();  
  int new_num = ia_cats->size();
} 