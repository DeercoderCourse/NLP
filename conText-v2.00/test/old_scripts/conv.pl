  #---  merge X and Y to make a training file for binary classification with SVMlight
  #---  one vs. others regarding "focus_lab" as the positive class and the rest as negative. 
  #---  feature#0 -> feature#x where x is the number of features. 
  #---     since in my format feature# starts with 0 and in the SVMlight format it starts with 1. 
  #---  Input files should be in the sparse format.  
  
  use strict 'vars'; 

  my $arg_num = $#ARGV + 1; 
  if ($arg_num != 2) {
    print STDERR "inpnm focus_lab\n"; 
    exit -1; 
  }

  my $argx = 0; 
  my $inpnm = $ARGV[$argx++]; 
  my $focus_lab = $ARGV[$argx++]; 

  my $x_fn = $inpnm . '.x'; 
  my $y_fn = $inpnm . '.y'; 
  
  open(X, $x_fn) or die("Can't open $x_fn\n"); 
  my $y_num = 0; 
  my $is_first = 1; 
  my @y; 
  if ($focus_lab >= 0) {
    open(Y, $y_fn) or die("Can't open $y_fn\n");
    while(<Y>) {
      my $line = $_; 
      chomp $line; 
      if ($is_first == 1) {
        if ($line !~ /sparse/) {
          print STDERR "Y: the first line should contain \"sparse\": $line\n"; 
          exit -1; 
        }
        $is_first = 0; 
      }
      elsif ($line !~ /\S/) {
        print STDERR "No y\n"; 
        exit -1; 
      }
      else {
        if ($line =~ /\s/) {
          print STDERR "invalid y: \[$line\]\n"; 
          exit -1; 
        }
        $y[$y_num] = $line;      
        ++$y_num; 
      }
    }
    close(Y); 
  }
  
  $is_first = 1; 
  my $x_num = 0; 
  my $feat_num = 0; 
  while(<X>) {
    my $line = $_; 
    chomp $line; 
    if ($is_first == 1) {
      if ($line =~ /sparse\s+(\d+)\s*$/) {
        $feat_num = $1; 
      }
      else {
        print STDERR "X: the first line should contain \"sparse\": $line\n"; 
        exit -1; 
      }
      $is_first = 0; 
    }
#    elsif ($line !~ /\S/) {} # ignore; 
    else {
      $line = &conv_feat($line, $feat_num); 
      my $my_y = '0';                   # unlabeled 
      if ($focus_lab >= 0) {
        if ($y[$x_num] == $focus_lab) { $my_y = '+1'; }
        else                          { $my_y = '-1'; }
      }
      print "$my_y  $line\n"; 
      ++$x_num; 
    }
  }
  close(X); 
  
  if ($focus_lab >= 0 && $x_num != $y_num) {
    print STDERR " number conflict: $x_num, $y_num\n"; 
    exit -1; 
  }
  
#---  replace feature#0 with ...   
sub conv_feat {
  my($inp, $feat_num) = @_; 
  my $out = $inp; 
  if ($out =~ /^\s+(\S.*)$/) {
    $out = $1; 
  }
  if ($out =~ /^(.*\S)\s+$/) {
    $out = $1; 
  }
  $out =~ s/\s/  /gs; 

  $out =~ s/(\s)(\d+)(\s)/$1$2\:1$3/gs; 
  $out =~ s/(\s)(\d+)$/$1$2\:1/gs; 
  $out =~ s/^(\d+)(\s)/$1\:1$2/gs; 
  $out =~ s/^(\d+)$/$1\:1/gs; 
  
  if ($out =~ /^0\:(\S+)$/) {
    my $val = $1; 
    $out = "$feat_num\:$val"; 
  }
  elsif ($out =~ /^0\:(\S+)\s+(.*)$/) {
    my $val = $1; 
    my $rest = $2; 
    $out = "$rest $feat_num\:$val"; 
  }
#  elsif ($out =~ /^0$/) {
#    $out = "$feat_num\:1"; 
#  }
#  elsif ($out =~ /^0\s+(.*)$/) {
#    my $rest = $1; 
#    $out = "$rest $feat_num\:1"; 
#  }
  
  $out =~ s/\s+/ /gs; 
  
  return $out; 
}  