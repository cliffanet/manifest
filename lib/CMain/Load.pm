package CMain::Load;

use Clib::strict8;

use xls2fly;

use Cache::Memcached::Fast;
use JSON::XS;


########################################
###
###     Доп опции при загрузке файла,
###     которые мы будем возвращать в json-формате
###

my %load_opts = (
    # summary по spec-персонам
    specsumm => sub {
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
    },
    
    flyers => sub {
        my %pers = ();
        foreach my $fly (@_) {
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
    },
);

########################################
###
###     Загрузка excel-файла
###

sub rerr {
    my $s = shift();
    $s = sprintf($s, @_) if @_;
    error($s);
    return 'ERROR '.$s;
}

sub _root :
        ReturnText
{
    # подготовка временной директории
    my $dir = c('tmpdir') || return rerr('No `tmpdir`');
    if (!(-d $dir) && !mkdir($dir)) {
        return rerr('Can\'t make tmpdir');
    }
    
    # Получение файла
    # Excel::Reader::XLSX не умеет работать с хендлами, только с физическим файлом
    my $fname = $dir.'/src.xlsx';
    my $p = wparam(file => { file => $fname });
    
    my $size = -s $fname;
    debug('file len: %d', $size);
    
    $size || return rerr('No file to load');
    
    # Парсим
    my $flylist = xls2fly->parse($fname, 'flylist');
    unlink $fname;
    
    $flylist || return rerr(xls2fly->err());
    
    # Дополнительные данные
    xls2fly->adding_data(@$flylist);
    
    # Получаем предыдущий список
    my $memd = Cache::Memcached::Fast->new( c('MemCached') )
        || return rerr('Memcached init');
    
    my $fprev = $memd->get('flyday');
    if ((ref($fprev) eq 'HASH') && (ref($fprev->{flylist}) eq 'ARRAY')) {
        # Дополнительные параметры, основанные на разнице от прошлой загрузки
        xls2fly->by_prev($flylist, $fprev->{flylist});
        
        # Вычисляем изменеия и сообщаем о необходимости оповестить
    }
    
    # Сохраняем
    if ($p->bool('nosave')) {
        debug('option \'nosave\'');
    }
    else {
        $memd->set(
            flyday => {
                    time    => time(),
                    flylist => $flylist,
            },
            3600
        ) || return rerr('Memcached write');
    }
    
    my @opt = ();
    # доп. опции, которые запрашивает клиентская программа,
    # это какие-то сводные данные на основании полученного excel-файла
    # Мы их будем выводить в json-формате построчно после OK.
    foreach my $ol ($p->allstr('opt')) {
        foreach my $o (split(/\s*,\s*/, $ol)) {
            my $sub = $load_opts{$o}
                || return rerr('Unknown required opt: %s', $o);
            
            my $r = $sub->(@$flylist)
                || return rerr('Run-fail on required opt: %s', $o);
            my $json = eval { JSON::XS->new->pretty(0)->encode($r) }
                || return rerr('returned format wrong on required opt: %s', $o);
            # каждая опция будет возвращена отдельной строчкой
            # в ответе в формате:
            # OPTNAME { ... }
            push @opt, uc($o) . ' ' . $json;
        }
    }
    
    return OK => @opt;
}

sub form :
        Simple
{
    return 'form';
}

1;
