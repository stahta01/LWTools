#!/usr/bin/env perl
#
# these tests determine if the opcodes for each instruction, in each
# addressing mode, are correct.
#
# The following list is used to construct the tests. The key is the
# mneumonic and the value is a list of address mode characters as follows
#
# R: register/inherent
# I: immediate
# E: extended
# D: direct
# i: indexed
# r: register to register (TFR, etc.)
# p: psh/pul
# each letter is followed by an = and the 2 or 4 digit opcode in hex
# each entry is separated by a comma

$lwasm = './lwasm/lwasm';

%insnlist = (
	'neg' => 'D=00,E=70,i=60',
	'com' => 'D=03,E=73,i=63',
	'lsr' => 'D=04,E=74,i=64',
	'ror' => 'D=06,E=76,i=66',
	'asr' => 'D=07,E=77,i=67',
	'lsl' => 'D=08,E=78,i=68',
	'rol' => 'D=09,E=79,i=69',
	'dec' => 'D=0A,E=7A,i=6A',
	'inc' => 'D=0C,E=7C,i=6C',
	'tst' => 'D=0D,E=7D,i=6D',
	'jmp' => 'D=0E,E=7E,i=6E',
	'clr' => 'D=0F,E=7F,i=6F',
	'nop' => 'R=12',
	'sync' => 'R=13',
	'lbra' => 'b=16',
	'lbsr' => 'b=17',
	'daa' => 'R=19',
	'orcc' => 'I=1A',
	'andcc' => 'I=1C',
	'sex' => 'R=1D',
	'exg' => 'r=1E',
	'tfr' => 'r=1F',
	'bra' => 'b=20',
	'brn' => 'b=21',
	'lbrn' => 'b=1021',
	'bhi' => 'b=22',
	'lbhi' => 'b=1022',
	'bls' => 'b=23',
	'lbls' => 'b=1023',
	'bcc' => 'b=24',
	'lbcc' => 'b=1024',
	'bhs' => 'b=24',
	'lbhs' => 'b=1024',
	'bcs' => 'b=25',
	'lbcs' => 'b=1025',
	'blo' => 'b=25',
	'lblo' => 'b=1025',
	'bne' => 'b=26',
	'lbne' => 'b=1026',
	'beq' => 'b=27',
	'lbeq' => 'b=1027',
	'bvc' => 'b=28',
	'lbvc' => 'b=1028',
	'bvs' => 'b=29',
	'lbvs' => 'b=1029',
	'bpl' => 'b=2A',
	'lbpl' => 'b=102A',
	'bmi' => 'b=2B',
	'lbmi' => 'b=102B',
	'bge' => 'b=2C',
	'lbge' => 'b=102C',
	'blt' => 'b=2D',
	'lblt' => 'b=102D',
	'bgt' => 'b=2E',
	'lbgt' => 'b=102E',
	'ble' => 'b=2F',
	'lble' => 'b=102F',
	'leax' => 'i=30',
	'leay' => 'i=31',
	'leas' => 'i=32',
	'leau' => 'i=33',
	'pshs' => 'p=34',
	'puls' => 'p=35',
	'pshu' => 'p=36',
	'pulu' => 'p=37',
	'rts' => 'R=39',
	'abx' => 'R=3A',
	'rti' => 'R=3B',
	'cwai' => 'I=3C',
	'mul' => 'R=3D',
	'swi' => 'R=3F',
	'swi2' => 'R=103F',
	'swi3' => 'R=113F',
	'nega' => 'R=40',
	'coma' => 'R=43',
	'lsra' => 'R=44',
	'rora' => 'R=46',
	'asra' => 'R=47',
	'lsla' => 'R=48',
	'rola' => 'R=49',
	'deca' => 'R=4A',
	'inca' => 'R=4C',
	'tsta' => 'R=4D',
	'clra' => 'R=4F',
	'negb' => 'R=50',
	'comb' => 'R=53',
	'lsrb' => 'R=54',
	'rorb' => 'R=56',
	'asrb' => 'R=57',
	'lslb' => 'R=58',
	'rolb' => 'R=59',
	'decb' => 'R=5A',
	'incb' => 'R=5C',
	'tstb' => 'R=5D',
	'clrb' => 'R=5F',
	'suba' => 'I=80,D=90,i=A0,E=B0',
	'cmpa' => 'I=81,D=91,i=A1,E=B1',
	'sbca' => 'I=82,D=92,i=A2,E=B2',
	'subd' => 'I=83,D=93,i=A3,E=B3',
	'cmpd' => 'I=1083,D=1093,i=10A3,E=10B3',
	'cmpu' => 'I=1183,D=1193,i=11A3,E=11B3',
	'anda' => 'I=84,D=94,i=A4,E=B4',
	'bita' => 'I=85,D=95,i=A5,E=B5',
	'lda' => 'I=86,D=96,i=A6,E=B6',
	'sta' => 'D=97,i=A7,E=B7',
	'eora' => 'I=88,D=98,i=A8,E=B8',
	'adca' => 'I=89,D=99,i=A9,E=B9',
	'ora' => 'I=8A,D=9A,i=AA,E=BA',
	'adda' => 'I=8B,D=9B,i=AB,E=BB',
	'cmpx' => 'I=8C,D=9C,i=AC,E=BC',
	'cmpy' => 'I=108C,D=109C,i=10AC,E=10BC',
	'cmps' => 'I=118C,D=119C,i=11AC,E=11BC',
	'bsr' => 'b=8D',
	'jsr' => 'D=9D,i=AD,E=BD',
	'ldx' => 'I=8E,D=9E,i=AE,E=BE',
	'ldy' => 'I=108E,D=109E,i=10AE,E=10BE',
	'stx' => 'D=9F,i=AF,E=BF',
	'sty' => 'D=109F,i=10AF,E=10BF',
	'subb' => 'I=C0,D=D0,i=E0,E=F0',
	'cmpb' => 'I=C1,D=D1,i=E1,E=F1',
	'sbcb' => 'I=C2,D=D2,i=E2,E=F2',
	'addd' => 'I=C3,D=D3,i=E3,E=F3',
	'andb' => 'I=C4,D=D4,i=E4,E=F4',
	'bitb' => 'I=C5,D=D5,i=E5,E=F5',
	'ldb' => 'I=C6,D=D6,i=E6,E=F6',
	'stb' => 'D=D7,i=E7,E=F7',
	'eorb' => 'I=C8,D=D8,i=E8,E=F8',
	'adcb' => 'I=C9,D=D9,i=E9,E=F9',
	'orb' => 'I=CA,D=DA,i=EA,E=FA',
	'addb' => 'I=CB,D=DB,i=EB,E=FB',
	'ldd' => 'I=CC,D=DC,i=EC,E=FC',
	'std' => 'D=DD,i=ED,E=FD',
	'ldu' => 'I=CE,D=DE,i=EE,E=FE',
	'lds' => 'I=10CE,D=10DE,i=10EE,E=10FE',
	'stu' => 'D=DF,i=EF,E=FF',
	'sts' => 'D=10DF,i=10EF,E=10FF'
	
);

foreach $i (keys %insnlist)
{
#	print "$i ... $insnlist{$i}\n";
	@modes = split(/,/, $insnlist{$i});
	foreach $j (@modes)
	{
		($mc, $oc) = split(/=/, $j);
		$operand = '';
		if ($mc eq 'D')
		{
			$operand = '<0';
		}
		elsif ($mc eq 'E')
		{
			$operand = '>0';
		}
		elsif ($mc eq 'I')
		{
			$operand = '#0';
		}
		elsif ($mc eq 'i')
		{
			$operand = ',x';
		}
		elsif ($mc eq 'r')
		{
			$operand = 'a,a';
		}
		elsif ($mc eq 'p')
		{
			$operand = 'cc';
		}
		elsif ($mc eq 'b')
		{
			$operand = '*';
		}
		$asmcode = "\t$i $operand";
		
		# now feed the asm code to the assembler and fetch the result
		$tf = ".asmtmp.$$.$i.$mc";
		open H, ">$tf.asm";
		print H "$asmcode\n";
		close H;
		$r = `$lwasm --raw --list -o $tf $tf.asm`;
		open H, "<$tf";
		binmode H;
		$buffer = '';
		$r = read(H, $buffer, 10);
		close H;
		unlink $tf;
		unlink "$tf.asm";
		if ($r == 0)
		{
			$st = 'FAIL (no result)';
		}
		else
		{
			@bytes = split(//,$buffer);
			$rc = sprintf('%02X', ord($bytes[0]));
			if (length($oc) > 2)
			{
				$rc .= sprintf('%02X', ord($bytes[1]));
			}
			if ($rc ne $oc)
			{
				$st = "FAIL ($rc ≠ $oc, $asmcode)";
			}
			else
			{
				$st = 'PASS';
			}
		}
		print "$i" . "_$mc $st\n";
	}
}
