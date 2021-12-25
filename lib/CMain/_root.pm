package CMain::_root;

use Clib::strict8;

use Cache::Memcached::Fast;

sub _root :
        Simple
{
    # Получаем предыдущий список
    my $memd = Cache::Memcached::Fast->new( c('MemCached') )
        || return 'main';
    
    my $flyday = $memd->get('flyday');
    my $flylist = ref($flyday) eq 'HASH' ? $flyday->{flylist} : undef;
    $flylist = [] if ref($flylist) ne 'ARRAY';
    
    # Распределяем взлёты по страницам
    my @sheet =
        reverse
        map {
            my $sheet = { %$_ };
            delete($sheet->{$_})
                foreach grep { ref $sheet->{$_} } keys %$sheet;
            
            my @fly = grep { $_->{sheetid} eq $sheet->{id} } @$flylist;
            my $sheet1 = { %$sheet }; # чтобы не было перекрёстной ссылки, ведущей к утечке памяти
            $_->{sheet} = $sheet1 foreach @fly;
            $sheet->{flylist} = [@fly];
            
            @fly ? $sheet : ();
        }
        @{ c('flySheet') || [] };
    
    # Данные для отображения
    my $time = time();
    foreach my $fly (@$flylist) {
        # обработка поля meta
        $fly->{$_} = 0 foreach qw/hidden closed closed_recently/;
        if ($fly->{meta} =~ /\-/) {
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
        elsif (($fly->{meta} =~ /^\s*(\d+)\s*$/) && $fly->{meta_time}) {
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
        
        # Персоны
        my $n = 0;
        foreach my $pers (@{ $fly->{pers} || [] }) {
            $pers->{i} = ++$n;
            
            ($pers->{t}) = grep { $_->{id} eq $pers->{type} } @{ c('persType') || [] };
            $pers->{type_name} = $pers->{t} ? $pers->{t}->{name} : $pers->{type};
        }
        
        # Добиваем до нужного числа посад. мест
        my $sheet = $fly->{sheet};
        if ($sheet && (my $cnt = $sheet->{count})) {
            my $pers = ($fly->{pers}||=[]);
            push(
                @$pers,
                {
                    i       => ++$n,
                    name    => '',
                }
            ) while @$pers < $cnt;
        }
    }
    
    return
        'main',
        tmload      => $flyday->{time},
        flylist     => $flylist,
        sheetlist   => \@sheet;
}

1;
