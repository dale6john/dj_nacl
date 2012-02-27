#!/usr/bin/perl -w

use strict;
use Data::Dumper qw/Dumper/;

########################################################################
package Shared;

our $colors = [ qw/WHITE RED BLUE/ ];

########################################################################
package Exec;

use Data::Dumper qw/Dumper/;
use Time::HiRes qw/sleep/;

our $SENSE = 1;
our $ZERO = 2;
our $INCR = 3;
our $BREAK = 4;
our $DO = 6;
our $DOMOD = 7;
our $RETURN = 8;
our $USEFRAME = 15;
our $TESTMM = 16;
our $TESTCM = 17;
our $TESTMC = 18;
our $JUMP = 32;
our $JUMPEQ = 33;
our $JUMPNE = 34;
our $NOP = 120;
our $SET = 121;
our $PUSH = 130;
our $POP = 131;

our $opcode_map = {
  0     => 0,
  ZERO  => $ZERO,
  INCR  => $INCR,
  BREAK  => $BREAK,
  SENSE => $SENSE,
  DO    => $DO,
  DOMOD => $DOMOD,
  RETURN => $RETURN,
  USEFRAME => $USEFRAME,
  TESTMM => $TESTMM,
  TESTCM => $TESTCM,
  TESTMC => $TESTMC,
  JUMP    => $JUMP,
  JUMPEQ  => $JUMPEQ,
  JUMPNE  => $JUMPNE,
  NOP => $NOP,
  SET => $SET,
  PUSH => $PUSH,
  POP  => $POP,
};
our $opcode_names = {
  $ZERO  => "ZERO",
  $INCR  => "INCR",
  $BREAK  => "BREAK",
  $SENSE => "SENSE",
  $DO    => "DO",
  $DOMOD => "DOMOD",
  $RETURN => "RETURN",
  $USEFRAME => "USEFRAME",
  $TESTMM => "TESTMM",
  $TESTCM => "TESTCM",
  $TESTMC => "TESTMC",
  $JUMP    => "JUMP",
  $JUMPEQ  => "JUMPEQ",
  $JUMPNE  => "JUMPNE",
  $NOP => "NOP",
  $SET => "SET",
  $PUSH => "PUSH",
  $POP  => "POP",
};
our $TURN = 1;
our $MOVE = 2;
our $FIGHT = 3;
our $EAT = 4;
our $EXIT = 5;
our $COLOR = 6;
our $action_names = {
  $TURN  => "TURN",
  $MOVE  => "MOVE",
  $FIGHT => "FIGHT",
  $EAT   => "EAT",
  $EXIT  => "EXIT",
  $COLOR => "COLOR",
};

our $FLAGEQ = 0x001;
our $FLAGGT = 0x002;
our $FLAGLT = 0x004;
our $FLAGBREAK = 0x008;  # not used
our $FLAGFRAME = 0x010;  # enable frame interrupts
our $FLAGSUPER = 0x020;
our $flag_names = "..SFBLGE";

our $exec_slots = {  # these are 2 bytes (16 bits) each
  _pc    => 2,
  _pc2   => 14,
  _flags2   => 15,
  _flags => 3,
  _user  => 4,
  _clock => 5,
  _frame => 6,
  _color => 7,
  _status => 8,
  _action => 9,
  _sense_food => 10,
  _sense_enemy => 11,
  _health => 12,
  _energy => 13,
};

sub new {
  my $class = shift;
  my $bytecode = shift;
  my $name = shift || "";
  my $this = { 
    _bytecode => [ @$bytecode ],
    _name => $name,
  };
  bless $this, $class;
  #$this->xs('_user', 5);
  #$this->xs('_pc', 0);
  return $this;
}

sub xs {
  my $this = shift;
  my $value = shift;
  my $set_to = shift;
  #print "xs($value) = " unless defined $set_to;
  #print "xs($value, $set_to)\n" if defined $set_to;
  die "$value" unless defined $exec_slots->{$value};
  my $slot = $exec_slots->{$value};
  if (defined $set_to) {
    if ($slot & 0x1) {
      $this->{_bytecode}->[$slot / 2] &= 0x0fff0000;
      $this->{_bytecode}->[$slot / 2] |= $set_to;
    } else {
      $this->{_bytecode}->[$slot / 2] &= 0x00000fff;
      $this->{_bytecode}->[$slot / 2] |= ($set_to << 16);
    }
  } else {
    my $ret;
    if ($slot & 0x1) {
      $ret = ($this->{_bytecode}->[$slot / 2] & 0x00000fff);
    } else {
      $ret = ($this->{_bytecode}->[$slot / 2] & 0x0fff0000) >> 16;
    }
    #print "$ret\n";
    return $ret;
  }
}
sub show_instruction {
  my $this = shift;
  my $at = shift;
  #print Dumper($this);
  my $pv = $this->{_bytecode}->[$at];
  #print "pv:$pv\n";
  my $opname = $opcode_names->{($pv & 0xff000000)>>24} || "";
  return sprintf("0x%04x: %6s 0x%03x 0x%03x",
      $at, substr($opname, 0, 6),
            ($pv & 0x00fff000) >> 12, $pv & 0x00000fff);
}
sub show {
  my $this = shift;
  print `clear`;
  printf "DISASSEMBLY: (%s)\n", $this->{_name};
  my $ix = 0;
  print $this->show_instruction($ix++) . "\n";
  my $slots = {};
  my $pc = $this->xs('_pc');
  map { $slots->{$exec_slots->{$_}} = $_ } keys %$exec_slots;
  my @regs = (sort { $a <=> $b } keys %$slots);
  while (@regs) {
    my $left = shift @regs;
    my $right = shift @regs;
    my $left_name = $slots->{$left};
    my $right_name = $slots->{$right};
    #print "$left_name/$right_name\n";
    printf "0x%04x: %6s 0x%03x 0x%03x  %-10s %-10s\n",
      $ix++,
      "CPU",
      $this->xs($left_name), 
      $this->xs($right_name),
      $left_name,
      $right_name;
  }
  while ($ix < scalar(@{$this->{_bytecode}})) {
    my $atpc = "";
    if ($ix == $pc) {
      $atpc = " *** ";
    }
    print $this->show_instruction($ix++) . " $atpc\n";
  }

}
sub execute {
  my $this = shift;
  my $ticks = shift;
  my $verbose = shift;
  #my $x = $this->{_execution_state};
  my $bkid = {};
  map { $bkid->{$opcode_map->{$_}} = substr($_,0,6) } keys %$opcode_map;
  my $p = $this->{_bytecode};
  die "only 2.1k ticks allowed" unless $ticks < 2100;
  $this->xs('_clock', 0);
  my $last_clock = $this->xs('_clock');
  #die;
  while(1) {
    $this->show() if $verbose;
    print "\n" if $verbose;

    # execute next opcode
    my $pc = $this->xs('_pc');
    my $code = $p->[$this->xs('_pc')];
    #print "pc:$pc code:$code\n";
    if (!defined $code) {
      my @a = map { sprintf "%08x", $_ } @{$this->{_bytecode}};
      my $ix = 0;
      for(@a) {
        print "$_ ";
        print "\n" if (++$ix % 8 == 0);
      }
      print "\n";
      #print Dumper($this);
      die;
    }
    if ($verbose) {
      my $f = $this->xs('_flags');
      my $ft = "";
      for (0..7) {
        $ft .= ($f & (1<<$_)) ? substr($flag_names, 7-$_, 1) : " ";
      }
      if ($f & $FLAGSUPER) {
        printf "\t\t\t\t\t\t\t[0x%04x]: %6s 0x%03x 0x%03x  F:%12s  %5d\n", 
            $this->xs('_pc'), substr($opcode_names->{($code & 0xff000000)>>24},0,6),
            ($code & 0x00fff000) >> 12, $code & 0x00000fff, 
            $ft, $this->xs('_clock');
      } else {
        printf "[0x%04x]: %6s 0x%03x 0x%03x  F:%12s  %5d\n",
            $this->xs('_pc'), substr($opcode_names->{($code & 0xff000000)>>24},0,6),
            ($code & 0x00fff000) >> 12, $code & 0x00000fff,
            $ft, $this->xs('_clock');
      }
    }
    my $opcode = ($code & 0xff000000) >> 24;
    my $op1 = ($code & 0x00fff000) >> 12;
    my $op2 = ($code & 0x00000fff);
    #print "pc:$pc opcode:$code op1:$op1 op2:$op2\n";
    if ($opcode == $ZERO) {  # zero
      $p->[$op2] = 0;
      $this->xs('_clock', $this->xs('_clock')+1);
    } elsif ($opcode == $INCR) {  # incr
      printf "INCR [0x%02x]: %d\n", $op2, $p->[$op2] if $verbose;
      $p->[$op2]++;
      $this->xs('_clock', $this->xs('_clock')+1);
    } elsif ($opcode == $TESTMC) {  # testmc
      my $a = $p->[$op1];
      my $b = $op2;
      #printf "TESTMC [0x%02x]: %d <=> %d\n", $op1, $a, $b if $verbose;
      my $flags = $this->xs('_flags');
      #printf "%12b\n", $x->{_flags};
      $flags |= $FLAGEQ if ($a == $b);
      $flags |= $FLAGGT if ($a > $b);
      $flags |= $FLAGLT if ($a < $b);
      $this->xs('_flags', $flags);
      #printf "%12b\n", $x->{_flags};
      $this->xs('_clock', $this->xs('_clock')+3);
    } elsif ($opcode == $JUMP) { # jump
      $this->xs('_pc', $op2 - 1);
      $this->xs('_clock', $this->xs('_clock')+1);
    } elsif ($opcode == $JUMPEQ) { # jumpeq
      $this->xs('_pc', $op2 - 1) if ($this->xs('_flags') & $FLAGEQ);
      $this->xs('_clock', $this->xs('_clock')+2);
    } elsif ($opcode == $JUMPNE) { # jumpne
      $this->xs('_pc', $op2 - 1) if (!($this->xs('_flags') & $FLAGEQ));
      $this->xs('_clock', $this->xs('_clock')+2);
    } elsif ($opcode == $USEFRAME) { # useframe
      if ($op1 == 0xfff) {
        $this->xs('_frame', 0xfff); # no frame to use
        $this->xs('_flags', $this->xs('_flags') & ~($FLAGFRAME));
      } else {
        $this->xs('_frame', $op2);
        $this->xs('_flags', $this->xs('_flags') | ($FLAGFRAME));
      }
      $this->xs('_clock', $this->xs('_clock')+1);
    } elsif ($opcode == $BREAK) { # break 
      # kick into user mode, jump to target
      $this->xs('_flags', 0);
      $this->xs('_user', 2);
      $this->xs('_pc', $op2 - 1);
      $this->xs('_clock', $this->xs('_clock')+8);
    } elsif ($opcode == $DO) { # do
      print "DO " . ($action_names->{$op1} || "??") . "\n" if $verbose;
      # In supervisor mode eat with no food with throw an exception
      # It will break out of the whole conditional using a RETURN
      # Also, move-into-wall or equivalent can be flagged from the
      # operational code as interrupts.
      if ($this->xs('_flags') & $FLAGSUPER && $op1 == $EAT) {
        if (rand() > 0.80) {
          # TODO: Factor supervisor-return?
          $this->xs('_pc', $this->xs('_pc2'));
          $this->xs('_flags', $this->xs('_flags2'));
          $this->xs('_flags', $this->xs('_flags') & ~($FLAGSUPER));
          $this->xs('_user', 1);
        }
      }
      # TODO:
      # in user-space, we need equivalent interrupts to exit loops
      $this->xs('_clock', $this->xs('_clock')+50); # depends
    } elsif ($opcode == $DOMOD) { # domod
      print "DO " . ($action_names->{$op1} || "??") . " " . 
              ($op2 == 1 ? "LEFT" : ($op2 == 2 ? "RIGHT" : "")) . "\n" if $verbose;
      $this->xs('_clock', $this->xs('_clock')+50); # depends
    } elsif ($opcode == $SENSE) { # do
      my ($amt, $sense, $dir, $dist) = 
        (($op1 & 0xf00) >> 8, ($op1 & 0xf0) >> 4, $op2, ($op1 & 0xf));
      printf "\t\t\t\t\t\t\tSENSE %s %s  %s.%s%s%s.%s  %s%s%s\n",
        $amt == 0 ? "few" : "many",
        $sense == 1 ? "FOOD" : "ENEMY",
        $dir & 0x10 ? "<<" : "  ",
        $dir & 0x08 ? "<" : " ",
        $dir & 0x04 ? "!" : " ",
        $dir & 0x02 ? ">" : " ",
        $dir & 0x01 ? ">>" : "  ",
        $dist & 0x04 ? "^" : " ",
        $dist & 0x02 ? "-" : " ",
        $dist & 0x01 ? "_" : " " if $verbose;
      $this->xs('_flags', $this->xs('_flags') ^ $FLAGEQ) if rand() > 0.5;
      $this->xs('_clock', $this->xs('_clock')+10);
    } elsif ($opcode == $RETURN) { # return
      if ($this->xs('_flags') & $FLAGSUPER) {
        $this->xs('_pc', $this->xs('_pc2'));
        $this->xs('_flags', $this->xs('_flags2'));
        $this->xs('_flags', $this->xs('_flags') & ~($FLAGSUPER));
        $this->xs('_user', 1);
        $this->xs('_clock', $this->xs('_clock')+3);
      } else {
        #print Dumper($x);
        die;
        return;
      }
    } else {
      #map { sprint "0x%08x", $_ } @
      die "PC: " . $this->xs('_pc') . "  OPCODE: $opcode $op1";
    }
    $this->xs('_pc', $this->xs('_pc') + 1);

    my $diff = $this->xs('_clock') - $last_clock;
    sleep($diff / 100) if $verbose;
    $last_clock = $this->xs('_clock');
    return if ($last_clock > $ticks);

    # what interrupt frame am i running?  call it
    if (!($this->xs('_flags') & $FLAGSUPER)
          && $this->xs('_flags') & $FLAGFRAME) {
      if ($this->xs('_user') == 0) {
        # switch to interrupt
        $this->xs('_pc2', $this->xs('_pc') - 1);
        $this->xs('_flags2', $this->xs('_flags'));
        $this->xs('_flags', $this->xs('_flags') | $FLAGSUPER);
        $this->xs('_pc', $this->xs('_frame'));
        #print Dumper($x);
        #die;
      } else {
        $this->xs('_user', $this->xs('_user') - 1);
      }
    }
  }
}

########################################################################
package Bytecode;
use Data::Dumper qw/Dumper/;

sub bytecode {
  my $code = shift;
  my $lazy = [];
  my $labels = {};
  my $program = [];
  my $bkid = {};
  map { $bkid->{$opcode_map->{$_}} = substr($_,0,6) } keys %$opcode_map;
  my $actions = {};
  # FIXME: bad namespace dependancy here
  map { $actions->{$action_names->{$_}} = $_ } keys %$action_names;
  #print Dumper($actions);
  #die ;
  my $action_mod = {
    LEFT => 1,
    RIGHT => 2,
  };
  my $senses = {
    FOOD => 1,
    ENEMY => 2,
  };
  my $breakto;

  # set up execution state header
  # FIXME: bad namespace dependancy
  my $execution_state_size = int(scalar(keys(%$exec_slots)) / 2 + 1);
  push @$lazy, [ scalar(@$program), "MAIN", 2 ];
  push @$program, [ $opcode_map->{JUMP}, 0xfff, 0xfff ];
  for (1 .. $execution_state_size) {
    push @$program, [ 0, 0, 0 ];
  }
  for my $item (@$code) {
    my ($op, $p1, $p2, $p3, $p4) = @$item;

    if ($op eq "LABEL") {
      $labels->{$p1} = scalar(@$program);
    } elsif ($op eq "USEFRAME") {
      if ($p1 eq "NONE") {
        push @$program, [ $opcode_map->{$op}, 0xfff, 0xfff ];
      } else {
        push @$lazy, [ scalar(@$program), $p1, 2 ];
        push @$program, [ $opcode_map->{$op}, 0, 0xfff ];
      }
    } elsif ($op eq "FRAME") {
      $labels->{$p1} = scalar(@$program);
    } elsif ($op eq "ENTER") {
      # enter a scope
    } elsif ($op eq "EXIT") {
      $labels->{$p1} = scalar(@$program);
    } elsif ($op eq "DATA") {
      $labels->{$p1} = scalar(@$program);
      push @$program, [ 0, 0, $p2 ];
    } elsif ($op eq "ZERO" || $op eq "INCR") {
      push @$lazy, [ scalar(@$program), $p1, 2 ];
      push @$program, [ $opcode_map->{$op}, 0xfff, 0xfff ];
    } elsif ($op =~ /^TEST/) {
      my ($am, $bm) = (1, 1);
      ($am, $bm) = (0, 1) if $op eq "TESTCM";
      ($am, $bm) = (1, 0) if $op eq "TESTMC";
      my ($a, $b) = ($p1, $p2);
      if ($am) {
        push @$lazy, [ scalar(@$program), $p1, 1 ];
        $a = 0xfff;
      }
      if ($bm) {
        push @$lazy, [ scalar(@$program), $p2, 2 ];
        $b = 0xfff;
      }
      push @$program, [ $opcode_map->{$op}, $a, $b ];
      #print "$op $a/$b\n";
    } elsif ($op =~ /^JUMP/) {
      push @$lazy, [ scalar(@$program), $p1, 2 ];
      push @$program, [ $opcode_map->{$op}, 0, 0xfff ];
    } elsif ($op eq "BREAKTO") {
      $breakto = $labels->{$p1};
    } elsif ($op eq "BREAK") {
      push @$program, [ $opcode_map->{$op}, 0xfff, $breakto ];
    } elsif ($op eq "DO") {
      push @$program, [ $opcode_map->{$op}, $actions->{$p1}, 0 ];
    } elsif ($op eq "DOMOD") {
      #print "p1/p2 $p1/$p2\n";
      if ($p2 =~ /^\d+/) {
        push @$program, [ $opcode_map->{$op}, $actions->{$p1}, $p2 ];
      } else {
        push @$program, [ $opcode_map->{$op}, $actions->{$p1}, $action_mod->{$p2} ];
      }
    } elsif ($op eq "RETURN") {
      push @$program, [ $opcode_map->{$op}, 0, 0 ];
    } elsif ($op eq "SENSE") {
      # 0x00000101 0x00bb00cc0eee 0x0000000ddddd
      #  A         B2     C2      D5        E3
      # operation amount sense  direction  distance
      my ($amt, $sense, $dir, $dist) = ($p2, $senses->{$p1}, $p3, $p4);
      #push @$program, [ $opcode_map->{$op}, ($p2 << 4) + $senses->{$p1}, $p3, $p4 ];
      push @$program, [ $opcode_map->{$op},
          (($amt << 8) | ($sense << 4) | $dist), $dir ];
    } elsif ($op =~ /^\s*#/) {
      # comment
    } else {
      die join(" ", @$item);
    }
    #my $ix = 0;
    #for (@$program) {
    #  printf "%03d: %4s %02x %02x %02x\n", $ix++, $bkid->{$_->[0]}, $_->[1], $_->[2], $_->[3];
    #}
    #print "------\n";
  }
  # fixup for labels
  for (@$lazy) {
    my ($at, $label, $offs) = @$_;
    #printf "lazy: %d %s 0x%x\n", $at, $label, $labels->{$label};
    $program->[$at]->[$offs] = $labels->{$label};
  }
  my $ix = 0;
  my $bytecode = [];
  for (@$program) {
    #printf "0x%04x: %6s %03x %03x\n", $ix++, $bkid->{$_->[0]}, $_->[1], $_->[2];
    push @$bytecode, ($_->[0] << 24) | ($_->[1] << 12) | ( $_->[2]);
    die "$ix $_->[0]/$_->[1]/$_->[2]" unless defined $_->[1] && defined $_->[2];
  }
  return $bytecode;
}

########################################################################
package Codegen;
# create assembly

sub new {
  my $class = shift;
  my $tree = shift;
  my $this = { _tree => $tree, _blockstack => [], _label => 0 };
  return bless $this, $class;
}
sub code {
  my $this = shift;
  #print "(( " . join(" ", @_) . " ))\n";
  push @{$this->{_code}}, [ @_ ];
}
sub nextid {
  my $this = shift;
  my $prefix = shift;
  if ($prefix eq "I") {
    return sprintf("$prefix%04d", $this->{_ids}++);
  } else {
    return sprintf("$prefix%04d", $this->{_label}++);
  }
}
sub datagen {
  my $this = shift;
  for (1 .. $this->{_ids}) {
    $this->code("DATA", sprintf("I%04d", $_ - 1), 0);
  }
}

sub codegen {
  my $this = shift;
  $this->code("# main");
  $this->code("LABEL", "MAIN");
  $this->codegen2($this->{_tree});
  $this->code("RETURN"); # ??
  $this->code("# interrupts");
  $this->showcx();
  $this->code("# data");
  $this->datagen();
  $this->code("# end");
  return $this->{_code};
}

sub optimize {
  my $this = shift;
  # eliminate empty FRAMEs
  # debatable whether this should go here or not
  my $ix = 0;
  my $frame;
  my $frameat;
  my $dropframe;
  for (@{$this->{_code}}) {
    if ($_->[0] eq "FRAME") {
      $frameat = $ix;
      $frame = $_->[1];
    }
    if ($_->[0] eq "RETURN" && defined $frameat) {
      if ($frameat + 1 == $ix) {
        #print "Eliminate $frame\n";
        $dropframe = $frame;
        last;
      }
    }
    $ix++;
  }
  for (@{$this->{_code}}) {
    if ($_->[0] eq "USEFRAME" && $_->[1] eq $dropframe) {
      $_->[1] = "NONE";
    }
  }
}

sub print {
  my $this = shift;
  for (@{$this->{_code}}) {
    if ($_->[0] eq "LABEL") {
      print "$_->[1]:\n";
    } elsif ($_->[0] eq "FRAME") {
      print "$_->[1]: (FRAME)\n";
    } elsif ($_->[0] eq "EXIT") {
      print "$_->[1]: (EXIT)\n";
    } elsif ($_->[0] eq "") {
    } elsif ($_->[0] eq "") {
    } else {
      print "   " . join(" ", @$_) . "\n";
    }
  }
}

sub blockcx {
  my $this = shift;

  # get net interrupts, see if cached
  my $blockstack = $this->{_blockstack};

  my $codetxt = "";
  for (@$blockstack) {
    my ($entry, $block) = @$_;
    $codetxt .= $entry . ":" . $block->show() . "\t";
  }
  my $frameid;
  if (exists $this->{_labelcache}->{$codetxt}) {
    $frameid = $this->{_labelcache}->{$codetxt};
  } else {
    $frameid = $this->nextid("F");
    $this->{_labelcache}->{$codetxt} = $frameid;
    $this->{_labelset}->{$frameid} = [ map { $_ } @$blockstack ];
  }
  #print "$frameid ----> $codetxt\n";

  # the fixup for this is a pain, let the optimization
  # (empty frame) happen in 'optimize'
  #if ($codetxt) {
  #  $this->code("USEFRAME", "NONE");
  #} else {
  #  $this->code("USEFRAME", $frameid);
  #}
  $this->code("USEFRAME", $frameid);

}

sub showcx {
  my $this = shift;
  for my $ctxt (keys %{$this->{_labelcache}}) {
    my $frameid = $this->{_labelcache}->{$ctxt};
    $this->code("FRAME", $frameid);
    my $code = $this->{_labelset}->{$frameid};
    #print Dumper($code);
    for (reverse @$code) {
      my ($entry, $block) = @$_;
      #print Dumper($_);
      $this->code("BREAKTO", $entry);
      $this->codegen2($block) if $block;
    }
    $this->code("RETURN");
  }
}

sub codegen2 {
  my $this = shift;
  my $at = shift;
  #print "CG2 " . $at->show() . "\n";

  if ($at->tag() eq "Repeat") {
    my $label = $this->nextid("L");
    my $index;
    my $count = $at->value()->value();
    if ($count ne '@') {
      $index = $this->nextid("I");
      $this->code("ZERO", $index);
    }
    $this->code("LABEL", $label);
    #print "Show: $at\n";
    #$at->show(1);
    #print Dumper($at);
    #$at->named('Target')->show(1);
    #print Dumper($at->named('Target'));
    $this->codegen2($at->named('Target'));
    if ($count eq '@') {
      $this->code("JUMP", $label);
    } else {
      $this->code("INCR", $index);
      $this->code("TESTMC", $index, $count);
      $this->code("JUMPNE", $label);
    }
  } elsif ($at->tag() eq "Action") {
    my $act = $at->value();
    my $actop = "";
    my $mod = $at->named('ActionMod');
    my $modop;
    if ($mod) {
      $mod = $mod->value();
    } else {
      if ($at->value() eq "Color") {
        $mod = $at->named('_extra');
        $modop = $mod;
        #die "$mod $modop $act $actop";
      } else {
        $mod = "";
      }
    }
    #print "DO action: $act $mod\n";
    $actop = "TURN" if ($act eq "t");
    $actop = "MOVE" if ($act eq "m");
    $actop = "FIGHT" if ($act eq "f");
    $actop = "EAT"  if ($act eq "e");
    $actop = "COLOR"  if ($act eq "Color");
    $modop = "LEFT" if $mod eq "<";
    $modop = "RIGHT" if $mod eq ">";
    if ($act eq "x") {
      $this->code("# x - break out");
      $this->code("BREAK");
    } elsif ($modop) {
      $this->code("DOMOD", $actop, $modop);
    } else {
      $this->code("DO", $actop);
    }
  } elsif ($at->tag() eq "CondTest") {
    #$at->show(1);
    my $cond = $at->value();
    my $dir = 0;
    for (@{$at->named('DirMod')}) {
      $dir += 0x10 if $_->value() eq "<<";
      $dir += 0x08 if $_->value() eq "<";
      $dir += 0x04 if $_->value() eq "!";
      $dir += 0x02 if $_->value() eq ">";
      $dir += 0x01 if $_->value() eq ">>";
    }
    $dir ||= 0x1f;
    my $dist = 0;
    for (@{$at->named('DistMod')}) {
      $dist += 0x1 if $_->value() eq "_";
      $dist += 0x2 if $_->value() eq "-";
      $dist += 0x4 if $_->value() eq "^";
    }
    $dist ||= 0x7;
    my $event = "";
    $event = "FOOD" if $cond eq "f" || $cond eq "F";
    $event = "ENEMY" if $cond eq "e" || $cond eq "E";
    my $amount = 0;
    $amount = 0 if $cond eq "f" || $cond eq "e";
    $amount = 1 if $cond eq "F" || $cond eq "E";
    $this->code("SENSE", $event, $amount, $dir, $dist);
  } elsif ($at->tag() eq "CondThen") {
    $this->code("# CondThen");
    #$at->show(1);
    my $cond = $at->value();
    my $dir = 0;
    my $actions = $at->named('ActionList');
    for (@$actions) {
      $this->codegen2($_);
    }
  } elsif ($at->tag() eq "CondElse") {
    $this->code("# CondElse");
    #$at->show(1);
    my $cond = $at->value();
    my $dir = 0;
    my $actions = $at->named('ActionList');
    for (@$actions) {
      $this->codegen2($_);
    }
  } elsif ($at->tag() eq "Condition") {
    my $test = $at->named('CondTest');
    my $then = $at->named('CondThen');
    my $else = $at->named('CondElse');
    my $label_endif = $this->nextid("L");

    $this->codegen2($test);  # set status
    if ($else) {
      my $label_else  = $this->nextid("L") if $else;
      $this->code("JUMPNE", $label_else);
      $this->codegen2($then);
      $this->code("JUMP", $label_endif);
      $this->code("LABEL", $label_else);
      $this->codegen2($else);
    } else {
      $this->code("JUMPNE", $label_endif);
      $this->codegen2($then);
    }
    $this->code("LABEL", $label_endif);

  } elsif ($at->tag() eq "Block") {
    my $ccount = 0;
    my $entry  = $this->nextid("E");
    $this->code("ENTER", $entry);
    for (@{$at->list()}) {
      if ($_->tag() eq "Condition") {
        #print "INSTALL condition - frame " . scalar(@{$this->{_blockstack}}) . "\n";
        #$_->show(1);
        #print Dumper($_);
        push @{$this->{_blockstack}}, [ $entry, $_ ];
        $ccount++;
      }
    }
    # get net interrupts, see if cached
    $this->blockcx();

    # run the statements
    for (@{$at->list()}) {
      if ($_->tag() ne "Condition") {
        #print "CODEGEN\n";
        $this->codegen2($_);
      }
    }

    for (0 .. $ccount) {
      pop @{$this->{_blockstack}};
    }
    $this->code("EXIT", $entry);
    $this->blockcx();

  } else {
    die $at->tag();
  }
}

sub visualize {
  my $this = shift;
  $this->visualize2($this->{_tree});
}

sub visualize2 {
  my $this = shift;
  my $t = shift;
  my $indent = shift || 0;
  my $verbose = shift;
  #print "\nVisualize $t\n";

  my $tag   = $t->tag();
  my $value = $t->value();
  my $list  = $t->list();
  my $names = $t->names();
  #print " ($tag) ";
  #print "$tag/$value/$list/$names\n";

  #print '[';
  if (!defined $tag) {
    print "UNDEF";
  } elsif ($tag eq "Repeat") {
    my $v2 = $value->value();
    my $block = $t->named('Target');
    if ($block) {
      print "$v2";
      $this->visualize2($block, $indent + 3, $verbose);
    } else {
      die;
    }
  } elsif ($tag eq "Block") {
    print '{';
    for (@$list) {
      $this->visualize2($_, $indent + 3, $verbose);
    }
    print '}';
  } elsif ($tag eq "Action") {
    if ($value eq "Color") {
      print $Shared::colors->[$t->named('_extra')];
    } else {
      print "$value";
      $this->visualize2($t->named('ActionMod'), 0, 0) if $t->named('ActionMod');
    }
  } elsif ($tag eq "CondThen") {
    for (@{$t->named('ActionList')}) {
      $this->visualize2($_, $indent + 3, $verbose);
    }
  } elsif ($tag eq "CondElse") {
    for (@{$t->named('ActionList')}) {
      $this->visualize2($_, $indent + 3, $verbose);
    }
  } elsif ($tag eq "CondTest") {
    print "$value";
    for (@{$t->named('DirMod')}) {
      $this->visualize2($_, $indent + 3, $verbose);
    }
    for (@{$t->named('DistMod')}) {
      $this->visualize2($_, $indent + 3, $verbose);
    }
  } elsif ($tag eq "ActionMod") {
    print "$value";
  } elsif ($tag eq "DirMod") {
    print "$value";
    #$this->visualize2($t->{'DirMod'}, 0, 0) if exists $t->{'DirMod'};
  } elsif ($tag eq "DistMod") {
    print "$value";
    #$this->visualize2($t->{'DistMod'}, 0, 0) if exists $t->{'DistMod'};
  } elsif ($tag eq "Condition") {
    print "(";
    $this->visualize2($t->named('CondTest'), $indent + 3, $verbose);
    print "?";
    if ($t->named('CondThen')) {
      $this->visualize2($t->named('CondThen'), $indent + 3, $verbose);
    }
    if ($t->named('CondElse')) {
      print ":";
      $this->visualize2($t->named('CondElse'), $indent + 3, $verbose);
    }
    #if (exists $t->{'CondElse'}) {
    #  $this->visualize2($t->{'CondElse'}, $indent + 3, $verbose);
    #}
    print ")";
  } elsif ($tag eq "") {
  }
  #print ']';
}

########################################################################
package Parse;

use Data::Dumper qw/Dumper/;

# 0
# 1 Block
# 2 Repeating
# 3 SimpleRepeat
# 4 Condition
# 5 CondTest
# 6 DirMod
# 7 DistMod
# 8 CondThen
# 9 Action         fight turn move eat   has-a  ActionMod (optional)
# 10 CondElse
# 16 ActionMod     '<' or '>'
#
# state 1 means IN-A Block, and is the code that generates that rule's output
#
my $statemap = {
   0 => { '@' => 2, '9' => 2, '{' => 1 },
   1 => { '{' => 1, '9' => 2, '(' => 4, ')' => 1, '@' => 2, 'ACTION' => 9 },
   2 => { '{' => 1, 'ACTION' => 9 },
   3 => { 'ACTION' => 9, '{' => 1 },
   4 => { 'f' => 5, 'F' => 5, 'e' => 5, 'E' => 5, '?' => 8, ':' => 10 },
   5 => { '!' => 6, '<' => 6, '>' => 6, '<<' => 6, '>>' => 6,
             '_' => 7, '-' => 7, '^' => 7 },
   6 => { '!' => 6, '<' => 6, '>' => 6, '<<' => 6, '>>' => 6 },
   7 => { '_' => 7, '-' => 7, '^' => 7 },
   8 => { 'ACTION' => 9, '9' => 3, '@' => 3, '{' => 1 },
   9 => { '<' => 16, '>' => 16 },
  10 => { 'ACTION' => 9, '9' => 3, '@' => 3 },
  16 => { },
};
my @actions = qw/f m t e x C/;

sub fixmap {
  my $map = shift;
  for my $state (keys %$map) {
    if (exists $map->{$state}->{ACTION}) {
      my $target = $map->{$state}->{ACTION};
      for (@actions) {
        $map->{$state}->{$_} = $target;
      }
    }
    delete $map->{$state}->{ACTION};
  }
}

sub parse {
  my $tokens = shift;
  my $verbose = shift;
  my $state_stack = [];
  my $stack = Stack->new();
  my $state = 0;
  my $zz = 0;
  my $sp = 0;

  my $prev_state;
  fixmap($statemap);
  #print Dumper($statemap);
  while (1) {
    print "TOP\n" if $verbose;
    my $tk = $tokens->[0];
    my $tkvalue = $tk->[1] if ref($tk);
    $tk = $tk->[0] if ref($tk);

    #print "/$tk/$tkvalue/\n";
    my $next_state  = $statemap->{$state}->{$tk};
    my $shift_state = $next_state;

    my $reduced = 0;

    if (defined $next_state) {
      print "State: $state  SHIFT ALLOWED (tk: '$tk') to state $next_state\t" if $verbose;
    } else {
      my ($nx, $sp) = @{pop @$state_stack};
      push @$state_stack, [$nx, $sp] if $nx;
      $nx ||= "undef";
      print "State: $state  REDUCE (tk: '$tk') then pop to $nx (sp:$sp)\t" if $verbose;
    }
    print "states: " . join("; ", map { "$_->[0], $_->[1]" } @$state_stack) . "\n" if $verbose;
    print "stack :\n" if $verbose;
    $stack->show(1) if $verbose;
    if (1) {
      # REDUCE
      
      #print "shift error (st: $state) (tk: '$tk')\n" if $verbose;
      #die;
      # stack things from current state
      #die "empty stack" unless @$state_stack;
      my $stack0 = $stack->top();

      my ($current, $scurrent);

      #$sp = $stack->pointer();

      #my $scurrent = pop @$stack;
      #print Dumper($scurrent);
      #my $current = $scurrent->{'v'} if $scurrent;
      #my $sp = $scurrent->{'sp'} if $scurrent;

      if ($state == 1 && $tk eq '}') {
        my $items = [];
        print "Block reduce: (sp:$sp)\n" if $verbose;
        my $frame = $stack->pop($sp);
        print "FRAME:\n" if $verbose;
        for (@$frame) {
          $_->show(1) if $verbose;
        }
        die "syntax" unless $frame->[0]->tag() eq "Token";
        die "syntax" unless $frame->[0]->value() eq "{";
        shift @$frame;
        my $item = Node->new("Block");
        push @{$item->list()}, @$frame;

        print Dumper($item) if $verbose;
        $item->show(1) if $verbose;
        $stack->push($item);
        shift @$tokens;
        $reduced = 1;
        
      } elsif ($state == 2) {
        my $reduce = $stack->top()->tag() eq "Action";
        if (!$next_state || $reduce) {
          print "Repeat reduce (forced) (sp:$sp)\n" if $verbose;
          my $frame = $stack->pop($sp);
          my $atsign = $frame->[0];
          my $stmt = $frame->[1];
          die "syntax" unless $stmt;
          my $item = Node->new("Repeat", $atsign);
          $item->add("Target", $stmt);
          $stack->push($item);
          $reduced = 1;
        } else {
          print "state 2 not reducing yet (nx:$next_state)\n" if $verbose;
        }

      } elsif ($state == 3) {
        my $reduce = $stack->top()->tag() eq "Action";
        if (!$next_state || $reduce) {
          my $frame = $stack->pop($sp);
          #$stack->show();
          print "FRAME START:\n" if $verbose;
          print Dumper($frame) if $verbose;
          print "FRAME END:\n" if $verbose;

          print "SimpleRepeat reduce (forced) (sp:$sp)\n" if $verbose;
          my $atsign = $frame->[0];
          my $stmt = $frame->[1];
          die "syntax" unless $stmt;
          my $item = Node->new("Repeat", $atsign);
          $item->add("Target", $stmt);
          $stack->push($item);
          $reduced = 1;
        }

      } elsif ($state == 4 && $tk eq ')') {
        if (1) {
          print "Condition Reduced on ')'\n" if $verbose;

          my $frame = $stack->pop($sp);
          #$stack->show();
          print "FRAME START:\n" if $verbose;
          print Dumper($frame) if $verbose;
          print "FRAME END:\n" if $verbose;

          my $n = shift @$frame; # '('
          die "syntax" unless $n->tag() eq "Token";
          my $test = shift @$frame;
          die "syntax: " . $test->tag() unless $test->tag() eq "CondTest";
          my ($then, $else);
          while (my $a = shift @$frame) {
            $then = $a if $a->tag() eq "CondThen";
            $else = $a if $a->tag() eq "CondElse";
          }
          my $item = Node->new("Condition");
          die "need then" unless $then;
          $item->add("CondTest", $test);
          $item->add("CondThen", $then) if $then;
          $item->add("CondElse", $else) if $else;
          $stack->push($item);
          shift @$tokens;
          $reduced = 1;
        }
        
      } elsif ($state == 5) {
        if (!$next_state) {
          print "CondTest Reduced FORCED\n" if $verbose;

          my $frame = $stack->pop($sp);
          #$stack->show();
          #print "FRAME START:\n";
          #print Dumper($frame);
          #print "FRAME END:\n";

          my $n = shift @$frame;
          die "syntax" unless $n->tag() eq "Token";
          my $item = Node->new("CondTest", $n->value());
          my @dirmods;
          my @distmods;
          while (my $tag = shift @$frame) {
            if ($tag->tag() eq "DirMod") {
              push @dirmods, $tag;
            } elsif ($tag->tag() eq "DistMod") {
              push @distmods, $tag;
            } else {
              die "syntax";
            }
          }
          $item->add("DirMod", \@dirmods);
          $item->add("DistMod", \@distmods);

          print Dumper($item) if $verbose;
          #print "ITEM : {" . $item->show() . "}\n";
          $stack->push($item);
          $reduced = 1;
        }

      } elsif ($state == 6) {
        my $n = $stack->pop();
        die "syntax" unless $n->tag() eq "Token";
        $stack->push(Node->new("DirMod", $n->{_v}));
        $reduced = 1;
        
      } elsif ($state == 7) {
        my $n = $stack->pop();
        die "syntax" unless $n->tag() eq "Token";
        $stack->push(Node->new("DistMod", $n->{_v}));
        $reduced = 1;
        
      } elsif ($state == 8) {
        if (!$next_state) {
          print "CondAction Reduced FORCED\n" if $verbose;
          my $frame = $stack->pop($sp);
          print "FRAME START:\n" if $verbose;
          print Dumper($frame) if $verbose;
          print "FRAME END:\n" if $verbose;
          my $n = shift @$frame; # is '?'
          die "syntax " . $n->tag() unless $n->tag() eq "Token";
          my $item = Node->new("CondThen");
          $item->add("ActionList", $frame);
          $item->show(1) if $verbose;
          $stack->push($item);
          $reduced = 1;
        }
        
      } elsif ($state == 9) {
        if ($stack0->tag() eq "Action" || !$next_state) {
          print "Action Reduce UNFORCED (sp:$sp)\n" if $next_state && $verbose;
          print "Action Reduce FORCED (sp:$sp)\n" if !$next_state && $verbose;
          #$stack->show();
          my $frame = $stack->pop($sp);
          #$stack->show();
          print "FRAME START:\n" if $verbose;
          print Dumper($frame) if $verbose;
          print "FRAME END:\n" if $verbose;
          my $item;
          if ($frame->[0]->tag() eq "Color") {
            $item = Node->new("Action", $frame->[0]->tag());
            $item->add("_extra", $frame->[0]->value());
          } else {
            $item = Node->new("Action", $frame->[0]->value());
          }
          if (scalar @$frame > 1) {
            print Dumper($frame->[1]) if $verbose;
            $frame->[1]->show(1) if $verbose;
            $item->add("ActionMod", $frame->[1]);
          }
          print Dumper($item) if $verbose;
          print "ITEM : {" . $item->show() . "}\n" if $verbose;
          $stack->push($item);
          $reduced = 1;
        } else {
          print "not ready to reduce 9\n" if $verbose;
        }
        
      } elsif ($state == 10) {
        if (!$next_state) {
          print "CondElse Reduced FORCED\n" if $verbose;
          my $frame = $stack->pop($sp);
          print "FRAME START:\n" if $verbose;
          print Dumper($frame) if $verbose;
          print "FRAME END:\n" if $verbose;
          my $n = shift @$frame; # is '?'
          die "syntax" unless $n->tag() eq "Token";
          my $item = Node->new("CondElse");
          $item->add("ActionList", $frame);
          $item->show(1) if $verbose;
          $stack->push($item);
          $reduced = 1;
        }
        
      } elsif ($state == 16) {
        my $n = $stack->pop();
        die "syntax" unless $n->tag() eq "Token";
        $stack->push(Node->new("ActionMod", $n->{_v}));
        $reduced = 1;
        
      }
    }
    if (!$reduced) {
      # SHIFT
      print "SHIFT (state: $state/$next_state)\n" if $verbose;
      if (defined $next_state) {
        #$sp = $stack->pointer();
        shift @$tokens;
        if ($tk eq 'C') {
          $stack->push(Node->new("Color", $tkvalue));
        } elsif (defined $tkvalue) {
          $stack->push(Node->new("Number", $tkvalue));
        } else {
          $stack->push(Node->new("Token", $tk));
        }
        push @$state_stack, [ $state, $sp ];
        $prev_state = $state;
        $state = $next_state;
        $sp = $stack->pointer();
        print "SP ASSIGNMENT: $sp\n" if $verbose;
      } else {
        die "can't shift this token ($tk)";
      }
    } else {
      if (!scalar @$state_stack) {
        print "DONE???\n" if $verbose;
        $stack->show(3) if $verbose;
        die "stack not 1 item" unless $stack->pointer() == 1;
        return $stack->pop();
      }
      ($state, $sp) = @{pop @$state_stack};
      print "SP ASSIGNMENT: $sp (FROM STATE STACK)\n" if $verbose;
      print "REDUCED to state: $state, sp: $sp\n" if $verbose;
    }

    #if (!defined $state || $state == 0) {
    #  printf "VERY HAPPY\n" if ($tk eq '*');
    #  die "stack state wrong" unless @$stack == 1;
    #  return $stack->[0];
    #}
    print "  STACK AFTER:\n" if $verbose;
    #print Dumper($stack);
    $stack->show(5) if $verbose;
  }

}

########################################################################
package Stack;

sub new {
  my $class = shift;
  my $arg = shift;
  my $this = { _stack => [] };
  return bless $this, $class;
}

sub push {
  my $this = shift;
  my $item = shift;
  push @{$this->{_stack}}, $item;
}

sub pointer {
  my $this = shift;
  return scalar @{$this->{_stack}};
}

sub pop {
  my $this = shift;
  my $range = shift;
  if (defined $range) {
    return $this->range($range, 1);
  } else {
    return pop @{$this->{_stack}};
  }
}

sub top {
  my $this = shift;
  my $count = scalar @{$this->{_stack}};
  if ($count) {
    return $this->{_stack}->[$count - 1];
  } else {
    return undef;
  }
}

sub range {
  my $this = shift;
  my $posn = shift;
  my $pop = shift || 0;
  my $ret = [];
  my $count = scalar @{$this->{_stack}};
  for ($posn .. $count) {
    if ($pop) {
      unshift @$ret, CORE::pop @{$this->{_stack}};
    } else {
      CORE::push @$ret, $this->{_stack}->[$_];
    }
  }
  return $ret;
}

sub show {
  my $this = shift;
  my $indent = shift || 0;
  for (@{$this->{_stack}}) {
    print " " x $indent . $_->show() . "\n";
  }
}

########################################################################
package Node;
use Data::Dumper qw/Dumper/;

sub new {
  my $class = shift;
  my $tag = shift;
  my $this = { _tag => $tag };
  my $v = shift;
  $this->{ _v } = $v if defined $v;
  return bless $this, $class;
}
sub add {
  my $this = shift;
  my $name = shift;
  my $item = shift;
  $this->{ $name } = $item;
}
sub tag {
  my $this = shift;
  return $this->{_tag};
}
sub value {
  my $this = shift;
  return $this->{_v};
}
sub list {
  my $this = shift;
  $this->{_list} ||= [];
  return $this->{_list};
}
sub names {
  my $this = shift;
  my $ret = [];
  for (keys %{$this}) {
    next if /^_/;
    push @$ret, $_;
  }
  return $ret;
}
sub named {
  my $this = shift;
  my $name = shift;
  return $this->{$name};
}
sub show {
  my $this = shift;
  my $lf = shift || 0;
  my $tag = $this->{_tag};
  my $v = $this->{_v};
  my $list = $this->{_list};
  my $ret = "";
  $ret .= "$tag. " unless $tag eq "Token";
  if ($v) {
    if (ref($v)) {
      $ret .= "." . $v->show();
    } elsif ($tag eq "Token") {
      $ret .= "'$v' ";
    } else {
      $ret .= "[$v] ";
    }
  }
  if ($list) {
    $ret .= "{ ";
    $ret .= join("; ", map { $_->show() } @$list);
    $ret .= " } ";
  }
  my @show;
  for (keys %$this) {
    next if /^_/;
    #$ret .= "$_" unless /^_/;
    push @show, [ $_, ref($this->{$_}) eq "ARRAY" 
        ? "[ " . join("; ", map { $_->show() } @{$this->{$_}}) . "]"
        : $this->{$_}->show() ];
  }
  #print Dumper(\@show);
  if (@show) {
    $ret .= " (" . join(", ",
        map { ($_->[0] . ": " . $_->[1]) } @show) . ") ";
  }
  print "$ret\n" if $lf;
  return $ret;
}

########################################################################
package Tokenize;

sub tokenize {
  my $input = shift;
  my $verbose = shift;
  my $tokens2 = [ '<<', '>>' ];
  my $tokens = [ '=', ';', '*', '@', '{', '}', '(', ')', '!', '<', '>', '_', '-', '^', '?', ':', qw/f m t e x/, 'F', 'E' ];
  #my $colors = [ qw/WHITE RED BLUE/ ];
  my $colors = $Shared::colors;
  my $ret = [];
  while ($input ne "") {
    my $token;
    my $len = 0;
    if ($input =~ /^(#[^\n]*)\n/) {
      print "Comment ($1)" if $verbose;
      $len = length($1);
      $token = 0;
    } elsif ($input =~ /^(\s+)/) {
      $len = length($1);
      $token = 0;
    } elsif ($input =~ /^\$([A-Za-z])/) {
      $len = length($1) + 1;
      $token = [ '$', $1 ];
    } else {
      my $c2 = substr($input, 0, 2);
      if (grep {$c2 eq $_} @$tokens2) {
        print "match(2): $c2\n" if $verbose;
        $token = $c2;
        $len = length($token);
      } else {
        my $c1 = substr($input, 0, 1);
        if (grep {$c1 eq $_} @$tokens) {
          print "match(1): $c1\n" if $verbose;
          $token = $c1;
          $len = length($token);
        } else {
          if ($input =~ /^(\d+)/) {
            print "match(d): $1\n" if $verbose;
            $token = [ '9', $1 ];
            $len = length($1);
          }
        }
      }
    }
    if (!$len) {
      my $ix = 0;
      for (@$colors) {
        if ($input =~ /^($_)\b/) {
          print "color: $1\n" if $verbose;
          $len = length($1);
          $token = [ 'C', $ix ];
          last;
        }
        $ix++;
      }
    }
    die "tokenization error near: $input" if (!defined $token);
    push @$ret, $token if $token;
    $input = substr($input, $len);
  }
  return $ret;
}



########################################################################
package main;
use Time::HiRes qw/gettimeofday/;

$|=1;

my $action = Node->new("Action", "f");
my $am = Node->new("ActionMod", ">");
$action->add("ActionMode", $am);

if(0) {
  $am->show(1);
  $action->show(1);
  die;
}
my $s = Stack->new();
$s->push($action);

#print "NODES:\n";
#print Dumper($action);
#$s->top()->show();
# main
my $verbose = 1;
#my $input = '@{9{(f!>>_?:e<)@m}}';
#my $input = '@{9{(f!>>_?e>:e<)@m}}';
#my $input = '@{2{3m>4f}}*';
#my $input = '6{(f<!>_-?5e4{fe}:3f2e)1{tt}}*';
#my $input = '@{(e!_?:9t>m)(f<!>_-?ef:fe)tt}*';
#my $input = '@{(e!_?:9t>m)}*';
#my $input = '6{(f?4{fe})}*';
#my $input = '6{(f?{(e?f)2m}:t>)}*';
#my $input = '6{m(f?e)m}*';
#my $input = '1{1{(F_<!>?@e)(e?x)3m{(f?t>)@e}t<}t>}*';
my $input = '
@{
  # comment
  2t
  {BLUE}
  RED
  { 
    (F_<!> ?
      m # $x=1;
      3e
    )
    3m3{t>}3m3{t<}
    {
      (f?t>)
      2{2{3e}t>}t>
    }
    t<
  }
  20{t>m}
}*
';
my $input2 = '@{fm}*';

print "Input:\n$input\n";

#my $tokens = tokenize($input, $verbose);
my $tokens = Tokenize::tokenize($input, 0);
#print "Tokenization:\n";
#print Dumper($tokens);

my $tree = Parse::parse($tokens, 0);
print "Parse Tree:\n";
#print Dumper($tree);

my $cg = Codegen->new($tree);

print "TREE VISUALIZATION:\n";
$cg->visualize();
print "\n";

my $assembly = $cg->codegen();
$cg->optimize();
#print "ASSEMBLY:\n";
#$cg->print();
print "\n";

my $bytecode = Bytecode::bytecode($assembly);
print Dumper($bytecode);

die;
#for (0 .. 34) {
#  printf "\"%s\",\n", $opcode_names->{$_} || "";
#}
#die;

my $tokens2 = Tokenize::tokenize($input2, 0);
my $tree2 = Parse::parse($tokens2, 0);
my $cg2 = Codegen->new($tree2);
my $assembly2 = $cg2->codegen();
my $bytecode2 = Bytecode::bytecode($assembly2);

my $x = Exec->new($bytecode, "x");
my $x2 = Exec->new($bytecode, "x2");
my $x3 = Exec->new($bytecode2, "x3");
#$x->visualize();
my $a = gettimeofday();
#$x->show();
for (1 .. 5) {
  $x->execute(100, 1);
  $x2->execute(45, 1);
  $x3->execute(85, 1);
}
my $b = gettimeofday();
#$x->show();
printf "%3.3f ms\n", ($b - $a) * 1000;
exit;
__END__

print "\n";
print "$input\n";
my $x = Exec->new($tree);
$x->visualize();
print "\nExec:\n";
my $code = $x->codegen();
for (@$code) { print join(" ", @$_) . "\n"; }
$x->bytecode();
$x->execute(1);
exit;

print "\n";
while(<STDIN>) {
  chomp;
  visualize(parse(tokenize("$_*", 0)));
  print "\n";
  print "\n";
}

