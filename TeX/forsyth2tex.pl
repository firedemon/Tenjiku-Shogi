$_=<>;
chomp();
@lines = split("/",$_);
$full='';

%halfchupieces = ( 'K' => '\Gyoku',
		'k' => '\OO',
		'DE' => '\ZO',
		'de' => '\Zo',
		'+DE' => '\Red{\SHII}',
		'+de' => '\Red{\Shii}',
		'g' => '\Kin',
		'G' => '\KIN',
		'+g' => '\Red{\Hi}',
		'+G' => '\Red{\HI}',
		's' => '\Gin',
		'S' => '\GIN',
		'+s' => '\Red{\Ken}',
		'+S' => '\Red{\KEN}',
		'c' => '\Do',
		'C' => '\DO',
		'+c' => '\Red{\Ko}',
		'+C' => '\Red{\KO}',
		'fl' => '\Hyo',
		'FL' => '\HYO',
		'+fl' => '\Red{\Kaku}',
		'+FL' => '\Red{\KAKU}',
		'l' => '\Kyo',
		'L' => '\KYO',
		'+l' => '\Red{\Haku}',
		'+L' => '\Red{\HAKU}',
		'rc' => '\Han',
		'RC' => '\HAN',
		'+rc' => '\Red{\Gei}',
		'+RC' => '\Red{\GEI}',
		'b' => '\Kaku',
		'B' => '\KAKU',
		'+b' => '\Red{\Ma}',
		'+B' => '\Red{\MA}',
		'bt' => '\Koii',
		'BT' => '\KOII',
		'+bt' => '\Red{\Roku}',
		'+BT' => '\Red{\ROKU}',
		'ph' => '\Ho',
		'PH' => '\HO',
		'+ph' => '\Red{\Hon}',
		'+PH' => '\Red{\HON}',
		'ky' => '\Ki',
		'KY' => '\KI',
		'+ky' => '\Red{\Shi}',
		'+KY' => '\Red{\SHI}',
		'sm' => '\Ko',
		'SM' => '\KO',
		'+sm' => '\Red{\Cho}',
		'+SM' => '\Red{\CHO}',
		'+vm' => '\Red{\Gyu}',
		'+VM' => '\Red{\GYU}',
		'vm' => '\Ken',
		'VM' => '\KEN',
		'r' => '\Hi',
		'R' => '\HI',
		'+r' => '\Red{\Ryu}',
		'+R' => '\Red{\RYU}',
		'dh' => '\Ma',
		'DH' => '\MA',
		'+dh' => '\Red{\Kuo}',
		'+DH' => '\Red{\KUO}',
		'dk' => '\Ryu',
		'DK' => '\RYU',
		'+dk' => '\Red{\Ju}',
		'+DK' => '\Red{\JU}',
		'fk' => '\Hon',
		'FK' => '\HON',
		'ln' => '\Shi',
		'LN' => '\SHI',
		'gb' => '\Chu',
		'GB' => '\CHU',
		'+gb' => '\Red{\Suizo}',
		'+GB' => '\Red{\SUIZO}',
		'P'  => '\FU',
		'p'  => '\Fu',
		'+P'  => '\Red{\TO}',
		'+p'  => '\Red{\To}',
);


%fullchupieces = ( 'K' => '\Gyokusho',
		'k' => '\OOSHO',
		'DE' => '\SUIZO',
		'de' => '\Suizo',
		'+DE' => '\Red{\Shishi}',
		'+de' => '\Red{\SHISHI}',
		'g' => '\Kinsho',
		'G' => '\KINSHO',
		'+g' => '\Red{\Hisha}',
		'+G' => '\Red{\HISHA}',
		's' => '\Ginsho',
		'S' => '\GINSHO',
		'+s' => '\Red{\Kengyo}',
		'+S' => '\Red{\KENGYO}',
		'c' => '\Dosho',
		'C' => '\DOSHO',
		'+c' => '\Red{\Ogyo}',
		'+C' => '\Red{\OGYO}',
		'fl' => '\Mohyo',
		'FL' => '\MOHYO',
		'+fl' => '\Red{\Kakugyo}',
		'+FL' => '\Red{\KAKUGYO}',
		'l' => '\Kyosha',
		'L' => '\KYOSHA',
		'+l' => '\Red{\Hakku}',
		'+L' => '\Red{\HAKKU}',
		'rc' => '\Hansha',
		'RC' => '\HANSHA',
		'+rc' => '\Red{\Keigei}',
		'+RC' => '\Red{\KEIGEI}',
		'b' => '\Kakugyo',
		'B' => '\KAKUGYO',
		'+b' => '\Red{\Ryume}',
		'+B' => '\Red{\RYUME}',
		'bt' => '\Moko',
		'BT' => '\MOKO',
		'+bt' => '\Red{\Hiroku}',
		'+BT' => '\Red{\HIROKU}',
		'ph' => '\Hoo',
		'PH' => '\HOO',
		'+ph' => '\Red{\Hono}',
		'+PH' => '\Red{\HONO}',
		'ky' => '\Kirin',
		'KY' => '\KIRIN',
		'+ky' => '\Red{\Shishi}',
		'+KY' => '\Red{\SHISHI}',
		'sm' => '\Ogyo',
		'SM' => '\OGYO',
		'+sm' => '\Red{\Honcho}',
		'+SM' => '\Red{\HONCHO}',
		'+vm' => '\Red{\Suigyu}',
		'+VM' => '\Red{\SUIGYU}',
		'vm' => '\Kengyo',
		'VM' => '\KENGYO',
		'r' => '\Hisha',
		'R' => '\HISHA',
		'+r' => '\Red{\Ryuo}',
		'+R' => '\Red{\RYUO}',
		'dh' => '\Ryume',
		'DH' => '\RYUME',
		'+dh' => '\Red{\Kakuo}',
		'+DH' => '\Red{\KAKUO}',
		'dk' => '\Ryuo',
		'DK' => '\RYUO',
		'+dk' => '\Red{\Hiju}',
		'+DK' => '\Red{\HIJU}',
		'fk' => '\Hono',
		'FK' => '\HONO',
		'ln' => '\Shishi',
		'LN' => '\SHISHI',
		'gb' => '\Chunin',
		'GB' => '\CHUNIN',
		'+gb' => '\Red{\Suizo}',
		'+GB' => '\Red{\SUIZO}',
		'P'  => '\FUGYO',
		'p'  => '\Fugyo',
		'+P'  => '\Red{\TO}',
		'+p'  => '\Red{\To}',
);


$shogivar=$full.'sho';

if ($#lines==9) {
    print "\\shodiag{%\n";
    $shogivar=$full.'sho';
} elsif ($#lines==12) {
    print "\\chudiag{%\n";
    $shogivar=$full.'chu';
} elsif ($#lines==16) {
    print "\\tenjikudiag{%\n";
    $shogivar=$full.'tenjiku';
} else {
    die "$#lines lines not supported\n";
}

for(my $l =1; $l <= $#lines; $l++) {
    print "\\shogirow{".$l."}{";
    @pieces = split(",",$lines[$l]);
    foreach my $p (@pieces) {
	if ( $p =~ /^[0-9]+$/ ) {
	    my $num = int($p);
	    while($num--) { print "," }
	} else {
	    if ( $shogivar eq 'chu' ) {
		print $halfchupieces{$p}.",";
	    } elsif ( $shogivar eq 'fullchu' ) {
		print $fullchupieces{$p}.",";
	    } elsif ( $shogivar eq 'tenjiku' ) {
		print $tenjikupieces{$p}.",";
	    } elsif ( $shogivar eq 'fulltenjiku' ) {
		print $fulltenjikupieces{$p}.",";
	    } elsif ( $shogivar eq 'sho' ) {
		print $shopieces{$p}.",";
	    } else  {
		die "Unknown shogi variant $shogivar\n";
	    }
	}
    }
    print "}\n";
}

print "}\n";
