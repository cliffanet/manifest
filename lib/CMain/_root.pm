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
    
    # Данные для отображения
    foreach my $fly (@$flylist) {
        # обработка поля meta
        flyinf->fstate($fly);
        
        # Персоны
        my $n = 0;
        foreach my $pers (@{ $fly->{pers} || [] }) {
            $pers->{i} = ++$n;
            
            ($pers->{t}) = grep { $_->{id} eq $pers->{type} } @{ c('persType') || [] };
            $pers->{type_name} = $pers->{t} ? $pers->{t}->{name} : $pers->{type};
        }
    }
    
    # Уберём все улетевшие.
    # Сделать это лучше до распределения по страницам
    @$flylist =
        grep { !$_->{closed} && !$_->{hidden} }
        @$flylist;
    
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

    # Добиваем до нужного числа посад. мест
    foreach my $fly (@$flylist) {
        my $sheet = $fly->{sheet} || next;
        my $cnt = $sheet->{count} || next;
        
        my $pers = ($fly->{pers}||=[]);
        push(
            @$pers,
            {
                i       => @$pers+1,
                name    => '',
            }
        ) while @$pers < $cnt;
    }
    
    return
        'main',
        tmload      => $flyday->{time},
        flylist     => $flylist,
        sheetlist   => \@sheet;
}

1;
