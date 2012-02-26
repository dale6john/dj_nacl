#!/usr/bin/perl -w

use strict;
use Data::Dumper qw/Dumper/;

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

our $TURN = 1;
our $MOVE = 2;
our $FIGHT = 3;
our $EAT = 4;
our $EXIT = 5;

sub new {
  my $class = shift;
  my $tree = shift;
  my $this = { 
    _tree => $tree, 
    _blockstack => [],
    _label => 0,
    _cx => { _x => 0, _y => 0, _hdg => 0 },
  };
  return bless $this, $class;
}

sub execute {
  my $this = shift;
  my $verbose = shift;
  my $x = {
    _frame => 0,
    _user => 5,
    _pc => 0,
    _flags => 0,
    _pc2 => 0,
    _flags2 => 0,
  };
  my $p = $this->{_bytecode};
  while(1) {
    # execute next opcode
    my $code = $p->[$x->{_pc}];
    if ($verbose) {
      if ($x->{_flags} & 0x800) {
        printf "\t\t\t\t[0x%04x]: 0x%08x  0x%04x\n", $x->{_pc}, $code, $x->{_flags};
      } else {
        printf "[0x%04x]: 0x%08x  0x%04x\n", $x->{_pc}, $code, $x->{_flags};
      }
    }
    my $opcode = ($code & 0xff000000) >> 24;
    my $op1 = ($code & 0x00fff000) >> 12;
    my $op2 = ($code & 0x00000fff);
    if ($opcode == $ZERO) {  # zero
      $p->[$op2] = 0;
    } elsif ($opcode == $INCR) {  # incr
      printf "INCR [0x%02x]: %d\n", $op2, $p->[$op2] if $verbose;
      $p->[$op2]++;
    } elsif ($opcode == $TESTMC) {  # testmc
      my $a = $p->[$op1];
      my $b = $op2;
      printf "TESTMC [0x%02x]: %d <=> %d\n", $op1, $a, $b if $verbose;
      my $flags = 0;
      $flags |= 0x1 if ($a == $b);
      $flags |= 0x2 if ($a > $b);
      $flags |= 0x4 if ($a < $b);
      $x->{_flags} = $flags;
    } elsif ($opcode == $JUMP) { # jump
      $x->{_pc} = $op2 - 1;
    } elsif ($opcode == $JUMPEQ) { # jumpeq
      $x->{_pc} = $op2 - 1 if ($x->{_flags} & 0x1);
    } elsif ($opcode == $JUMPNE) { # jumpne
      $x->{_pc} = $op2 - 1 if (!($x->{_flags} & 0x1));
    } elsif ($opcode == $USEFRAME) { # useframe
      $x->{_frame} = $op2;
    } elsif ($opcode == $BREAK) { # useframe
      die;
    } elsif ($opcode == $DO) { # do
      print "DO " . 
          ($op1 == $MOVE ? "MOVE"
            : ($op1 == $EAT ? "EAT"
              : ($op1 == $TURN ? "TURN"
                : ($op1 == $FIGHT ? "FIGHT"
                  : ($op1 == $EXIT ? "EXIT" : "??")))))
                  . "\n";
    } elsif ($opcode == $DOMOD) { # domod
      print "DO " .
          ($op1 == $MOVE ? "MOVE"
            : ($op1 == $EAT ? "EAT"
              : ($op1 == $TURN ? "TURN"
                : ($op1 == $FIGHT ? "FIGHT"
                  : ($op1 == $EXIT ? "EXIT" : "??")))))
              . " " . 
              ($op2 == 1 ? "LEFT" : ($op2 == 2 ? "RIGHT" : "")) . "\n";
    } elsif ($opcode == $SENSE) { # do
      my ($amt, $sense, $dir, $dist) = 
        (($op1 & 0xf00) >> 8, ($op1 & 0xf0) >> 4, $op2, ($op1 & 0xf));
      printf "\t\t\t\tSENSE %s %s  %s.%s%s%s.%s  %s%s%s\n",
        $amt == 0 ? "few" : "many",
        $sense == 1 ? "FOOD" : "ENEMY",
        $dir & 0x10 ? "<<" : "  ",
        $dir & 0x08 ? "<" : " ",
        $dir & 0x04 ? "!" : " ",
        $dir & 0x02 ? ">" : " ",
        $dir & 0x01 ? ">>" : "  ",
        $dist & 0x04 ? "^" : " ",
        $dist & 0x02 ? "-" : " ",
        $dist & 0x01 ? "_" : " ";
      $x->{_flags} ^= 0x1 if rand() > 0.5;
    } elsif ($opcode == $RETURN) { # return
      if ($x->{_flags} & 0x800) {
        $x->{_pc} = $x->{_pc2};
        $x->{_flags} = $x->{_flags2};
        $x->{_flags} |= 0x800;
        $x->{_flags} ^= 0x800;
        $x->{_user} = 1;
      } else {
        return;
      }
    } else {
      die "$opcode $op1";
    }
    $x->{_pc}++;
    sleep(0.1) if $verbose;

    # what interrupt frame am i running?  call it
    if (!($x->{_flags} & 0x800)) {
      if ($x->{_user}-- <= 0) {
        # switch to interrupt
        $x->{_pc2} = $x->{_pc} - 1;
        $x->{_flags2} = $x->{_flags};
        $x->{_flags} ^= 0x800;
        $x->{_pc} = $x->{_frame};
      }
    }
  }
}
sub code {
  my $this = shift;
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
  $this->codegen2($this->{_tree});
  $this->code("RETURN"); # ??
  $this->code("# interrupts");
  $this->showcx();
  $this->code("# data");
  $this->datagen();
  $this->code("# end");
  return $this->{_code};
}

sub bytecode {
  my $this = shift;
  my $lazy = [];
  my $labels = {};
  my $program = [];
  my $id = { 
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
    EXIT => $EXIT,
  };
  my $bkid = {};
  map { $bkid->{$id->{$_}} = substr($_,0,6) } keys %$id;
  my $actions = {
    TURN => 1,
    MOVE => 2,
    FIGHT  => 3,
    EAT  => 4,
    EXIT => 5,
  };
  my $action_mod = {
    LEFT => 1,
    RIGHT => 2,
  };
  my $senses = {
    FOOD => 1,
    ENEMY => 2,
  };
  for my $item (@{$this->{_code}}) {
    my ($op, $p1, $p2, $p3, $p4) = @$item;

    if ($op eq "LABEL") {
      $labels->{$p1} = scalar(@$program);
    } elsif ($op eq "USEFRAME") {
      push @$lazy, [ scalar(@$program), $p1, 2 ];
      push @$program, [ $id->{$op}, 0, 0xfff ];
    } elsif ($op eq "FRAME") {
      $labels->{$p1} = scalar(@$program);
    } elsif ($op eq "DATA") {
      $labels->{$p1} = scalar(@$program);
      push @$program, [ 0, 0, $p2 ];
    } elsif ($op eq "ZERO" || $op eq "INCR") {
      push @$lazy, [ scalar(@$program), $p1, 2 ];
      push @$program, [ $id->{$op}, substr($p1,1), 0xfff ];
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
      push @$program, [ $id->{$op}, $a, $b ];
      print "$op $a/$b\n";
    } elsif ($op =~ /^JUMP/) {
      push @$lazy, [ scalar(@$program), $p1, 2 ];
      push @$program, [ $id->{$op}, 0, 0xfff ];
    } elsif ($op eq "BREAK") {
      push @$program, [ $id->{$op}, 0, 0 ];
    } elsif ($op eq "DO") {
      push @$program, [ $id->{$op}, $actions->{$p1}, 0 ];
    } elsif ($op eq "DOMOD") {
      push @$program, [ $id->{$op}, $actions->{$p1}, $action_mod->{$p2} ];
    } elsif ($op eq "RETURN") {
      push @$program, [ $id->{$op}, 0, 0 ];
    } elsif ($op eq "SENSE") {
      # 0x00000101 0x00bb00cc0eee 0x0000000ddddd
      #  A         B2     C2      D5        E3
      # operation amount sense  direction  distance
      my ($amt, $sense, $dir, $dist) = ($p2, $senses->{$p1}, $p3, $p4);
      #push @$program, [ $id->{$op}, ($p2 << 4) + $senses->{$p1}, $p3, $p4 ];
      push @$program, [ $id->{$op},
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
  for (@$lazy) {
    my ($at, $label, $offs) = @$_;
    printf "lazy: %d %s 0x%x\n", $at, $label, $labels->{$label};
    $program->[$at]->[$offs] = $labels->{$label};
  }
  my $ix = 0;
  $this->{_bytecode} = [];
  for (@$program) {
    printf "0x%04x: %6s %03x %03x\n", $ix++, $bkid->{$_->[0]}, $_->[1], $_->[2];
    push @{$this->{_bytecode}}, 
          ($_->[0] << 24) | ($_->[1] << 12) | ( $_->[2]);
  }
}

sub blockcx {
  my $this = shift;

  # get net interrupts, see if cached
  my $blockstack = $this->{_blockstack};

  my $codetxt = "";
  for my $block (@$blockstack) {
    $codetxt .= $block->show() . "\t";
  }
  my $frameid;
  if (exists $this->{_labelcache}->{$codetxt}) {
    $frameid = $this->{_labelcache}->{$codetxt};
  } else {
    $frameid = $this->nextid("F");
    $this->{_labelcache}->{$codetxt} = $frameid;
    $this->{_labelset}->{$frameid} = [ map { $_ } @$blockstack ];
  }
  #print "----> $codetxt\n";

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
      #print Dumper($_);
      $this->codegen2($_) if $_;
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
    my $mod = $at->named('ActionMod');
    if ($mod) {
      $mod = $mod->value();
    } else {
      $mod = "";
    }
    #print "DO action: $act $mod\n";
    my $actop = "";
    $actop = "TURN" if ($act eq "t");
    $actop = "MOVE" if ($act eq "m");
    $actop = "FIGHT" if ($act eq "f");
    $actop = "EAT"  if ($act eq "e");
    $actop = "EXIT"  if ($act eq "x");
    my $modop;
    $modop = "LEFT" if $mod eq "<";
    $modop = "RIGHT" if $mod eq ">";
    if ($modop) {
      $this->code("DOMOD", $actop, $modop);
    } elsif ($act eq "x") {
      $this->code("BREAK");  # FIXME: lazy fixup??
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
    for (@{$at->list()}) {
      if ($_->tag() eq "Condition") {
        #print "INSTALL condition - frame " . scalar(@{$this->{_blockstack}}) . "\n";
        push @{$this->{_blockstack}}, $_;
        $ccount++;
      }
    }
    # get net interrupts, see if cached
    $this->blockcx();

    # run the statements
    for (@{$at->list()}) {
      if ($_->tag() ne "Condition") {
        $this->codegen2($_);
      }
    }

    for (0 .. $ccount) {
      pop @{$this->{_blockstack}};
    }
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
    print "$value";
    $this->visualize2($t->named('ActionMod'), 0, 0) if $t->named('ActionMod');
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


package main;

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
my $map = {
  0 => { '@' => 2, '9' => 2, '{' => 1 },
  1 => { '{' => 1, '9' => 2, '(' => 4, ')' => 1, '@' => 2, 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9 },
  2 => { '{' => 1, 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9, 'x' => 9 },
  3 => { 'f' => 9, 'm' => 9, 't' => '9', 'e' => 9, 'x' => 9, '{' => 1 },
  4 => { 'f' => 5, 'F' => 5, 'e' => 5, 'E' => 5, '?' => 8, ':' => 10 },
  5 => { '!' => 6, '<' => 6, '>' => 6, '<<' => 6, '>>' => 6,
            '_' => 7, '-' => 7, '^' => 7 },
  6 => { '!' => 6, '<' => 6, '>' => 6, '<<' => 6, '>>' => 6 },
  7 => { '_' => 7, '-' => 7, '^' => 7 },
  8 => { 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9, 'x' => 9, '9' => 3, '@' => 3, '{' => 1 },
  9 => { '<' => 16, '>' => 16 },
  10 => { 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9, 'x' => 9, '9' => 3, '@' => 3 },
  16 => { },
};

sub tokenize {
  my $input = shift;
  my $verbose = shift;
  my $tokens2 = [ '<<', '>>' ];
  my $tokens = [ '*', '@', '{', '}', '(', ')', '!', '<', '>', '_', '-', '^', '?', ':', qw/f m t e x/, 'F', 'E' ];
  my $ret = [];
  while ($input ne "") {
    my $c2 = substr($input, 0, 2);
    my $token;
    my $len = 0;
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
    die "tokenization error near: $input" if (!defined $token);
    push @$ret, $token;
    $input = substr($input, $len);
  }
  return $ret;
}

sub visualize {
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
      visualize($block, $indent + 3, $verbose);
    } else {
      die;
    }
  } elsif ($tag eq "Block") {
    print '{';
    for (@$list) {
      visualize($_, $indent + 3, $verbose);
    }
    print '}';
  } elsif ($tag eq "Action") {
    print "$value";
    visualize($t->named('ActionMod'), 0, 0) if $t->named('ActionMod');
  } elsif ($tag eq "CondThen") {
    for (@{$t->named('ActionList')}) {
      visualize($_, $indent + 3, $verbose);
    }
  } elsif ($tag eq "CondElse") {
    for (@{$t->named('ActionList')}) {
      visualize($_, $indent + 3, $verbose);
    }
  } elsif ($tag eq "CondTest") {
    print "$value";
    for (@{$t->named('DirMod')}) {
      visualize($_, $indent + 3, $verbose);
    }
    for (@{$t->named('DistMod')}) {
      visualize($_, $indent + 3, $verbose);
    }
  } elsif ($tag eq "ActionMod") {
    print "$value";
  } elsif ($tag eq "DirMod") {
    print "$value";
    #visualize($t->{'DirMod'}, 0, 0) if exists $t->{'DirMod'};
  } elsif ($tag eq "DistMod") {
    print "$value";
    #visualize($t->{'DistMod'}, 0, 0) if exists $t->{'DistMod'};
  } elsif ($tag eq "Condition") {
    print "(";
    visualize($t->named('CondTest'), $indent + 3, $verbose);
    print "?";
    if ($t->named('CondThen')) {
      visualize($t->named('CondThen'), $indent + 3, $verbose);
    }
    if ($t->named('CondElse')) {
      print ":";
      visualize($t->named('CondElse'), $indent + 3, $verbose);
    }
    #if (exists $t->{'CondElse'}) {
    #  visualize($t->{'CondElse'}, $indent + 3, $verbose);
    #}
    print ")";
  } elsif ($tag eq "") {
  }
  #print ']';
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
  while (1) {
    print "TOP\n" if $verbose;
    my $tk = $tokens->[0];
    my $tkvalue = $tk->[1] if ref($tk);
    $tk = $tk->[0] if ref($tk);

    #print "/$tk/$tkvalue/\n";
    my $next_state  = $map->{$state}->{$tk};
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
          my $item = Node->new("Action", $frame->[0]->value());
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
        if ($tkvalue) {
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
my $input = '1{(F_<!>?e)(e?x)3m{(f?t>)2m}t<}*';
#my $input = '3{fm}*';
print "Input:\n$input\n";
#my $tokens = tokenize($input, $verbose);
my $tokens = tokenize($input, 0);
#print "Tokenization:\n";
#print Dumper($tokens);
my $tree = parse($tokens, 0);
print "Parse Tree:\n";
print Dumper($tree);
#print "Pretty print:\n";
#visualize($tree);
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

