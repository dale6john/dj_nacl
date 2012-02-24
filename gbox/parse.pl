#!/usr/bin/perl -w

use strict;
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
my $map = {
  0 => { '@' => 2, '9' => 2, '{' => 1 },
  1 => { '{' => 1, '9' => 2, '(' => 4, ')' => 1, '@' => 2, 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9 },
  2 => { '{' => 1, 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9 },
  3 => { 'f' => 9, 'm' => 9, 't' => '9', 'e' => 9 },
  4 => { 'f' => 5, 'F' => 5, 'e' => 5, 'E' => 5, '?' => 8, ':' => 10 },
  5 => { '!' => 6, '<' => 6, '>' => 6, '<<' => 6, '>>' => 6,
            '_' => 7, '-' => 7, '^' => 7 },
  6 => { '!' => 6, '<' => 6, '>' => 6, '<<' => 6, '>>' => 6 },
  7 => { '_' => 7, '-' => 7, '^' => 7 },
  8 => { 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9, '9' => 3, '@' => 3 },
  9 => { '<' => 16, '>' => 16 },
  10 => { 'f' => 9, 'm' => 9, 't' => 9, 'e' => 9, '9' => 3, '@' => 3 },
  16 => { },
};

sub tokenize {
  my $input = shift;
  my $verbose = shift;
  my $tokens2 = [ '<<', '>>' ];
  my $tokens = [ '*', '@', '{', '}', '(', ')', '!', '<', '>', '_', '-', '^', '?', ':', qw/f m b e/ ];
  my $ret = [];
  while ($input ne "") {
    my $c2 = substr($input, 0, 2);
    my $token;
    if (grep {$c2 eq $_} @$tokens2) {
      print "match(2): $c2\n" if $verbose;
      $token = $c2;
    } else {
      my $c1 = substr($input, 0, 1);
      if (grep {$c1 eq $_} @$tokens) {
        print "match(1): $c1\n" if $verbose;
        $token = $c1;
      } else {
        if ($input =~ /^(\d+)/) {
          print "match(d): $1\n" if $verbose;
          $token = $1;
        }
      }
    }
    die "tokenization error near: $input" if (!defined $token);
    push @$ret, $token;
    $input = substr($input, length($token));
  }
  return $ret;
}

sub testStack {
  my $result_stack = shift;
  my $target = shift;
  return undef unless @$result_stack;
  my $ret = pop @$result_stack;
  die "test $target" unless $ret;
  if ($ret->{'id'} ne $target) {
    push @$result_stack, $ret;
    $ret = undef;
  }
  return $ret;
}

sub show_result_stack {
  my $st = shift;
  my $indent = shift || 0;
  print " " x $indent . $st->{'id'} . "\t";
  if (exists $st->{'v'}) {
    print "(" . $st->{'v'} . ")";
  }
  print "\n";
  for my $item (keys %$st) {
    next if $item eq 'id';
    next unless ref($st->{$item}) eq "HASH";
    next unless exists $st->{$item}->{'id'};
    show_result_stack($st->{$item}, $indent + 3);
  }
}

sub parse {
  my $tokens = shift;
  my $verbose = shift;
  my $state_stack = [];
  my $stack = [];
  my $result_stack = [];
  my $state = 0;
  my $zz = 0;

  my $prev_state;
  while (1) {
    my $tk = $tokens->[0];
    my $next_state = $map->{$state}->{$tk};

    # state 2 with Action on the stack, force reduce
    my $force_reduce = 0;
    if ($state == 2 && @$result_stack && $result_stack->[@$result_stack-1]->{'id'} eq "Action") {
      $force_reduce = 1;
    }

    if (defined $next_state && !$force_reduce) {
      print "State: $state  SHIFT  (tk: '$tk') to state $next_state\t" if $verbose;
    } else {
      my $nx = pop @$state_stack;
      push @$state_stack, $nx if $nx;
      $nx ||= "undef";
      print "State: $state  REDUCE " . ($force_reduce?"forced":"") . " (tk: '$tk') then pop to $nx\t" if $verbose;
    }
    print "states: " . join(", ", @$state_stack) . "\n" if $verbose;
    print "stack : " . Dumper($stack) if $verbose;
    print "result stack:\n" if $verbose;
    print Dumper($result_stack) if $verbose;
    map { show_result_stack($_, 4) } @$result_stack if $verbose;

    # try to shift
    if (defined $next_state && !$force_reduce) {
      shift @$tokens;
      push @$stack, { 'id' => 'TOKEN', 'v' => $tk, 'sp' => scalar(@$result_stack) };
      push @$state_stack, $state;
      $prev_state = $state;
      $state = $next_state;
    } else {
      #print "shift error (st: $state) (tk: '$tk')\n" if $verbose;
      # try to reduce
      # stack things from current state
      #die "empty stack" unless @$state_stack;
      my $scurrent = pop @$stack;
      print Dumper($scurrent);
      die unless $scurrent->{'id'} eq "TOKEN";
      my $current = $scurrent->{'v'} if $scurrent;
      my $sp = $scurrent->{'sp'} if $scurrent;
      my $ss = pop @$state_stack;
      print "reduce st: $state($ss), curr: $current\n" if $verbose;
      if ($state == 1) {
        my $items = [];
        while(scalar(@$result_stack) > $sp) {
          while (my $repeat = testStack($result_stack, "Repeat")) {
            unshift @$items, $repeat;
          }
          while (my $cond = testStack($result_stack, "Condition")) {
            unshift @$items, $cond;
          }
          while (my $cond = testStack($result_stack, "Block")) {
            unshift @$items, $cond;
          }
          while (my $cond = testStack($result_stack, "Action")) {
            unshift @$items, $cond;
          }
        }
        push @$result_stack, { 'id' => 'Block', 'List' => [ @$items ] };
        $state = $ss;
        shift @$tokens if ($tk eq '}');
        
      } elsif ($state == 2) {
        my $action = testStack($result_stack, "Action");
        my $block = testStack($result_stack, "Block");
        my $add = { 'id' => 'Repeat', 'v' => $current };
        $add->{'Action'} = $action if $action;
        $add->{'Block'} = $block if $block;
        push @$result_stack, $add;
        $state = $ss;

      } elsif ($state == 3) {
        my $action = testStack($result_stack, "Action");
        push @$result_stack, { 'id' => 'SimpleRepeat',
            'v' => $current, 'Action' => $action };
        $state = $ss;

      } elsif ($state == 4) {
        my $else   = testStack($result_stack, "CondElse");
        my $then   = testStack($result_stack, "CondThen");
        my $action = testStack($result_stack, "CondTest");
        print "syntax state: $state $action/$then" unless $action && $then;
        die "syntax state: $state $action/$then" unless $action && $then;
        my $item = { 'id' => 'Condition', 'CondTest' => $action };
        if ($else) {
          $item->{'CondElse'} = $else;
        } elsif ($then) {
          $item->{'CondThen'} = $then;
        }
        push @$result_stack, $item;
        $state = $ss;
        
      } elsif ($state == 5) {
        my $distmod = testStack($result_stack, "DistMod");
        my $dirmod  = testStack($result_stack, "DirMod");
        push @$result_stack, { 'id' => 'CondTest', 'v' => $current,
            'DistMod' => $distmod, 'DirMod' => $dirmod };
        $state = $ss;

      } elsif ($state == 6) {
        my $dirmod  = testStack($result_stack, "DirMod");
        if ($dirmod) {
          push @$result_stack, { 'id' => 'DirMod', 'v' => $current, 'DirMod' => $dirmod };
        } else {
          push @$result_stack, { 'id' => 'DirMod', 'v' => $current };
        }
        $state = $ss;
        
      } elsif ($state == 7) {
        my $distmod  = testStack($result_stack, "DistMod");
        if ($distmod) {
          push @$result_stack, { 'id' => 'DistMod', 'v' => $current, 'DistMod' => $distmod };
        } else {
          push @$result_stack, { 'id' => 'DistMod', 'v' => $current };
        }
        $state = $ss;
        
      } elsif ($state == 8) {
        my $then = testStack($result_stack, "Action");
        push @$result_stack, { 'id' => 'CondThen', 'Action' => $then };
        $state = $ss;
        
      } elsif ($state == 9) {
        my $ck = testStack($result_stack, "ActionMod");
        if ($ck) {
          push @$result_stack, { 'id' => 'Action', 'v' => $current,
              'ActionMod' => $ck };
        } else {
          push @$result_stack, { 'id' => 'Action', 'v' => $current };
        }
        $state = $ss;
        
      } elsif ($state == 10) {
        my $else = testStack($result_stack, "Action");
        push @$result_stack, { 'id' => 'CondElse', 'Action' => $else };
        $state = $ss;
        
      } elsif ($state == 13) {
        my $ck = testStack($result_stack, "ActionMod");
        if ($ck) {
          push @$result_stack, { 'id' => 'SimpleAction', 'v' => $current,
              'ActionMod' => $ck };
        } else {
          push @$result_stack, { 'id' => 'SimpleAction', 'v' => $current };
        }
        $state = $ss;
        
      } elsif ($state == 15) {
        my $actionmod = testStack($result_stack, "ActionMod");
        if ($actionmod) { 
          push @$result_stack, { 'id' => 'Action',
              'v' => $current, 'ActionMod' => $actionmod };
        } else {
          push @$result_stack, { 'id' => 'Action', 'v' => $current };
        }
        $state = $ss;

      } elsif ($state == 16) {
        push @$result_stack, { 'id' => 'ActionMod', 'v' => $current };
        $state = $ss;
        
      } else {
        die $state;
      }
    }
    if (!defined $state || $state == 0) {
      printf "VERY HAPPY\n" if ($tk eq '*');
      die "stack state wrong" unless @$result_stack == 1;
      return $result_stack->[0];
    }
  }

}

sub visualize {
  my $t = shift;
  my $indent = shift || 0;
  my $verbose = shift;

  my $id = $t->{'id'};
  print '[';
  if (!defined $id) {
    print "UNDEF";
  } elsif ($id eq "Repeat") {
    my $block = (exists $t->{'Block'});
    my $opt = $t->{'v'};
    if ($block) {
      print "$opt" . '{';
      visualize($t->{'Block'}, $indent + 3, $verbose);
      print '}';
    } else {
      print "$opt";
      visualize($t->{'Action'}, $indent + 3, $verbose);
    }
  } elsif ($id eq "Block") {
    for (@{$t->{'List'}}) {
      visualize($_, $indent + 3, $verbose);
    }
  } elsif ($id eq "Action") {
    print "$t->{'v'}";
    visualize($t->{'ActionMod'}, 0, 0) if exists $t->{'ActionMod'};
  } elsif ($id eq "CondThen") {
    visualize($t->{'Action'}, $indent + 3, $verbose);
  } elsif ($id eq "CondElse") {
    visualize($t->{'Action'}, $indent + 3, $verbose);
  } elsif ($id eq "CondTest") {
    print "$t->{'v'}";
    visualize($t->{'DirMod'}, 0, 0) if exists $t->{'DirMod'};
    visualize($t->{'DistMod'}, 0, 0) if exists $t->{'DistMod'};
  } elsif ($id eq "ActionMod") {
    print "$t->{'v'}";
  } elsif ($id eq "DirMod") {
    print "$t->{'v'}";
    visualize($t->{'DirMod'}, 0, 0) if exists $t->{'DirMod'};
  } elsif ($id eq "DistMod") {
    print "$t->{'v'}";
    visualize($t->{'DistMod'}, 0, 0) if exists $t->{'DistMod'};
  } elsif ($id eq "Condition") {
    print "(";
    visualize($t->{'CondTest'}, $indent + 3, $verbose);
    print "?";
    visualize($t->{'CondThen'}, $indent + 3, $verbose);
    print ":";
    visualize($t->{'CondElse'}, $indent + 3, $verbose);
    #if (exists $t->{'CondElse'}) {
    #  visualize($t->{'CondElse'}, $indent + 3, $verbose);
    #}
    print ")";
  } elsif ($id eq "") {
  }
  print ']';
}

# main
my $verbose = 0;
my $input = '@{9{(f!>>_?:e<)@m}}';
#my $input = '@{9{(f!>>_?e>:e<)@m}}';
#my $input = '@{9m>f}*';
#my $input = '@{f}*';
print "Input:\n$input\n";
my $tokens = tokenize($input, $verbose);
print "Tokenization:\n";
print Dumper($tokens);
my $tree = parse($tokens, $verbose);
print "Parse Tree:\n";
print Dumper($tree);
print "Pretty print:\n";
visualize($tree);
print "\n";
print "$input\n";

print "\n";
while(<STDIN>) {
  chomp;
  visualize(parse(tokenize($_, 0)));
  print "\n";
  print "\n";
}

