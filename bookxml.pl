#!/usr/bin/perl

use XML::Dumper;
my $dump = new XML::Dumper;

my $book = [
	    { 'move' => 'P9l-9k',
	      'next' => [] },
	    { 'move' => 'P8l-8k',
	      'next' => [] },
	    { 'move' => 'P7l-7k',
	      'comment'=>'Threatening mate with VGn8m-2g.',
	      'next' => [{'move'=>'SEg5d-3f','comment'=>'Probably the worse possibility'},
			 {'move'=>'P2e-2f','comment'=>'Probably the better possibility'}] },

];


print $dump->pl2xml($book);
