#define _AZ_MAIN_
#include "AzUtil.hpp"
#include "AzPrepText.hpp"
#include "AzPrepText2.hpp"

void help() {
  cout << "action:  gen_vocab | gen_regions | gen_regions_unsup | gen_regions_parsup | show_regions | gen_nbw | gen_nbwfeat | gen_b_feat | split_text" << endl;  
  cout << endl; 
  cout << "Enter, for example, \"prepText gen_vocab\" to print help for a specific action." << endl; 
}

/*******************************************************************/
/*     main                                                        */
/*******************************************************************/
int main(int argc, const char *argv[]) 
{
  AzException *stat = NULL; 

  if (argc < 2) {
    help(); 
    return -1; 
  }
  
  const char *action = argv[1]; 
  int oo = 1; /* pass action too */
  
  try {
    Az_check_system2_(); 
    AzPrepText prep(log_out);    
    AzPrepText2 prep2(log_out); 
    if      (strcmp(action, "gen_vocab") == 0)    prep.gen_vocab(argc-oo, argv+oo); 
    else if (strcmp(action, "gen_regions") == 0)  prep.gen_regions(argc-oo, argv+oo); 
    else if (strcmp(action, "show_regions") == 0) prep.show_regions(argc-oo, argv+oo); 
    else if (strcmp(action, "gen_nbw") == 0)      prep.gen_nbw(argc-oo, argv+oo);     
    else if (strcmp(action, "gen_nbwfeat") == 0)  prep.gen_nbwfeat(argc-oo, argv+oo);   
    else if (strcmp(action, "gen_b_feat") == 0)   prep.gen_b_feat(argc-oo, argv+oo); 
    else if (strcmp(action, "split_text") == 0)   prep.split_text(argc-oo, argv+oo);       
    else if (strcmp(action, "gen_regions_unsup") == 0)  prep2.gen_regions_unsup(argc-oo, argv+oo);   
    else if (strcmp(action, "gen_regions_parsup") == 0) prep2.gen_regions_parsup(argc-oo, argv+oo);    
    else {
      help(); 
      return -1; 
    }
  }
  catch (AzException *e) {
    stat = e; 
  }

  if (stat != NULL) {
    cout << stat->getMessage() << endl; 
    return -1; 
  }

  return 0; 
}

