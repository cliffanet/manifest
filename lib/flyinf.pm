package flyinf;

use strict;
use warnings;
use utf8;

use Clib::Const;
use Clib::Log;

use Excel::Reader::XLSX;

#######################################################################
#       Функционал для пост-обработки данных по взлётам,
#       которые находятся во временной БД (MemCached)
#######################################################################


####################################################
##
##  Определение текущей закладки (тип ЛА)
##
sub fsheet {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $fly = shift() || return;
    
    ($fly->{sheet}) =
        grep { $fly->{sheetid} eq $_->{id} }
        @{ c('flySheet') || [] };
    
    $fly->{sheet} ||= {
        id  => $fly->{sheetid},
        name=> '',
    };
    
    return $fly;
}

####################################################
##
##  Определение состояния взлёта
    # обработка поля meta
##
sub fstate {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $fly = shift() || return;
    
    my $time = time();
    $fly->{$_} = 0 foreach qw/hidden closed closed_recently/;
    if ($fly->{meta} =~ /[\-x]/) {
        # скрытый взлёт
        $fly->{hidden} = 1;
    }
    elsif ($fly->{meta} =~ /\*/) {
        # Взлёт, помеченный как "улетевший"
        if ((my $tm = $fly->{meta_time}) && ($fly->{meta_prev} !~ /^\s*(\d+)\s*$/)) {
            $fly->{closed_recently} = $time - $tm;
            $fly->{closed} = 1 if $fly->{closed_recently} > 600;
        }
        else {
            $fly->{closed} = 1;
        }
    }
    elsif ($fly->{meta} =~ /^\s*(\d+)\s*$/) {
        if ($fly->{meta_time}) {
            # Взлёт, которому дали готовность
            my $tm = $fly->{meta_time} + ($1 * 60);
            if ($tm >= $time) {
                $fly->{before} = $tm - $time + 1;
            }
            else {
                $fly->{closed_recently} = $time - $tm;
                $fly->{closed} = 1 if $fly->{closed_recently} > 600;
            
            }
        }
        else {
            $fly->{closed} = 1;
        }
    }
    
    # Поле state:
    # P - подъём с начинающими - он не мониторится и не выводится в статистику
    # H - скрытый взлёт
    # q - в очереди
    # b - взлёт, которому дана готовность
    # f - недавно улетевший (ещё летающий)
    # h - давно улетевший (выполненный взлёт)
    if ($fly->{meta} =~ /x/) {
        $fly->{state} = 'P';
    }
    elsif ($fly->{hidden}) {
        $fly->{state} = 'H';
    }
    elsif (my $before = $fly->{before}) {
        $fly->{state} = 'b';
    }
    elsif (my $closer = $fly->{closed_recently}) {
        $fly->{state} = $fly->{closed_recently} > 900 ? 'h' : 't';
    }
    elsif ($fly->{closed}) {
        $fly->{state} = 'h';
    }
    else {
        $fly->{state} = 'q';
    }
    
    return $fly;
}


####################################################
##
##  Различные представления списка взлётов
##

# summary по spec-персонам
sub _view_specsumm {
    my %spec = ();
    foreach my $fly (@_) {
        my ($sheet) = grep { $_->{id} eq $fly->{sheetid} } @{ c('flySheet') || [] };
        my $flyname = ($sheet||{})->{name} . ': ' . $fly->{name};
        my $perscnt = @{ $fly->{pers}||[] };
        
        foreach my $s (@{ $fly->{spec}||[] }) {
            my $sc = ($spec{ $s->{code} } ||= {});
            my $sn = ($sc->{ $s->{name} } ||= 
                        {
                            code => $s->{code},
                            name => $s->{name},
                            perscnt => 0,
                            fly => [],
                        });
            $sn->{perscnt} += $perscnt;
            push @{ $sn->{fly} }, $flyname . ' = ' . $perscnt;
        }
    }
    
    my @spec = 
        sort {
            ($a->{code} cmp $b->{code}) ||
            ($a->{name} cmp $b->{name})
        }
        map { values %$_; } 
        values %spec;
    foreach my $s (@spec) {
        $s->{flycnt} = @{ $s->{fly} };
        $s->{fly} = join '; ', @{ $s->{fly} };
    }
    
    return [@spec];
}

# участники взлётов
sub _view_flyers {
    my %pers = ();
    foreach my $fly (@_) {
        next if $fly->{meta} eq 'x'; # отфильтровываем начинающих
        my ($sheet) = grep { $_->{id} eq $fly->{sheetid} } @{ c('flySheet') || [] };
        my $flyname = ($sheet||{})->{name} . ': ' . $fly->{name};
        
        foreach my $p (@{ $fly->{pers}||[] }) {
            my $c = $p->{code};
            $c =~ s/^\s+//;
            $c =~ s/\s+$//;
            my ($code, $summ) = $c =~ /^(?:(.+)\s+)?(\d+)$/;
            ($code, $summ) = ($c, 0) if !defined($summ);
            $code = '' if !defined($code);
            my $pn = ($pers{ $p->{name} } ||= {});
            my $pi = ($pn->{ $code } ||= 
                        {
                            name => $p->{name},
                            code => $code,
                            summ => 0,
                            fly => [],
                        });
            $pi->{summ} += $summ;
            push @{ $pi->{fly} }, $flyname;
        }
    }
    
    my @pers = 
        sort {
            ($a->{code} cmp $b->{code}) ||
            ($a->{name} cmp $b->{name})
        }
        map { values %$_; } 
        values %pers;
    foreach my $p (@pers) {
        $p->{flycnt} = @{ $p->{fly} };
        $p->{fly} = join '; ', @{ $p->{fly} };
    }
    
    return [@pers];
}

# Сводка по взлётам и количеству прыжков
sub _view_flysumm {
    my %info = ();
    my @info = ();
    my $sum = {
        %{ c('flySummary')||{} },
        flycnt  => 0,
        perscnt => 0,
        speccnt => 0,
        summ    => 0,
        issumm  => 1,
    };
    foreach my $fly (@_) {
        my $inf = $info{ $fly->{sheetid} };
        if (!$inf) {
            $inf = {
                sheetid => $fly->{sheetid},
                flycnt  => 0,
                perscnt => 0,
                speccnt => 0,
                summ    => 0,
            };
            $info{ $fly->{sheetid} } = $inf;
            push @info, $inf;
            
            my ($sheet) =
                grep { $fly->{sheetid} eq $_->{id} }
                @{ c('flySheet') || [] };
            $sheet ||= { name => $fly->{sheetid} };
            $inf->{name} = $sheet->{name};
        }

        $inf->{flycnt} ++;
        $sum->{flycnt} ++;
        
        my $perscnt = @{ $fly->{pers}||[] };
        $inf->{perscnt} += $perscnt;
        $sum->{perscnt} += $perscnt;
        
        my $speccnt = @{ $fly->{spec}||[] };
        $inf->{speccnt} += $speccnt;
        $sum->{speccnt} += $speccnt;
        
        my $summ = 0;
        foreach my $p (@{ $fly->{pers}||[] }) {
            $summ += $1 if $p->{code} =~ /(\d+)/;
        }
        $inf->{summ}    += $summ;
        $sum->{summ}    += $summ;
    }
    
    push @info, $sum;
    
    return [@info];
}

# Общая инфа по взлётам, которые видны на табло
sub _view_flyinfo {
    my @fly = ();
    foreach (@_) {
        # Пересобираем, чтобы не коцать оригинальные данные
        my $fly = { %$_ };
        # обработка поля meta
        flyinf->fstate($fly);
        next if $fly->{closed} || $fly->{hidden};

        flyinf->fsheet($fly);
        $fly->{perscnt} = @{ delete($fly->{pers}) || [] };
        $fly->{speccnt} = @{ delete($fly->{spec}) || [] };
        
        push @fly, $fly;
    }
    
    return [@fly];
}

my %view = (
    specsumm    => \&_view_specsumm,
    flyers      => \&_view_flyers,
    flysumm     => \&_view_flysumm,
    flyinfo     => \&_view_flyinfo,
);
sub view {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $view = shift;
    my $log = log_prefix('xls2fly->view(\'%s\')', $view);
    return $view{ $view };
}
####################################################
##
##  Получение изменений по участникам взлётов
##


####################################################
##
##  Получение изменений по участникам взлётов
##
sub pers2log {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    
    my $flist = shift() || return;
    my $fprev = shift() || return;
    
    # Сформируем хеш участников из предыдущего сохранения
    # структура: имя_участника => { взлёт1 => { fly => { поля взлёта }, pers => { поля участника } }, взлётN => { ... } }
    # Поля взлёта нам нужны, чтобы при логировании удалённых из взлётов участников у нас были данные по взлёту, из которого его удалили
    # Поля pers нужны, чтобы проверить изменения, если участник остался во взлёте
    my %pers = ();
    my $fnum = 0;
    foreach my $fly (@$fprev) {
        $fnum ++;
        my $flykey =  join ' :: ', $fly->{sheetid}, lc($fly->{name});
        my %fly = %$fly;
        delete($fly{$_}) foreach qw/pers spec/;
        my @pers = @{ $fly->{pers}||[] }; # Возможно, сюда надо будет добавить ещё каких-то спец-персон
        foreach my $p (@pers) {
            my $perskey = lc($p->{name}) || next; # персон без имени не логируем
            
            my $pf = ($pers{$perskey} ||= {});
            $pf->{$flykey} = { fly => { %fly }, pers => $p, n => $fnum };
        }
    }
    
    # теперь для каждого участника смотрим изменения по взлётам
    # $flykey и $perskey надо определять по такому же алгоритму, что и в предыдущем цикле
    my @log = ();
    my %dup = ();
    foreach my $fly (@$flist) {
        my $flykey =  join ' :: ', $fly->{sheetid}, lc($fly->{name});
        my %fly = %$fly;
        delete($fly{$_}) foreach qw/pers spec/;
        my @pers = @{ $fly->{pers}||[] }; # Возможно, сюда надо будет добавить ещё каких-то спец-персон
        foreach my $p (@pers) {
            my $perskey = lc($p->{name}) || next; # персон без имени не логируем
            my $pfkey = join ' :: ', $flykey, $perskey;
            
            if ($dup{$pfkey}) {
                # Контроль повтора во взлёте такой же фамилии
                push @log, {
                    op  => 'D',
                    fly => { %fly },
                    pers=> $p
                };
                next;
            }
            
            $dup{$pfkey} = 1;
            my $pf = ($pers{$perskey} || {});
            if (my $prev = delete $pf->{$flykey}) {
                # Как был так и остался во взлёте,
                # проверим, есть ли изменения в полях
                my $pp = $prev->{pers};
                my %pn = %$p;
                foreach my $f (keys %$pp) {
                    my $vp = $pp->{$f};
                    my $vn = delete $pn{$f};
                    next if defined($vp) && !defined($vn);
                    next if !defined($vp) && defined($vn);
                    next if defined($vp) && defined($vn) && ($vp ne $vn);
                    # удаляем все поля из предыдущей версии, которые не изменились
                    delete($pp->{$f});
                }
                $pp->{$_} = $pn{$_} foreach keys %pn;
                
                # в итоге после всех сравнений - %$pp хранит все изменившиеся поля
                %$pp || next; # и если изменений нет, то и не логируем
                
                push @log, {
                    op  => 'e',
                    fly => { %fly },
                    prev=> $pp,     # prev - хранит только изменившиеся поля с предыдущего сохранения
                    pers=> $p       # pers - в любом случае хранит все текущие поля
                };
            }
            else {
                # был добавлен во взлёт
                push @log, {
                    op  => 'a',
                    fly => { %fly },
                    pers=> $p
                };
            }
        }
    }
    
    # оставшиеся в %pers взлёты - это те взлёты, откуда участник был удалён
    # 
    # Для красоты вывода информации, будем оповещать об удалении в самом начале списка,
    # для этого все удаления добавим в отдельный список
    my @del = ();
    foreach my $perskey (%pers) {
        my $pf = $pers{$perskey} || next;
        my @prev =
            sort { $a->{n} <=> $b->{n} } # сортировка взлётов в исходном порядке (для красоты вывода)
            values %$pf;
        
        foreach my $prev (@prev) {
            push @del, {
                op  => 'd',
                fly => $prev->{fly},
                pers=> $prev->{pers}
            };
        }
    }
    
    return @del, @log;
}

####################################################

1;
