#!/usr/bin/perl -w

# read the book data
# with each move, throw away the unneeded lines
# of the needed lines, select the best move
# and return it

# use XML::Dumper;

my %dict;

open(FH,"book.dat") || die("Can't read book.");

while(<FH>) {
  if (/^\#/) { next };
  @moves = split();
  for ( my $i = 0; $i < $#moves; $i++ ) {
    # my $key = ":".join(":",@moves[0..$i]);
    my $key = join(":",@moves[0..$i]);
    # we need the ! indicating igui */
    $key =~ s/[x?-]//g;
    $key =~ s/[!?]:/:/g;

    if ( $moves[$i+1] =~ /\?$/ ) { next; } # don't suggest bad moves 

    if ( !defined($dict{$key}) || ( $dict{$key} =~ /\!$/ )) {
      # enter normal move
      # or override with good move
      $dict{$key} = $moves[$i+1];
      print STDOUT $key." ".$moves[$i+1]."\n";
    }
  }
}

close(FH);


# my $dump = new XML::Dumper;

# print STDERR $dump->pl2xml(%dict);

exit(0);

my $static_key = '';

while(<>) {
  # always remember the old moves and concatenate for new key
  chomp;
  s/[x-]//g;
  $static_key .= ":".$_;
  $tmp = $dict{$static_key};

  if ( $tmp =~ /!$/ ) {
    chop($tmp);
  }
  print "$dict{$static_key}\n";

  $tmp =~ s/[x-]//g;
  $static_key .= ":".$tmp;
}
